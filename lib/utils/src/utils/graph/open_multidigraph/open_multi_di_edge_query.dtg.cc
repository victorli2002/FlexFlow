// THIS FILE WAS AUTO-GENERATED BY proj. DO NOT MODIFY IT!
// If you would like to modify this datatype, instead modify
// lib/utils/include/utils/graph/open_multidigraph/open_multi_di_edge_query.struct.toml
/* proj-data
{
  "generated_from": "86fc384b53b6b27982dfe6ab8fff2d04"
}
*/

#include "utils/graph/open_multidigraph/open_multi_di_edge_query.dtg.h"

#include "utils/graph/multidigraph/multi_di_edge_query.dtg.h"
#include "utils/graph/open_multidigraph/input_multi_di_edge_query.dtg.h"
#include "utils/graph/open_multidigraph/output_multi_di_edge_query.dtg.h"
#include <sstream>

namespace FlexFlow {
OpenMultiDiEdgeQuery::OpenMultiDiEdgeQuery(
    ::FlexFlow::InputMultiDiEdgeQuery const &input_edge_query,
    ::FlexFlow::MultiDiEdgeQuery const &standard_edge_query,
    ::FlexFlow::OutputMultiDiEdgeQuery const &output_edge_query)
    : input_edge_query(input_edge_query),
      standard_edge_query(standard_edge_query),
      output_edge_query(output_edge_query) {}
bool OpenMultiDiEdgeQuery::operator==(OpenMultiDiEdgeQuery const &other) const {
  return std::tie(this->input_edge_query,
                  this->standard_edge_query,
                  this->output_edge_query) ==
         std::tie(other.input_edge_query,
                  other.standard_edge_query,
                  other.output_edge_query);
}
bool OpenMultiDiEdgeQuery::operator!=(OpenMultiDiEdgeQuery const &other) const {
  return std::tie(this->input_edge_query,
                  this->standard_edge_query,
                  this->output_edge_query) !=
         std::tie(other.input_edge_query,
                  other.standard_edge_query,
                  other.output_edge_query);
}
bool OpenMultiDiEdgeQuery::operator<(OpenMultiDiEdgeQuery const &other) const {
  return std::tie(this->input_edge_query,
                  this->standard_edge_query,
                  this->output_edge_query) < std::tie(other.input_edge_query,
                                                      other.standard_edge_query,
                                                      other.output_edge_query);
}
bool OpenMultiDiEdgeQuery::operator>(OpenMultiDiEdgeQuery const &other) const {
  return std::tie(this->input_edge_query,
                  this->standard_edge_query,
                  this->output_edge_query) > std::tie(other.input_edge_query,
                                                      other.standard_edge_query,
                                                      other.output_edge_query);
}
bool OpenMultiDiEdgeQuery::operator<=(OpenMultiDiEdgeQuery const &other) const {
  return std::tie(this->input_edge_query,
                  this->standard_edge_query,
                  this->output_edge_query) <=
         std::tie(other.input_edge_query,
                  other.standard_edge_query,
                  other.output_edge_query);
}
bool OpenMultiDiEdgeQuery::operator>=(OpenMultiDiEdgeQuery const &other) const {
  return std::tie(this->input_edge_query,
                  this->standard_edge_query,
                  this->output_edge_query) >=
         std::tie(other.input_edge_query,
                  other.standard_edge_query,
                  other.output_edge_query);
}
} // namespace FlexFlow

namespace std {
size_t hash<FlexFlow::OpenMultiDiEdgeQuery>::operator()(
    ::FlexFlow::OpenMultiDiEdgeQuery const &x) const {
  size_t result = 0;
  result ^= std::hash<::FlexFlow::InputMultiDiEdgeQuery>{}(x.input_edge_query) +
            0x9e3779b9 + (result << 6) + (result >> 2);
  result ^= std::hash<::FlexFlow::MultiDiEdgeQuery>{}(x.standard_edge_query) +
            0x9e3779b9 + (result << 6) + (result >> 2);
  result ^=
      std::hash<::FlexFlow::OutputMultiDiEdgeQuery>{}(x.output_edge_query) +
      0x9e3779b9 + (result << 6) + (result >> 2);
  return result;
}
} // namespace std

namespace FlexFlow {
std::string format_as(OpenMultiDiEdgeQuery const &x) {
  std::ostringstream oss;
  oss << "<OpenMultiDiEdgeQuery";
  oss << " input_edge_query=" << x.input_edge_query;
  oss << " standard_edge_query=" << x.standard_edge_query;
  oss << " output_edge_query=" << x.output_edge_query;
  oss << ">";
  return oss.str();
}
std::ostream &operator<<(std::ostream &s, OpenMultiDiEdgeQuery const &x) {
  return s << fmt::to_string(x);
}
} // namespace FlexFlow
