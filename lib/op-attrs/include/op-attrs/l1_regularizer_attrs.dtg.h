// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/op-attrs/include/op-attrs/l1_regularizer_attrs.struct.toml
/* proj-data
{
  "generated_from": "50968fb8a3d43395d0eab7594f4935c0"
}
*/

#ifndef _FLEXFLOW_LIB_OP_ATTRS_INCLUDE_OP_ATTRS_L1_REGULARIZER_ATTRS_DTG_H
#define _FLEXFLOW_LIB_OP_ATTRS_INCLUDE_OP_ATTRS_L1_REGULARIZER_ATTRS_DTG_H

#include "fmt/format.h"
#include "nlohmann/json.hpp"
#include "rapidcheck.h"
#include <functional>
#include <ostream>
#include <tuple>

namespace FlexFlow {
struct L1RegularizerAttrs {
  L1RegularizerAttrs() = delete;
  explicit L1RegularizerAttrs(float const &lambda);

  bool operator==(L1RegularizerAttrs const &) const;
  bool operator!=(L1RegularizerAttrs const &) const;
  bool operator<(L1RegularizerAttrs const &) const;
  bool operator>(L1RegularizerAttrs const &) const;
  bool operator<=(L1RegularizerAttrs const &) const;
  bool operator>=(L1RegularizerAttrs const &) const;
  float lambda;
};
} // namespace FlexFlow

namespace std {
template <>
struct hash<::FlexFlow::L1RegularizerAttrs> {
  size_t operator()(::FlexFlow::L1RegularizerAttrs const &) const;
};
} // namespace std

namespace nlohmann {
template <>
struct adl_serializer<::FlexFlow::L1RegularizerAttrs> {
  static ::FlexFlow::L1RegularizerAttrs from_json(json const &);
  static void to_json(json &, ::FlexFlow::L1RegularizerAttrs const &);
};
} // namespace nlohmann

namespace rc {
template <>
struct Arbitrary<::FlexFlow::L1RegularizerAttrs> {
  static Gen<::FlexFlow::L1RegularizerAttrs> arbitrary();
};
} // namespace rc

namespace FlexFlow {
std::string format_as(L1RegularizerAttrs const &);
std::ostream &operator<<(std::ostream &, L1RegularizerAttrs const &);
} // namespace FlexFlow

#endif // _FLEXFLOW_LIB_OP_ATTRS_INCLUDE_OP_ATTRS_L1_REGULARIZER_ATTRS_DTG_H
