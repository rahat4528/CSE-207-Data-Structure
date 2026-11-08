// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 7: Queue Operations
// Implement operations on Linear Queue and Circular Queue:
// a. enqueue – insert an element onto the queue
// b. dequeue – delete the first item from the queue
// c. print – display all the items from the queue
// =============================================================================

#include <iostream>
using namespace std;

const int CAPACITY = 10;

// -----------------------------------------------------------------------------
// 1. Linear Queue Implementation
// -----------------------------------------------------------------------------
class LinearQueue {
private:
    int queue[CAPACITY];
    int front;
    int rear;

public:
    LinearQueue() {
        front = 0;
        rear = 0;
    }

    // a. enqueue
    void enqueue(int item) {
        if (rear == CAPACITY) {
            cout << "[Linear Queue] The queue is full" << endl;
        } else {
            queue[rear] = item;
            rear = rear + 1;
            cout << "Enqueued: " << item << endl;
        }
    }

    // b. dequeue
    void dequeue() {
        if (front == rear) {
            cout << "[Linear Queue] The queue is empty" << endl;
        } else {
            cout << "Dequeued: " << queue[front] << endl;
            front = front + 1;
        }
    }

    // c. print
    void print() {
        if (front == rear) {
            cout << "[Linear Queue] The queue is empty" << endl;
            return;
        }
        cout << "Linear Queue items: ";
        for (int i = front; i < rear; i++) {
            cout << queue[i] << " ";
        }
        cout << endl;
    }
};

// -----------------------------------------------------------------------------
// 2. Circular Queue Implementation
// -----------------------------------------------------------------------------
class CircularQueue {
private:
    int queue[CAPACITY];
    int front;
    int rear;
    int count;

public:
    CircularQueue() {
        front = 0;
        rear = 0;
        count = 0;
    }

    // a. enqueue
    void enqueue(int item) {
        if (count == CAPACITY) {
            cout << "[Circular Queue] The queue is full" << endl;
        } else {
            queue[rear] = item;
            rear = (rear + 1) % CAPACITY;
            count++;
            cout << "Enqueued: " << item << endl;
        }
    }

    // b. dequeue
    void dequeue() {
        if (count == 0) {
            cout << "[Circular Queue] The queue is empty" << endl;
        } else {
            cout << "Dequeued: " << queue[front] << endl;
            front = (front + 1) % CAPACITY;
            count--;
        }
    }

    // c. print
    void print() {
        if (count == 0) {
            cout << "[Circular Queue] The queue is empty" << endl;
            return;
        }
        cout << "Circular Queue items: ";
        for (int i = front; i < (count + front); i++) {
            cout << queue[i % CAPACITY] << " ";
        }
        cout << endl;
    }
};

int main() {
    cout << "========================================\n";
    cout << "       TESTING LINEAR QUEUE             \n";
    cout << "========================================\n";
    LinearQueue lq;
    lq.enqueue(10);
    lq.enqueue(20);
    lq.enqueue(30);
    lq.print();

    lq.dequeue();
    lq.print();

    cout << "\n========================================\n";
    cout << "      TESTING CIRCULAR QUEUE            \n";
    cout << "========================================\n";
    CircularQueue cq;
    cq.enqueue(100);
    cq.enqueue(200);
    cq.enqueue(300);
    cq.print();

    cq.dequeue();
    cq.print();

    cq.enqueue(400);
    cq.print();

    return 0;
}
