#include <iostream>
#include <random>

#ifdef AMALGAMATED
#include "task_manager.hpp"
#else
#include "task_manager.h"
#include "task_manager.cpp"
#endif

using namespace unmined;
using namespace std::chrono_literals;

int randInt(int min, int max) {
  std::random_device rd;
  std::mt19937 rng(rd());
  std::uniform_int_distribution<int> uni(min, max);
  return uni(rng);
}

int main() {
  auto *tm = task_manager<4>::get_instance();
  tm->set(KILL_ON_EMPTY, true);
  tm->set(IN_ORDER, true);

  tm->task_start_callback = [](const task &t, int wid) {
    printf("[%i]: Task '%s' -> Running\n", wid, t.name.c_str());
    std::cout.flush();
  };
  tm->task_stop_callback = [](const task &t, int wid) {
    printf("[%i]: Task '%s' -> Finished\n", wid, t.name.c_str());
    std::cout.flush();
  };

  tm->pause();

  for (int i = 0; i < 10; ++i) {
    tm->add({"task" + std::to_string(i),
             []() -> int {
               std::this_thread::sleep_for(1000ms);
               return 0;
             },
             {{AFTER, ""}}
            });
  }

  tm->start();
  tm->join();
  return 0;
}
