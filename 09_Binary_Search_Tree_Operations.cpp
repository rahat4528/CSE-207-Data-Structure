// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 9: Binary Search Tree (BST) Operations
// a. Insert an element into the BST
// b. Delete an element from the BST (Handling Leaf, 1 Child, 2 Children)
// c. Search an element from the BST
// d. Print the nodes using Pre-order, In-order, Post-order & Level-order
// =============================================================================

#include <iostream>
#include <queue>
using namespace std;

class Node {
public:
    int data;
    Node* left;
    Node* right;

    Node(int val = 0) {
        data = val;
        left = nullptr;
        right = nullptr;
    }
};

class BinarySearchTree {
public:
    Node* root;

    BinarySearchTree() {
        root = nullptr;
    }

    Node* getNode(int item) {
        return new Node(item);
    }

    // a. Insert an element into the BST
    Node* insertRecursive(Node* temp, int item) {
        if (temp == nullptr) {
            temp = getNode(item);
            return temp;
        } else if (item < temp->data) {
            temp->left = insertRecursive(temp->left, item);
        } else if (item > temp->data) {
            temp->right = insertRecursive(temp->right, item);
        }
        return temp;
    }

    void insert(int item) {
        root = insertRecursive(root, item);
    }

    // Helper for finding minimum value (Inorder Successor)
    int minimum(Node* temp) {
        while (temp->left != nullptr) {
            temp = temp->left;
        }
        return temp->data;
    }

    // b. Delete an element from the BST
    Node* deleteDataRecursive(Node* temp, int item) {
        // Base case: tree is empty or item not found
        if (temp == nullptr) {
            return nullptr;
        }

        // Step 1: Search for the node to delete
        if (item < temp->data) {
            temp->left = deleteDataRecursive(temp->left, item);
        } else if (item > temp->data) {
            temp->right = deleteDataRecursive(temp->right, item);
        } else {
            // Step 2: Node found — handle deletion

            // Case 1: No child (leaf node)
            if (temp->left == nullptr && temp->right == nullptr) {
                delete temp;
                return nullptr;
            }
            // Case 2: One child (right child)
            if (temp->left == nullptr) {
                Node* rightChild = temp->right;
                delete temp;
                return rightChild;
            }
            // Case 2: One child (left child)
            else if (temp->right == nullptr) {
                Node* leftChild = temp->left;
                delete temp;
                return leftChild;
            }
            // Case 3: Two children
            // Find inorder successor (minimum in right subtree)
            int successorValue = minimum(temp->right);
            // Replace current node's data
            temp->data = successorValue;
            // Delete the inorder successor
            temp->right = deleteDataRecursive(temp->right, successorValue);
        }
        return temp;
    }

    void deleteNode(int item) {
        root = deleteDataRecursive(root, item);
    }

    // c. Search an element from the BST
    bool searchDataRecursive(Node* current, int item) {
        if (current == nullptr) {
            return false;
        } else if (item < current->data) {
            return searchDataRecursive(current->left, item);
        } else if (item > current->data) {
            return searchDataRecursive(current->right, item);
        } else {
            return true;
        }
    }

    bool search(int item) {
        return searchDataRecursive(root, item);
    }

    // d. Traversals
    void preOrderTraversal(Node* temp) {
        if (temp != nullptr) {
            cout << temp->data << " ";
            preOrderTraversal(temp->left);
            preOrderTraversal(temp->right);
        }
    }

    void inOrderTraversal(Node* temp) {
        if (temp != nullptr) {
            inOrderTraversal(temp->left);
            cout << temp->data << " ";
            inOrderTraversal(temp->right);
        }
    }

    void postOrderTraversal(Node* temp) {
        if (temp != nullptr) {
            postOrderTraversal(temp->left);
            postOrderTraversal(temp->right);
            cout << temp->data << " ";
        }
    }

    void levelOrderTraversal(Node* temp) {
        if (temp == nullptr) return;
        queue<Node*> q;
        q.push(temp);
        while (!q.empty()) {
            Node* current = q.front();
            q.pop();
            cout << current->data << " ";
            if (current->left != nullptr) q.push(current->left);
            if (current->right != nullptr) q.push(current->right);
        }
    }
};

int main() {
    BinarySearchTree bst;

    cout << "--- a. Inserting Elements (50, 25, 36, 33, 49, 60, 72) ---\n";
    int items[] = {50, 25, 36, 33, 49, 60, 72};
    for (int x : items) bst.insert(x);

    cout << "\n--- d. Tree Traversals ---\n";
    cout << "Pre-order:   "; bst.preOrderTraversal(bst.root); cout << endl;
    cout << "In-order:    "; bst.inOrderTraversal(bst.root); cout << " (Sorted Order)" << endl;
    cout << "Post-order:  "; bst.postOrderTraversal(bst.root); cout << endl;
    cout << "Level-order: "; bst.levelOrderTraversal(bst.root); cout << endl;

    cout << "\n--- c. Search Operation ---\n";
    cout << "Search 36: " << (bst.search(36) ? "Item found." : "Item not found.") << endl;
    cout << "Search 99: " << (bst.search(99) ? "Item found." : "Item not found.") << endl;

    cout << "\n--- b. Delete Operation (Deleting 36) ---\n";
    bst.deleteNode(36);
    cout << "In-order after deleting 36: "; bst.inOrderTraversal(bst.root); cout << endl;

    return 0;
}
