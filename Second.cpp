#include <iostream>
#include <fstream>
#include <complex>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>

const int MAX_ITER = 1000; // Максимальное количество итераций для проверки

// Функция для проверки принадлежности точки к множеству Мандельброта
bool mandelbrot(const std::complex<double>& c) {
    std::complex<double> z = 0;
    for (int i = 0; i < MAX_ITER; ++i) {
        z = z * z + c;
        if (std::abs(z) >= 2.0) {
            return false;
        }
    }
    return true;
}

// Функция для обработки участка сетки точек
void compute_mandelbrot(int id, int npoints, int nthreads, double min_x, double max_x, double min_y, double max_y, std::vector<std::pair<double, double>>& results, std::mutex& result_mutex) {
    int points_per_thread = npoints / nthreads;
    double dx = (max_x - min_x) / npoints;
    double dy = (max_y - min_y) / npoints;

    for (int i = id * points_per_thread; i < (id + 1) * points_per_thread; ++i) {
        for (int j = 0; j < npoints; ++j) {
            std::complex<double> c(min_x + i * dx, min_y + j * dy);
            if (mandelbrot(c)) {
                std::lock_guard<std::mutex> guard(result_mutex);
                results.push_back({c.real(), c.imag()});
            }
        }
    }
}

int main(int argc, char* argv[]) {
    auto start = std::chrono::high_resolution_clock::now();
    if (argc != 3) {
        std::cerr << "Usage: ./program nthreads npoints" << std::endl;
        return 1;
    }

    int nthreads = std::stoi(argv[1]);
    int npoints = std::stoi(argv[2]);

    double min_x = -2.0, max_x = 1.0, min_y = -1.5, max_y = 1.5;

    std::vector<std::pair<double, double>> results;
    std::mutex result_mutex;
    
    std::vector<std::thread> threads;

    // Запускаем потоки для параллельного вычисления
    for (int i = 0; i < nthreads; ++i) {
        threads.emplace_back(compute_mandelbrot, i, npoints, nthreads, min_x, max_x, min_y, max_y, std::ref(results), std::ref(result_mutex));
    }

    // Ждем завершения всех потоков
    for (auto& thread : threads) {
        thread.join();
    }

    // Выводим время работы алгоритма до сохранения результатов в файл
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> elapsed = end - start;
    std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;

    // Сохраняем результат в файл
    std::ofstream file("mandelbrot.csv");
    for (const auto& point : results) {
        file << point.first << "," << point.second << "\n";
    }
    file.close();

    return 0;
}
