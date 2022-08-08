# Task Manager

A simple enough library built to make running a ton of tasks slightly easier

## Design goals

The design goals for this project were:

1. Make It 1 file: I wanted to make this library as easy as possible to implement into my other programs, so having it
   in 1 file would help. The way I got to that was by using a python script to take all the source and header files and
   turn them into 1.
2. SPEED: If the task manager were to be slow, it only adds to the slowness of the parent program, and that's no fun
   when you are say, scraping the web.
3. Controllable: I wanted to be able to make certain tasks run after another in a certain order, for better control,
   that is built in.

## Getting started

### Running a single task

The following is a simple program that runs 1 task named wait_1000ms, that waits 1000ms.

```c++
#include <thread>
#include "task_manager.hpp"

using namespace unmined;
using namespace std;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
  auto *tm = task_manager<4>::get_instance(); // get the singleton instance
  tm->set(KILL_ON_EMPTY, true); // will kill the task manager when it becomes empty
  tm->pause(); // pause fulfilling tasks

  // add a task named wait_1000ms, that waits 1000ms, with default settings
  tm->add({"wait_1000ms",
           []() {
             this_thread::sleep_for(1000ms);
             return 0;
           }});

  tm->start(); // start back fulfillment of tasks
  tm->join(); // wait until all tasks are complete
}
```

### Running multiple tasks

The following pauses the task manager, adds 10 tasks, then resumes the task manager, it will run the tasks in blocks of
4, corresponding to the amount of workers

```c++
#include <thread>
#include "task_manager.hpp"

using namespace unmined;
using namespace std;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
  auto *tm = task_manager<4>::get_instance(); // get the singleton
  tm->set(KILL_ON_EMPTY, true); // will kill the task manager when it becomes empty
  tm->pause(); // pause fulfilling tasks
  
  // add tasks wait_1000ms-[0...9]
  for (int i = 0; i < 10; ++i) {
    tm->add({"wait_1000ms-" + to_string(i),
             []() {
               this_thread::sleep_for(1000ms);
               return 0;
             }});
  }
  
  tm->start(); // start back fulfillment of tasks
  tm->join(); // wait until all tasks are done
}
```

### Running one task after another

The `AFTER` setting is used to make sure a task is run after another, it is defined after the function, and it's value
should be the name of the task to be after

```cpp
#include <thread>
#include "task_manager.hpp"

using namespace unmined;
using namespace std;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
  auto *tm = task_manager<4>::get_instance(); // get the singleton
  tm->set(KILL_ON_EMPTY, true); // will kill the task manager when it becomes empty
  tm->pause(); // pause fulfilling tasks
  
  // add a task to run first
  tm->add({"wait_1000ms-1", 
           []() {
             this_thread::sleep_for(1000ms);
             return 0;
           }});
  
  // add a task to run after
  tm->add({"wait_1000ms-2",
           []() {
             this_thread::sleep_for(1000ms);
             return 0;
           },
           {
             {AFTER, "wait_1000ms-1"}
           }});
           
  tm->start(); // start back fulfillment of tasks
  tm->join(); // wait until all tasks are done
}
```

### Callbacks

There are some callback functions built such that they will be run before and after a task is run, as well as if/when it fails

```c++
#include <thread>
#include "task_manager.hpp"

using namespace unmined;
using namespace std;
using namespace std::chrono_literals;

int main(int argc, char **argv) {
  auto *tm = task_manager<4>::get_instance(); // get the singleton
  tm->set(KILL_ON_EMPTY, true); // will kill the task manager when it becomes empty
  
  // adds a function to be run when a task starts
  tm->task_start_callback = [](const task &t, int wid) {
    printf("[%i]: Task '%s' -> Running\n", wid, t.name.c_str());
    std::cout.flush();
  };
  
  // adds a function to be run when a task ends
  tm->task_stop_callback = [](const task &t, int wid) {
    printf("[%i]: Task '%s' -> Finished\n", wid, t.name.c_str());
    std::cout.flush();
  };
  
  tm->task_fail_callback = [](const task &t, int wid, int err) {
    printf("[%i]: Task '%s' -> ERROR %i", wid, t.name.c_str(), err);
    std::cout.flush();
  };
  
  tm->pause(); // pause fulfilling tasks
  tm->add({"wait_1000ms", []() {this_thread::sleep_for(1000ms);return 0;}}); // add a task
  tm->start(); // start back fulfillment of tasks
  tm->join(); // wait until all tasks are done
}
```