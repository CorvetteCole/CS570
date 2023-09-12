#include <gtest/gtest.h>

#include <cstdlib>

#include "SharedDatabase.h"

// must avoid using pointers in the struct
struct StudentInfo {
  char name[50];
  int id;
  char address[250];
  char phone[10];
};

std::string DB_PASSWORD = "password";
constexpr int DB_ID = 175;

// function to generate a number n of random students
std::vector<StudentInfo> generateRandomStudents(int n) {
  std::vector<StudentInfo> students;
  // init random
  srand(time(nullptr));
  for (int i = 0; i < n; i++) {
    StudentInfo student{};
    // generate random id
    student.id = rand() % 1000;
    // generate random name, make sure it is null terminated and the correct
    // length
    for (int j = 0; j < sizeof(student.name) - 1; j++) {
      student.name[j] = rand() % 26 + 'a';
    }
    student.name[sizeof(student.name) - 1] = '\0';
    // generate random address, make sure it is null terminated and the correct
    // length
    for (int j = 0; j < sizeof(student.address) - 1; j++) {
      student.address[j] = rand() % 26 + 'a';
    }
    student.address[sizeof(student.address) - 1] = '\0';
    // generate random phone, make sure it is null terminated and the correct
    // length
    for (int j = 0; j < sizeof(student.phone) - 1; j++) {
      student.phone[j] = rand() % 10 + '0';
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
    db = new SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID);
  }

  ~SharedDatabaseTest() override {
    delete db;
    auto metadata_key = ftok(".", DB_ID + METADATA_OFFSET);
    auto database_key = ftok(".", DB_ID + DATABASE_OFFSET);
    auto metadata_shmid = shmget(metadata_key, sizeof(DatabaseMetadata), 0);
    auto database_shmid =
        shmget(database_key, sizeof(Database<StudentInfo>), 0);
    shmctl(metadata_shmid, IPC_RMID, nullptr);
    shmctl(database_shmid, IPC_RMID, nullptr);
  }

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

  SharedDatabase<StudentInfo> *db;
};

TEST_F(SharedDatabaseTest, coexist_basic) {
  fillStudents();
  auto db2 = SharedDatabase<StudentInfo>(DB_PASSWORD, DB_ID);
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
}

TEST_F(SharedDatabaseTest, start_empty) { EXPECT_EQ(db->size(), 0); }

TEST_F(SharedDatabaseTest, push_back) {
  fillStudents();
  // verify that we can't add more than the max size
  EXPECT_THROW(db->push_back(generateRandomStudents(1).front()),
               std::out_of_range);
}

TEST_F(SharedDatabaseTest, set) {
  fillStudents();
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
  // test that we can't get an index that is out of range
  EXPECT_THROW(db->get(db->size()), std::out_of_range);
}

TEST_F(SharedDatabaseTest, at) {
  fillStudents();
  auto testStudent = generateRandomStudents(1).front();
  auto db_student = db->at(0);
  EXPECT_EQ(db_student->id, testStudent.id);
  EXPECT_STREQ(db_student->name, testStudent.name);
  EXPECT_STREQ(db_student->address, testStudent.address);
  EXPECT_STREQ(db_student->phone, testStudent.phone);

  // test that semaphore is held and released as expected
  EXPECT_THROW(db->get(0), std::system_error);
  db_student.reset();
  EXPECT_NO_THROW(db->get(0));

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