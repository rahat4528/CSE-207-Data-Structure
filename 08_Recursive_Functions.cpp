// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 8: Recursive Functions
// a. Calculate the factorial of an integer number
// b. Calculate the Fibonacci number, given the index value
// c. Print the numbers 1 to 100 in reverse order
// d. Print the message “JAVA” 100 times
// e. Calculate whether a given word is palindrome or not
// =============================================================================

#include <iostream>
#include <string>

using namespace std;

// a. Factorial
int factorial(int n) {
    if (n == 0) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

// b. Fibonacci
int fib(int index) {
    if (index == 0) {
        return 0;
    } else if (index == 1) {
        return 1;
    } else {
        return fib(index - 1) + fib(index - 2);
    }
}

// c. Print numbers 1 to 100 in reverse order
void printNumber(int n) {
    if (n > 0) {
        cout << n << " ";
        printNumber(n - 1);
    }
}

// d. Print message "JAVA" 100 times
void printJava(int n) {
    if (n > 0) {
        cout << "Java ";
        printJava(n - 1);
    }
}

// e. Calculate whether a given word is palindrome or not
bool isPalindrome(string word) {
    if (word.length() <= 1) {
        return true;
    }
    if (word[0] != word[word.length() - 1]) {
        return false;
    }
    return isPalindrome(word.substr(1, word.length() - 2));
}

int main() {
    cout << "--- a. Factorial of 5 ---\n";
    cout << "5! = " << factorial(5) << endl;

    cout << "\n--- b. Fibonacci at index 6 ---\n";
    cout << "Fib(6) = " << fib(6) << endl;

    cout << "\n--- c. Print numbers 100 to 1 in reverse order ---\n";
    printNumber(100);
    cout << endl;

    cout << "\n--- d. Print message 'Java' 100 times ---\n";
    printJava(100);
    cout << endl;

    cout << "\n--- e. Palindrome Check ---\n";
    string w1 = "madam";
    string w2 = "hello";
    cout << "'" << w1 << "' is palindrome? " << (isPalindrome(w1) ? "Yes" : "No") << endl;
    cout << "'" << w2 << "' is palindrome? " << (isPalindrome(w2) ? "Yes" : "No") << endl;

    return 0;
}
