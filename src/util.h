//
// Created by christian on 7/11/22.
//

#ifndef TASK_MANAGER_SRC_UTIL_H_
#define TASK_MANAGER_SRC_UTIL_H_

#include <algorithm>
#include <string>
#include "task_manager.h"

namespace unmined::util {

template<typename T, typename K>
bool has(const T &arr, const K &to_find);

template<typename T>
T get(const T &x, std::mutex &mut);

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
