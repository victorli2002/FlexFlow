#ifndef _FLEXFLOW_LIB_OP_ATTRS_INCLUDE_OP_ATTRS_RELATIVE_FF_DIM_T_H
#define _FLEXFLOW_LIB_OP_ATTRS_INCLUDE_OP_ATTRS_RELATIVE_FF_DIM_T_H

#include "op-attrs/relative_ff_dim_t.dtg.h"
#include "rapidcheck.h"

namespace rc {
template <>
struct Arbitrary<FlexFlow::relative_ff_dim_t> {
  static Gen<FlexFlow::relative_ff_dim_t> arbitrary() {
    return gen::construct<FlexFlow::relative_ff_dim_t>(
        gen::inRange<int>(0, MAX_TENSOR_DIM));
  }
};
} // namespace rc

#endif // _FLEXFLOW_LIB_OP_ATTRS_INCLUDE_OP_ATTRS_RELATIVE_FF_DIM_T_H
