//
// Created by christian on 7/11/22.
//

#ifndef TASK_MANAGER_SRC_UTIL_H_
#define TASK_MANAGER_SRC_UTIL_H_

#include <algorithm>
#include <string>
#include "task_manager.h"

namespace unmined::util {

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

}

#endif //TASK_MANAGER_SRC_UTIL_H_
