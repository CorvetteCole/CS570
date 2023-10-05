//
// Created by coleg on 9/18/23.
//

#ifndef ASSIGNMENT_1_UTILITIES_H
#define ASSIGNMENT_1_UTILITIES_H

// Locks a semaphore, returning a handle that automatically unlocks it when
// destroyed.
#include <semaphore.h>

#include <chrono>
#include <memory>

using namespace std::chrono_literals;

// Get a lock on the database entry. Da a sem_timedwait() on it and
// throw an exception if it times out.
[[nodiscard]] std::shared_ptr<sem_t> acquireSem(
    sem_t *semaphore, std::chrono::milliseconds semTimeout = 1000ms);

template <typename F>
void runWithLock(sem_t *semaphore, F &&lambda,
                 std::chrono::milliseconds semTimeout = 1000ms) {
  auto lock = acquireSem(semaphore, semTimeout);
  try {
    lambda();
  } catch (...) {
    lock.reset();
    throw;
  }
}

#endif  // ASSIGNMENT_1_UTILITIES_H
