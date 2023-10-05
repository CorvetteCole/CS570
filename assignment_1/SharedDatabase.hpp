#ifndef ASSIGNMENT_1_SHAREDDATABASE_HPP
#define ASSIGNMENT_1_SHAREDDATABASE_HPP

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
#include <thread>
#include <unordered_map>
#include <vector>

#include "Utilities.h"

using namespace std::chrono_literals;

constexpr static int METADATA_OFFSET = 0;
constexpr static int DATABASE_OFFSET = 1;
constexpr static int DB_VERSION = 2;
constexpr static int MAX_ENTRIES = 50;

struct DatabaseMetadata {
  // Must be locked before editing the metadata
  sem_t lock;
  uint32_t version;
  std::size_t passwordHash;
  int readerCount;
  size_t numEntries;
  size_t maxEntries;
};

template <typename T>
struct Database {
  // Must be locked before editing the database
  sem_t lock;
  T entries[MAX_ENTRIES];
};

template <typename T>
//  Implements a database of objects, stored in shared memory, that can be
//  accessed by multiple processes concurrently.
class SharedDatabase {
 public:
  // Creates a new shared database with the given identifier. If a database
  // with the same identifier already exists, and the password matches, the
  // database will be opened in read-write mode. Otherwise, it will be opened in
  // read-only mode. If the database does not exist, it will be created.
  explicit SharedDatabase(std::string &password, int id = 0,
                          const std::chrono::milliseconds semTimeout = 5s,
                          const std::chrono::milliseconds semSleep = 0s,
                          const bool clean = false)
      : semTimeout_(semTimeout), clean_(clean), semSleep_(semSleep) {
    // this is a terrible hash function, but it works for the purposes of this
    // project
    std::size_t passwordHash = std::hash<std::string>{}(password);

    // generate key_t using ftok() from current executable file path and the id
    auto metadataKey = ftok(".", id + METADATA_OFFSET);
    auto databaseKey = ftok(".", id + DATABASE_OFFSET);

    // create shared memory segment for metadata
    metadataShmid_ = shmget(metadataKey, sizeof(DatabaseMetadata),
                            IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    if (metadataShmid_ != 1 || errno == EEXIST) {
      if (errno == EEXIST) {
        // get shmid of existing database metadata
        metadataShmid_ = shmget(metadataKey, sizeof(DatabaseMetadata), 0);
      }
      // attach metadata_ to shared memory segment
      metadata_ =
          static_cast<DatabaseMetadata *>(shmat(metadataShmid_, nullptr, 0));
      if (metadata_ == reinterpret_cast<DatabaseMetadata *>(-1)) {
        throw std::system_error(
            errno, std::generic_category(),
            "Attaching to database metadata shared memory segment failed");
      }

      // if the metadata segment was created, initialize it
      if (errno != EEXIST) {
        sem_init(&metadata_->lock, 1, 1);
        auto metadataLock = acquireSem(&metadata_->lock);
        metadata_->version = DB_VERSION;
        metadata_->passwordHash = passwordHash;
        metadata_->readerCount = 0;
        metadata_->numEntries = 0;
        metadata_->maxEntries = MAX_ENTRIES;
        metadataLock.reset();
      }

    } else {
      throw std::system_error(
          errno, std::generic_category(),
          "Creating/attaching database metadata shared memory segment failed");
    }

    if (metadata_->version != DB_VERSION) {
      throw std::runtime_error("Database version mismatch");
    }

    readOnly_ = passwordHash != metadata_->passwordHash;

    databaseShmid_ = shmget(databaseKey, sizeof(Database<T>),
                            IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

    if (databaseShmid_ != -1 || errno == EEXIST) {
      if (errno == EEXIST) {
        // get shmid of existing database
        databaseShmid_ = shmget(databaseKey, sizeof(Database<T>), 0);
      }

      // attach database_ to shared memory segment
      database_ = static_cast<Database<T> *>(shmat(databaseShmid_, nullptr, 0));
      if (database_ == reinterpret_cast<Database<T> *>(-1)) {
        throw std::system_error(
            errno, std::generic_category(),
            "Attaching to database shared memory segment failed");
      }
      if (errno != EEXIST) {
        sem_init(&database_->lock, 1, 1);
      }
    } else {
      throw std::system_error(
          errno, std::generic_category(),
          "Creating/attaching database shared memory segment failed");
    }
    //    std::cout << "Database shmid: " << databaseShmid << std::endl;
  };

  ~SharedDatabase() {
    if (clean_) {
      shmctl(metadataShmid_, IPC_RMID, nullptr);
      shmctl(databaseShmid_, IPC_RMID, nullptr);
    }
  };

  // Returns a copy of the element at the given index.
  [[nodiscard]] T get(size_t index) const {
    if (index >= metadata_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    auto readLock = getReadLock();
    // Return a copy of the data
    return database_->entries[index];
  }

  // Clear all elements in the database
  void clear() {
    auto writeLock = getWriteLock();
    for (size_t i = 0; i < metadata_->numEntries; i++) {
      database_->entries[i] = {};
    }
    metadata_->numEntries = 0;
  }

  // Returns a shared pointer to the element at the given index. The database is
  // locked while the pointer is in use, and unlocked when the pointer is
  // destroyed.
  [[nodiscard]] std::shared_ptr<T> at(size_t index) {
    // check if index is within bounds
    if (index >= metadata_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    auto writeLock = getWriteLock();
    // Return a shared pointer to the data, with a deleter that holds a
    // reference to the lock pointer until
    return {&database_->entries[index], [writeLock](T *data) {}};
  };

  // Deletes the element at the given index.
  void erase(size_t index) {
    // check if index is within bounds
    if (index >= metadata_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    // decrement numEntries
    auto writeLock = getWriteLock();
    // shift all entries after the one being deleted down by one
    for (size_t i = index; i < metadata_->numEntries - 1; i++) {
      database_->entries[i] = database_->entries[i + 1];
    }
    metadata_->numEntries--;
  };

  // Adds an element to the end of the database.
  void push_back(T data) {
    if (metadata_->numEntries >= metadata_->maxEntries) {
      throw std::out_of_range("Database is full");
    }
    auto writeLock = getWriteLock();
    database_->entries[metadata_->numEntries] = data;
    metadata_->numEntries++;
  };

  // Sets the element at the given index to the given data.
  void set(size_t index, T data) {
    // check if index is within bounds
    if (index >= metadata_->numEntries) {
      throw std::out_of_range("Index out of bounds");
    }

    auto writeLock = getWriteLock();
    database_->entries[index] = data;
  };

  // Returns the number of elements in the database.
  [[nodiscard]] size_t size() const { return metadata_->numEntries; };

  // Returns a shared pointer to the size of the database.
  //
  // This guarantees that the size of the database will not change while the
  // consumer is holding the shared pointer.
  [[nodiscard]] std::shared_ptr<size_t> smartSize() const {
    auto readLock = getReadLock();
    return {&metadata_->numEntries, [readLock](size_t *size) {
              std::cout << "smartSize() deleter" << std::endl;
              // do nothing, but hold a reference to the lock in the deleter so
              // that it doesn't get destroyed before the shared pointer
            }};
  }

  // Returns the maximum number of elements in the database.
  [[nodiscard]] size_t maxSize() const { return metadata_->maxEntries; };

 private:
  DatabaseMetadata *metadata_;
  Database<T> *database_;
  bool readOnly_;
  bool clean_;
  std::chrono::milliseconds semTimeout_;
  std::chrono::milliseconds semSleep_;
  int metadataShmid_;
  int databaseShmid_;

  // readLock(). Returns something that automatically unlocks the semaphore when
  // destroyed. (out of scope)
  [[nodiscard]] std::shared_ptr<void> getReadLock() const {
    runWithLock(
        &metadata_->lock, [&]() { metadata_->readerCount++; }, semTimeout_);
    std::this_thread::sleep_for(semSleep_);
    // can't just init a shared_ptr<void> with nullptr
    return {nullptr, [this](void *) {
              runWithLock(
                  &metadata_->lock, [&]() { metadata_->readerCount--; },
                  semTimeout_);
            }};
  }

  [[nodiscard]] std::shared_ptr<void> getWriteLock() const {
    // throw an error if we are in read-only mode
    if (readOnly_) {
      throw std::runtime_error("Database is read-only");
    }

    auto currentTime = std::chrono::system_clock::now();
    // while there are readers, wait for them to finish. Use the semTimeout_
    // to prevent deadlock
    while (metadata_->readerCount > 0) {
      if (std::chrono::system_clock::now() - currentTime > semTimeout_) {
        throw std::runtime_error("Timed out waiting for readers to finish");
      }
      std::this_thread::sleep_for(1ms);  // so we don't spinlock
    }
    auto metadataLock = acquireSem(&metadata_->lock);
    auto databaseLock = acquireSem(&database_->lock);
    std::this_thread::sleep_for(semSleep_);
    return {nullptr, [metadataLock, databaseLock](void *) {}};
  }
};

#endif  // ASSIGNMENT_1_SHAREDDATABASE_HPP
