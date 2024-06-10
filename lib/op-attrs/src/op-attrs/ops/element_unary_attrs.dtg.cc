// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/op-attrs/include/op-attrs/ops/element_unary_attrs.struct.toml
/* proj-data
{
  "generated_from": "75272cff78d3db866122dbb1001aedbe"
}
*/

#include "op-attrs/ops/element_unary_attrs.dtg.h"

#include "op-attrs/operator_type.h"
#include <sstream>

namespace FlexFlow {
ElementUnaryAttrs::ElementUnaryAttrs(::FlexFlow::OperatorType const &op_type)
    : op_type(op_type) {}
bool ElementUnaryAttrs::operator==(ElementUnaryAttrs const &other) const {
  return std::tie(this->op_type) == std::tie(other.op_type);
}
bool ElementUnaryAttrs::operator!=(ElementUnaryAttrs const &other) const {
  return std::tie(this->op_type) != std::tie(other.op_type);
}
bool ElementUnaryAttrs::operator<(ElementUnaryAttrs const &other) const {
  return std::tie(this->op_type) < std::tie(other.op_type);
}
bool ElementUnaryAttrs::operator>(ElementUnaryAttrs const &other) const {
  return std::tie(this->op_type) > std::tie(other.op_type);
}
bool ElementUnaryAttrs::operator<=(ElementUnaryAttrs const &other) const {
  return std::tie(this->op_type) <= std::tie(other.op_type);
}
bool ElementUnaryAttrs::operator>=(ElementUnaryAttrs const &other) const {
  return std::tie(this->op_type) >= std::tie(other.op_type);
}
} // namespace FlexFlow

namespace std {
size_t hash<FlexFlow::ElementUnaryAttrs>::operator()(
    ::FlexFlow::ElementUnaryAttrs const &x) const {
  size_t result = 0;
  result ^= std::hash<::FlexFlow::OperatorType>{}(x.op_type) + 0x9e3779b9 +
            (result << 6) + (result >> 2);
  return result;
}
} // namespace std

namespace nlohmann {
::FlexFlow::ElementUnaryAttrs
    adl_serializer<::FlexFlow::ElementUnaryAttrs>::from_json(json const &j) {
  return ::FlexFlow::ElementUnaryAttrs{
      j.at("op_type").template get<::FlexFlow::OperatorType>()};
}
void adl_serializer<::FlexFlow::ElementUnaryAttrs>::to_json(
    json &j, ::FlexFlow::ElementUnaryAttrs const &v) {
  j["__type"] = "ElementUnaryAttrs";
  j["op_type"] = v.op_type;
}
} // namespace nlohmann

namespace rc {
Gen<::FlexFlow::ElementUnaryAttrs>
    Arbitrary<::FlexFlow::ElementUnaryAttrs>::arbitrary() {
  return gen::construct<::FlexFlow::ElementUnaryAttrs>(
      gen::arbitrary<::FlexFlow::OperatorType>());
}
} // namespace rc

namespace FlexFlow {
std::string format_as(ElementUnaryAttrs const &x) {
  std::ostringstream oss;
  oss << "<ElementUnaryAttrs";
  oss << " op_type=" << x.op_type;
  oss << ">";
  return oss.str();
}
std::ostream &operator<<(std::ostream &s, ElementUnaryAttrs const &x) {
  return s << fmt::to_string(x);
}
} // namespace FlexFlow
