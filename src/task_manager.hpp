// ***********************************************
// *                                             *
// * An amalgamation of the task_manager library *
// * By Christian                                *
// * 2 .h                                        *
// * 1 .cpp                                      *
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
#define EVAL(x, y) ((x & y) == (y))
#define SET_ON(x, y) x |= y
#define SET_OFF(x, y) x &= ~y

namespace unmined {

using namespace std::chrono_literals;

inline void spinner(int a = 4) {
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
  KILL_ON_EMPTY = 1,
  IN_ORDER = 1 << 1,
};

enum task_settings {
  AFTER = 0,
};

struct task {
  std::string name; // the name of the task
  int (*func)(); // the function to be _run for said task
  std::unordered_map<enum task_settings, std::string> settings{ // the settings of the tesk
      {AFTER, ""}
  };
};

template<int WORKER_COUNT = 4>
class task_manager {
 private:
  static task_manager<WORKER_COUNT> *instance_;

  std::thread main_thread_;
  std::array<std::thread, WORKER_COUNT> workers_;

  std::deque<task> queue_;
  std::vector<std::string> done_;
//  bool in_order_ = true;
  bool is_paused_ = true;
  bool stop_ = false;
  uint16_t settings_ = 0 | (~KILL_ON_EMPTY | IN_ORDER);

  std::mutex in_order_lock_;
  std::mutex is_paused_lock_;
  std::mutex kill_lock_;
  std::mutex done_lock_;
  std::mutex queue_lock_;

 public:
  void (*task_start_callback)(const task &t, int wid) =[](const task &t, int wid) {};
  void (*task_stop_callback)(const task &t, int wid) =[](const task &t, int wid) {};
  void (*task_fail_callback)(const task &t, int wid, int err) =[](const task &t, int wid, int err) {};

 private:
  task_manager();
  ~task_manager();

  void _run();
  void _run_worker(int id);

  task _pop_queue() {
    GUARD(queue_lock_);
    task t = queue_.front();
    queue_.pop_front();
    return t;
  }

 public:
  task_manager(task_manager &other) = delete;
  void operator=(const task_manager &) = delete;

  static task_manager *get_instance();

  void add(const struct task &task);

  void start();
  void pause();
  void stop();
  bool is_done();

  void join();

  std::vector<std::string> tasks();

  void set(tm_settings t, bool val) {
    if (val) SET_ON(settings_, t);
    else
      SET_OFF(settings_, ~t);
  }

  bool get(tm_settings mask) {
    return EVAL(settings_, mask);
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
#include <string>

namespace unmined::util {
template<typename T, typename K>
bool has(const T &arr, const K &to_find) {
  return std::any_of(
      arr.begin(),
      arr.end(),
      [to_find](const K &s) {
        return s == to_find;
      });
}

/**
 * @brief Gets a variable safely with its lock
 * @tparam T The type of the variable
 * @param x The variable
 * @param mut The mutex of the variable (the lock)
 * @return A const copy of the variable to get
 */
template<typename T>
T get(const T &x, std::mutex &mut) {
  GUARD(mut);
  T o = x;
  return o;
}

template<typename T>
void task_on(T &x, std::mutex &mut, void(*func)(T &x)) {
  GUARD(mut);
  func(x);
}

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

// region singleton stuff

template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT> *unmined::task_manager<WORKER_COUNT>::instance_ = nullptr;

template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT>::task_manager() {
  main_thread_ = std::thread(&task_manager::_run, this);
}

template<int WORKER_COUNT>
unmined::task_manager<WORKER_COUNT>::~task_manager() {
  pause();
  GUARD(queue_lock_);
  while (!queue_.empty()) {
    queue_.pop_front();
  }
//  pause();
  stop();
}

// endregion

/**
 * @brief Runs the set amount of workers
 * @tparam WORKER_COUNT The amount of workers the task manager allows
 */
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
  printf("[%i]: Started\n", id);
  std::cout.flush();

  while (!util::get(stop_, kill_lock_)) {
    if (util::get(is_paused_, is_paused_lock_)) continue;
    if (util::get(queue_, queue_lock_).empty()) {
      if (get(KILL_ON_EMPTY) == true) break;
      continue;
    }

    struct task task = _pop_queue();

    if (!task.settings[AFTER].empty()) {
      if (!util::has(done_, task.name)) {
        queue_.push_back(task); // TODO: rework this, its garbo
        continue;
      }
      while (!util::has(done_, task.name)) std::this_thread::sleep_for(100ms);
    }

    task_start_callback(task, id);
    int err = task.func();
    if (err < 0) task_fail_callback(task, id, err);
    GUARD(done_lock_);
    done_.push_back(task.name);
    task_stop_callback(task, id);
  }
  printf("[%i]: Stopped\n", id);
  std::cout.flush();
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
  std::cout << "Settings: ";
  std::cout << std::endl;
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
