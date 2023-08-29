#include "SharedDatabase.h"

template <class T>
SharedDatabase<T>::SharedDatabase(const std::string &id,
                                  std::string &password) {
  // TODO generate key_t using ftok() from id
  // Setup shared memory segment(s) to share:
  // - database (objects stored in shared memory as an array?)
  // - database password
  // - either use named semaphores or somehow share a shared_mutex

  // Need to handle creating this all if it doesn't exist, or opening it if it
  // does exist. So the first thing to do is check if a particular shared memory
  // segment exists whose key is derived from the id as mentioned above.


}