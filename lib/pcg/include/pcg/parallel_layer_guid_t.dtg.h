// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/pcg/include/pcg/parallel_layer_guid_t.struct.toml
/* proj-data
{
  "generated_from": "c31301efeb92e151b04943786aa7bec1"
}
*/

#ifndef _FLEXFLOW_LIB_PCG_INCLUDE_PCG_PARALLEL_LAYER_GUID_T_DTG_H
#define _FLEXFLOW_LIB_PCG_INCLUDE_PCG_PARALLEL_LAYER_GUID_T_DTG_H

#include "fmt/format.h"
#include "utils/graph.h"
#include <functional>
#include <ostream>
#include <tuple>

namespace FlexFlow {
struct parallel_layer_guid_t {
  parallel_layer_guid_t() = delete;
  parallel_layer_guid_t(::FlexFlow::Node const &raw_graph_node);

  bool operator==(parallel_layer_guid_t const &) const;
  bool operator!=(parallel_layer_guid_t const &) const;
  bool operator<(parallel_layer_guid_t const &) const;
  bool operator>(parallel_layer_guid_t const &) const;
  bool operator<=(parallel_layer_guid_t const &) const;
  bool operator>=(parallel_layer_guid_t const &) const;
  ::FlexFlow::Node raw_graph_node;
};
} // namespace FlexFlow

namespace std {
template <>
struct hash<FlexFlow::parallel_layer_guid_t> {
  size_t operator()(FlexFlow::parallel_layer_guid_t const &) const;
};
} // namespace std

namespace FlexFlow {
std::string format_as(parallel_layer_guid_t const &);
std::ostream &operator<<(std::ostream &, parallel_layer_guid_t const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_PCG_INCLUDE_PCG_PARALLEL_LAYER_GUID_T_DTG_H
