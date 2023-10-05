#ifndef ASSIGNMENT_1_STUDENTINFO_H
#define ASSIGNMENT_1_STUDENTINFO_H

// must avoid using pointers in the struct
struct StudentInfo {
  char name[51];
  int id;
  char address[251];
  char phone[11];
};

#endif  // ASSIGNMENT_1_STUDENTINFO_H
