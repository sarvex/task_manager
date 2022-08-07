// ***********************************************
// *                                             *
// * An amalgamation of the task_manager library *
// * By Christian                                *
// *                                             *
// ***********************************************

#include <vector>
#include <functional>
#include <algorithm>
#include <mutex>
#include <string>
#include <any>
#include <queue>
#include <thread>
#include <iostream>
#include <future>


// *******************************
// * Start of src/task_manager.h *
// *******************************

//
// Created by christian on 7/5/22.
//

#ifndef TASK_MANAGER__TASK_MANAGER_H_
#define TASK_MANAGER__TASK_MANAGER_H_


#define GUARD(x) std::lock_guard<std::mutex> guard(x)

namespace unmined {

using namespace std::chrono_literals;

void spinner(int a = 4) {
  for (int idx = 0; idx < a; ++idx) {
    std::cout << ("\\");
    std::this_thread::sleep_for(100ms);
    std::cout.flush();
    printf("\033[1D\r");
    std::cout << ("|");
    std::this_thread::sleep_for(100ms);
    std::cout.flush();
    printf("\033[1D\r");
    std::cout << ("/");
    std::this_thread::sleep_for(100ms);
    std::cout.flush();
    printf("\033[1D\r");
    std::cout << ("-");
    std::this_thread::sleep_for(100ms);
    std::cout.flush();
    printf("\033[1D\r");
  }
}

enum tm_settings {
  KILL_ON_EMPTY = 0,
  TM_SETTINGS_SIZE,
};

enum task_settings {
  AFTER = 0,
};

struct task {
  std::string name;
  std::function<int()> func;
  std::unordered_map<enum task_settings, std::string> settings{
      {AFTER, ""}
  };
};

template<int WORKER_COUNT = 4>
class task_manager {
  static task_manager<WORKER_COUNT> *instance_;

  std::thread main_thread_;
  std::array<std::thread, WORKER_COUNT> workers_;

  std::deque<task> queue_;
  std::vector<std::string> done_;
  std::unordered_map<int, bool> settings_;
  bool in_order_ = true;
  bool is_paused_ = true;
  bool stop_ = false;

  std::mutex in_order_lock_;
  std::mutex is_paused_lock_;
  std::mutex kill_lock_;
  std::mutex done_lock_;
  std::mutex queue_lock_;

  task_manager();
  ~task_manager();

 public:
  task_manager(task_manager &other) = delete;
  void operator=(const task_manager &) = delete;

  static task_manager *get_instance();

  void add(const struct task &task);

  void start();
  void pause();
  void stop();
  bool is_done();

  void _run();
  void _run_worker(int id);
  void join();

  std::vector<std::string> tasks();

  void set(tm_settings t, bool b) {
    settings_[t] = b;
  }

  bool get(tm_settings t) {
    return settings_[t];
  }

};

}

#endif //TASK_MANAGER__TASK_MANAGER_H_

// ***********************
// * Start of src/util.h *
// ***********************

//
// Created by christian on 7/11/22.
//

#ifndef TASK_MANAGER_SRC_UTIL_H_
#define TASK_MANAGER_SRC_UTIL_H_

#include <algorithm>

template<typename T, typename K>
bool has(const T &arr, const K &to_find) {
  return std::any_of(
      arr.begin(),
      arr.end(),
      [to_find](const K &s) {
        return s == to_find;
      });
}

template<typename T>
void print(std::vector<T> in) {
  for (auto &i : in) {
    std::cout << i << " ";
  }
  std::cout << std::endl;
}

#endif //TASK_MANAGER_SRC_UTIL_H_

// *********************************
// * Start of src/task_manager.cpp *
// *********************************

//
// Created by christian on 7/11/22.
//

#include <thread>
#include <iostream>

using millis = std::chrono::milliseconds;
using namespace std::chrono_literals;

template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT> *unmined::task_manager<WORKER_COUNT>::instance_ = nullptr;

template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT>::task_manager() {
  main_thread_ = std::thread(&task_manager::_run, this);
  for (int i = 0; i < tm_settings::TM_SETTINGS_SIZE; ++i) {
    settings_[i] = false;
  }
}

template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT>::~task_manager() {
  GUARD(queue_lock_);
  while (!queue_.empty()) {
    queue_.pop_front();
  }
  pause();
  stop();
}

template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::_run() {
  for (int i = 0; i < WORKER_COUNT; ++i) {
    workers_[i] = std::thread(&task_manager::_run_worker, this, i);
  }
  for (int i = 0; i < WORKER_COUNT; ++i) {
    workers_[i].join();
  }
}

template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::_run_worker(int id) {
  printf("running worker %i\n", id);
  while (!stop_) {
    struct task task;
    { // do the stuff with the guard and remove it by leaving the scope
      GUARD(queue_lock_);
      if (is_paused_) continue;
      if (queue_.empty() && get(KILL_ON_EMPTY) == true) break;
      task = queue_.front();
      queue_.pop_front();
    }
    if (!task.settings[AFTER].empty()) {
      if (!has(done_, task.name)) {
//        printf("skipped: %s\n", task.name.c_str());
        queue_.push_back(task); // TODO: rework this, its garbo
        continue;
      }
      while (!has(done_, task.name)) std::this_thread::sleep_for(100ms);
    }

    printf("running task: %s on worker %i\n", task.name.c_str(), id);
    task.func();
    done_.push_back(task.name);
//    print(done_);
  }
}
template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT> *unmined::task_manager<WORKER_COUNT>::get_instance() {
  if (instance_ == nullptr) instance_ = new task_manager();
  return instance_;
}
template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::add(const task &task) {
  GUARD(queue_lock_);
  queue_.push_back(task);
}

template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::start() {
  GUARD(is_paused_lock_);
  is_paused_ = false;
}
template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::pause() {
  GUARD(is_paused_lock_);
  is_paused_ = true;
}
template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::stop() {
  GUARD(kill_lock_);
  instance_ = nullptr;
  stop_ = true;
}
template<int WORKER_COUNT>
bool unmined::task_manager<WORKER_COUNT>::is_done() {
  GUARD(queue_lock_);
  std::cout << "A" << queue_.size() << "B" << std::endl;
  return queue_.empty();
}
template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::join() {
  main_thread_.join();
  printf("done\n");
}

template<int WORKER_COUNT>
std::vector<std::string> unmined::task_manager<WORKER_COUNT>::tasks() {
  std::vector<std::string> out;
  for (auto &i: queue_) {
    out.push_back(i.name);
  }
  return out;
}
