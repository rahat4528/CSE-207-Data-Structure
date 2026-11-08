// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 4: Singly Linked List Operations
// a. Insert a node at the start
// b. Insert a node at the end
// c. Insert a node at any position other than start and end
// d. Delete the start node
// e. Delete the last node
// f. Delete any node other than start and end nodes
// g. Traverse the singly linked list to print all the values
// =============================================================================

#include <iostream>
using namespace std;

class Node {
public:
    int data;
    Node* next;

    Node(int val = 0) {
        data = val;
        next = nullptr;
    }
};

class SinglyLinkedList {
public:
    Node* start;

    SinglyLinkedList() {
        start = nullptr;
    }

    int nodeCounter() {
        int count = 0;
        Node* temp = start;
        while (temp != nullptr) {
            count++;
            temp = temp->next;
        }
        return count;
    }

    // a. Insert a node at the start of a singly linked list
    void insertFirst(int val) {
        Node* newNode = new Node(val);
        if (start == nullptr) {
            start = newNode;
        } else {
            newNode->next = start;
            start = newNode;
        }
    }

    // b. Insert a node at the end of a singly linked list
    void insertLast(int val) {
        Node* newNode = new Node(val);
        if (start == nullptr) {
            start = newNode;
        } else {
            Node* temp = start;
            while (temp->next != nullptr) {
                temp = temp->next;
            }
            temp->next = newNode;
        }
    }

    // c. Insert a node at any position other than start and end
    void insertMiddle(int val, int position) {
        if (start == nullptr) {
            start = new Node(val);
            return;
        }
        if (position > 1 && position <= nodeCounter()) {
            Node* newNode = new Node(val);
            Node* temp = start;
            int ctr = 1;
            while (ctr < position - 1) {
                temp = temp->next;
                ctr++;
            }
            newNode->next = temp->next;
            temp->next = newNode;
        } else {
            cout << "Invalid Position" << endl;
        }
    }

    // d. Delete the start node of a singly linked list
    void deleteFirst() {
        if (start == nullptr) {
            cout << "No node to delete" << endl;
        } else {
            Node* toDelete = start;
            start = start->next;
            delete toDelete;
        }
    }

    // e. Delete the end node of a singly linked list
    void deleteLast() {
        if (start == nullptr) {
            cout << "No node to delete" << endl;
        } else if (start->next == nullptr) {
            delete start;
            start = nullptr;
        } else {
            Node* temp = start;
            while (temp->next->next != nullptr) {
                temp = temp->next;
            }
            delete temp->next;
            temp->next = nullptr;
        }
    }

    // f. Delete any node other than start and end nodes
    void deleteMiddle(int position) {
        if (start == nullptr) {
            cout << "No node to delete" << endl;
        } else {
            if (position > 1 && position <= nodeCounter()) {
                Node* temp = start;
                int ctr = 1;
                while (ctr < position - 1) {
                    temp = temp->next;
                    ctr++;
                }
                Node* toDelete = temp->next;
                temp->next = temp->next->next;
                delete toDelete;
            } else {
                cout << "Invalid Position" << endl;
            }
        }
    }

    // g. Traverse the singly linked list to print all the values
    void printList() {
        Node* temp = start;
        if (temp == nullptr) {
            cout << "List is empty" << endl;
            return;
        }
        while (temp != nullptr) {
            cout << temp->data << " -> ";
            temp = temp->next;
        }
        cout << "NULL" << endl;
    }
};

int main() {
    SinglyLinkedList list;

    cout << "--- a. Insert at Start (20, 10) ---\n";
    list.insertFirst(20);
    list.insertFirst(10);
    list.printList();

    cout << "\n--- b. Insert at End (40, 50) ---\n";
    list.insertLast(40);
    list.insertLast(50);
    list.printList();

    cout << "\n--- c. Insert at Position 3 (30) ---\n";
    list.insertMiddle(30, 3);
    list.printList();

    cout << "\n--- d. Delete Start Node ---\n";
    list.deleteFirst();
    list.printList();

    cout << "\n--- e. Delete Last Node ---\n";
    list.deleteLast();
    list.printList();

    cout << "\n--- f. Delete Middle Node at Position 2 ---\n";
    list.deleteMiddle(2);
    list.printList();

    cout << "\n--- g. Final Traversal ---\n";
    list.printList();

    return 0;
}
