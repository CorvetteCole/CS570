#ifndef ASSIGNMENT_1_SHAREDDATABASE_H
#define ASSIGNMENT_1_SHAREDDATABASE_H

#include <fcntl.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

// using chrono types
using namespace std::chrono_literals;

constexpr std::chrono::seconds LOCK_TIMEOUT = 5s;

// Represents the metadata of the database. Contains the version of the database
// and a hash of the password. This is stored in shared memory and processes can
// use it to determine if they should mount the database shared memory segment
// as read-only or read-write.
struct DatabaseMetadata {
  sem_t lock;
  uint32_t version;
  std::size_t passwordHash;
};

template <typename T>
// Represents a single entry in the database. Contains the data and a semaphore
// for locking the entry.
struct DatabaseEntry {
  sem_t lock;
  T data;
};

template <typename T>
// Represents the database. Contains metadata about the database; a hash of the
// password, and an array of entries.
struct Database {
  size_t size;
  size_t maxSize;
  DatabaseEntry<T> entries[];
};

template <typename T>
// Implements a database of objects, stored in shared memory, that can be
// accessed by multiple processes concurrently.
class SharedDatabase : private Database<T> {
 public:
  // Creates a new shared database with the given identifier. If a database
  // with the same identifier already exists, and the password matches, the
  // database will be opened in read-write mode. Otherwise, it will be opened in
  // read-only mode. If the database does not exist, it will be created.
  SharedDatabase(const std::string &id, std::string &password);

  // Destroys the shared database. If this is the last process that is using
  // the database, it will be cleaned up from shared memory as well.
  ~SharedDatabase();

  [[nodiscard]] size_t getSize() const;

  [[nodiscard]] size_t getMaxSize() const;

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

  // Database metadata
  DatabaseMetadata *metadata_;

  // Gets a semaphore associated with the given index. Automatically unlocks the
  // semaphore when the returned shared pointer is destroyed.
  [[nodiscard]] std::shared_ptr<sem_t> lockEntry(size_t index) const;
};


#endif  // ASSIGNMENT_1_SHAREDDATABASE_H
