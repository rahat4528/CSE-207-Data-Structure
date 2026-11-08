# Mini Project: Student Record Management System (BST)

[![C++ Standard](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://en.cppreference.com/w/cpp/17)
[![Course](https://img.shields.io/badge/Course-CSE%20207-orange.svg)]()
[![Institution](https://img.shields.io/badge/EWU-East%20West%20University-green.svg)]()

An interactive, menu-driven **Student Record Management System** implemented in standard C++17 using a custom **Binary Search Tree (BST)** data structure for the **East West University CSE 207: Data Structures** lab curriculum.

---

## 🔬 Core Features & Architecture

- **Binary Search Tree Indexing:** Student records are indexed using their unique Roll Number as the key ($O(\log N)$ average search, insert, and delete).
- **BST Invariant Enforcement:** Left child roll numbers are strictly smaller, right child roll numbers are strictly larger, and duplicate roll numbers are strictly rejected.
- **3-Case Node Deletion:**
  - Case 1: Deleting a leaf node.
  - Case 2: Deleting a node with a single child (left or right).
  - Case 3: Deleting a node with two children via in-order successor substitution.
- **Traversals:**
  - **In-order Traversal:** Yields student records sorted in ascending numerical order of roll numbers.
  - **Pre-order & Post-order Traversals:** Display tree hierarchy and subtree structures.
- **Encapsulated Design:** Clear separation of concerns with `Student` (entity), `Node` (tree node), and `StudentBST` (algorithms).

---

## 🛠️ Build and Run

### Batch Build (Windows)
```cmd
cd "00 - Mini Project - Student Record System"
build.bat
bin\StudentRecordSystem.exe
```

### CMake Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
./StudentRecordSystem
```

