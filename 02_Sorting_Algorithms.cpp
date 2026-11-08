// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 2: Sorting Algorithms
// a. Bubble Sort
// b. Selection Sort
// =============================================================================

#include <iostream>
using namespace std;

// a. Bubble Sort
void bubbleSort(int data[], int length) {
    for (int i = 0; i < length - 1; i++) {
        for (int j = 0; j < length - 1 - i; j++) {
            if (data[j] > data[j + 1]) {
                int temp = data[j];
                data[j] = data[j + 1];
                data[j + 1] = temp;
            }
        }
    }
}

// b. Selection Sort
void selectionSort(int data[], int length) {
    for (int i = 0; i < length - 1; i++) {
        int minIndex = i;
        for (int j = i + 1; j < length; j++) {
            if (data[j] < data[minIndex]) {
                minIndex = j;
            }
        }
        int temp = data[minIndex];
        data[minIndex] = data[i];
        data[i] = temp;
    }
}

void printArray(int data[], int length) {
    for (int i = 0; i < length; i++) {
        cout << data[i] << " ";
    }
    cout << endl;
}

int main() {
    int arr1[] = {64, 34, 25, 12, 22, 11, 90};
    int n1 = sizeof(arr1) / sizeof(arr1[0]);

    cout << "Original Array for Bubble Sort: ";
    printArray(arr1, n1);

    bubbleSort(arr1, n1);
    cout << "Sorted Array using Bubble Sort: ";
    printArray(arr1, n1);

    cout << "\n--------------------------------------------\n";

    int arr2[] = {29, 10, 14, 37, 13};
    int n2 = sizeof(arr2) / sizeof(arr2[0]);

    cout << "Original Array for Selection Sort: ";
    printArray(arr2, n2);

    selectionSort(arr2, n2);
    cout << "Sorted Array using Selection Sort: ";
    printArray(arr2, n2);

    return 0;
}
