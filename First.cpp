#include <iostream>
#include <thread>
#include <vector>
#include <random>
#include <atomic>
#include <cstdlib>
#include <chrono>

// Глобальные переменные
long total_trials;
std::atomic<long> total_hits(0); // Используем атомарную переменную для синхронизации доступа
int nthreads;

// Функция, выполняемая каждым потоком
void monte_carlo_pi(long trials_per_thread) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(-1.0, 1.0);

    long local_hits = 0;
    for (long i = 0; i < trials_per_thread; i++) {
        double x = dis(gen);
        double y = dis(gen);

        if (x * x + y * y <= 1.0) {
            local_hits++;
        }
    }

    total_hits += local_hits; // Атомарное обновление общего счетчика
}

int main(int argc, char* argv[]) {
  auto start = std::chrono::high_resolution_clock::now();
  if (argc != 3) {
    std::cerr << "Usage: " << argv[0] << " nthreads ntrials\n";
    return 1;
  }

  nthreads = strtol(argv[1], nullptr, 10);
  total_trials = strtol(argv[2], nullptr, 10);

  std::vector<std::thread> threads;
  long trials_per_thread = total_trials / nthreads;

  // Запуск потоков
  for (int i = 0; i < nthreads; i++) {
    threads.emplace_back(monte_carlo_pi, trials_per_thread);
  }

  // Ожидание завершения всех потоков
  for (auto& th : threads) {
    th.join();
  }

  // Вычисляем pi
  double pi = 4.0 * static_cast<double>(total_hits) / static_cast<double>(total_trials);
  std::cout << "Estimated value of pi: " << pi << std::endl;

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> elapsed = end - start;
  std::cout << "Time taken: " << elapsed.count() << " seconds" << std::endl;

  return 0;
}
