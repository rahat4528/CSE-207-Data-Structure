#ifndef STUDENT_H
#define STUDENT_H

#include <string>
#include <iostream>

class Student {
private:
    std::string name;
    int rollNumber;     // BST-এর Unique Key হিসেবে কাজ করবে
    std::string course;
    double totalMarks;
    double cgpa;

public:
    // Default Constructor
    Student();

    // Parameterized Constructor
    Student(std::string name, int rollNumber, std::string course, double totalMarks, double cgpa);

    // Getter Methods (ডাটা পড়ার জন্য)
    std::string getName() const;
    int getRollNumber() const;
    std::string getCourse() const;
    double getTotalMarks() const;
    double getCgpa() const;

    // Setter Methods (ডাটা পরিবর্তন করার জন্য)
    void setName(const std::string& name);
    void setRollNumber(int rollNumber);
    void setCourse(const std::string& course);
    void setTotalMarks(double totalMarks);
    void setCgpa(double cgpa);

    // Record সুন্দরভাবে কনসোলে প্রিন্ট করার মেথড
    void display() const;
};

#endif // STUDENT_H
