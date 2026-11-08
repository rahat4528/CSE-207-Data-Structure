#ifndef STUDENT_BST_H
#define STUDENT_BST_H

#include "Node.h"

class StudentBST {
private:
    Node* root; // BST-এর মূল রুট পয়েন্টার

    // Private Helper Recursive Methods (BST Core Algorithms)
    Node* insertHelper(Node* current, const Student& student, bool& isInserted);
    Node* searchHelper(Node* current, int rollNumber) const;
    Node* deleteHelper(Node* current, int rollNumber, bool& isDeleted);
    Node* findMinNode(Node* current) const;

    // Traversals
    void inorderHelper(Node* current) const;
    void preorderHelper(Node* current) const;
    void postorderHelper(Node* current) const;

    // Tree Metrics
    int countNodes(Node* current) const;
    int calculateHeight(Node* current) const;

    // Memory Cleanup
    void destroyTree(Node* current);

public:
    // Constructor & Destructor
    StudentBST();
    ~StudentBST();

    // Core BST Operations
    bool insert(const Student& student);
    Student* search(int rollNumber) const;
    bool deleteStudent(int rollNumber);
    bool exists(int rollNumber) const;
    bool isEmpty() const;

    // Traversal Methods
    void showAll() const;        // Inorder (Default: Sorted ascending by roll)
    void showInOrder() const;    // Left -> Root -> Right
    void showPreOrder() const;   // Root -> Left -> Right
    void showPostOrder() const;  // Left -> Right -> Root

    // Statistics
    int getTotalStudents() const;
    int getHeight() const;
};

#endif // STUDENT_BST_H
