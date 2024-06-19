// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/utils/include/utils/graph/multidigraph/multi_di_edge.struct.toml
/* proj-data
{
  "generated_from": "73b001bfb7a0b75c42cd5037bb8dc686"
}
*/

#ifndef _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_MULTIDIGRAPH_MULTI_DI_EDGE_DTG_H
#define _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_MULTIDIGRAPH_MULTI_DI_EDGE_DTG_H

#include "fmt/format.h"
#include "utils/graph/node/node.dtg.h"
#include <functional>
#include <ostream>
#include <tuple>

namespace FlexFlow {
struct MultiDiEdge {
  MultiDiEdge() = delete;
  explicit MultiDiEdge(::FlexFlow::Node const &src,
                       ::FlexFlow::Node const &dst,
                       std::pair<int, int> const &raw_edge_uid);

  bool operator==(MultiDiEdge const &) const;
  bool operator!=(MultiDiEdge const &) const;
  bool operator<(MultiDiEdge const &) const;
  bool operator>(MultiDiEdge const &) const;
  bool operator<=(MultiDiEdge const &) const;
  bool operator>=(MultiDiEdge const &) const;
  ::FlexFlow::Node src;
  ::FlexFlow::Node dst;
  std::pair<int, int> raw_edge_uid;
};
} // namespace FlexFlow

namespace std {
template <>
struct hash<::FlexFlow::MultiDiEdge> {
  size_t operator()(::FlexFlow::MultiDiEdge const &) const;
};
} // namespace std

namespace FlexFlow {
std::string format_as(MultiDiEdge const &);
std::ostream &operator<<(std::ostream &, MultiDiEdge const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_MULTIDIGRAPH_MULTI_DI_EDGE_DTG_H
