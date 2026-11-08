// =============================================================================
// East West University | CSE 207: Data Structures
// Experiment 10: Graph Operations
// a. Breadth First Traversal (BFT) techniques to print all the nodes
// b. Depth First Traversal (DFT) techniques to print all the nodes
// =============================================================================

#include <iostream>
#include <vector>
#include <queue>
#include <stack>

using namespace std;

class Graph {
public:
    vector<int> adj[9];
    char nodes[9] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'J', 'k'};

    void createGraph() {
        adj[0].push_back(5); adj[0].push_back(2); adj[0].push_back(1);
        adj[1].push_back(0); adj[1].push_back(2); adj[1].push_back(6);
        adj[2].push_back(0); adj[2].push_back(1); adj[2].push_back(3);
        adj[2].push_back(4); adj[2].push_back(5); adj[2].push_back(6);
        adj[3].push_back(2); adj[3].push_back(4); adj[3].push_back(5); adj[3].push_back(7);
        adj[4].push_back(2); adj[4].push_back(3); adj[4].push_back(7); adj[4].push_back(8);
        adj[5].push_back(0); adj[5].push_back(2); adj[5].push_back(3);
        adj[6].push_back(1); adj[6].push_back(2); adj[6].push_back(3); adj[6].push_back(8);
        adj[7].push_back(3); adj[7].push_back(4); adj[7].push_back(8);
        adj[8].push_back(4); adj[8].push_back(6); adj[8].push_back(7);
    }

    // a. Breadth First Traversal (BFT) techniques to print all the nodes
    void breadthFirstTraversal(int u) {
        queue<int> q;
        bool visited[9] = {false};

        q.push(u);
        visited[u] = true;

        while (!q.empty()) {
            int n = q.front();
            q.pop();
            cout << nodes[n] << " "; // print the vertex

            for (int v : adj[n]) {
                if (!visited[v]) {
                    q.push(v);
                    visited[v] = true;
                }
            }
        }
        cout << endl;
    }

    // b. Depth First Traversal (DFT) techniques to print all the nodes
    void DepthFirstTraversal(int u) {
        stack<int> s;
        bool visited[9] = {false};

        s.push(u);
        visited[u] = true;

        while (!s.empty()) {
            int n = s.top();
            s.pop();
            cout << nodes[n] << " "; // print the vertex

            for (int v : adj[n]) {
                if (!visited[v]) {
                    s.push(v);
                    visited[v] = true;
                }
            }
        }
        cout << endl;
    }
};

int main() {
    Graph g;
    g.createGraph();

    cout << "--- a. Breadth First Traversal (BFT starting from node 'A') ---\n";
    g.breadthFirstTraversal(0);

    cout << "\n--- b. Depth First Traversal (DFT starting from node 'A') ---\n";
    g.DepthFirstTraversal(0);

    return 0;
}
