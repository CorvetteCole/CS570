#include <cassert>
#include <iostream>

#include "SharedDatabase.hpp"

struct StudentInfo {
  int id;
  int testId;
};

int main() {
  std::string password = "password";
  SharedDatabase<StudentInfo> db(password, 36);

  try {
    if (db.size() != 3) {
      std::cout << "Database is empty" << std::endl;
      StudentInfo student1 = {1, 3};
      StudentInfo student2 = {2, 2};
      StudentInfo student3 = {3, 1};
      assert(db.size() == 0);
      db.push_back(student1);
      assert(db.size() == 1);
      db.push_back(student2);
      assert(db.size() == 2);
      db.push_back(student3);
      assert(db.size() == 3);
    } else {
      std::cout << "Database is not empty" << std::endl;
    }

    for (int i = 0; i < db.size(); i++) {
      auto studentInfo = db.get(i);
      std::cout << "Student " << studentInfo.id << std::endl;
    }

    // erase student with id 2
    db.erase(1);
    std::cout << "########" << std::endl;
    for (int i = 0; i < db.size(); i++) {
      auto studentInfo = db.get(i);
      std::cout << "Student " << studentInfo.id << std::endl;
    }

    db.push_back({4, 4});
    std::cout << "########" << std::endl;
    for (int i = 0; i < db.size(); i++) {
      auto studentInfo = db.get(i);
      std::cout << "Student " << studentInfo.id << std::endl;
    }

    db.set(0, {5, 5});
    std::cout << "########" << std::endl;
    for (int i = 0; i < db.size(); i++) {
      auto studentInfo = db.get(i);
      std::cout << "Student " << studentInfo.id << std::endl;
    }

    std::cout << "##### shared pointer test" << std::endl;
    auto testStudent0 = db.at(0);
    std::cout << "Student " << testStudent0->id << std::endl;
    testStudent0->id = 6;
    std::cout << "Student " << testStudent0->id << std::endl;
    testStudent0.reset();
    std::cout << "########" << std::endl;

    auto size = db.smartSize();  // grab a shared pointer to the size so it doesn't change while we're iterating
    for (int i = 0; i < *size; i++) {
      auto studentInfo = db.get(i);
      std::cout << "Student " << studentInfo.id << std::endl;
    }
    size.reset();
    return 0;
  } catch (const std::exception& e) {
    std::cout << "Exception: " << e.what() << std::endl;
    return -1;
  }
}
