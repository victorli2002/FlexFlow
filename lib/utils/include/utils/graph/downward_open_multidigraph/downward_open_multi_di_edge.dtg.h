// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/utils/include/utils/graph/downward_open_multidigraph/downward_open_multi_di_edge.variant.toml
/* proj-data
{
  "generated_from": "a48025d66b3bdc8eec931e33694b0a22"
}
*/

#ifndef _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_DOWNWARD_OPEN_MULTIDIGRAPH_DOWNWARD_OPEN_MULTI_DI_EDGE_DTG_H
#define _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_DOWNWARD_OPEN_MULTIDIGRAPH_DOWNWARD_OPEN_MULTI_DI_EDGE_DTG_H

#include "fmt/format.h"
#include "utils/graph/multidigraph/multi_di_edge.dtg.h"
#include "utils/graph/open_multidigraph/output_multi_di_edge.dtg.h"
#include <cstddef>
#include <functional>
#include <ostream>
#include <type_traits>
#include <variant>

namespace FlexFlow {
struct DownwardOpenMultiDiEdge {
  DownwardOpenMultiDiEdge() = delete;
  explicit DownwardOpenMultiDiEdge(::FlexFlow::OutputMultiDiEdge const &);
  explicit DownwardOpenMultiDiEdge(::FlexFlow::MultiDiEdge const &);
  template <typename T>
  static constexpr bool IsPartOfDownwardOpenMultiDiEdge_v =
      std::is_same_v<T, ::FlexFlow::OutputMultiDiEdge> ||
      std::is_same_v<T, ::FlexFlow::MultiDiEdge>;
  template <typename ReturnType, typename Visitor>
  ReturnType visit(Visitor &&v) const {
    switch (this->index()) {
      case 0: {
        ReturnType result = v(this->get<::FlexFlow::OutputMultiDiEdge>());
        return result;
      }
      case 1: {
        ReturnType result = v(this->get<::FlexFlow::MultiDiEdge>());
        return result;
      }
      default: {
        throw std::runtime_error(
            fmt::format("Unknown index {} for type DownwardOpenMultiDiEdge",
                        this->index()));
      }
    }
  }
  template <typename ReturnType, typename Visitor>
  ReturnType visit(Visitor &&v) {
    switch (this->index()) {
      case 0: {
        ReturnType result = v(this->get<::FlexFlow::OutputMultiDiEdge>());
        return result;
      }
      case 1: {
        ReturnType result = v(this->get<::FlexFlow::MultiDiEdge>());
        return result;
      }
      default: {
        throw std::runtime_error(
            fmt::format("Unknown index {} for type DownwardOpenMultiDiEdge",
                        this->index()));
      }
    }
  }
  template <typename T>
  bool has() const {
    static_assert(
        IsPartOfDownwardOpenMultiDiEdge_v<T>,
        "DownwardOpenMultiDiEdge::has() expected one of "
        "[::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge], received T");
    return std::holds_alternative<T>(this->raw_variant);
  }
  template <typename T>
  T const &get() const {
    static_assert(
        IsPartOfDownwardOpenMultiDiEdge_v<T>,
        "DownwardOpenMultiDiEdge::get() expected one of "
        "[::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge], received T");
    return std::get<T>(this->raw_variant);
  }
  template <typename T>
  T &get() {
    static_assert(
        IsPartOfDownwardOpenMultiDiEdge_v<T>,
        "DownwardOpenMultiDiEdge::get() expected one of "
        "[::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge], received T");
    return std::get<T>(this->raw_variant);
  }
  size_t index() const {
    return this->raw_variant.index();
  }
  bool operator==(DownwardOpenMultiDiEdge const &) const;
  bool operator!=(DownwardOpenMultiDiEdge const &) const;
  bool operator<(DownwardOpenMultiDiEdge const &) const;
  bool operator>(DownwardOpenMultiDiEdge const &) const;
  bool operator<=(DownwardOpenMultiDiEdge const &) const;
  bool operator>=(DownwardOpenMultiDiEdge const &) const;
  std::variant<::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge>
      raw_variant;
};
} // namespace FlexFlow
namespace std {
template <>
struct hash<::FlexFlow::DownwardOpenMultiDiEdge> {
  size_t operator()(::FlexFlow::DownwardOpenMultiDiEdge const &) const;
};
} // namespace std
namespace FlexFlow {
std::string format_as(::FlexFlow::DownwardOpenMultiDiEdge const &);
std::ostream &operator<<(std::ostream &,
                         ::FlexFlow::DownwardOpenMultiDiEdge const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_DOWNWARD_OPEN_MULTIDIGRAPH_DOWNWARD_OPEN_MULTI_DI_EDGE_DTG_H
