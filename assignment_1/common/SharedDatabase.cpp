#include "SharedDatabase.h"

#include <time.h>

template <typename T>
SharedDatabase<T>::SharedDatabase(const std::string &id,
                                  std::string &password) {
  // TODO generate key_t using ftok() from id
  // Setup shared memory segment(s) to share:
  // - database
  // - database metadata

  // Also need to set the named semaphore for tracking processes using the
  // database

  // Need to handle creating this all if it doesn't exist, or opening it if it
  // does exist. So the first thing to do is check if a particular shared memory
  // segment exists whose key is derived from the id as mentioned above.
}

template <typename T>
SharedDatabase<T>::~SharedDatabase() {

}

template <typename T>
T SharedDatabase<T>::at(size_t index) const {
  // check if index is within bounds
  if (index >= this->numEntries) {
    throw std::out_of_range("Index out of bounds");
  }

  auto lock = acquireSem(this->entries[index].lock);
  // Return a copy of the data
  return this->entries[index].data;
}

template <typename T>
std::shared_ptr<T> SharedDatabase<T>::at(size_t index) {
  // check if index is within bounds
  if (index >= this->numEntries) {
    throw std::out_of_range("Index out of bounds");
  }

  auto lock = acquireSem(this->entries[index].lock);
  // Return a shared pointer to the data, with a custom deleter that unlocks
  // the entry
  return std::make_shared<T>(&this->entries[index].data,
                             [lock](T *data) { lock.reset(); });
}

template <typename T>
void SharedDatabase<T>::erase(size_t index) {
  // check if index is within bounds
  if (index >= this->numEntries) {
    throw std::out_of_range("Index out of bounds");
  }

  auto lock = acquireSem(this->entries[index].lock);
  delete this->entries[index].data;
}

template <typename T>
void SharedDatabase<T>::push_back(T data) {
  if (this->numEntries >= this->maxEntries) {
    throw std::out_of_range("Database is full");
  }
  auto countLock = acquireSem(this->lock);
  auto entryLock = acquireSem(this->entries[this->numEntries].lock);
  this->entries[this->numEntries].data = data;
  this->numEntries++;
}

template <typename T>
void SharedDatabase<T>::set(size_t index, T data) {
  // check if index is within bounds
  if (index >= this->numEntries) {
    throw std::out_of_range("Index out of bounds");
  }

  auto lock = acquireSem(this->entries[index].lock);
  this->entries[index].data = data;
}

template <typename T>
size_t SharedDatabase<T>::maxSize() const {
  return this->maxEntries;
}
template <typename T>
size_t SharedDatabase<T>::size() const {
  return this->numEntries;
}

template <typename T>
std::shared_ptr<sem_t> SharedDatabase<T>::acquireSem(sem_t *semaphore) const {
  // Get a lock on the database entry. Da a sem_timedwait() on it and
  // throw an exception if it times out.
  timespec timeout{LOCK_TIMEOUT.count()};
  switch (sem_timedwait(semaphore, &timeout)) {
    case 0:
      break;
    case ETIMEDOUT:
      throw std::runtime_error(
          "Failed to acquire exclusive lock within timeout");
    default:
      throw std::runtime_error("Failed to acquire exclusive lock");
  }

  // Return a shared pointer to the semaphore, with a custom deleter that
  // unlocks the semaphore
  return std::make_shared<sem_t>(semaphore,
                                 [](sem_t *lock) { sem_post(lock); });
}