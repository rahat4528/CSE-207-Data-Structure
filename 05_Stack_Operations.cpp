// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 5: Stack Operations
// a. push – insert an item onto the stack
// b. pop – delete the last item from the stack
// c. peek – return the last item from the stack
// d. print – display all the items of the stack
// =============================================================================

#include <iostream>
using namespace std;

class Stack {
private:
    int arr[10];
    int top;

public:
    Stack() {
        top = 0;
    }

    // a. push – insert an item onto the stack
    void push(int item) {
        if (top == 10) {
            cout << "stack is full" << endl;
        } else {
            arr[top] = item;
            top++;
            cout << "Pushed: " << item << endl;
        }
    }

    // b. pop – delete the last item from the stack
    void pop() {
        if (top == 0) {
            cout << "Stack is empty" << endl;
        } else {
            top--;
            cout << "Popped: " << arr[top] << endl;
        }
    }

    // c. peek – return the last item from the stack
    int peek() {
        if (top == 0) {
            cout << "Stack is empty" << endl;
            return -1;
        }
        return arr[top - 1];
    }

    // d. print – display all the items of the stack
    void print() {
        if (top == 0) {
            cout << "Stack is empty" << endl;
            return;
        }
        cout << "Stack items (Top to Bottom):" << endl;
        for (int i = top - 1; i >= 0; i--) {
            cout << arr[i] << endl;
        }
    }
};

int main() {
    Stack s;

    cout << "--- a. Push Items ---\n";
    s.push(10);
    s.push(20);
    s.push(30);

    cout << "\n--- d. Display Items ---\n";
    s.print();

    cout << "\n--- c. Peek Top Item ---\n";
    cout << "Top element: " << s.peek() << endl;

    cout << "\n--- b. Pop Item ---\n";
    s.pop();

    cout << "\n--- Stack After Pop ---\n";
    s.print();

    return 0;
}
