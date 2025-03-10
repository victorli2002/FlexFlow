/* Copyright 2023 CMU, Facebook, LANL, MIT, NVIDIA, and Stanford (alphabetical)
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "flexflow/ops/aggregate.h"
#include "flexflow/model.h"
#include "flexflow/utils/hash_utils.h"
#include "legion/legion_utilities.h"

namespace FlexFlow {

// declare Legion names
using Legion::ArgumentMap;
using Legion::Context;
using Legion::coord_t;
using Legion::Domain;
using Legion::FutureMap;
using Legion::IndexLauncher;
using Legion::PhysicalRegion;
using Legion::Predicate;
using Legion::Rect;
using Legion::RegionRequirement;
using Legion::Runtime;
using Legion::Task;
using Legion::TaskArgument;
using Legion::TaskLauncher;
using PCG::Node;

Tensor FFModel::aggregate(
    Tensor const *inputs, /* gate_preds, gate_assign, gate assign TopK,
                             full_gate_pred, exp_pred_1, ... , exp_pred_n */
    int n,
    float lambda_bal,
    char const *name) {
  Layer *li = new Layer(this,
                        OP_AGGREGATE,
                        DT_FLOAT,
                        name,
                        n + 4 /*inputs*/,
                        0 /*weights*/,
                        1 /*outputs*/,
                        inputs);
  {
    int num_dim = inputs[4]->num_dims;
    // Set output shape
    int dims[MAX_TENSOR_DIM];
    for (int i = 0; i < num_dim - 1; i++) {
      dims[i] = inputs[4]->dims[i];
    }
    dims[num_dim - 1] = inputs[0]->dims[num_dim - 1];
    li->outputs[0] = create_tensor_legion_ordering(
        num_dim, dims, DT_FLOAT, li, 0, true /*create_grad*/);
  }
  li->add_int_property("n", n);
  li->add_float_property("lambda_bal", lambda_bal);
  layers.push_back(li);
  return li->outputs[0];
}

Op *Aggregate::create_operator_from_layer(
    FFModel &model,
    Layer const *layer,
    std::vector<ParallelTensor> const &inputs) {
  long long value1;
  layer->get_int_property("n", value1);
  int n = value1;
  float value2;
  layer->get_float_property("lambda_bal", value2);
  float lambda_bal = value2;
  return new Aggregate(model, inputs.data(), n, lambda_bal, layer->name);
}

AggregateParams Aggregate::get_params() const {
  AggregateParams params;
  params.n = this->n;
  params.lambda_bal = this->lambda_bal;
  if (strlen(this->name) < MAX_OPNAME) {
    strcpy(params.name, this->name);
  }
  return params;
}

bool AggregateParams::is_valid(std::vector<ParallelTensorShape> const &) const {
  // Aggregate is always valid
  return true;
}

bool operator==(AggregateParams const &lhs, AggregateParams const &rhs) {
  return lhs.n == rhs.n && lhs.lambda_bal == rhs.lambda_bal;
}

Aggregate::Aggregate(FFModel &model,
                     ParallelTensor const *_inputs,
                     int _n,
                     float _lambda_bal,
                     char const *name)
    : Op(model,
         OP_AGGREGATE,
         DT_FLOAT,
         name,
         _n + 4 /*inputs*/,
         0 /*weights*/,
         1 /*outputs*/,
         _inputs),
      n(_n), lambda_bal(_lambda_bal) {
  // FIXME: For now, set upper limits Better: Do as follows, but memory is
  // assigned per block, so requires to check that
  // https://stackoverflow.com/questions/5531247/allocating-shared-memory/5531640#5531640
  assert(n <= AGGREGATE_MAX_N && "Increase AGGREGATE_MAX_N in #define");
  assert(inputs[0]->dims[0].size <= AGGREGATE_MAX_K &&
         "Increase AGGREGATE_MAX_K in #define");
  assert(inputs[0]->dims[1].size <= AGGREGATE_MAX_BATCH_SIZE &&
         "Increase AGGREGATE_MAX_BATCH_SIZE in #define");

  assert(n + 4 == numInputs);
  assert(n > 0);
  assert(inputs[0]->num_dims == 2 + 1);
  assert(inputs[1]->num_dims == 2 + 1);
  assert(inputs[2]->num_dims == 2 + 1);
  assert(inputs[3]->num_dims == 2 + 1);

  for (int i = 0; i < inputs[0]->num_dims; i++) {
    assert(inputs[0]->dims[i] == inputs[1]->dims[i]);
    assert(inputs[0]->dims[i] == inputs[2]->dims[i]);
  }
  assert(inputs[0]->dims[1] == inputs[3]->dims[1]);
  assert(inputs[3]->dims[0].size == n);

  // expert inputs
  int num_dim = inputs[4]->num_dims;
  int out_dim = inputs[4]->dims[0].size;
  for (int i = 1; i < n; i++) {
    assert(inputs[i + 4]->num_dims == num_dim);
    assert(inputs[i + 4]->dims[0].size == out_dim);
  }
  // Set output shape
  ParallelDim dims[MAX_TENSOR_DIM];
  for (int i = 0; i < num_dim - 1; i++) {
    dims[i] = inputs[4]->dims[i];
  }
  dims[num_dim - 2] = inputs[0]->dims[num_dim - 2];
  dims[num_dim - 1] = inputs[0]->dims[num_dim - 1];
  numOutputs = 1;
  outputs[0] = model.create_parallel_tensor_legion_ordering(
      num_dim, dims, DT_FLOAT, this);

  numWeights = 0;
}

Aggregate::Aggregate(FFModel &model,
                     Aggregate const &other,
                     std::vector<ParallelTensor> const &inputs)
    : Aggregate(model, inputs.data(), other.n, other.lambda_bal, other.name) {}

Aggregate::Aggregate(FFModel &model,
                     AggregateParams const &params,
                     std::vector<ParallelTensor> const &inputs,
                     char const *name)
    : Aggregate(
          model, inputs.data(), params.n, params.lambda_bal, params.name) {}

using PCG::Node;
Node Aggregate::deserialize(FFModel &ff,
                            Legion::Deserializer &dez,
                            std::vector<ParallelTensor> const &inputs,
                            int num_inputs) {
  int n;
  float lambda_bal;
  dez.deserialize(n);
  dez.deserialize(lambda_bal);
  size_t name_len;
  char name[MAX_OPNAME] = {0};
  dez.deserialize(name_len);
  dez.deserialize(name, name_len);
  assert(num_inputs == n + 4);
  AggregateParams params;
  params.n = n;
  params.lambda_bal = lambda_bal;
  strcpy(params.name, name);
  return ff.get_or_create_node<Aggregate>(inputs, params);
}

void Aggregate::init_inference(FFModel const &ff,
                               std::vector<ParallelTensor> const &batch_inputs,
                               std::vector<ParallelTensor> const &batch_outputs,
                               MachineView const *mv) {
  assert(check_output_input_weight_same_parallel_is());
  parallel_is = batch_outputs[0]->parallel_is;
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  MachineView const *view = mv ? mv : &batch_outputs[0]->machine_view;
  size_t machine_view_hash = view->hash();
  set_argumentmap_for_init_inference(ff, argmap, batch_outputs[0]);
  IndexLauncher launcher(AGGREGATE_INIT_TASK_ID,
                         parallel_is,
                         TaskArgument(this, sizeof(Aggregate)),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         machine_view_hash);
  FutureMap fm = runtime->execute_index_space(ctx, launcher);
  fm.wait_all_results();
  set_opmeta_from_futuremap_inference(ff, fm, batch_outputs[0]);
}

void Aggregate::init(FFModel const &ff) {
  assert(check_output_input_weight_same_parallel_is());
  parallel_is = outputs[0]->parallel_is;
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  set_argumentmap_for_init(ff, argmap);
  IndexLauncher launcher(AGGREGATE_INIT_TASK_ID,
                         parallel_is,
                         TaskArgument(this, sizeof(Aggregate)),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         outputs[0]->machine_view.hash());
  FutureMap fm = runtime->execute_index_space(ctx, launcher);
  fm.wait_all_results();
  set_opmeta_from_futuremap(ff, fm);
}

OpMeta *Aggregate::init_task(Task const *task,
                             std::vector<PhysicalRegion> const &regions,
                             Context ctx,
                             Runtime *runtime) {
  Aggregate *agg = (Aggregate *)task->args;
  FFHandler handle = *((FFHandler *)task->local_args);
  AggregateMeta *m = new AggregateMeta(handle, agg);
  m->profiling = agg->profiling;
  m->inference_debugging = agg->inference_debugging;
  std::strcpy(m->op_name, agg->name);
  m->layer_guid = agg->layer_guid;
  return m;
}

void Aggregate::forward(FFModel const &ff) {
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  set_argumentmap_for_forward(ff, argmap);
  IndexLauncher launcher(AGGREGATE_FWD_TASK_ID,
                         parallel_is,
                         TaskArgument(nullptr, 0),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         outputs[0]->machine_view.hash());
  // gate_preds
  launcher.add_region_requirement(RegionRequirement(inputs[0]->part,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    inputs[0]->region));
  launcher.add_field(0, FID_DATA);
  // gate_assign
  launcher.add_region_requirement(RegionRequirement(inputs[1]->part,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    inputs[1]->region));
  launcher.add_field(1, FID_DATA);
  // exp_preds
  for (int i = 0; i < n; i++) {
    launcher.add_region_requirement(RegionRequirement(inputs[i + 4]->part,
                                                      0 /*projection id*/,
                                                      READ_WRITE,
                                                      EXCLUSIVE,
                                                      inputs[i + 4]->region));
    launcher.add_field(i + 2, FID_DATA);
  }
  // output
  launcher.add_region_requirement(RegionRequirement(outputs[0]->part,
                                                    0 /*projection id*/,
                                                    WRITE_ONLY,
                                                    EXCLUSIVE,
                                                    outputs[0]->region));
  launcher.add_field(n + 2, FID_DATA);
  runtime->execute_index_space(ctx, launcher);
}

FutureMap Aggregate::inference(FFModel const &ff,
                               BatchConfigFuture const &bc,
                               std::vector<ParallelTensor> const &batch_inputs,
                               std::vector<ParallelTensor> const &batch_outputs,
                               MachineView const *mv) {
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  parallel_is = batch_outputs[0]->parallel_is;
  MachineView const *view = mv ? mv : &batch_outputs[0]->machine_view;
  set_argumentmap_for_inference(ff, argmap, batch_outputs[0]);
  size_t machine_view_hash = view->hash();
  /* std::cout << "Aggregate op machine_view: " << *(MachineView const *)mv
            << std::endl; */
  IndexLauncher launcher(AGGREGATE_FWD_TASK_ID,
                         parallel_is,
                         TaskArgument(nullptr, 0),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         machine_view_hash);
  // gate_preds
  launcher.add_region_requirement(RegionRequirement(batch_inputs[0]->part,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    batch_inputs[0]->region));
  launcher.add_field(0, FID_DATA);
  // gate_assign
  launcher.add_region_requirement(RegionRequirement(batch_inputs[1]->part,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    batch_inputs[1]->region));
  launcher.add_field(1, FID_DATA);
  // exp_preds
  for (int i = 0; i < n; i++) {
    launcher.add_region_requirement(
        RegionRequirement(batch_inputs[i + 4]->part,
                          0 /*projection id*/,
                          READ_WRITE,
                          EXCLUSIVE,
                          batch_inputs[i + 4]->region));
    launcher.add_field(i + 2, FID_DATA);
  }
  // output
  launcher.add_region_requirement(RegionRequirement(batch_outputs[0]->part,
                                                    0 /*projection id*/,
                                                    WRITE_ONLY,
                                                    EXCLUSIVE,
                                                    batch_outputs[0]->region));
  launcher.add_field(n + 2, FID_DATA);
  return runtime->execute_index_space(ctx, launcher);
}

void Aggregate::forward_task(Task const *task,
                             std::vector<PhysicalRegion> const &regions,
                             Context ctx,
                             Runtime *runtime) {
  assert(regions.size() == task->regions.size());
  int n = regions.size() - 3;

  AggregateMeta const *m = *((AggregateMeta **)task->local_args);

  // get gate_pred, gate_assign, output
  AccessorRO<float, 3> const acc_gate_pred(regions[0], FID_DATA);
  AccessorRO<int, 3> const acc_gate_assign(regions[1], FID_DATA);
  AccessorWO<float, 3> const acc_output(regions[n + 2], FID_DATA);

  Rect<3> rect_gate_pred = runtime->get_index_space_domain(
      ctx, task->regions[0].region.get_index_space());
  Rect<3> rect_gate_assign = runtime->get_index_space_domain(
      ctx, task->regions[1].region.get_index_space());
  Rect<3> rect_output = runtime->get_index_space_domain(
      ctx, task->regions[n + 2].region.get_index_space());

  coord_t batch_size = rect_gate_pred.hi[1] - rect_gate_pred.lo[1] + 1;
  assert(batch_size == rect_gate_assign.hi[1] - rect_gate_assign.lo[1] + 1);
  assert(rect_gate_pred.hi[0] - rect_gate_pred.lo[0] ==
         rect_gate_assign.hi[0] - rect_gate_assign.lo[0]);
  assert(batch_size == rect_output.hi[1] - rect_output.lo[1] + 1);
  coord_t out_dim = rect_output.hi[0] - rect_output.lo[0] + 1;

  // get exp_preds
  float *exp_preds[n];
  // get first exp_pred and row and out_dim
  Domain exp_domain = runtime->get_index_space_domain(
      ctx, task->regions[2].region.get_index_space());
  exp_preds[0] = helperGetTensorPointerWO<float>(
      regions[2], task->regions[2], FID_DATA, ctx, runtime);
  coord_t rows = exp_domain.hi()[1] - exp_domain.lo()[1] + 1;
  assert(out_dim == exp_domain.hi()[0] - exp_domain.lo()[0] + 1);

  for (int i = 1; i < n; i++) {
    exp_domain = runtime->get_index_space_domain(
        ctx, task->regions[i + 2].region.get_index_space());
    exp_preds[i] = helperGetTensorPointerWO<float>(
        regions[i + 2], task->regions[i + 2], FID_DATA, ctx, runtime);

    assert(rows == exp_domain.hi()[1] - exp_domain.lo()[1] + 1);
    assert(out_dim == exp_domain.hi()[0] - exp_domain.lo()[0] + 1);
  }

  int k = (int)(rect_gate_assign.hi[0] - rect_gate_assign.lo[0] + 1);

  Aggregate::forward_kernel_wrapper(m,
                                    exp_preds,
                                    acc_gate_assign.ptr(rect_gate_assign),
                                    acc_gate_pred.ptr(rect_gate_pred),
                                    acc_output.ptr(rect_output),
                                    n,
                                    k,
                                    rows,
                                    batch_size,
                                    out_dim);
}

void Aggregate::backward(FFModel const &ff) {
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  set_argumentmap_for_backward(ff, argmap);
  IndexLauncher launcher(AGGREGATE_BWD_TASK_ID,
                         parallel_is,
                         TaskArgument(this, sizeof(Aggregate)),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         outputs[0]->machine_view.hash());
  // gate_preds
  launcher.add_region_requirement(RegionRequirement(inputs[0]->part,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    inputs[0]->region));
  launcher.add_field(0, FID_DATA);
  // gate_assign
  launcher.add_region_requirement(RegionRequirement(inputs[1]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    inputs[1]->region));
  launcher.add_field(1, FID_DATA);
  // true gate_assign
  launcher.add_region_requirement(RegionRequirement(inputs[2]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    inputs[2]->region));
  launcher.add_field(2, FID_DATA);
  // full_gate gradients
  launcher.add_region_requirement(RegionRequirement(inputs[3]->part_grad,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    inputs[3]->region_grad));
  launcher.add_field(3, FID_DATA);
  // exp_preds
  for (int i = 0; i < n; i++) {
    launcher.add_region_requirement(RegionRequirement(inputs[i + 4]->part,
                                                      0 /*projection id*/,
                                                      READ_WRITE,
                                                      EXCLUSIVE,
                                                      inputs[i + 4]->region));
    launcher.add_field(i + 4, FID_DATA);
  }
  // exp_preds gradients
  for (int i = 0; i < n; i++) {
    launcher.add_region_requirement(
        RegionRequirement(inputs[i + 4]->part_grad,
                          0 /*projection id*/,
                          READ_WRITE,
                          EXCLUSIVE,
                          inputs[i + 4]->region_grad));
    launcher.add_field(i + n + 4, FID_DATA);
  }

  // output
  launcher.add_region_requirement(RegionRequirement(outputs[0]->part_grad,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    outputs[0]->region_grad));
  launcher.add_field(2 * n + 4, FID_DATA);

  runtime->execute_index_space(ctx, launcher);
}

void Aggregate::backward_task(Task const *task,
                              std::vector<PhysicalRegion> const &regions,
                              Context ctx,
                              Runtime *runtime) {
  AggregateMeta const *m = *((AggregateMeta **)task->local_args);
  int n = ((Aggregate *)task->args)->n;
  float lambda_bal = ((Aggregate *)task->args)->lambda_bal;

  assert((int)regions.size() == 2 * n + 5);
  assert((int)task->regions.size() == 2 * n + 5);

  // get gate_pred, gate_grad, gate_assign, output_grad
  AccessorRO<float, 3> const acc_gate_pred(regions[0], FID_DATA);
  AccessorRO<int, 3> const acc_gate_assign(regions[1], FID_DATA);
  AccessorRO<int, 3> const acc_true_gate_assign(regions[2], FID_DATA);
  AccessorWO<float, 3> const full_acc_gate_grad(regions[3], FID_DATA);
  AccessorRO<float, 3> const acc_output_grad(regions[2 * n + 4], FID_DATA);

  Rect<3> rect_gate_pred = runtime->get_index_space_domain(
      ctx, task->regions[0].region.get_index_space());
  Rect<3> rect_gate_assign = runtime->get_index_space_domain(
      ctx, task->regions[1].region.get_index_space());
  Rect<3> rect_true_gate_assign = runtime->get_index_space_domain(
      ctx, task->regions[2].region.get_index_space());
  Rect<3> rect_full_gate_grad = runtime->get_index_space_domain(
      ctx, task->regions[3].region.get_index_space());
  Rect<3> rect_out_grad = runtime->get_index_space_domain(
      ctx, task->regions[2 * n + 4].region.get_index_space());

  coord_t batch_size = rect_gate_pred.hi[1] - rect_gate_pred.lo[1] + 1;
  assert(batch_size == rect_gate_assign.hi[1] - rect_gate_assign.lo[1] + 1);
  assert(rect_gate_assign == rect_true_gate_assign);
  assert(batch_size == rect_out_grad.hi[1] - rect_out_grad.lo[1] + 1);
  assert(batch_size ==
         rect_full_gate_grad.hi[1] - rect_full_gate_grad.lo[1] + 1);
  coord_t k = rect_gate_assign.hi[0] - rect_gate_assign.lo[0] + 1;
  assert(rect_gate_pred.hi[0] - rect_gate_pred.lo[0] + 1 == k);
  coord_t out_dim = rect_out_grad.hi[0] - rect_out_grad.lo[0] + 1;
  assert(n == rect_full_gate_grad.hi[0] - rect_full_gate_grad.lo[0] + 1);

  // get exp_preds
  float *exp_preds[n];
  // get first exp_pred and row
  Domain exp_domain = runtime->get_index_space_domain(
      ctx, task->regions[4].region.get_index_space());
  exp_preds[0] = helperGetTensorPointerRW<float>(
      regions[4], task->regions[4], FID_DATA, ctx, runtime);
  coord_t rows = exp_domain.hi()[1] - exp_domain.lo()[1] + 1;
  assert(out_dim == exp_domain.hi()[0] - exp_domain.lo()[0] + 1);

  for (int i = 1; i < n; i++) {
    exp_domain = runtime->get_index_space_domain(
        ctx, task->regions[i + 4].region.get_index_space());
    exp_preds[i] = helperGetTensorPointerRW<float>(
        regions[i + 4], task->regions[i + 4], FID_DATA, ctx, runtime);
    assert(rows == exp_domain.hi()[1] - exp_domain.lo()[1] + 1);
    assert(out_dim == exp_domain.hi()[0] - exp_domain.lo()[0] + 1);
  }

  // get chosen_exp_grads
  float *exp_grads[n];
  for (int i = 0; i < n; i++) {
    exp_domain = runtime->get_index_space_domain(
        ctx, task->regions[n + i + 4].region.get_index_space());
    exp_grads[i] = helperGetTensorPointerRW<float>(
        regions[n + i + 4], task->regions[n + i + 4], FID_DATA, ctx, runtime);
    assert(rows == exp_domain.hi()[1] - exp_domain.lo()[1] + 1);
    assert(out_dim == exp_domain.hi()[0] - exp_domain.lo()[0] + 1);
  }

  Aggregate::backward_kernel_wrapper(
      m,
      exp_preds,
      exp_grads,
      acc_gate_assign.ptr(rect_gate_assign),
      acc_true_gate_assign.ptr(rect_true_gate_assign),
      acc_gate_pred.ptr(rect_gate_pred),
      full_acc_gate_grad.ptr(rect_full_gate_grad),
      acc_output_grad.ptr(rect_out_grad),
      n,
      k,
      rows,
      lambda_bal,
      batch_size,
      out_dim);
}

void Aggregate::serialize(Legion::Serializer &sez) const {
  sez.serialize(this->n);
  sez.serialize(this->lambda_bal);
  sez.serialize(strlen(this->name));
  sez.serialize(this->name, strlen(this->name));
}

bool Aggregate::measure_operator_cost(Simulator *sim,
                                      MachineView const &mv,
                                      CostMetrics &cost_metrics) const {
  assert(numInputs <= MAX_NUM_INPUTS);
  ParallelTensorBase sub_inputs[MAX_NUM_INPUTS], sub_pred, sub_assign,
      sub_output;

  for (int i = 0; i < numInputs; ++i) {
    if (!inputs[i + 4]->get_sub_tensor(mv, sub_inputs[i])) {
      return false;
    }
  }
  if (!inputs[0]->get_sub_tensor(mv, sub_pred)) {
    return false;
  }
  if (!inputs[1]->get_sub_tensor(mv, sub_assign)) {
    return false;
  }

  if (!outputs[0]->get_sub_tensor(mv, sub_output)) {
    return false;
  }

  AggregateMeta *m = new AggregateMeta(sim->handler, this);

  // allocate
  sim->free_all();
  float *input_ptrs[MAX_NUM_INPUTS];
  bool out_of_memory = false;
  for (int i = 0; i < numInputs; ++i) {
    input_ptrs[i] =
        (float *)sim->allocate(sub_inputs[i].get_volume(), DT_FLOAT);
    out_of_memory = out_of_memory || (input_ptrs[i] == NULL);
  }
  int *assign_ptr = (int *)sim->allocate(sub_assign.get_volume(), DT_INT32);
  out_of_memory = out_of_memory || (assign_ptr == NULL);
  float *pred_ptr = (float *)sim->allocate(sub_pred.get_volume(), DT_FLOAT);
  out_of_memory = out_of_memory || (pred_ptr == NULL);
  cost_metrics.inputs_memory += cost_metrics.total_mem_diff_from(sim->offset);

  float *output_ptr = (float *)sim->allocate(sub_output.get_volume(), DT_FLOAT);
  cost_metrics.outputs_memory += cost_metrics.total_mem_diff_from(sim->offset);
  out_of_memory = out_of_memory || (output_ptr == NULL);

  if (out_of_memory) {
    cost_metrics.forward_time = Simulator::MAXIMUM_TASK_RUN_TIME;
    cost_metrics.backward_time = Simulator::MAXIMUM_TASK_RUN_TIME;
    return true;
  }

  assert(m->profiling == false);

  // compute
  std::function<void()> forward, backward;
  Domain assign_domain = sub_assign.get_domain();
  Domain exp_domain = sub_inputs[0].get_domain();

  int k = assign_domain.hi()[0] - assign_domain.lo()[0] + 1;
  int batch_size = assign_domain.hi()[1] - assign_domain.lo()[1] + 1;
  int rows = exp_domain.hi()[1] - exp_domain.lo()[1] + 1;
  int out_dim = exp_domain.hi()[0] - exp_domain.lo()[0] + 1;

  forward = [&] {
    forward_kernel_wrapper(m,
                           input_ptrs,
                           assign_ptr,
                           pred_ptr,
                           output_ptr,
                           n,
                           k,
                           rows,
                           batch_size,
                           out_dim);
  };

  inner_measure_operator_cost(sim, forward, backward, cost_metrics);
  log_measure.debug("[Measure Aggregate] name(%s) forward_time(%.4lf)\n",
                    name,
                    cost_metrics.forward_time);

  cost_metrics.backward_time = 0.0f; // not implemented for backward
  delete m;
  return true;
}

}; // namespace FlexFlow

namespace std {
size_t hash<FlexFlow::AggregateParams>::operator()(
    FlexFlow::AggregateParams const &params) const {
  size_t key = 0;
  hash_combine(key, params.n);
  hash_combine(key, params.lambda_bal);
  return key;
}
}; // namespace std
