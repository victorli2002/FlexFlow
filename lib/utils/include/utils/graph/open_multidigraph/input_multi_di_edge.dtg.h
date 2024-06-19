// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/utils/include/utils/graph/open_multidigraph/input_multi_di_edge.struct.toml
/* proj-data
{
  "generated_from": "d779a19c1f8f096dc1dfabf95633b115"
}
*/

#ifndef _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_INPUT_MULTI_DI_EDGE_DTG_H
#define _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_INPUT_MULTI_DI_EDGE_DTG_H

#include "fmt/format.h"
#include "utils/graph/node/node.dtg.h"
#include <cstddef>
#include <functional>
#include <ostream>
#include <tuple>

namespace FlexFlow {
struct InputMultiDiEdge {
  InputMultiDiEdge() = delete;
  explicit InputMultiDiEdge(::FlexFlow::Node const &dst, size_t const &raw_uid);

  bool operator==(InputMultiDiEdge const &) const;
  bool operator!=(InputMultiDiEdge const &) const;
  bool operator<(InputMultiDiEdge const &) const;
  bool operator>(InputMultiDiEdge const &) const;
  bool operator<=(InputMultiDiEdge const &) const;
  bool operator>=(InputMultiDiEdge const &) const;
  ::FlexFlow::Node dst;
  size_t raw_uid;
};
} // namespace FlexFlow

namespace std {
template <>
struct hash<::FlexFlow::InputMultiDiEdge> {
  size_t operator()(::FlexFlow::InputMultiDiEdge const &) const;
};
} // namespace std

namespace FlexFlow {
std::string format_as(InputMultiDiEdge const &);
std::ostream &operator<<(std::ostream &, InputMultiDiEdge const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_INPUT_MULTI_DI_EDGE_DTG_H
