import java.util.*;

/**
 * Project 05: Dependency Resolver - Tertiary Java Demonstration
 * Demonstrates Directed Acyclic Graph (DAG) construction,
 * Kahn's Algorithm (BFS) Topological Sorting, and Cycle Detection.
 */
public class DependencyResolverDemo {

    static class DependencyGraph {
        private final Map<String, List<String>> adj = new HashMap<>();
        private final Map<String, List<String>> revAdj = new HashMap<>();
        private final Set<String> nodes = new TreeSet<>();

        public void addEdge(String u, String v) {
            nodes.add(u);
            nodes.add(v);
            adj.computeIfAbsent(u, k -> new ArrayList<>()).add(v);
            revAdj.computeIfAbsent(v, k -> new ArrayList<>()).add(u);
        }

        public List<String> sortKahnBfs() {
            Map<String, Integer> prereqs = new HashMap<>();
            for (String node : nodes) {
                prereqs.put(node, adj.getOrDefault(node, Collections.emptyList()).size());
            }

            Queue<String> ready = new PriorityQueue<>();
            for (Map.Entry<String, Integer> entry : prereqs.entrySet()) {
                if (entry.getValue() == 0) {
                    ready.add(entry.getKey());
                }
            }

            List<String> order = new ArrayList<>();
            while (!ready.isEmpty()) {
                String u = ready.poll();
                order.add(u);

                for (String dependent : revAdj.getOrDefault(u, Collections.emptyList())) {
                    int updated = prereqs.get(dependent) - 1;
                    prereqs.put(dependent, updated);
                    if (updated == 0) {
                        ready.add(dependent);
                    }
                }
            }

            if (order.size() < nodes.size()) {
                throw new IllegalStateException("Circular dependency detected in graph!");
            }
            return order;
        }

        public boolean hasCycleDfs() {
            // 0: unvisited, 1: visiting (gray), 2: visited (black)
            Map<String, Integer> color = new HashMap<>();
            for (String node : nodes) color.put(node, 0);

            for (String node : nodes) {
                if (color.get(node) == 0) {
                    if (dfsCheckCycle(node, color)) return true;
                }
            }
            return false;
        }

        private boolean dfsCheckCycle(String u, Map<String, Integer> color) {
            color.put(u, 1);
            for (String v : adj.getOrDefault(u, Collections.emptyList())) {
                if (color.get(v) == 1) return true; // back edge
                if (color.get(v) == 0 && dfsCheckCycle(v, color)) return true;
            }
            color.put(u, 2);
            return false;
        }
    }

    public static void main(String[] args) {
        System.out.println("============================================================");
        System.out.println(" PROJECT 05: DEPENDENCY RESOLVER (Java Tertiary Demo)");
        System.out.println("============================================================");

        DependencyGraph graph = new DependencyGraph();
        graph.addEdge("webApp", "backendApi");
        graph.addEdge("webApp", "uiComponents");
        graph.addEdge("backendApi", "databaseDriver");
        graph.addEdge("uiComponents", "databaseDriver");

        System.out.println("\n--- Checking Cycle ---");
        System.out.println("Has Cycle: " + graph.hasCycleDfs());

        System.out.println("\n--- Kahn's Algorithm Installation Order ---");
        List<String> order = graph.sortKahnBfs();
        for (int i = 0; i < order.size(); i++) {
            System.out.println("  " + (i + 1) + ". " + order.get(i));
        }
    }
}

