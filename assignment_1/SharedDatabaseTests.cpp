#include <gtest/gtest.h>

#include <condition_variable>
#include <future>
#include <mutex>
#include <random>
#include <thread>

#include "SharedDatabase.hpp"

using namespace std::chrono_literals;

// must avoid using pointers in the struct
//struct StudentInfo {
//  char name[50];
//  int id;
//  char address[250];
//  char phone[10];
//};

std::string DB_PASSWORD = "password";
constexpr int DB_ID = 120;

// function to generate a number n of random students
std::vector<StudentInfo> generateRandomStudents(int n) {
  std::vector<StudentInfo> students;
  // init random
  std::random_device rd;
  std::mt19937 gen(rd());
  for (int i = 0; i < n; i++) {
    StudentInfo student{};
    // generate random id
    student.id = gen() % 1000;
    // generate random name, make sure it is null terminated and the correct
    // length
    for (int j = 0; j < sizeof(student.name) - 1; j++) {
      student.name[j] = gen() % 26 + 'a';
    }
    student.name[sizeof(student.name) - 1] = '\0';
    // generate random address, make sure it is null terminated and the correct
    // length
    for (int j = 0; j < sizeof(student.address) - 1; j++) {
      student.address[j] = gen() % 26 + 'a';
    }
    student.address[sizeof(student.address) - 1] = '\0';
    // generate random phone, make sure it is null terminated and the correct
    // length
    for (int j = 0; j < sizeof(student.phone) - 1; j++) {
      student.phone[j] = gen() % 10 + '0';
    }
    student.phone[sizeof(student.phone) - 1] = '\0';

    students.push_back(student);
  }
  return students;
}

class SharedDatabaseTest : public ::testing::Test {
 protected:
  SharedDatabaseTest() {
    auto metadata_key = ftok(".", DB_ID + METADATA_OFFSET);
    auto database_key = ftok(".", DB_ID + DATABASE_OFFSET);

    auto metadata_shmid = shmget(metadata_key, sizeof(DatabaseMetadata), 0);
    //    std::cout << "Deleting existing shared memory segments" << std::endl;
    shmctl(metadata_shmid, IPC_RMID, nullptr);
    auto database_shmid =
        shmget(database_key, sizeof(Database<StudentInfo>), 0);
    shmctl(database_shmid, IPC_RMID, nullptr);
    db = new SharedDatabase(DB_PASSWORD, DB_ID,
                            50ms, 0ms, true);  //<StudentInfo>(DB_PASSWORD, DB_ID, 50ms);
  }

  ~SharedDatabaseTest() override {
    delete db;
  }

  //  void SetUp() override { fillStudents(); }

  void fillStudents() {
    EXPECT_EQ(db->size(), 0);
    auto students = generateRandomStudents(db->maxSize());
    for (auto student : students) {
      db->push_back(student);
    }

    EXPECT_EQ(db->size(), db->maxSize());
    // verify that the students were added correctly
    for (int i = 0; i < db->size(); i++) {
      auto db_student = db->get(i);
      EXPECT_EQ(db_student.id, students[i].id);
      EXPECT_STREQ(db_student.name, students[i].name);
      EXPECT_STREQ(db_student.address, students[i].address);
      EXPECT_STREQ(db_student.phone, students[i].phone);
    }
  }

  SharedDatabase *db;  //<StudentInfo> *db;
};

// TEST_F(SharedDatabaseTest, simultaneous_read) {
//   fillStudents();
//   auto firstStudent = db->at(0);  // acquire lock on first student
//   auto writeDB = SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID, 5s);
//
//   std::future<StudentInfo> studentFuture1 =
//       std::async(std::launch::async, [&]() {
//         auto db = SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID, 5s);
//         return db.get(0);
//       });
//
//   std::future<StudentInfo> studentFuture2 =
//       std::async(std::launch::async, [&]() {
//         auto db = SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID, 5s);
//         return db.get(0);
//       });
//
//   // now that the threads are spun up, release the lock on the first student
//   firstStudent.reset();
//
//   // continuously change the value of the first student until both threads
//   are
//   // complete
//   while (studentFuture1.wait_for(0ms) != std::future_status::ready ||
//          studentFuture2.wait_for(0ms) != std::future_status::ready) {
//     writeDB.set(0, generateRandomStudents(1).front());
//   }
//
//   // verify that the students are the same for both threads
//   auto student1 = studentFuture1.get();
//   auto student2 = studentFuture2.get();
//   EXPECT_EQ(student1.id, student2.id);
//   EXPECT_STREQ(student1.name, student2.name);
//   EXPECT_STREQ(student1.address, student2.address);
//   EXPECT_STREQ(student1.phone, student2.phone);
// }

// TEST_F(SharedDatabaseTest, write_wait_for_read) {
//   fillStudents();
//   auto writeDB = SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID, 0s);
//   const auto initialStudent = db->get(0);
//   const auto newStudent = generateRandomStudents(1).front();
//   auto studentLock = db->at(0);  // acquire lock on first student
//
//   std::future<StudentInfo> studentReadFuture =
//       std::async(std::launch::async, []() {
//         auto db = SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID, 5s);
//         return db.get(0);
//       });
//
//   studentLock.reset();
//   // wait for the read thread to acquire the lock
//   while (studentReadFuture.wait_for(0ms) != std::future_status::timeout) {
//   }
//   EXPECT_THROW(writeDB.set(0, newStudent), std::system_error);
//
//   auto readStudent = studentReadFuture.get();
//
//   EXPECT_EQ(readStudent.id, initialStudent.id);
//   EXPECT_STREQ(readStudent.name, initialStudent.name);
//   EXPECT_STREQ(readStudent.address, initialStudent.address);
//   EXPECT_STREQ(readStudent.phone, initialStudent.phone);
// }

// TEST_F(SharedDatabaseTest, write_wait_for_write) {
//   // where the first one is the initial student and the last two will be
//   written in sequence auto randomStudents = generateRandomStudents(3);
//
//
//
// }

// TEST_F(SharedDatabaseTest, read_wait_for_write) {
//   fillStudents();
//
//   const auto initialStudent = db->get(0);
//   const auto newStudent = generateRandomStudents(1).front();
//   std::future<void> studentWriteFuture =
//       std::async(std::launch::async, [&]() { db->set(0, newStudent); });
//   const auto readStudent = db->get(0);
//
//   EXPECT_EQ(readStudent.id, newStudent.id);
//   EXPECT_STREQ(readStudent.name, newStudent.name);
//   EXPECT_STREQ(readStudent.address, newStudent.address);
//   EXPECT_STREQ(readStudent.phone, newStudent.phone);
// }

TEST_F(SharedDatabaseTest, shared_memory_matches) {
  fillStudents();
  auto db2 = SharedDatabase(DB_PASSWORD, DB_ID);
  EXPECT_EQ(db2.size(), db->size());
  // verify that the students are the same on both databases
  for (int i = 0; i < db->size(); i++) {
    auto db_student = db->get(i);
    auto db2_student = db2.get(i);
    EXPECT_EQ(db_student.id, db2_student.id);
    EXPECT_STREQ(db_student.name, db2_student.name);
    EXPECT_STREQ(db_student.address, db2_student.address);
    EXPECT_STREQ(db_student.phone, db2_student.phone);
  }

  // verify that changes on db2 are reflected on db
  auto testStudent = generateRandomStudents(1).front();
  db2.set(0, testStudent);
  auto db_student = db->get(0);
  EXPECT_EQ(db_student.id, testStudent.id);
  EXPECT_STREQ(db_student.name, testStudent.name);
  EXPECT_STREQ(db_student.address, testStudent.address);
  EXPECT_STREQ(db_student.phone, testStudent.phone);
}

TEST_F(SharedDatabaseTest, readonly) {
  fillStudents();
  std::string incorrectPassword = "nooooo";
  auto db2 = SharedDatabase(incorrectPassword, DB_ID);
  EXPECT_EQ(db2.size(), db->size());
  // verify that the students are the same on both databases
  for (int i = 0; i < db->size(); i++) {
    auto db_student = db->get(i);
    auto db2_student = db2.get(i);
    EXPECT_EQ(db_student.id, db2_student.id);
    EXPECT_STREQ(db_student.name, db2_student.name);
    EXPECT_STREQ(db_student.address, db2_student.address);
    EXPECT_STREQ(db_student.phone, db2_student.phone);
  }

  // attempt to modify the database
  EXPECT_THROW(db2.set(0, generateRandomStudents(1).front()),
               std::runtime_error);
}

TEST_F(SharedDatabaseTest, start_empty) { EXPECT_EQ(db->size(), 0); }

TEST_F(SharedDatabaseTest, push_back) {
  fillStudents();
  // verify that we can't add more than the max size
  EXPECT_THROW(db->push_back(generateRandomStudents(1).front()),
               std::out_of_range);
}

TEST_F(SharedDatabaseTest, set) {
  std::cout << "fillStudents" << std::endl;
  fillStudents();
  std::cout << "fillStudents done" << std::endl;
  auto testStudent = generateRandomStudents(1).front();
  db->set(0, testStudent);
  auto db_student = db->get(0);
  EXPECT_EQ(db_student.id, testStudent.id);
  EXPECT_STREQ(db_student.name, testStudent.name);
  EXPECT_STREQ(db_student.address, testStudent.address);
  EXPECT_STREQ(db_student.phone, testStudent.phone);

  // now test that we can't set an index that is out of range
  EXPECT_THROW(db->set(db->size(), testStudent), std::out_of_range);
}

TEST_F(SharedDatabaseTest, erase) {
  fillStudents();
  std::vector<StudentInfo> referenceStudents;
  for (int i = 0; i < db->size(); i++) {
    referenceStudents.push_back(db->get(i));
  }
  referenceStudents.erase(referenceStudents.begin());
  db->erase(0);
  EXPECT_EQ(db->size(), db->maxSize() - 1);
  EXPECT_EQ(db->size(), referenceStudents.size());
  // verify that the students were moved correctly
  for (int i = 0; i < db->size(); i++) {
    auto db_student = db->get(i);
    EXPECT_EQ(db_student.id, referenceStudents[i].id);
    EXPECT_STREQ(db_student.name, referenceStudents[i].name);
    EXPECT_STREQ(db_student.address, referenceStudents[i].address);
    EXPECT_STREQ(db_student.phone, referenceStudents[i].phone);
  }

  // now test that we can't erase an index that is out of range
  EXPECT_THROW(db->erase(db->size()), std::out_of_range);
}

TEST_F(SharedDatabaseTest, get) {
  fillStudents();
  // test that we can't get an index that is out of range
  EXPECT_THROW(db->get(db->size()), std::out_of_range);
  // test that a copy of the student is returned
  auto db_student = db->get(0);
  // change the student
  db_student.id = 100;

  // verify that the student was not changed in the database
  EXPECT_NE(db->get(0).id, db_student.id);

  db->at(0)->id = 347;

  EXPECT_NE(db_student.id, db->get(0).id);
}

TEST_F(SharedDatabaseTest, at) {
  fillStudents();
  auto testStudent = generateRandomStudents(1).front();
  auto db_student = db->at(0);
  db_student->id = testStudent.id;
  strcpy(db_student->name, testStudent.name);
  strcpy(db_student->address, testStudent.address);
  strcpy(db_student->phone, testStudent.phone);

  EXPECT_EQ(db_student->id, testStudent.id);
  EXPECT_STREQ(db_student->name, testStudent.name);
  EXPECT_STREQ(db_student->address, testStudent.address);
  EXPECT_STREQ(db_student->phone, testStudent.phone);

  // test that semaphore is held and released as expected
  EXPECT_THROW(db->get(0), std::system_error);
  db_student.reset();
  EXPECT_NO_THROW(db->get(0));

  auto db_student2 = db->get(0);
  EXPECT_EQ(db_student2.id, testStudent.id);
  EXPECT_STREQ(db_student2.name, testStudent.name);
  EXPECT_STREQ(db_student2.address, testStudent.address);
  EXPECT_STREQ(db_student2.phone, testStudent.phone);

  // now test that we can't set an index that is out of range
  EXPECT_THROW(db->at(db->size()), std::out_of_range);
}

TEST_F(SharedDatabaseTest, clear) {
  fillStudents();
  db->clear();
  EXPECT_EQ(db->size(), 0);

  // ensure it respects the semaphore
  fillStudents();
  auto db_student = db->at(0);
  EXPECT_THROW(db->clear(), std::system_error);
}