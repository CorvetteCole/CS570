#ifndef ASSIGNMENT_1_SHAREDDATABASE_H
#define ASSIGNMENT_1_SHAREDDATABASE_H

#include <fcntl.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

constexpr static std::chrono::seconds LOCK_TIMEOUT = 5s;
constexpr static int METADATA_OFFSET = 1;
constexpr static int DATABASE_OFFSET = 2;
constexpr static int DB_VERSION = 1;
constexpr static int MAX_ENTRIES = 50;

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
  size_t maxEntries;
  DatabaseEntry<T> entries[MAX_ENTRIES];
};

template <typename T>
// Implements a database of objects, stored in shared memory, that can be
// accessed by multiple processes concurrently.
class SharedDatabase {
 public:
  // Creates a new shared database with the given identifier. If a database
  // with the same identifier already exists, and the password matches, the
  // database will be opened in read-write mode. Otherwise, it will be opened in
  // read-only mode. If the database does not exist, it will be created.
  explicit SharedDatabase(std::string &password, int id = 0) {
    // this is a terrible hash function, but it works for the purposes of this
    // project
    std::size_t passwordHash = std::hash<std::string>{}(password);

    // generate key_t using ftok() from current executable file path and the id
    auto metadata_key = ftok(".", id + METADATA_OFFSET);
    auto database_key = ftok(".", id + DATABASE_OFFSET);

    // create shared memory segment for metadata
    auto metadata_shmid = shmget(metadata_key, sizeof(DatabaseMetadata),
                                 IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
    bool metadata_created = false;

    if (metadata_shmid != -1) {
      // if it didn't already exist, initialize it
      // attach temporarily in read-write, initialize metadata
      auto metadata =
          static_cast<DatabaseMetadata *>(shmat(metadata_shmid, nullptr, 0));
      if (metadata == reinterpret_cast<DatabaseMetadata *>(-1)) {
        throw std::system_error(
            errno, std::generic_category(),
            "Attaching to database metadata shared memory segment failed");
      }
      metadata->version = DB_VERSION;
      metadata->passwordHash = passwordHash;

      metadata_created = true;

    } else if (errno == EEXIST) {
      // get shmid of existing database metadata
      metadata_shmid = shmget(metadata_key, sizeof(DatabaseMetadata), 0);
    } else {
      // catch errors that aren't "already exists"
      throw std::system_error(
          errno, std::generic_category(),
          "Creating database metadata shared memory segment failed");
    }

    // attach metadata_ to shared memory segment for metadata in read-only mode
    metadata_ = static_cast<DatabaseMetadata *>(
        shmat(metadata_shmid, nullptr, SHM_RDONLY));
    if (metadata_ == reinterpret_cast<DatabaseMetadata *>(-1)) {
      throw std::system_error(
          errno, std::generic_category(),
          "Attaching to database metadata shared memory segment failed");
    }

    if (metadata_->version != DB_VERSION) {
      throw std::runtime_error("Database version mismatch");
    }

    // TODO DB setup tasks. Similar to metadata
    auto database_shmid = shmget(database_key, sizeof(Database<T>),
                                 IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    if (database_shmid != -1) {
      // if it didn't already exist, initialize it
      // attach temporarily in read-write, initialize db
      auto database =
          static_cast<Database<T> *>(shmat(database_shmid, nullptr, 0));
      if (database == reinterpret_cast<Database<T> *>(-1)) {
        throw std::system_error(
            errno, std::generic_category(),
            "Attaching to database shared memory segment failed");
      }
      database->maxEntries = MAX_ENTRIES;
      database->numEntries = 0;
      sem_init(&database->lock, 1, 1);
      for (size_t i = 0; i < database->maxEntries; i++) {
        sem_init(&database->entries[i].lock, 1, 1);
      }
    } else if (!metadata_created &&
               errno == EEXIST) {  // TODO better handle condition where
                                   // metadata didn't exist but we do
      // get shmid of existing database
      database_shmid = shmget(database_key, sizeof(Database<T>), 0);
    } else {
      // catch errors that aren't "already exists"
      throw std::system_error(errno, std::generic_category(),
                              "Creating database shared memory segment failed");
    }

//    std::cout << "Database shmid: " << database_shmid << std::endl;

    // attach database_ to shared memory segment for database in read-only mode
    // if the password hash does not match
    database_ = static_cast<Database<T> *>(
        shmat(database_shmid, nullptr,
              passwordHash != metadata_->passwordHash ? SHM_RDONLY : 0));
  };

  // Destroys the shared database. If this is the last process that is using
  // the database, it will be cleaned up from shared memory as well.
  ~SharedDatabase() = default;

  // Returns a copy of the element at the given index.
  T get(size_t index) const {
    if (index >= database_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    auto lock = acquireSem(&database_->entries[index].lock);
    // Return a copy of the data
    return database_->entries[index].data;
  }

  // Clear all elements in the database
  void clear() {
    auto countLock = acquireSem(&database_->lock);
    // lock all entries
    for (size_t i = 0; i < database_->numEntries; i++) {
      auto entryLock = acquireSem(&database_->entries[i].lock);
      database_->entries[i].data = {};
    }
    database_->numEntries = 0;
  }

  // Returns a shared pointer to the element at the given index. The database is
  // locked while the pointer is in use, and unlocked when the pointer is
  // destroyed.
  [[nodiscard]] std::shared_ptr<T> at(size_t index) {
    // check if index is within bounds
    if (index >= database_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    auto lock = acquireSem(&database_->entries[index].lock);
    // Return a shared pointer to the data, with a custom deleter that unlocks
    // the entry
    return std::shared_ptr<T>(&database_->entries[index].data, [lock](T *data) {
      // do nothing, the lock will be released when the shared pointer is
      // destroyed
    });
  };

  // Deletes the element at the given index.
  void erase(size_t index) {
    // check if index is within bounds
    if (index >= database_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    // decrement numEntries
    auto countLock = acquireSem(&database_->lock);
    std::vector<std::shared_ptr<sem_t>> locks;
    // lock all entries after the one being deleted
    for (size_t i = index; i < database_->numEntries; i++) {
      locks.push_back(acquireSem(&database_->entries[i].lock));
    }
    // shift all entries after the one being deleted down by one
    for (size_t i = index; i < database_->numEntries - 1; i++) {
      database_->entries[i].data = database_->entries[i + 1].data;
    }
    database_->numEntries--;
  };

  // Adds an element to the end of the database.
  void push_back(T data) {
    if (database_->numEntries >= database_->maxEntries) {
      throw std::out_of_range("Database is full");
    }
    auto countLock = acquireSem(&database_->lock);
    auto entryLock =
        acquireSem(&database_->entries[database_->numEntries].lock);
    database_->entries[database_->numEntries].data = data;
    database_->numEntries++;
  };

  // Sets the element at the given index to the given data.
  void set(size_t index, T data) {
    // check if index is within bounds
    if (index >= database_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    auto lock = acquireSem(&database_->entries[index].lock);
    database_->entries[index].data = data;
  };

  // Returns the number of elements in the database.
  [[nodiscard]] size_t size() const {
//    std::cout << "size()" << std::endl;
    auto lock = acquireSem(&database_->lock);
    return database_->numEntries;
  };

  // Returns a shared pointer to the size of the database.
  //
  // The consumer cannot edit the size of the database, but can read it. This
  // also guarantees that the size of the database will not change while the
  // consumer is holding the shared pointer.
  [[nodiscard]] std::shared_ptr<size_t> smartSize() const {
//    std::cout << "smartSize()" << std::endl;
    auto lock = acquireSem(&database_->lock);
    return std::shared_ptr<size_t>(
        &database_->numEntries, [lock](size_t *size) {
          std::cout << "smartSize() deleter" << std::endl;
          // do nothing, but hold a reference to the lock in the deleter so that
          // it doesn't get destroyed before the shared pointer
        });
  }

  // Returns the maximum number of elements in the database.
  [[nodiscard]] size_t maxSize() const {
    auto countLock = acquireSem(&database_->lock);
    return database_->maxEntries;
  };

 private:
  // Database metadata
  DatabaseMetadata *metadata_;

  // Database
  Database<T> *database_;

  // Locks a semaphore, returning a handle that automatically unlocks it when
  // destroyed.
  [[nodiscard]] std::shared_ptr<sem_t> acquireSem(sem_t *semaphore) const {
    // Get a lock on the database entry. Da a sem_timedwait() on it and
    // throw an exception if it times out.

    auto timeout_time = std::chrono::system_clock::now() + LOCK_TIMEOUT;
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
    return std::shared_ptr<sem_t>(semaphore, [](sem_t *sem) {
//      std::cout << "Releasing lock" << std::endl;
      if (sem_post(sem) == -1) {
        throw std::system_error(errno, std::generic_category(),
                                "Failed to release exclusive lock");
      }
    });
  };
};

#endif  // ASSIGNMENT_1_SHAREDDATABASE_H
