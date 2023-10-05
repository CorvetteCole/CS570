//
// Created by coleg on 9/18/23.
//

#include "Utilities.h"

#include <iostream>

[[nodiscard]] std::shared_ptr<sem_t> acquireSem(
    sem_t *semaphore, std::chrono::milliseconds semTimeout) {
  auto timeout_time = std::chrono::system_clock::now() + semTimeout;
  timespec timeout{};
  timeout.tv_sec = std::chrono::duration_cast<std::chrono::seconds>(
                       timeout_time.time_since_epoch())
                       .count();
  if (sem_timedwait(semaphore, &timeout) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to acquire exclusive lock");
  }

  // Return a shared pointer to the semaphore, with a custom deleter that
  // unlocks the semaphore
  return {semaphore, [](sem_t *sem) {
            // check if semaphore is locked (user could have unlocked it
            // manually)
            if (sem_trywait(sem) == -1) {
              // if it is, unlock it
              if (sem_post(sem) == -1) {
                throw std::system_error(errno, std::generic_category(),
                                        "Failed to release exclusive lock");
              }
            }
          }};
}