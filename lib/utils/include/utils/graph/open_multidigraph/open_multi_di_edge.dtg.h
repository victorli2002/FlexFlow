// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/utils/include/utils/graph/open_multidigraph/open_multi_di_edge.variant.toml
/* proj-data
{
  "generated_from": "f7a6881be7d51ba916f3740828c23d91"
}
*/

#ifndef _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_OPEN_MULTI_DI_EDGE_DTG_H
#define _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_OPEN_MULTI_DI_EDGE_DTG_H

#include "fmt/format.h"
#include "utils/graph/multidigraph/multi_di_edge.dtg.h"
#include "utils/graph/open_multidigraph/input_multi_di_edge.dtg.h"
#include "utils/graph/open_multidigraph/output_multi_di_edge.dtg.h"
#include <cstddef>
#include <functional>
#include <ostream>
#include <type_traits>
#include <variant>

namespace FlexFlow {
struct OpenMultiDiEdge {
  OpenMultiDiEdge() = delete;
  explicit OpenMultiDiEdge(::FlexFlow::InputMultiDiEdge const &);
  explicit OpenMultiDiEdge(::FlexFlow::OutputMultiDiEdge const &);
  explicit OpenMultiDiEdge(::FlexFlow::MultiDiEdge const &);
  template <typename T>
  static constexpr bool IsPartOfOpenMultiDiEdge_v =
      std::is_same_v<T, ::FlexFlow::InputMultiDiEdge> ||
      std::is_same_v<T, ::FlexFlow::OutputMultiDiEdge> ||
      std::is_same_v<T, ::FlexFlow::MultiDiEdge>;
  template <typename ReturnType, typename Visitor>
  ReturnType visit(Visitor &&v) const {
    switch (this->index()) {
      case 0: {
        ReturnType result = v(this->get<::FlexFlow::InputMultiDiEdge>());
        return result;
      }
      case 1: {
        ReturnType result = v(this->get<::FlexFlow::OutputMultiDiEdge>());
        return result;
      }
      case 2: {
        ReturnType result = v(this->get<::FlexFlow::MultiDiEdge>());
        return result;
      }
      default: {
        throw std::runtime_error(fmt::format(
            "Unknown index {} for type OpenMultiDiEdge", this->index()));
      }
    }
  }
  template <typename ReturnType, typename Visitor>
  ReturnType visit(Visitor &&v) {
    switch (this->index()) {
      case 0: {
        ReturnType result = v(this->get<::FlexFlow::InputMultiDiEdge>());
        return result;
      }
      case 1: {
        ReturnType result = v(this->get<::FlexFlow::OutputMultiDiEdge>());
        return result;
      }
      case 2: {
        ReturnType result = v(this->get<::FlexFlow::MultiDiEdge>());
        return result;
      }
      default: {
        throw std::runtime_error(fmt::format(
            "Unknown index {} for type OpenMultiDiEdge", this->index()));
      }
    }
  }
  template <typename T>
  bool has() const {
    static_assert(
        IsPartOfOpenMultiDiEdge_v<T>,
        "OpenMultiDiEdge::has() expected one of [::FlexFlow::InputMultiDiEdge, "
        "::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge], received T");
    return std::holds_alternative<T>(this->raw_variant);
  }
  template <typename T>
  T const &get() const {
    static_assert(
        IsPartOfOpenMultiDiEdge_v<T>,
        "OpenMultiDiEdge::get() expected one of [::FlexFlow::InputMultiDiEdge, "
        "::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge], received T");
    return std::get<T>(this->raw_variant);
  }
  template <typename T>
  T &get() {
    static_assert(
        IsPartOfOpenMultiDiEdge_v<T>,
        "OpenMultiDiEdge::get() expected one of [::FlexFlow::InputMultiDiEdge, "
        "::FlexFlow::OutputMultiDiEdge, ::FlexFlow::MultiDiEdge], received T");
    return std::get<T>(this->raw_variant);
  }
  size_t index() const {
    return this->raw_variant.index();
  }
  bool operator==(OpenMultiDiEdge const &) const;
  bool operator!=(OpenMultiDiEdge const &) const;
  bool operator<(OpenMultiDiEdge const &) const;
  bool operator>(OpenMultiDiEdge const &) const;
  bool operator<=(OpenMultiDiEdge const &) const;
  bool operator>=(OpenMultiDiEdge const &) const;
  std::variant<::FlexFlow::InputMultiDiEdge,
               ::FlexFlow::OutputMultiDiEdge,
               ::FlexFlow::MultiDiEdge>
      raw_variant;
};
} // namespace FlexFlow
namespace std {
template <>
struct hash<::FlexFlow::OpenMultiDiEdge> {
  size_t operator()(::FlexFlow::OpenMultiDiEdge const &) const;
};
} // namespace std
namespace FlexFlow {
std::string format_as(::FlexFlow::OpenMultiDiEdge const &);
std::ostream &operator<<(std::ostream &, ::FlexFlow::OpenMultiDiEdge const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_UTILS_INCLUDE_UTILS_GRAPH_OPEN_MULTIDIGRAPH_OPEN_MULTI_DI_EDGE_DTG_H
