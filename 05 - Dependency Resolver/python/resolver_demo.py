#!/usr/bin/env python3
"""
Project 05: Dependency Resolver - Secondary Python Demonstration
Demonstrates DAG Topological Sorting (Kahn's BFS), DFS Cycle Detection,
and Dependency Resolution Order in Python.
"""

from collections import deque, defaultdict
from typing import Dict, List, Set, Tuple

class DependencyGraph:
    def __init__(self):
        self.adj: Dict[str, List[str]] = defaultdict(list)
        self.rev_adj: Dict[str, List[str]] = defaultdict(list)
        self.nodes: Set[str] = set()

    def add_edge(self, u: str, v: str):
        """Add directed edge u -> v (u depends on v)."""
        self.nodes.add(u)
        self.nodes.add(v)
        self.adj[u].append(v)
        self.rev_adj[v].append(u)

    def detect_cycle_dfs(self) -> Tuple[bool, List[str]]:
        """Iterative 3-color DFS to detect cycle and return exact loop path."""
        # 0: white (unvisited), 1: gray (in-progress), 2: black (done)
        color = {node: 0 for node in self.nodes}
        path = []

        for root in sorted(self.nodes):
            if color[root] != 0:
                continue

            stack = [(root, 0, sorted(self.adj[root]))]
            color[root] = 1
            path.append(root)

            while stack:
                u, idx, neighbors = stack[-1]

                if idx < len(neighbors):
                    v = neighbors[idx]
                    stack[-1] = (u, idx + 1, neighbors)

                    if color[v] == 1:
                        # Cycle detected
                        v_idx = path.index(v)
                        cycle = path[v_idx:] + [v]
                        return True, cycle
                    elif color[v] == 0:
                        color[v] = 1
                        path.append(v)
                        stack.append((v, 0, sorted(self.adj[v])))
                else:
                    color[u] = 2
                    path.pop()
                    stack.pop()

        return False, []

    def sort_kahn_bfs(self) -> List[str]:
        """Kahn's Algorithm: returns valid installation order (prerequisites first)."""
        prereqs = {node: len(self.adj[node]) for node in self.nodes}
        ready = deque(sorted([node for node, count in prereqs.items() if count == 0]))

        order = []
        while ready:
            u = ready.popleft()
            order.append(u)

            # Dependents requiring u can now decrement prerequisite count
            for dependent in sorted(self.rev_adj[u]):
                prereqs[dependent] -= 1
                if prereqs[dependent] == 0:
                    ready.append(dependent)

        if len(order) < len(self.nodes):
            raise ValueError("Graph contains a cycle; cannot generate topological order.")

        return order


def run_demo():
    print("=" * 60)
    print(" PROJECT 05: DEPENDENCY RESOLVER (Python Secondary Demo)")
    print("=" * 60)

    # 1. Acyclic Scenario (Web Application Diamond Dependency)
    print("\n--- Scenario 1: Acyclic Diamond Dependency ---")
    g1 = DependencyGraph()
    g1.add_edge("web_app", "service_auth")
    g1.add_edge("web_app", "service_data")
    g1.add_edge("service_auth", "core_db")
    g1.add_edge("service_data", "core_db")

    has_cycle, cycle_path = g1.detect_cycle_dfs()
    print(f"Cycle Detected: {has_cycle}")

    install_order = g1.sort_kahn_bfs()
    print(f"Valid Installation Order (Prerequisites first):")
    for i, pkg in enumerate(install_order, 1):
        print(f"  {i}. {pkg}")

    # 2. Cyclic Scenario (Deadlock)
    print("\n--- Scenario 2: Circular Dependency Cycle Detection ---")
    g2 = DependencyGraph()
    g2.add_edge("pkgA", "pkgB")
    g2.add_edge("pkgB", "pkgC")
    g2.add_edge("pkgC", "pkgA")  # Cycle loop

    has_cycle, cycle_path = g2.detect_cycle_dfs()
    print(f"Cycle Detected: {has_cycle}")
    if has_cycle:
        print(f"Exact Cycle Path: {' -> '.join(cycle_path)}")


if __name__ == "__main__":
    run_demo()

