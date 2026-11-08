#include "StudentBST.h"
#include <iostream>
#include <algorithm>

// Constructor
StudentBST::StudentBST() {
    root = nullptr;
}

// Destructor: সম্পূর্ণ ট্রি ডিলিট করে মেমোরি ফ্রী করে
StudentBST::~StudentBST() {
    destroyTree(root);
}

void StudentBST::destroyTree(Node* current) {
    if (current != nullptr) {
        destroyTree(current->left);
        destroyTree(current->right);
        delete current;
    }
}

// -------------------------------------------------------------------------
// ১. INSERT OPERATION (BST ইনসার্ট অ্যালগরিদম)
// ছোট রোল বাঁয়ে, বড় রোল ডানে, ডুপ্লিকেট রোল রিজেক্ট করা হবে
// -------------------------------------------------------------------------
Node* StudentBST::insertHelper(Node* current, const Student& student, bool& isInserted) {
    if (current == nullptr) {
        isInserted = true;
        return new Node(student);
    }

    int newRoll = student.getRollNumber();
    int currentRoll = current->data.getRollNumber();

    if (newRoll < currentRoll) {
        current->left = insertHelper(current->left, student, isInserted);
    } 
    else if (newRoll > currentRoll) {
        current->right = insertHelper(current->right, student, isInserted);
    } 
    else {
        isInserted = false;
        std::cout << "\n[Error] Duplicate Roll Number: " << newRoll << " already exists in the system!\n";
    }

    return current;
}

bool StudentBST::insert(const Student& student) {
    bool isInserted = false;
    root = insertHelper(root, student, isInserted);
    return isInserted;
}

// -------------------------------------------------------------------------
// ২. SEARCH OPERATION (BST সার্চ অ্যালগরিদম - O(log N))
// -------------------------------------------------------------------------
Node* StudentBST::searchHelper(Node* current, int rollNumber) const {
    if (current == nullptr || current->data.getRollNumber() == rollNumber) {
        return current;
    }

    if (rollNumber < current->data.getRollNumber()) {
        return searchHelper(current->left, rollNumber);
    }

    return searchHelper(current->right, rollNumber);
}

Student* StudentBST::search(int rollNumber) const {
    Node* result = searchHelper(root, rollNumber);
    if (result != nullptr) {
        return &(result->data);
    }
    return nullptr;
}

bool StudentBST::exists(int rollNumber) const {
    return searchHelper(root, rollNumber) != nullptr;
}

// -------------------------------------------------------------------------
// ৩. DELETE OPERATION (BST ডিলিশনের ৩টি কেস হ্যান্ডেলিং)
// -------------------------------------------------------------------------
Node* StudentBST::findMinNode(Node* current) const {
    while (current && current->left != nullptr) {
        current = current->left;
    }
    return current;
}

Node* StudentBST::deleteHelper(Node* current, int rollNumber, bool& isDeleted) {
    if (current == nullptr) {
        isDeleted = false;
        return nullptr;
    }

    int currentRoll = current->data.getRollNumber();

    if (rollNumber < currentRoll) {
        current->left = deleteHelper(current->left, rollNumber, isDeleted);
    } 
    else if (rollNumber > currentRoll) {
        current->right = deleteHelper(current->right, rollNumber, isDeleted);
    } 
    else {
        isDeleted = true;

        // Case 1: Leaf Node (কোনো সন্তান নেই)
        if (current->left == nullptr && current->right == nullptr) {
            delete current;
            return nullptr;
        }
        // Case 2: শুধু ডান সন্তান আছে
        else if (current->left == nullptr) {
            Node* temp = current->right;
            delete current;
            return temp;
        }
        // Case 2: শুধু বাম সন্তান আছে
        else if (current->right == nullptr) {
            Node* temp = current->left;
            delete current;
            return temp;
        }
        // Case 3: দুটি সন্তানই আছে (Node with two children)
        else {
            Node* temp = findMinNode(current->right); // Inorder Successor
            current->data = temp->data;
            bool subDeleted = false;
            current->right = deleteHelper(current->right, temp->data.getRollNumber(), subDeleted);
        }
    }
    return current;
}

bool StudentBST::deleteStudent(int rollNumber) {
    bool isDeleted = false;
    root = deleteHelper(root, rollNumber, isDeleted);
    return isDeleted;
}

// -------------------------------------------------------------------------
// ৪. TRAVERSALS (Inorder, Preorder, Postorder)
// -------------------------------------------------------------------------
void StudentBST::inorderHelper(Node* current) const {
    if (current == nullptr) return;
    inorderHelper(current->left);
    current->data.display();
    inorderHelper(current->right);
}

void StudentBST::preorderHelper(Node* current) const {
    if (current == nullptr) return;
    current->data.display();
    preorderHelper(current->left);
    preorderHelper(current->right);
}

void StudentBST::postorderHelper(Node* current) const {
    if (current == nullptr) return;
    postorderHelper(current->left);
    postorderHelper(current->right);
    current->data.display();
}

void StudentBST::showAll() const {
    showInOrder();
}

void StudentBST::showInOrder() const {
    if (root == nullptr) {
        std::cout << "\n[Notice] No student records found. The BST is empty!\n";
        return;
    }
    std::cout << "\n========== INORDER TRAVERSAL (Sorted Ascending by Roll Number) ==========\n";
    inorderHelper(root);
    std::cout << "=========================================================================\n";
}

void StudentBST::showPreOrder() const {
    if (root == nullptr) {
        std::cout << "\n[Notice] No student records found. The BST is empty!\n";
        return;
    }
    std::cout << "\n================= PREORDER TRAVERSAL (Root -> Left -> Right) =============\n";
    preorderHelper(root);
    std::cout << "=========================================================================\n";
}

void StudentBST::showPostOrder() const {
    if (root == nullptr) {
        std::cout << "\n[Notice] No student records found. The BST is empty!\n";
        return;
    }
    std::cout << "\n================ POSTORDER TRAVERSAL (Left -> Right -> Root) ============\n";
    postorderHelper(root);
    std::cout << "=========================================================================\n";
}

// -------------------------------------------------------------------------
// ৫. TREE METRICS (Total Nodes and Tree Height)
// -------------------------------------------------------------------------
int StudentBST::countNodes(Node* current) const {
    if (current == nullptr) return 0;
    return 1 + countNodes(current->left) + countNodes(current->right);
}

int StudentBST::getTotalStudents() const {
    return countNodes(root);
}

int StudentBST::calculateHeight(Node* current) const {
    if (current == nullptr) return 0;
    return 1 + std::max(calculateHeight(current->left), calculateHeight(current->right));
}

int StudentBST::getHeight() const {
    return calculateHeight(root);
}

bool StudentBST::isEmpty() const {
    return root == nullptr;
}
