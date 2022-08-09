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

/**
 * @brief The possible settings for the task manager
 */
enum tm_settings {
  KILL_ON_EMPTY = 1,
  IN_ORDER = 1 << 1,
};

/**
 * @brief The possible settings for a task
 */
enum task_settings {
  AFTER = 0,
};

/**
 * @brief The task container, contains a name, a function, and settings
 */
struct task {
  std::string name; // the name of the task
  std::function<int()> func; // the function to be _run for said task
  std::unordered_map<enum task_settings, std::string> settings{ // the settings of the tesk
      {AFTER, ""}
  };
};

/**
 * @brief The task manager class
 * @tparam WORKER_COUNT The amount of worker threads, default 4
 */
template<int WORKER_COUNT = 4>
class task_manager {
 private:
  /// The singleton instance
  static task_manager<WORKER_COUNT> *instance_;

  /// The main running thread, so that the main program can do other things
  std::thread main_thread_;
  /// The array of worker threads
  std::array<std::thread, WORKER_COUNT> workers_;

  /// The queue of tasks
  std::deque<task> queue_;
  /// A vector of the done tasks' names
  std::vector<std::string> done_;
  /// If the task manager is paused
  bool is_paused_ = true;
  /// If the task manager is stopped, will kill the task manager cleanly
  bool stop_ = false;
  /// The settings, only 2 exist, but there are 16 possible
  uint16_t settings_ = 0 | (~KILL_ON_EMPTY | IN_ORDER);

  // the mutexes for each variable that will be modified
  std::mutex settings_lock_;
  std::mutex is_paused_lock_;
  std::mutex kill_lock_;
  std::mutex done_lock_;
  std::mutex queue_lock_;

 public:
  // TODO: REWORK THIS, maybe make them private or sm

  /**
   * @brief The function called when a task starts
   * @param t
   * @param wid
   */
  std::function<void(const task &t, const int &wid)> task_start_callback
      = [](const task &t, const int &wid) {};
  /**
   * @brief The function called when a task finishes
   * @param t
   * @param wid
   */
  std::function<void(const task &t, const int &wid)> task_stop_callback
      = [](const task &t, const int &wid) {};
  /**
   * @brief The function called when a task fails
   * @param t task - The task
   * @param wid int - The worker ID
   * @param err int - The error returned from the task
   */
  std::function<void(const task &t, const int &wid, const int &err)> task_fail_callback
      = [](const task &t, const int &wid, const int &err) {};

  /**
   * @brief The function called when a worker starts
   * @param wid int - The worker ID
   */
  std::function<void(const int &wid)> worker_start_callback = [](const int &wid) {};

  /**
   * @brief The function called when a worker stops
   * @param wid int - The worker ID
   */
  std::function<void(const int &wid)> worker_stop_callback = [](const int &wid) {};

 private:
  /// makes a new task manager, do not use this
  task_manager();
  /// deletes the task manager safely
  ~task_manager();

  /**
   * @brief The run function that starts all the workers and main thread
   */
  void _run();
  /**
   * @brief Starts a worker with an ID
   * @param id int - The ID of the worker
   */
  void _run_worker(int id);

  /**
   * @brief Gets the first item in the queue, and removes it
   * @return task - The first item in the queue
   */
  task _pop_queue();

 public:
  /// you can't use this
  task_manager(task_manager &other) = delete;
  /// you can't use this
  void operator=(const task_manager &) = delete;

  /**
   * @brief Gets the instance of the task manager
   * @return task_manager* - The task manager instance
   */
  static task_manager *get_instance();

  /**
   * @brief Adds a task to the queue of tasks to be done
   * @param task task - The task to be done, will be last in the order
   */
  void add(const struct task &task);

  /**
   * @brief Starts fulfilling tasks
   */
  void start();
  /**
   * @brief Pauses fulfilling tasks
   */
  void pause();
  /**
   * @brief Stops the task manager and cleans it up
   */
  void stop();
  /**
   * @brief Checks if the the queue is empty
   * @return bool - If the queue is empty
   */
  bool is_done();

  /**
   * @brief Joins the main task manager thread, which will wait until all tasks are complete
   */
  void join();

  /**
   * @brief Returns a vector of task names
   * @return vector<string> - vector of task names
   */
  std::vector<std::string> tasks();

  /**
   * @brief Sets a setting
   * @param t tm_settings - The setting to set
   * @param val bool - The value of the setting
   */
  void set(tm_settings t, bool val) {
    GUARD(settings_lock_);
    if (val) SET_ON(settings_, t);
    else
      SET_OFF(settings_, ~t);
  }

  /**
   * @brief Gets the current value of a setting
   * @param mask tm_settings - The setting(s) to get
   * @return bool - The value of the setting (true or false)
   */
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
bool has(const T &arr, const K &to_find);

template<typename T>
T get(const T &x, std::mutex &mut);

template<typename T>
void modify(T &x, std::mutex &mut, void(*func)(T &x));

inline std::string to_string(const unmined::task &t);

template<typename T>
inline std::string to_string(std::deque<T> q);

inline std::string to_string(const std::vector<std::string> &q);

/**
 * @brief Checks if an array has a certain element
 * @tparam T The type of the array
 * @tparam K The type of the element
 * @param arr The array to search
 * @param to_find The element to find
 * @return true if the element exists, false otherwise
 */
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

/**
 * @brief Modifies a variable thread-safely
 * @tparam T The type of the variable
 * @param x The variable to modify
 * @param mut The mutex (lock) for the variable
 * @param func The function acting upon the variable
 */
template<typename T>
void modify(T &x, std::mutex &mut, void(*func)(T &x)) {
  GUARD(mut);
  func(x);
}

inline std::string to_string(const unmined::task &t) {
  return t.name;
}

template<typename T>
inline std::string to_string(std::deque<T> q) {
  std::string out;
  for (auto i: q) {
    out += to_string(i);
    out += ", ";
  }
  return out;
}

inline std::string to_string(const std::vector<std::string> &q) {
  std::string out;
  for (const auto &i: q) {
    out += i;
    out += ", ";
  }
  return out;
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
  worker_start_callback(id);

  while (!util::get(stop_, kill_lock_)) {
    if (util::get(is_paused_, is_paused_lock_)) continue;
    struct task task = _pop_queue();
    if (!task.func) {
      if (get(KILL_ON_EMPTY)) break;
      continue;
    }

    std::string after = task.settings[AFTER];
    if (!after.empty()) {
      if (!util::has(util::get(done_, done_lock_), after)) {
        {
          GUARD(queue_lock_);
          queue_.push_back(task); // TODO: rework this, its garbo
        }
        continue;
      }
    }

    task_start_callback(task, id);
    int err = task.func();
    if (err < 0) task_fail_callback(task, id, err);
    GUARD(done_lock_);
    done_.push_back(task.name);
    task_stop_callback(task, id);
  }
  worker_stop_callback(id);
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
  return queue_.empty();
}
template<int WORKER_COUNT>
void unmined::task_manager<WORKER_COUNT>::join() {
  main_thread_.join();
}

template<int WORKER_COUNT>
std::vector<std::string> unmined::task_manager<WORKER_COUNT>::tasks() {
  std::vector<std::string> out;
  for (auto &i: queue_) {
    out.push_back(i.name);
  }
  return out;
}
template<int WORKER_COUNT>
unmined::task unmined::task_manager<WORKER_COUNT>::_pop_queue() {
  GUARD(queue_lock_);
  if (queue_.empty()) return task();
  task t = queue_.front();
  queue_.pop_front();
  return t;
}
