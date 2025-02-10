#include "utils/graph/multidigraph/algorithms/get_incoming_edges.h"
#include "utils/graph/algorithms.h"
#include "utils/graph/instances/adjacency_multidigraph.h"
#include "utils/graph/multidigraph/algorithms/add_edges.h"
#include "utils/graph/multidigraph/algorithms/add_nodes.h"
#include "utils/graph/multidigraph/multidigraph.h"
#include <doctest/doctest.h>
#include <unordered_set>
#include <vector>

using namespace FlexFlow;

TEST_SUITE(FF_TEST_SUITE) {
  TEST_CASE("get_incoming_edges") {
    MultiDiGraph g = MultiDiGraph::create<AdjacencyMultiDiGraph>();
    std::vector<Node> n = add_nodes(g, 3_n);

    std::vector<MultiDiEdge> edges = add_edges(g,
                                               {{n.at(0), n.at(0)},
                                                {n.at(0), n.at(1)},
                                                {n.at(0), n.at(1)},
                                                {n.at(1), n.at(0)},
                                                {n.at(2), n.at(0)}});

    SUBCASE("get_incoming_edges(MultiDiGraphView, Node)") {

      SUBCASE("node has incoming edges") {
        std::unordered_set<MultiDiEdge> result = get_incoming_edges(g, n.at(1));
        std::unordered_set<MultiDiEdge> correct = {edges.at(1), edges.at(2)};
        CHECK(result == correct);
      }

      SUBCASE("node has no incoming edges") {
        std::unordered_set<MultiDiEdge> result = get_incoming_edges(g, n.at(2));
        std::unordered_set<MultiDiEdge> correct = {};
        CHECK(result == correct);
      }
    }

    SUBCASE("get_incoming_edges(MultiDiGraphView, std::unordered_set<Node>)") {

      std::unordered_set<Node> ns = {n.at(0), n.at(2)};
      std::unordered_map<Node, std::unordered_set<MultiDiEdge>> result =
          get_incoming_edges(g, ns);

      std::unordered_map<Node, std::unordered_set<MultiDiEdge>> correct = {
          {n.at(0), {edges.at(0), edges.at(3), edges.at(4)}}, {n.at(2), {}}};

      CHECK(result == correct);
    }
  }
}
