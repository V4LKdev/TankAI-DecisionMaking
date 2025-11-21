#pragma once

/// @brief A simple undirected graph structure for pathfinding centerlines.

#include <vector>
#include <Play.h>

namespace Pathfinding {

struct Edge {
  int to{ -1 };
  float cost{ 0.0f };
};

struct Node {
  Play::Vector2D pos{};
  std::vector<Edge> edges{};
};

class Graph {

public:
  int addNode(const Play::Vector2D& p) {
    m_nodes.push_back(Node{ p, {} });
    return static_cast<int>(m_nodes.size()) - 1;
  }

  void addEdge(const int a, const int b, const float cost) {
    if (a < 0 || b < 0 || a >= size() || b >= size() || a == b) return;

    auto ensure = [&](const int u, const int v) {
      auto& es = m_nodes[u].edges;
      for (const auto& e : es)
        if (e.to == v) return;
      es.push_back({ v, cost });
    };

    ensure(a, b);
    ensure(b, a); // undirected centerline graph
  }

  [[nodiscard]] int size() const { return static_cast<int>(m_nodes.size()); }
  [[nodiscard]] const std::vector<Node>& nodes() const { return m_nodes; }
  std::vector<Node>& nodes() { return m_nodes; }
  void clear() { m_nodes.clear(); }

private:
  std::vector<Node> m_nodes{};
};

} // namespace Pathfinding

