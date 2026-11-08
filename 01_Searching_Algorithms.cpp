// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 1: Searching Algorithms
// a. Linear Search
// b. Binary Search
// =============================================================================

#include <iostream>
using namespace std;

// a. Linear Search
void linearSearch(int data[], int length, int item) {
    bool found = false;
    int loc = -1;

    for (int i = 0; i < length; i++) {
        if (item == data[i]) {
            found = true;
            loc = i;
            break;
        }
    }

    if (found == true) {
        cout << "[Linear Search] Item is found in location: " << loc << endl;
    } else {
        cout << "[Linear Search] Item is not found" << endl;
    }
}

// b. Binary Search (Requires sorted array)
void binarySearch(int data[], int length, int item) {
    int low = 0;
    int high = length - 1;
    bool found = false;
    int loc = -1;

    while (low <= high) {
        int mid = (low + high) / 2;
        if (item == data[mid]) {
            found = true;
            loc = mid;
            break;
        }
        if (item < data[mid]) {
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    if (found == true) {
        cout << "[Binary Search] Item is found in location: " << loc << endl;
    } else {
        cout << "[Binary Search] Item is not found" << endl;
    }
}

int main() {
    int data[] = {12, 25, 33, 49, 50, 60, 72};
    int length = sizeof(data) / sizeof(data[0]);

    cout << "Array elements: ";
    for (int i = 0; i < length; i++) cout << data[i] << " ";
    cout << "\n\n";

    // Test Search for existing and non-existing items
    linearSearch(data, length, 33);
    linearSearch(data, length, 99);

    cout << endl;

    binarySearch(data, length, 60);
    binarySearch(data, length, 100);

    return 0;
}
