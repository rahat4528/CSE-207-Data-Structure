// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 6: Stack Applications
// a. Convert the infix expression to its equivalent postfix expression
// b. Evaluate the postfix expression
// =============================================================================

#include <iostream>
#include <stack>
#include <string>
#include <cmath>
#include <cctype>

using namespace std;

int precedence(char ch) {
    if (ch == '+' || ch == '-') return 1;
    if (ch == '*' || ch == '/') return 2;
    if (ch == '^') return 3;
    return -1;
}

// a. Convert the infix expression to its equivalent postfix expression
string infixToPostfix(string expression) {
    stack<char> st;
    string result = "";

    for (size_t i = 0; i < expression.length(); i++) {
        char ch = expression[i];

        if (ch == ' ') continue;

        // If the symbol is '(', push it onto the stack
        if (ch == '(') {
            st.push(ch);
        }
        // If the symbol is an operand, add it to postfix result
        else if (isalnum(ch)) {
            result.push_back(ch);
        }
        // If the symbol is ')', pop until '(' is found
        else if (ch == ')') {
            while (!st.empty() && st.top() != '(') {
                result.push_back(st.top());
                st.pop();
            }
            // Remove '(' from stack
            if (!st.empty() && st.top() == '(') {
                st.pop();
            }
        }
        // If the symbol is an operator
        else {
            if (st.empty() || st.top() == '(') {
                st.push(ch);
            } else {
                while (!st.empty() && precedence(ch) <= precedence(st.top()) && st.top() != '(') {
                    result.push_back(st.top());
                    st.pop();
                }
                st.push(ch);
            }
        }
    }

    // Pop all remaining operators from the stack
    while (!st.empty()) {
        result.push_back(st.top());
        st.pop();
    }

    return result;
}

// b. Evaluate the postfix expression
int evaluatePostfix(string postFixString) {
    stack<int> st;

    for (size_t i = 0; i < postFixString.length(); i++) {
        // If the character is a digit (operand)
        if (isdigit(postFixString[i])) {
            st.push(postFixString[i] - '0'); // converting character to integer
        } else if (postFixString[i] == ' ') {
            continue;
        }
        // If the character is an operator
        else {
            char op = postFixString[i];
            int operand2 = st.top(); st.pop();
            int operand1 = st.top(); st.pop();
            int result = 0;

            switch (op) {
                case '+':
                    result = operand1 + operand2;
                    break;
                case '-':
                    result = operand1 - operand2;
                    break;
                case '*':
                    result = operand1 * operand2;
                    break;
                case '/':
                    result = operand1 / operand2;
                    break;
                case '^':
                    result = static_cast<int>(pow(operand1, operand2));
                    break;
            }
            st.push(result);
        }
    }
    return st.top();
}

int main() {
    cout << "--- a. Infix to Postfix Conversion ---\n";
    string infix = "A+(B*C-(D/E^F)*G)*H";
    cout << "Infix:   " << infix << endl;
    cout << "Postfix: " << infixToPostfix(infix) << endl;

    cout << "\n--- b. Postfix Expression Evaluation ---\n";
    string postfix = "623+-382/+*2^3+";
    cout << "Postfix Expression: " << postfix << endl;
    cout << "Evaluation Result:  " << evaluatePostfix(postfix) << endl;

    return 0;
}
