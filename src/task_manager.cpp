//
// Created by christian on 7/11/22.
//

#include <thread>
#include <iostream>
#include <future>
#include "task_manager.h"
#include "util.h"

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
  printf("[%i]: Started\n", id);
  std::cout.flush();

  while (!util::get(stop_, kill_lock_)) {
    if (util::get(is_paused_, is_paused_lock_)) continue;
//    if (util::get(queue_, queue_lock_).empty()) {
//      printf("queue_ empty\n");
//      if (get(KILL_ON_EMPTY) == true) break;
//      continue;
//    }
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
//      while (!util::has(util::get(done_, done_lock_), task.name)) std::this_thread::sleep_for(100ms);
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
template<int WORKER_COUNT>
unmined::task unmined::task_manager<WORKER_COUNT>::_pop_queue() {
  GUARD(queue_lock_);
  if (queue_.empty()) return task();
  task t = queue_.front();
  queue_.pop_front();
  return t;
}
