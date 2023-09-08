#ifndef _FLEXFLOW_UTILS_GRAPH_LABELLED_NODE_LABELLED_OPEN
#define _FLEXFLOW_UTILS_GRAPH_LABELLED_NODE_LABELLED_OPEN

namespace FlexFlow {

template <typename NodeLabel>
struct NodeLabelledOpenMultiDiGraph {
  NodeLabelledOpenMultiDiGraph() = delete;
  NodeLabelledOpenMultiDiGraph(NodeLabelledOpenMultiDiGraph const &) = default;
  NodeLabelledOpenMultiDiGraph &
      operator=(NodeLabelledOpenMultiDiGraph const &) = default;

  operator OpenMultiDiGraphView();
};

} // namespace FlexFlow

#endif
