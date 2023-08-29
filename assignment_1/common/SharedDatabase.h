#ifndef ASSIGNMENT_1_SHAREDDATABASE_H
#define ASSIGNMENT_1_SHAREDDATABASE_H

#include <fcntl.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <memory>
#include <string>
#include <vector>

#include "StudentInfo.h"

template <class T>
// Implements a database of objects, stored in shared memory, that can be
// accessed by multiple processes concurrently.
class SharedDatabase {
 public:
  // Creates a new shared database with the given identifier. If a database
  // with the same identifier already exists, and the password matches, the
  // database will be opened in read-write mode. Otherwise, it will be opened in
  // read-only mode. If the database does not exist, it will be created.
  SharedDatabase(const std::string &id, std::string &password);

  // Destroys the shared database. If this is the last process that is using
  // the database, it will be cleaned up from shared memory as well.
  ~SharedDatabase();

  [[nodiscard]] size_t size() const;

  [[nodiscard]] size_t max_size() const;

  // Returns a copy of the element at the given index.
  [[nodiscard]] T get(size_t index) const;

  // Returns a copy of all elements in the database.
  [[nodiscard]] std::vector<T> getAll() const;

  // Returns a shared pointer to the element at the given index. The database is
  // locked while the pointer is in use, and unlocked when the pointer is
  // destroyed.
  [[nodiscard]] std::shared_ptr<T> at(size_t index);

  // Returns a shared pointer to the element at the given index. The database is
  // locked while the pointer is in use, and unlocked when the pointer is
  // destroyed.
  [[nodiscard]] std::shared_ptr<T> operator[](std::size_t index);

  // Adds a new element to the database.
  void add(const T &element);

  // Removes the element at the given index from the database.
  void remove(size_t index);

 private:
  // The identifier of the database. Used to identify shared memory segments.
  // Up to 100 characters, containing only letters, digits, and underscores.
  const std::string id_;

  // Named semaphore for tracking the number of processes using the database.
  sem_t *shared_;

  // Named semaphore for locking the database.
  sem_t *exclusive_;
};

#endif  // ASSIGNMENT_1_SHAREDDATABASE_H
