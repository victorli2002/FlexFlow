// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/utils/include/utils/graph/open_multidigraph/output_multi_di_edge_query.struct.toml
/* proj-data
{
  "generated_from": "4833874bcc5268ec7a7f8fe92186ba17"
}
*/

#ifndef _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_OUTPUT_MULTI_DI_EDGE_QUERY_DTG_H
#define _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_OUTPUT_MULTI_DI_EDGE_QUERY_DTG_H

#include "fmt/format.h"
#include "utils/graph/node/node.dtg.h"
#include "utils/graph/query_set.h"
#include <functional>
#include <ostream>
#include <tuple>

namespace FlexFlow {
struct OutputMultiDiEdgeQuery {
  OutputMultiDiEdgeQuery() = delete;
  explicit OutputMultiDiEdgeQuery(
      ::FlexFlow::query_set<::FlexFlow::Node> const &srcs);

  bool operator==(OutputMultiDiEdgeQuery const &) const;
  bool operator!=(OutputMultiDiEdgeQuery const &) const;
  bool operator<(OutputMultiDiEdgeQuery const &) const;
  bool operator>(OutputMultiDiEdgeQuery const &) const;
  bool operator<=(OutputMultiDiEdgeQuery const &) const;
  bool operator>=(OutputMultiDiEdgeQuery const &) const;
  ::FlexFlow::query_set<::FlexFlow::Node> srcs;
};
} // namespace FlexFlow

namespace std {
template <>
struct hash<::FlexFlow::OutputMultiDiEdgeQuery> {
  size_t operator()(::FlexFlow::OutputMultiDiEdgeQuery const &) const;
};
} // namespace std

namespace FlexFlow {
std::string format_as(OutputMultiDiEdgeQuery const &);
std::ostream &operator<<(std::ostream &, OutputMultiDiEdgeQuery const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_OUTPUT_MULTI_DI_EDGE_QUERY_DTG_H
