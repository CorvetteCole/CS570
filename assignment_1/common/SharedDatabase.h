//
// Created by coleg on 8/29/23.
//

#ifndef ASSIGNMENT_1_SHAREDDATABASE_H
#define ASSIGNMENT_1_SHAREDDATABASE_H

#include <string>

class SharedDatabase {
 public:
  SharedDatabase(const std::string &identifier);

  ~SharedDatabase();

  const std::string &getIdentifier() const;

 private:
  std::string identifier;
};

#endif  // ASSIGNMENT_1_SHAREDDATABASE_H
