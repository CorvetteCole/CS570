#include "SharedDatabase.h"

#include <cstring>
#include <ctime>
#include <iostream>
#include <system_error>

template <typename T>
SharedDatabase<T>::SharedDatabase(std::string &password) {
  // this is a terrible hash function, but it works for the purposes of this
  // project
  std::size_t passwordHash = std::hash<std::string>{}(password);

  // generate key_t using ftok() from current executable file path and the id
  auto metadata_key = ftok(".", METADATA_MEM_ID);
  auto database_key = ftok(".", DATABASE_MEM_ID);

  // create shared memory segment for metadata
  auto metadata_shmid = shmget(metadata_key, sizeof(DatabaseMetadata),
                               IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);

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

  // TODO DB setup tasks
}

template <typename T>
SharedDatabase<T>::~SharedDatabase() {}

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
  auto lock = acquireSem(this->lock);
  return this->numEntries;
}

template <typename T>
std::shared_ptr<sem_t> SharedDatabase<T>::acquireSem(sem_t *semaphore) const {
  // Get a lock on the database entry. Da a sem_timedwait() on it and
  // throw an exception if it times out.
  timespec timeout{LOCK_TIMEOUT.count()};
  if (sem_timedwait(semaphore, &timeout) == -1) {
    throw std::system_error(errno, std::generic_category(),
                            "Failed to acquire exclusive lock");
  }

  // Return a shared pointer to the semaphore, with a custom deleter that
  // unlocks the semaphore
  return std::make_shared<sem_t>(semaphore, [](sem_t *lock) {
    std::cout << "unlocking semaphore" << std::endl;
    sem_post(lock);
  });
}