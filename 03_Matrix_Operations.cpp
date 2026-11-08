// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 3: Matrix Operations
// a. Matrix Addition
// b. Matrix Subtraction
// c. Matrix Multiplication
// d. Matrix Transpose
// =============================================================================

#include <iostream>
using namespace std;

const int MAX = 10;

void printMatrix(int matrix[MAX][MAX], int rows, int cols) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            cout << matrix[i][j] << "\t";
        }
        cout << endl;
    }
}

// a. Matrix Addition
void matrixAddition(int A[MAX][MAX], int B[MAX][MAX], int C[MAX][MAX], int row, int col) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            C[i][j] = A[i][j] + B[i][j];
        }
    }
}

// b. Matrix Subtraction
void matrixSubtraction(int A[MAX][MAX], int B[MAX][MAX], int C[MAX][MAX], int row, int col) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            C[i][j] = A[i][j] - B[i][j];
        }
    }
}

// c. Matrix Multiplication
void matrixMultiplication(int A[MAX][MAX], int B[MAX][MAX], int C[MAX][MAX], int row1, int col1, int col2) {
    for (int i = 0; i < row1; i++) {
        for (int j = 0; j < col2; j++) {
            int sum = 0;
            for (int k = 0; k < col1; k++) {
                sum = sum + A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}

// d. Matrix Transpose
void matrixTranspose(int A[MAX][MAX], int C[MAX][MAX], int row, int col) {
    for (int i = 0; i < row; i++) {
        for (int j = 0; j < col; j++) {
            C[j][i] = A[i][j];
        }
    }
}

int main() {
    int A[MAX][MAX] = {
        {1, 2, 3},
        {4, 5, 6}
    };

    int B[MAX][MAX] = {
        {7, 8, 9},
        {1, 2, 3}
    };

    int C[MAX][MAX];

    cout << "--- Matrix A (2x3) ---\n";
    printMatrix(A, 2, 3);

    cout << "\n--- Matrix B (2x3) ---\n";
    printMatrix(B, 2, 3);

    // a. Addition
    cout << "\n--- a. Matrix Addition (A + B) ---\n";
    matrixAddition(A, B, C, 2, 3);
    printMatrix(C, 2, 3);

    // b. Subtraction
    cout << "\n--- b. Matrix Subtraction (A - B) ---\n";
    matrixSubtraction(A, B, C, 2, 3);
    printMatrix(C, 2, 3);

    // c. Multiplication
    int M1[MAX][MAX] = {
        {1, 2},
        {3, 4}
    };
    int M2[MAX][MAX] = {
        {2, 0},
        {1, 2}
    };
    cout << "\n--- c. Matrix Multiplication (M1 x M2) ---\n";
    matrixMultiplication(M1, M2, C, 2, 2, 2);
    printMatrix(C, 2, 2);

    // d. Transpose
    cout << "\n--- d. Matrix Transpose of A ---\n";
    matrixTranspose(A, C, 2, 3);
    printMatrix(C, 3, 2);

    return 0;
}
