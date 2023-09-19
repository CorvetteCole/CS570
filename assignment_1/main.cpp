#include <argparse/argparse.hpp>
#include <cstring>
#include <fstream>

#include "SharedDatabase.hpp"

// must avoid using pointers in the struct
// struct StudentInfo {
//  char name[50];
//  int id;
//  char address[250];
//  char phone[10];
//};

int main(int argc, char **argv) {
  argparse::ArgumentParser program("assignment_1");
  program.add_argument("-p", "--password")
      .help(
          "password for the database, enables write-mode when matching "
          "existing")
      .default_value("1234");
  program.add_argument("-c", "--clean")
      .help("cleanup shared memory on exit")
      .default_value(false)
      .implicit_value(true);
  program.add_argument("-l", "--load")
      .help("load the database from the specified file");
  program.add_argument("-o", "--output")
      .help("save the database to the specified file");
  program.add_argument("-q", "--query")
      .help("query the database for the specified student ID")
      .scan<'i', int>();
  program.add_argument("-s", "--sleep")
      .help(
          "sleep for the specified number of seconds after requesting a "
          "semaphore")
      .default_value(0)
      .scan<'i', int>();

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error &err) {
    std::cout << err.what() << std::endl;
    std::cout << program;
    exit(1);
  }

  auto password = program.get<std::string>("--password");
  auto clean = program.get<bool>("--clean");
  auto sleep = program.get<int>("--sleep");

  auto load = program.present("--load");
  auto output = program.present("--output");

  auto db = SharedDatabase(password, 0, std::chrono::seconds(300),
                           std::chrono::seconds(sleep), clean);

  if (load) {
    if (load.has_value()) {
      auto load_file = load.value();
      std::ifstream file(load_file);
      if (!file.is_open()) {
        std::cerr << "Failed to open file: " << load_file << std::endl;
        exit(1);
      }

      std::string line;
      while (std::getline(file, line)) {
        StudentInfo student{};
        std::strncpy(student.name, line.c_str(), 51);
        std::getline(file, line);
        student.id = std::stoi(line);
        std::getline(file, line);
        std::strncpy(student.address, line.c_str(), 251);
        std::getline(file, line);
        std::strncpy(student.phone, line.c_str(), 11);
        db.push_back(student);
      }
    }
  }

  if (program.present<int>("--query")) {
    auto query_id = program.get<int>("--query");
    // request user to input student id, then search for it in the database
    StudentInfo queryStudent{};
    for (int i = 0; i < db.size(); i++) {
      auto student = db.get(i);
      if (student.id == query_id) {
        queryStudent = student;
        break;
      }
    }
    if (queryStudent.id == 0) {
      std::cout << "Student not found" << std::endl;
    } else {
      std::cout << queryStudent.name << std::endl;
      std::cout << queryStudent.id << std::endl;
      std::cout << queryStudent.address << std::endl;
      std::cout << queryStudent.phone << std::endl;
    }
  }

  if (output) {
    if (output.value() != "console") {
      std::ofstream file(output.value());
      if (!file.is_open()) {
        std::cerr << "Failed to open file: " << output.value() << std::endl;
        exit(1);
      }
      for (int i = 0; i < db.size(); i++) {
        auto student = db.get(i);
        file << student.name << std::endl;
        file << student.id << std::endl;
        file << student.address << std::endl;
        file << student.phone << std::endl;
      }
      file.close();
    }
  }

  bool shouldExit = false;
  while (!shouldExit) {
    std::cout << "1. Add new student" << std::endl;
    std::cout << "2. Delete student" << std::endl;
    std::cout << "3. Update student" << std::endl;
    std::cout << "4. Print students" << std::endl;
    std::cout << "5. Exit" << std::endl;
    std::cout << "Enter a command: ";
    int command;
    std::cin >> command;
    switch (command) {
      case 1: {
        std::cout << "Enter student name: ";
        std::string name;
        std::cin >> name;
        std::cout << "Enter student id: ";
        int id;
        std::cin >> id;
        std::cout << "Enter student address: ";
        std::string address;
        std::cin >> address;
        std::cout << "Enter student phone: ";
        std::string phone;
        std::cin >> phone;
        StudentInfo student{};
        std::strncpy(student.name, name.c_str(), 51);
        student.id = id;
        std::strncpy(student.address, address.c_str(), 251);
        std::strncpy(student.phone, phone.c_str(), 11);
        db.push_back(student);
        break;
      }
      case 2: {
        std::cout << "Enter student id: " << std::endl;
        int id;
        std::cin >> id;
        // find student with id
        bool erased = false;
        for (int i = 0; i < db.size(); i++) {
          auto student = db.get(i);
          if (student.id == id) {
            db.erase(i);
            erased = true;
            break;
          }
        }
        if (!erased) {
          std::cout << "Student not found!" << std::endl;
        }
        break;
      }
      case 3: {
        std::cout << "Enter student id: " << std::endl;
        int id;
        std::cin >> id;

        // find student with id
        int studentIndex = -1;
        for (int i = 0; i < db.size(); i++) {
          auto student = db.get(i);
          if (student.id == id) {
            studentIndex = i;
            break;
          }
        }

        if (studentIndex == -1) {
          std::cout << "Student not found!" << std::endl;
          break;
        }

        auto student = db.at(studentIndex);
        std::cout << "Enter student name: " << std::endl;
        std::string name;
        std::cin >> name;
        std::cout << "Enter student address: " << std::endl;
        std::string address;
        std::cin >> address;
        std::cout << "Enter student phone: " << std::endl;
        std::string phone;
        std::cin >> phone;
        std::strncpy(student->name, name.c_str(), 51);
        std::strncpy(student->address, address.c_str(), 251);
        std::strncpy(student->phone, phone.c_str(), 11);
        break;
      }

      case 4: {
        std::cout << "== Students ==" << std::endl;
        for (int i = 0; i < db.size(); i++) {
          auto student = db.get(i);
          std::cout << "Name: " << student.name << std::endl;
          std::cout << "ID: " << student.id << std::endl;
          std::cout << "Address: " << student.address << std::endl;
          std::cout << "Phone: " << student.phone << std::endl << std::endl;
        }
        break;
      }
      default: {
        shouldExit = true;
        break;
      }
    }
  }
}
