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

using namespace std::chrono_literals;

constexpr std::chrono::seconds LOCK_TIMEOUT = 5s;
constexpr int METADATA_MEM_ID = 1;
constexpr int DATABASE_MEM_ID = 2;
constexpr int DB_VERSION = 1;

// Represents the metadata of the database. Contains the version of the database
// and a hash of the password. This is stored in shared memory and processes can
// use it to determine if they should mount the database shared memory segment
// as read-only or read-write.
struct DatabaseMetadata {
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
  // Must be locked before accessing numEntries
  sem_t lock;
  size_t numEntries;
  const size_t maxEntries;
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
  explicit SharedDatabase(std::string &password);

  // Destroys the shared database. If this is the last process that is using
  // the database, it will be cleaned up from shared memory as well.
  ~SharedDatabase();

  // Returns a copy of the element at the given index.
  [[nodiscard]] T at(size_t index) const;

  // Returns a shared pointer to the element at the given index. The database is
  // locked while the pointer is in use, and unlocked when the pointer is
  // destroyed.
  [[nodiscard]] std::shared_ptr<T> at(size_t index);

  // Deletes the element at the given index.
  void erase(size_t index);

  // Adds an element to the end of the database.
  void push_back(T data);

  // Sets the element at the given index to the given data.
  void set(size_t index, T data);

  // Returns the number of elements in the database.
  [[nodiscard]] size_t size() const;

  // Returns the maximum number of elements in the database.
  [[nodiscard]] size_t maxSize() const;

 private:
  // Named semaphore for tracking the number of processes using the database.
  sem_t shared_;

  // Database metadata
  DatabaseMetadata *metadata_;

  // Locks a semaphore associated with the given index. Automatically unlock
  // the semaphore when the returned shared pointer is destroyed.
  [[nodiscard]] std::shared_ptr<sem_t> acquireSem(sem_t *semaphore) const;
};

#endif  // ASSIGNMENT_1_SHAREDDATABASE_H
