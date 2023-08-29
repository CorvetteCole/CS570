#ifndef ASSIGNMENT_1_SHAREDDATABASE_H
#define ASSIGNMENT_1_SHAREDDATABASE_H

#include <memory>
#include <string>
#include <vector>

#include "StudentInfo.h"

template <class T>
// Implements a database, stored in shared memory, that can be accessed by
// multiple processes concurrently.
class SharedDatabase {
 public:
  // Creates a new shared database with the given identifier. If a database
  // with the same identifier already exists, it will be opened instead.
  SharedDatabase(const std::string &id);

  // Destroys the shared database. If this is the last process that is using
  // the database, it will be cleaned up from shared memory as well.
  ~SharedDatabase();

  [[nodiscard]] const std::string &getId() const;

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
  [[nodiscard]] std::shared_ptr<T> operator[](size_t index);

  // Adds a new element to the database.
  void add(const T &element);

  // Removes the element at the given index from the database.
  void remove(size_t index);

 private:
  // The identifier of the database. Used to identify shared memory segments.
  // Up to 100 characters, containing only letters, digits, and underscores.
  const std::string id_;
};

#endif  // ASSIGNMENT_1_SHAREDDATABASE_H
