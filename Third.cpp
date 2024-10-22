#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <list>
#include <pthread.h>
#include <vector>
#include <chrono>
#include <algorithm>

// Структура для реализации read-write lock
struct custom_rwlock {
    std::mutex mtx;
    std::condition_variable read_cv, write_cv;
    int readers_count = 0;
    int waiting_writers = 0;
    int waiting_readers = 0;
    bool writer_active = false;

    // Захват блокировки на чтение
    void rdlock() {
        std::unique_lock<std::mutex> lock(mtx);
        waiting_readers++;
        read_cv.wait(lock, [this]() { return !writer_active && waiting_writers == 0; });
        waiting_readers--;
        readers_count++;
    }

    // Захват блокировки на запись
    void wrlock() {
        std::unique_lock<std::mutex> lock(mtx);
        waiting_writers++;
        write_cv.wait(lock, [this]() { return !writer_active && readers_count == 0; });
        waiting_writers--;
        writer_active = true;
    }

    // Освобождение блокировки
    void unlock() {
        std::unique_lock<std::mutex> lock(mtx);
        if (writer_active) {
            writer_active = false;
        } else {
            readers_count--;
        }

        if (readers_count == 0 && waiting_writers > 0) {
            write_cv.notify_one();
        } else if (!writer_active && waiting_readers > 0) {
            read_cv.notify_all();
        }
    }
};

// Односвязный список для тестирования
template<typename T>
class ThreadSafeList {
private:
    std::list<T> data;
    custom_rwlock lock;

public:
    // Вставка элемента (запись)
    void insert(const T& value) {
        lock.wrlock();
        data.push_back(value);
        lock.unlock();
    }

    // Поиск элемента (чтение)
    bool find(const T& value) {
        lock.rdlock();
        bool found = (std::find(data.begin(), data.end(), value) != data.end());
        lock.unlock();
        return found;
    }
};

// Аналог с pthread_rwlock_t
template<typename T>
class PthreadSafeList {
private:
    std::list<T> data;
    pthread_rwlock_t lock;

public:
    PthreadSafeList() {
        pthread_rwlock_init(&lock, nullptr);
    }

    ~PthreadSafeList() {
        pthread_rwlock_destroy(&lock);
    }

    void insert(const T& value) {
        pthread_rwlock_wrlock(&lock);
        data.push_back(value);
        pthread_rwlock_unlock(&lock);
    }

    bool find(const T& value) {
        pthread_rwlock_rdlock(&lock);
        bool found = (std::find(data.begin(), data.end(), value) != data.end());
        pthread_rwlock_unlock(&lock);
        return found;
    }
};

// Тестирование работы
void reader(ThreadSafeList<int>& lst, int value) {
    lst.find(value);
}

void writer(ThreadSafeList<int>& lst, int value) {
    lst.insert(value);
}

void reader_pthread(PthreadSafeList<int>& lst, int value) {
    lst.find(value);
}

void writer_pthread(PthreadSafeList<int>& lst, int value) {
    lst.insert(value);
}

int main() {
    ThreadSafeList<int> custom_list;
    PthreadSafeList<int> pthread_list;

    // Количество потоков и элементов
    int num_threads = 6;
    int num_elements = 10000;

    std::vector<std::thread> threads;

    // Замер времени для кастомной реализации
    auto start_custom = std::chrono::high_resolution_clock::now();

    // Создаем потоки для кастомного rwlock
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread([&custom_list, num_elements, i]() {
            for (int j = i * num_elements; j < (i + 1) * num_elements; ++j) {
                writer(custom_list, j);
                reader(custom_list, j);
            }
        }));
    }

    for (auto& th : threads) {
        th.join();
    }

    auto end_custom = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_custom = end_custom - start_custom;
    std::cout << "Custom rwlock time: " << duration_custom.count() << " seconds" << std::endl;

    threads.clear();

    // Замер времени для pthread_rwlock
    auto start_pthread = std::chrono::high_resolution_clock::now();

    // Создаем потоки для pthread_rwlock
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread([&pthread_list, num_elements, i]() {
            for (int j = i * num_elements; j < (i + 1) * num_elements; ++j) {
                writer_pthread(pthread_list, j);
                reader_pthread(pthread_list, j);
            }
        }));
    }

    for (auto& th : threads) {
        th.join();
    }

    auto end_pthread = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration_pthread = end_pthread - start_pthread;
    std::cout << "pthread_rwlock time: " << duration_pthread.count() << " seconds" << std::endl;

    return 0;
}
