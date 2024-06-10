// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/pcg/include/pcg/initializers/uniform_initializer_attrs.struct.toml
/* proj-data
{
  "generated_from": "f887e1db5d5dc710793ec5fa99bb7cd4"
}
*/

#include "pcg/initializers/uniform_initializer_attrs.dtg.h"

#include <sstream>

namespace FlexFlow {
UniformInitializerAttrs::UniformInitializerAttrs(int const &seed,
                                                 float const &min_val,
                                                 float const &max_val)
    : seed(seed), min_val(min_val), max_val(max_val) {}
bool UniformInitializerAttrs::operator==(
    UniformInitializerAttrs const &other) const {
  return std::tie(this->seed, this->min_val, this->max_val) ==
         std::tie(other.seed, other.min_val, other.max_val);
}
bool UniformInitializerAttrs::operator!=(
    UniformInitializerAttrs const &other) const {
  return std::tie(this->seed, this->min_val, this->max_val) !=
         std::tie(other.seed, other.min_val, other.max_val);
}
bool UniformInitializerAttrs::operator<(
    UniformInitializerAttrs const &other) const {
  return std::tie(this->seed, this->min_val, this->max_val) <
         std::tie(other.seed, other.min_val, other.max_val);
}
bool UniformInitializerAttrs::operator>(
    UniformInitializerAttrs const &other) const {
  return std::tie(this->seed, this->min_val, this->max_val) >
         std::tie(other.seed, other.min_val, other.max_val);
}
bool UniformInitializerAttrs::operator<=(
    UniformInitializerAttrs const &other) const {
  return std::tie(this->seed, this->min_val, this->max_val) <=
         std::tie(other.seed, other.min_val, other.max_val);
}
bool UniformInitializerAttrs::operator>=(
    UniformInitializerAttrs const &other) const {
  return std::tie(this->seed, this->min_val, this->max_val) >=
         std::tie(other.seed, other.min_val, other.max_val);
}
} // namespace FlexFlow

namespace std {
size_t hash<FlexFlow::UniformInitializerAttrs>::operator()(
    ::FlexFlow::UniformInitializerAttrs const &x) const {
  size_t result = 0;
  result ^=
      std::hash<int>{}(x.seed) + 0x9e3779b9 + (result << 6) + (result >> 2);
  result ^= std::hash<float>{}(x.min_val) + 0x9e3779b9 + (result << 6) +
            (result >> 2);
  result ^= std::hash<float>{}(x.max_val) + 0x9e3779b9 + (result << 6) +
            (result >> 2);
  return result;
}
} // namespace std

namespace nlohmann {
::FlexFlow::UniformInitializerAttrs
    adl_serializer<::FlexFlow::UniformInitializerAttrs>::from_json(
        json const &j) {
  return ::FlexFlow::UniformInitializerAttrs{
      j.at("seed").template get<int>(),
      j.at("min_val").template get<float>(),
      j.at("max_val").template get<float>()};
}
void adl_serializer<::FlexFlow::UniformInitializerAttrs>::to_json(
    json &j, ::FlexFlow::UniformInitializerAttrs const &v) {
  j["__type"] = "UniformInitializerAttrs";
  j["seed"] = v.seed;
  j["min_val"] = v.min_val;
  j["max_val"] = v.max_val;
}
} // namespace nlohmann

namespace FlexFlow {
std::string format_as(UniformInitializerAttrs const &x) {
  std::ostringstream oss;
  oss << "<UniformInitializerAttrs";
  oss << " seed=" << x.seed;
  oss << " min_val=" << x.min_val;
  oss << " max_val=" << x.max_val;
  oss << ">";
  return oss.str();
}
std::ostream &operator<<(std::ostream &s, UniformInitializerAttrs const &x) {
  return s << fmt::to_string(x);
}
} // namespace FlexFlow
