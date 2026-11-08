#include "Node.h"

// Constructor: যখনই নতুন নোড তৈরি হবে, বাম এবং ডান পয়েন্টার প্রাথমিকভাবে nullptr (খালি) থাকবে
Node::Node(const Student& student) {
    this->data = student;
    this->left = nullptr;
    this->right = nullptr;
}

// Destructor: নোড ডিলিট হওয়ার সময় কল হয়
Node::~Node() {
    // পয়েন্টারগুলো স্বয়ংক্রিয়ভাবে ক্লিন হবে
}
