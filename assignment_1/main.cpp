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
      .help(
          "load the database from the specified file, or from stdin if not "
          "provided");
  program.add_argument("-o", "--output")
      .help(
          "save the database to the specified file or to stdout if not "
          "provided")
//      .default_value(std::string("-"))
      .implicit_value(std::string("console"));
  program.add_argument("-q", "--query")
      .help("query the database for the specified student ID")
      .scan<'i', int>();
  program.add_argument("-s", "--sleep")
      .help(
          "sleep for the specified number of seconds after requesting a "
          "semaphore")
      .default_value(2)
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

  auto db = SharedDatabase(password, 0, std::chrono::seconds(5),
                           std::chrono::seconds(sleep), clean);

  if (load) {
    if (load.has_value()) {
      auto load_file = load.value();
      // TODO: read from file with format:
      // John Blakeman
      // 111223333
      // 560 Southbrook Dr. Lexington,  KY 40522
      // 8591112222

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

    } else {
      // TODO: read from stdin
    }
  }

  if (program.present("--query")) {
    auto query_id = program.get<int>("--query");
    // TODO: do query
  }

  if (output) {
    if (output.value() != "console") {
      // TODO: output students to file in format:
      // John Blakeman
      // 111223333
      // 560 Southbrook Dr. Lexington,  KY 40522
      // 8591112222

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

    } else {
      // TODO: print students as JSON to console
      for (int i = 0; i < db.size(); i++) {
        auto student = db.get(i);
        std::cout << student.name << std::endl;
        std::cout << student.id << std::endl;
        std::cout << student.address << std::endl;
        std::cout << student.phone << std::endl;
      }
    }
  }

  // TODO: change
}
