#ifndef NODE_H
#define NODE_H

#include "Student.h"

class Node {
public:
    Student data;       // এই নোডে সংরক্ষিত Student অবজেক্ট
    Node* left;         // বাম দিকের চাইল্ডের অ্যাড্রেস নির্দেশকারী পয়েন্টার (ছোট Roll Number)
    Node* right;        // ডান দিকের চাইল্ডের অ্যাড্রেস নির্দেশকারী পয়েন্টার (বড় Roll Number)

    // Constructor: একটি Student অবজেক্ট নিয়ে নতুন Node তৈরি করে
    Node(const Student& student);

    // Destructor: মেমোরি ক্লিনআপে সাহায্য করে
    ~Node();
};

#endif // NODE_H
