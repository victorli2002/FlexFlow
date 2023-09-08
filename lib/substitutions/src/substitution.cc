#include "substitutions/substitution.h"

namespace FlexFlow {

struct DeriveValidOperatorAttributeExpr {
  template <typename T>
  std::unordered_set<AttributeExpr<OperatorAttributeKey>>
      operator()(T const &t) {
    return derive_valid_operator_attribute_expr(t);
  }

  std::unordered_set<AttributeExpr<OperatorAttributeKey>>
      derive_valid_operator_attribute_expr(OperatorAttributeKey const &key) {
    return {key};
  }

  std::unordered_set<AttributeExpr<OperatorAttributeKey>>
      derive_valid_operator_attribute_expr(
          ListIndexAccess<OperatorAttributeKey> const &access) {
    return {access, access.attribute_key};
  }

  std::unordered_set<AttributeExpr<OperatorAttributeKey>>
      derive_valid_operator_attribute_expr(
          ListSize<OperatorAttributeKey> const &ls) {
    return {ls, ls.attribute_key};
  }
};

std::unordered_set<AttributeExpr<OperatorAttributeKey>>
    get_valid_operator_attribute_exprs(OperatorPattern const &pattern) {
  return set_union(transform(
      pattern.attribute_constraints, [](OperatorAttributeConstraint const &t) {
        return visit(DeriveValidOperatorAttributeExpr{}, t.attribute_expr);
      }));
}

bool is_valid_operator_attribute_expr(
    OperatorPattern const &pattern,
    AttributeExpr<OperatorAttributeKey> const &expr) {
  return contains(get_valid_operator_attribute_exprs(pattern), expr);
}

struct IsValidOperatorAttributeExprFunctor {
  GraphPattern const &graph_pattern;

  template <typename T>
  bool operator()(T const &t) const {
    return is_valid(t);
  }

  bool is_valid(OperatorAttrAccess const &t) const {
    return is_valid_operator_attribute_expr(graph_pattern.value().at(t.node),
                                            t.attr_expr);
  }

  bool is_valid(AttrConstant const &t) const {
    return true;
  }
};

bool is_valid_operator_attribute_expr(GraphPattern const &pattern,
                                      OperatorAttributeExpr const &expr) {
  return visit(IsValidOperatorAttributeExprFunctor{pattern}, expr);
}

bool is_valid_substitution(Substitution const &s) {
  for (Node const &node : get_nodes(s.output_graph_expr)) {
    for (OperatorAttributeExpr expr :
         values(s.output_graph_expr->at(node).assignment)) {
      if (!is_valid_operator_attribute_expr(s.input_graph, expr)) {
        return false;
      }
    }
  }
  return true;
}

struct EvaluateOperatorAttributeExpr {
  ParallelComputationGraph const &graph;
  MultiDiGraphPatternMatch const &match;

  template <typename T>
  OperatorAttributeExpr operator()(T const &t) {
    return evaluate(t);
  }

  OperatorAttributeValue evaluate(OperatorAttrAccess const &t) {
    Node node_in_pattern = t.node;
    Node node_in_pcg = match.node_assignment.at_l(node_in_pattern);
    return evaluate_attribute_expr(graph.value().at(node_in_pcg), t.attr_expr)
        .value();
  }

  OperatorAttributeValue evaluate(AttrConstant const &t) {
    return t.value;
  }
};

OperatorAttributeExpr
    evaluate_graph_attribute_expr(ParallelComputationGraph const &g,
                                  MultiDiGraphPatternMatch const &match,
                                  OperatorAttributeExpr const &expr) {
  return visit(EvaluateOperatorAttributeExpr{g, match}, expr);
}

Operator get_operator_attrs(ParallelComputationGraph const &graph,
                            MultiDiGraphPatternMatch const &match,
                            OperatorAttrAssignment const &assignment) {
  std::unordered_map<OperatorAttributeKey, OperatorAttributeValue> assignments;
  for (auto const &[key, expr] : assignment.assignments) {
    assignments.emplace(key, evaluate_graph_attribute_expr(graph, match, expr));
  }
  assert(contains_key(assignments, OperatorAttributeKey::OP_TYPE));
  assert(holds_alternative<OperatorType>(
      assignments.at(OperatorAttributeKey::OP_TYPE)));
  OperatorType op_type =
      get<OperatorType>(assignments.at(OperatorAttributeKey::OP_TYPE));
  switch (op_type) {
    case Op::BATCHMATMUL:
      return Operator(
          BatchMatmulAttrs{
              get<int>(assignments.at(OperatorAttributeKey::A_SEQ_LENGTH_DIM)),
              get<int>(assignments.at(OperatorAttributeKey::B_SEQ_LENGTH_DIM))},
          nullopt);
    case Op::BATCHNORM:
      return Operator(
          BatchNormAttrs{get<bool>(assignments.at(OperatorAttributeKey::RELU))},
          nullopt);
    case Op::CAST:
      return Operator(CastAttrs{get<DataType>(
                          assignments.at(OperatorAttributeKey::DATA_TYPE))},
                      nullopt);
    case Op::CONCAT:
      return Operator(ConcatAttrs{get<ff_dim_t>(
                          assignments.at(OperatorAttributeKey::AXIS))},
                      nullopt);
    case Op::CONV2D:
      return Operator(
          Conv2DAttrs{
              get<int>(assignments.at(OperatorAttributeKey::OUT_CHANNELS)),
              get<int>(assignments.at(OperatorAttributeKey::KERNEL_H)),
              get<int>(assignments.at(OperatorAttributeKey::KERNEL_W)),
              get<int>(assignments.at(OperatorAttributeKey::STRIDE_H)),
              get<int>(assignments.at(OperatorAttributeKey::STRIDE_W)),
              get<int>(assignments.at(OperatorAttributeKey::PADDING_H)),
              get<int>(assignments.at(OperatorAttributeKey::PADDING_W)),
              get<int>(assignments.at(OperatorAttributeKey::GROUPS)),
              get<Activation>(assignments.at(OperatorAttributeKey::ACTIVATION)),
              get<bool>(assignments.at(OperatorAttributeKey::USE_BIAS))},
          nullopt);
    case Op::DROPOUT:
      return Operator(
          DropoutAttrs{get<float>(assignments.at(OperatorAttributeKey::RATE)),
                       get<unsigned long long>(
                           assignments.at(OperatorAttributeKey::SEED))},
          nullopt);
    case Op::EW_ADD:
    case Op::EW_DIV:
    case Op::EW_EQUAL:
    case Op::EW_GREATER:
    case Op::EW_LESS:
    case Op::EW_MAX:
    case Op::EW_MIN:
    case Op::EW_MUL:
    case Op::EW_SUB:
      return Operator(
          ElementBinaryAttrs{
              op_type,
              get<DataType>(assignments.at(OperatorAttributeKey::DATA_TYPE)),
              get<bool>(
                  assignments.at(OperatorAttributeKey::SHOULD_BROADCAST_LHS)),
              get<bool>(
                  assignments.at(OperatorAttributeKey::SHOULD_BROADCAST_RHS))},
          nullopt);
    case Op::SCALAR_ADD:
    case Op::SCALAR_FLOOR_DIV:
    case Op::SCALAR_MULTIPLY:
    case Op::SCALAR_SUB:
    case Op::SCALAR_TRUE_DIV:
      return Operator(
          ElementScalarUnaryAttrs{
              op_type,
              get<float>(assignments.at(OperatorAttributeKey::SCALAR))},
          nullopt);
    case Op::EMBEDDING:
      return Operator(
          EmbeddingAttrs{
              get<int>(assignments.at(OperatorAttributeKey::NUM_ENTRIES)),
              get<int>(assignments.at(OperatorAttributeKey::OUT_CHANNELS)),
              get<AggregateOp>(assignments.at(OperatorAttributeKey::AGGR)),
              get<DataType>(assignments.at(OperatorAttributeKey::OP_TYPE))},
          nullopt);
    case Op::FLAT:
      return Operator(FlatAttrs{}, nullopt);
    case Op::GATHER:
      return Operator(
          GatherAttrs{get<ff_dim_t>(assignments.at(OperatorAttributeKey::DIM))},
          nullopt);
    case Op::INPUT:
      return Operator(InputAttrs{}, nullopt);
    case Op::LAYERNORM:
      return Operator(
          LayerNormAttrs{
              get<stack_vector<ff_dim_t, MAX_TENSOR_DIM>>(
                  assignments.at(OperatorAttributeKey::AXES)),
              get<bool>(
                  assignments.at(OperatorAttributeKey::ELEMENTWISE_AFFINE)),
              get<float>(assignments.at(OperatorAttributeKey::EPSILON))},
          nullopt);
    case Op::LINEAR:
      return Operator(
          LinearAttrs{
              get<int>(assignments.at(OperatorAttributeKey::OUT_CHANNELS)),
              get<bool>(assignments.at(OperatorAttributeKey::USE_BIAS)),
              get<DataType>(assignments.at(OperatorAttributeKey::DATA_TYPE)),
              get<Activation>(assignments.at(OperatorAttributeKey::DATA_TYPE)),
              get<RegularizerAttrs>(
                  assignments.at(OperatorAttributeKey::REGULARIZER))},
          nullopt);
    case Op::MULTIHEAD_ATTENTION:
      return Operator(
          MultiHeadAttentionAttrs{
              get<int>(assignments.at(OperatorAttributeKey::EMBED_DIM)),
              get<int>(assignments.at(OperatorAttributeKey::NUM_HEADS)),
              get<int>(assignments.at(OperatorAttributeKey::NUM_HEADS)),
              get<int>(assignments.at(OperatorAttributeKey::VDIM)),
              get<float>(assignments.at(OperatorAttributeKey::DROPOUT)),
              get<bool>(assignments.at(OperatorAttributeKey::BIAS)),
              get<bool>(assignments.at(OperatorAttributeKey::ADD_BIAS_KV)),
              get<bool>(assignments.at(OperatorAttributeKey::ADD_ZERO_ATTN))},
          nullopt);
    case Op::NOOP:
      return Operator(NoopAttrs{}, nullopt);
    case Op::POOL2D:
      return Operator(
          Pool2DAttrs{
              get<int>(assignments.at(OperatorAttributeKey::KERNEL_H)),
              get<int>(assignments.at(OperatorAttributeKey::KERNEL_W)),
              get<int>(assignments.at(OperatorAttributeKey::STRIDE_H)),
              get<int>(assignments.at(OperatorAttributeKey::STRIDE_W)),
              get<int>(assignments.at(OperatorAttributeKey::PADDING_H)),
              get<int>(assignments.at(OperatorAttributeKey::PADDING_W)),
              get<PoolOp>(assignments.at(OperatorAttributeKey::POOL_TYPE)),
              get<Activation>(
                  assignments.at(OperatorAttributeKey::ACTIVATION))},
          nullopt);
    case Op::REDUCE_ARGMAX:
    case Op::REDUCE_ARGMIN:
    case Op::REDUCE_MAX:
    case Op::REDUCE_MEAN:
    case Op::REDUCE_MIN:
    case Op::REDUCE_PROD:
    case Op::REDUCE_SUM:
      return Operator(
          ReduceAttrs{
              get<stack_vector<ff_dim_t, MAX_TENSOR_DIM>>(
                  assignments.at(OperatorAttributeKey::AXES)),
              op_type,
              get<bool>(assignments.at(OperatorAttributeKey::KEEP_DIMS))},
          nullopt);
    case Op::REVERSE:
      return Operator(ReverseAttrs{get<ff_dim_t>(
                          assignments.at(OperatorAttributeKey::AXIS))},
                      nullopt);
    case Op::RESHAPE:
      return Operator(ReshapeAttrs{get<TensorShape>(
                          assignments.at(OperatorAttributeKey::SHAPE))},
                      nullopt);
    case Op::SPLIT:
      return Operator(
          SplitAttrs{get<stack_vector<int, MAX_NUM_OUTPUTS>>(
                         assignments.at(OperatorAttributeKey::SPLITS)),
                     get<ff_dim_t>(assignments.at(OperatorAttributeKey::AXIS))},
          nullopt);
    case Op::SOFTMAX:
      return Operator(SoftmaxAttrs{get<ff_dim_t>(
                          assignments.at(OperatorAttributeKey::DIM))},
                      nullopt);
    case Op::TOPK:
      return Operator(
          TopKAttrs{get<int>(assignments.at(OperatorAttributeKey::K)),
                    get<bool>(assignments.at(OperatorAttributeKey::SORTED))},
          nullopt);
    case Op::TRANSPOSE:
      return Operator(
          TransposeAttrs{get<stack_vector<ff_dim_t, MAX_TENSOR_DIM>>(
              assignments.at(OperatorAttributeKey::PERMUTATION))},
          nullopt);
    default:
      break;
  }
}

ParallelComputationGraph
    apply_substitution(ParallelComputationGraph const &pcg,
                       Substitution const &substitution,
                       MultiDiGraphPatternMatch const &match) {
  ParallelComputationGraph new_pcg =
      OutputLabelledMultiDiGraph<Operator, ParallelTensor>::create<
          UnorderedOutputLabelledMultiDiGraph<Operator, ParallelTensor>>();
  bidict<Node, Node> node_mapping; // Refactor it with global nodes
  for (Node const &node : get_nodes(pcg)) {
    if (!contains_r(match.node_assignment, node)) {
      node_mapping.equate(node, new_pcg.value().add_node(pcg.value().at(node)));
    }
  }
  for (MultiDiEdge const &edge : get_edges(pcg)) {
    if (!contains_r(match.edge_assignment, edge)) {
      new_pcg.value().add_edge(MultiDiEdge{node_mapping.at_l(edge.src),
                                           node_mapping.at_r(edge.dst),
                                           new_pcg.value().add_node_port(),
                                           new_pcg.value().add_node_port()});
    }
  }
  for (Node const &output_node : get_nodes(substitution.output_graph_expr)) {
    Node new_node = new_pcg.value().add_node(get_operator_attrs(
        pcg, match, substitution.output_graph_expr->at(output_node)));
    node_mapping.equate(output_node, new_node);
  }
  for (OpenMultiDiEdge const &output_edge :
       get_edges(substitution.output_graph_expr)) {
    if (holds_alternative<InputMultiDiEdge>(output_edge)) {
      InputMultiDiEdge e = get<InputMultiDiEdge>(output_edge);
      MultiDiEdge original_edge =
          match.edge_assignment.at_l(substitution.input_mapping.at_r(e));
      new_pcg.value().add_edge(
          MultiDiOutput{node_mapping.at_l(original_edge.src),
                        new_pcg.value().add_node_port()},
          MultiDiInput{node_mapping.at_l(e.dst),
                       new_pcg.value().add_node_port()});
    } else if (holds_alternative<OutputMultiDiEdge>(output_edge)) {
      OutputMultiDiEdge e = get<OutputMultiDiEdge>(output_edge);
      MultiDiEdge original_edge =
          match.edge_assignment.at_l(substitution.output_mapping.at_r(e));
      new_pcg.value().add_edge(MultiDiEdge{node_mapping.at_l(e.src),
                                           node_mapping.at_l(original_edge.dst),
                                           new_pcg.value().add_node_port(),
                                           new_pcg.value().add_node_port()});
    } else {
      assert(holds_alternative<MultiDiEdge>(output_edge));
      MultiDiEdge e = get<MultiDiEdge>(output_edge);
      new_pcg.value().add_edge(MultiDiEdge{node_mapping.at_l(e.src),
                                           node_mapping.at_l(e.dst),
                                           new_pcg.value().add_node_port(),
                                           new_pcg.value().add_node_port()});
    }
  }

  return new_pcg;
}

} // namespace FlexFlow
