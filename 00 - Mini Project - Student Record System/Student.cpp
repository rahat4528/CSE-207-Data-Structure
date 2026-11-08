#include "Student.h"
#include <iomanip>

// Default Constructor: প্রাথমিক মান সেট করে
Student::Student() {
    name = "";
    rollNumber = 0;
    course = "";
    totalMarks = 0.0;
    cgpa = 0.0;
}

// Parameterized Constructor: অবজেক্ট তৈরির সময়ই সব মান দিয়ে ইনিশিয়ালাইজ করে
Student::Student(std::string name, int rollNumber, std::string course, double totalMarks, double cgpa) {
    this->name = name;
    this->rollNumber = rollNumber;
    this->course = course;
    this->totalMarks = totalMarks;
    this->cgpa = cgpa;
}

// Getters: প্রাইভেট ফিল্ডগুলোর মান পাওয়ার জন্য
std::string Student::getName() const {
    return name;
}

int Student::getRollNumber() const {
    return rollNumber;
}

std::string Student::getCourse() const {
    return course;
}

double Student::getTotalMarks() const {
    return totalMarks;
}

double Student::getCgpa() const {
    return cgpa;
}

// Setters: প্রাইভেট ফিল্ডগুলোর মান আপডেট করার জন্য
void Student::setName(const std::string& name) {
    this->name = name;
}

void Student::setRollNumber(int rollNumber) {
    this->rollNumber = rollNumber;
}

void Student::setCourse(const std::string& course) {
    this->course = course;
}

void Student::setTotalMarks(double totalMarks) {
    this->totalMarks = totalMarks;
}

void Student::setCgpa(double cgpa) {
    this->cgpa = cgpa;
}

// Display: একজন শিক্ষার্থীর সম্পূর্ণ রেকর্ড সুন্দর ফরম্যাটে দেখায়
void Student::display() const {
    std::cout << "--------------------------------------------\n";
    std::cout << " Roll Number : " << rollNumber << "\n";
    std::cout << " Name        : " << name << "\n";
    std::cout << " Course      : " << course << "\n";
    std::cout << " Total Marks : " << std::fixed << std::setprecision(2) << totalMarks << "\n";
    std::cout << " CGPA        : " << std::fixed << std::setprecision(2) << cgpa << "\n";
    std::cout << "--------------------------------------------\n";
}
