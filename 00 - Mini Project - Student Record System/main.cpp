#include <iostream>
#include <string>
#include <random>
#include <limits>
#include "StudentBST.h"

using namespace std;

// রেন্ডম ইউনিক রোল নম্বর জেনারেটর (১০০১ থেকে ৯৯৯৯ এর মধ্যে)
int generateUniqueRoll(const StudentBST& bst) {
    static random_device rd;
    static mt19937 gen(rd());
    uniform_int_distribution<int> dist(1001, 9999);

    int roll;
    do {
        roll = dist(gen);
    } while (bst.exists(roll)); // নিশ্চিত করে যে রোলটি BST-তে আগে থেকেই নেই

    return roll;
}

// ইনপুট বাফার ক্লিয়ার করার হেল্পার
void clearInput() {
    cin.clear();
    cin.ignore(numeric_limits<streamsize>::max(), '\n');
}

int main() {
    StudentBST bst;

    // প্রাথমিক স্যাম্পল রেকর্ড দিয়ে ইনিশিয়ালাইজেশন
    bst.insert(Student("Sajib Ahmed", 1050, "CSE207 Data Structures", 92.5, 3.90));
    bst.insert(Student("Afsana Mim", 1020, "CSE207 Data Structures", 88.0, 3.80));
    bst.insert(Student("Tanvir Ahmed", 1080, "CSE207 Data Structures", 85.5, 3.75));
    bst.insert(Student("Sadia Islam", 1010, "CSE207 Data Structures", 95.0, 4.00));
    bst.insert(Student("Rakib Hossain", 1065, "CSE207 Data Structures", 78.0, 3.50));

    int choice;
    do {
        cout << "\n=======================================================\n";
        cout << "   STUDENT RECORD MANAGEMENT SYSTEM (BST Data Structure)\n";
        cout << "      East West University - CSE207 Lab Project\n";
        cout << "=======================================================\n";
        cout << " Current Status: " << bst.getTotalStudents() << " Students | Tree Height: " << bst.getHeight() << "\n";
        cout << "-------------------------------------------------------\n";
        cout << " 1. Insert Student Record (Auto Unique Roll / Manual)\n";
        cout << " 2. Search Student Record by Roll Number\n";
        cout << " 3. Delete Student Record by Roll Number\n";
        cout << " 4. Show All Student Records (Inorder Traversal - Sorted)\n";
        cout << " 5. Show Tree Traversals (Preorder / Inorder / Postorder)\n";
        cout << " 6. Exit System\n";
        cout << "=======================================================\n";
        cout << " Enter your choice (1-6): ";

        if (!(cin >> choice)) {
            cout << "\n[!] Invalid input! Please enter a number between 1 and 6.\n";
            clearInput();
            continue;
        }

        switch (choice) {
            case 1: {
                cout << "\n--- [1] INSERT STUDENT RECORD ---\n";
                int rollChoice;
                int rollNumber = 0;
                cout << " Roll Number Selection:\n";
                cout << "   1. Automatically generate random unique Roll Number\n";
                cout << "   2. Enter custom Roll Number manually\n";
                cout << " Choice (1/2): ";
                cin >> rollChoice;

                if (rollChoice == 1) {
                    rollNumber = generateUniqueRoll(bst);
                    cout << " >> Generated Unique Roll Number: " << rollNumber << "\n";
                } else {
                    cout << " Enter Roll Number: ";
                    cin >> rollNumber;
                    if (bst.exists(rollNumber)) {
                        cout << "\n[Error] Roll Number " << rollNumber << " already exists in BST! Insertion aborted.\n";
                        break;
                    }
                }

                clearInput();
                string name, course;
                double marks, cgpa;

                cout << " Enter Student Name: ";
                getline(cin, name);

                cout << " Enter Course Name: ";
                getline(cin, course);

                cout << " Enter Total Marks: ";
                while (!(cin >> marks)) {
                    cout << " Invalid input. Enter numeric marks: ";
                    clearInput();
                }

                cout << " Enter CGPA (0.00 - 4.00): ";
                while (!(cin >> cgpa) || cgpa < 0.0 || cgpa > 4.0) {
                    cout << " Invalid CGPA. Enter again: ";
                    clearInput();
                }

                Student newStudent(name, rollNumber, course, marks, cgpa);
                if (bst.insert(newStudent)) {
                    cout << "\n[Success] Student record successfully inserted into BST!\n";
                }
                break;
            }

            case 2: {
                cout << "\n--- [2] SEARCH STUDENT RECORD ---\n";
                if (bst.isEmpty()) {
                    cout << "[Notice] The BST is currently empty.\n";
                    break;
                }
                int searchRoll;
                cout << " Enter Roll Number to Search: ";
                cin >> searchRoll;

                Student* found = bst.search(searchRoll);
                if (found != nullptr) {
                    cout << "\n[Record Found in BST]:\n";
                    found->display();
                } else {
                    cout << "\n[!] Record Not Found: No student exists with Roll Number " << searchRoll << ".\n";
                }
                break;
            }

            case 3: {
                cout << "\n--- [3] DELETE STUDENT RECORD ---\n";
                if (bst.isEmpty()) {
                    cout << "[Notice] The BST is currently empty.\n";
                    break;
                }
                int deleteRoll;
                cout << " Enter Roll Number to Delete: ";
                cin >> deleteRoll;

                if (bst.deleteStudent(deleteRoll)) {
                    cout << "\n[Success] Record for Roll " << deleteRoll << " successfully deleted from BST.\n";
                } else {
                    cout << "\n[!] Record Not Found: Cannot delete non-existent Roll Number " << deleteRoll << ".\n";
                }
                break;
            }

            case 4: {
                cout << "\n--- [4] SHOW ALL RECORDS (INORDER TRAVERSAL) ---\n";
                cout << "Note: Inorder Traversal (Left -> Root -> Right) produces ascending sorted order.\n";
                bst.showInOrder();
                break;
            }

            case 5: {
                cout << "\n--- [5] BST TRAVERSAL SELECTION ---\n";
                cout << " 1. Preorder Traversal  (Root -> Left -> Right)\n";
                cout << " 2. Inorder Traversal   (Left -> Root -> Right - Sorted)\n";
                cout << " 3. Postorder Traversal (Left -> Right -> Root)\n";
                cout << " Choice (1-3): ";
                int tChoice;
                cin >> tChoice;
                if (tChoice == 1) {
                    bst.showPreOrder();
                } else if (tChoice == 2) {
                    bst.showInOrder();
                } else if (tChoice == 3) {
                    bst.showPostOrder();
                } else {
                    cout << "[!] Invalid traversal choice!\n";
                }
                break;
            }

            case 6:
                cout << "\nThank you for using the Student Record Management System!\n";
                break;

            default:
                cout << "\n[!] Invalid choice! Please select an option between 1 and 6.\n";
        }
    } while (choice != 6);

    return 0;
}
