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

#include "flexflow/ops/add_bias_residual_layer_norm.h"
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
using Legion::InlineLauncher;
using Legion::Machine;
using Legion::Memory;
using Legion::PhysicalRegion;
using Legion::Predicate;
using Legion::Rect;
using Legion::RegionRequirement;
using Legion::Runtime;
using Legion::Task;
using Legion::TaskArgument;
using Legion::TaskLauncher;

bool operator==(AddBiasResidualLayerNormParams const &lhs,
                AddBiasResidualLayerNormParams const &rhs) {
  return lhs.layer_guid == rhs.layer_guid && lhs.axes == rhs.axes &&
         lhs.elementwise_affine == rhs.elementwise_affine &&
         lhs.use_bias == rhs.use_bias &&
         lhs.inplace_residual == rhs.inplace_residual;
}

bool AddBiasResidualLayerNormParams::is_valid(
    std::pair<ParallelTensorShape, ParallelTensorShape> const &input) const {
  return input.first.is_valid() && input.second.is_valid();
}

AddBiasResidualLayerNormParams AddBiasResidualLayerNorm::get_params() const {
  AddBiasResidualLayerNormParams params;
  params.layer_guid = this->layer_guid;
  params.axes = this->axes;
  params.elementwise_affine = this->elementwise_affine;
  params.eps = this->eps;
  params.use_bias = this->use_bias;
  params.inplace_residual = this->inplace_residual;
  if (strlen(this->name) < MAX_OPNAME) {
    strcpy(params.name, this->name);
  }
  return params;
}

void FFModel::add_bias_residual_layer_norm(const Tensor input,
                                           const Tensor residual,
                                           Tensor *outputs,
                                           std::vector<int> const &axes,
                                           bool elementwise_affine,
                                           float eps,
                                           bool use_bias,
                                           bool inplace_residual,
                                           DataType data_type,
                                           char const *name) {
  // In PyTorch, axes must be the sizes of the last axes.size() dimensions of
  // the input tensor. However, since the tensor dimensions are reversed in
  // FlexFlow (batch size is the last dimension), we require that axes must be
  // the sizes of the FIRST axes.size() dimensions of the input tensor.

  // Another difference is that in PyTorch, the axes vector should contain the
  // sizes of the dimensions with respect to which you want to compute the
  // layernorm. In FlexFlow, instead, axes should contain the INDICES of the
  // dimensions in question. We do this because the size of a dimension might be
  // different when splitting a tensor in model parallelism.
  assert(
      axes.size() <= input->num_dims &&
      "number of axes must be less than tensor dimensions"); // input does not
                                                             // have replica
                                                             // dimension here
  for (int i = 0; i < axes.size(); i++) {
    assert(axes[i] == i && "axes must be the first axes.size() dimensions");
  }

  // Check dims
  assert(input->num_dims == residual->num_dims);
  for (int i = 0; i < input->num_dims; i++) {
    assert(input->dims[i] == residual->dims[i]);
  }

  if (data_type == DT_NONE) {
    data_type = input->data_type;
  }
  int num_weights =
      1 + (elementwise_affine ? (use_bias ? 2 : 1)
                              : 0); // attention bias + layernorm weights
  Layer *ln = nullptr;
  Tensor casted_input =
      (data_type != input->data_type)
          ? cast(input, data_type, "type cast for add_bias_residual_layer_norm")
          : input;
  Tensor casted_residual =
      (data_type != residual->data_type)
          ? cast(residual,
                 data_type,
                 "type cast for add_bias_residual_layer_norm")
          : residual;
  ln = new Layer(this,
                 OP_ADD_BIAS_RESIDUAL_LAYERNORM,
                 data_type,
                 name,
                 2 /*inputs*/,
                 num_weights,
                 2 /*outputs*/,
                 casted_input,
                 residual);
  // added: attn_output + final attention bias + residual. To be added to the
  // output of FC2
  ln->outputs[0] = create_tensor_legion_ordering(
      input->num_dims, input->dims, data_type, ln, 0, false /*create_grad*/);
  // layer_norm(added)
  ln->outputs[1] = create_tensor_legion_ordering(
      input->num_dims, input->dims, data_type, ln, 1, false /*create_grad*/);
  {
    int numdims = axes.size();
    int dims[numdims];
    for (int i = 0; i < numdims; i++) {
      dims[i] = input->dims[axes[i]];
    }
    // Attention bias
    int attn_bias_dims[1] = {dims[0]};
    ln->weights[0] = create_weight_legion_ordering(1,
                                                   attn_bias_dims,
                                                   data_type,
                                                   ln,
                                                   false /*create_grad*/,
                                                   nullptr,
                                                   CHOSEN_SYNC_TYPE);
    if (num_weights > 1) {
      assert(elementwise_affine);
      ln->weights[1] = create_weight_legion_ordering(numdims,
                                                     dims,
                                                     data_type,
                                                     ln,
                                                     false /*create_grad*/,
                                                     nullptr,
                                                     CHOSEN_SYNC_TYPE);
      if (num_weights == 3) {
        assert(use_bias);
        ln->weights[2] = create_weight_legion_ordering(numdims,
                                                       dims,
                                                       data_type,
                                                       ln,
                                                       false /*create_grad*/,
                                                       nullptr,
                                                       CHOSEN_SYNC_TYPE);
      }
    }
  }
  ln->add_int_property("elementwise_affine", elementwise_affine);
  ln->add_int_property("use_bias", use_bias);
  ln->add_int_vector_property("axes", axes);
  ln->add_float_property("eps", eps);
  ln->add_int_property("inplace_residual", inplace_residual);
  layers.push_back(ln);
  outputs[0] = ln->outputs[0];
  outputs[1] = ln->outputs[1];
}

Op *AddBiasResidualLayerNorm::create_operator_from_layer(
    FFModel &model,
    Layer const *layer,
    std::vector<ParallelTensor> const &inputs) {
  long long value;
  layer->get_int_property("elementwise_affine", value);
  bool elementwise_affine = (bool)value;
  layer->get_int_property("use_bias", value);
  bool use_bias = (bool)value;
  std::vector<int> axes;
  layer->get_int_vector_property("axes", axes);
  float eps;
  layer->get_float_property("eps", eps);
  layer->get_int_property("inplace_residual", value);
  bool inplace_residual = (bool)value;
  return new AddBiasResidualLayerNorm(model,
                                      layer->layer_guid,
                                      inputs[0],
                                      inputs[1],
                                      axes,
                                      elementwise_affine,
                                      use_bias,
                                      eps,
                                      inplace_residual,
                                      false, // allocate_weights
                                      layer->name);
}

AddBiasResidualLayerNorm::AddBiasResidualLayerNorm(
    FFModel &model,
    AddBiasResidualLayerNormParams const &params,
    std::pair<ParallelTensor, ParallelTensor> const &inputs,
    char const *name,
    bool allocate_weights)
    : AddBiasResidualLayerNorm(model,
                               params.layer_guid,
                               inputs.first,
                               inputs.second,
                               params.axes,
                               params.elementwise_affine,
                               params.use_bias,
                               params.eps,
                               params.inplace_residual,
                               allocate_weights,
                               params.name) {}

AddBiasResidualLayerNorm::AddBiasResidualLayerNorm(
    FFModel &model,
    LayerID const &_layer_guid,
    const ParallelTensor _input,
    const ParallelTensor _residual,
    std::vector<int> const &_axes,
    bool _elementwise_affine,
    bool _use_bias,
    float _eps,
    bool _inplace_residual,
    bool allocate_weights,
    char const *name)
    : Op(model,
         OP_ADD_BIAS_RESIDUAL_LAYERNORM,
         _input->data_type,
         name,
         2 /*inputs*/,
         1 + (_elementwise_affine ? (_use_bias ? 2 : 1) : 0) /*weights*/,
         2 /*outputs*/,
         _input,
         _residual),
      elementwise_affine(_elementwise_affine), eps(_eps), axes(_axes),
      use_bias(_use_bias), inplace_residual(_inplace_residual) {
  // overwrite layer_guid
  layer_guid = _layer_guid;
  outputs[0] = model.create_parallel_tensor_legion_ordering(
      _input->num_dims, _input->dims, _input->data_type, this, 0 /*owner_idx*/);
  outputs[1] = model.create_parallel_tensor_legion_ordering(
      _input->num_dims, _input->dims, _input->data_type, this, 1 /*owner_idx*/);
  assert(check_output_input_weight_parallel_dims(allocate_weights));

  int M = 1;
  for (int i = 0; i < axes.size(); i++) {
    M *= inputs[0]->dims[axes[i]].size;
  }
  int num_replicas = 1;
  for (int i = 0; i < inputs[0]->num_dims; i++) {
    if (inputs[0]->dims[i].is_replica_dim) {
      num_replicas *= inputs[0]->dims[i].size;
    }
  }
  effective_num_elements = M;
  effective_batch_size = (inputs[0]->get_volume() / num_replicas) / M;
  if (!elementwise_affine) {
    assert(numWeights == 1); // attn bias
  } else {
    if (!use_bias) {
      assert(numWeights == 2); // attn bias + weight
    } else {
      assert(numWeights == 3); // attn bias + weight + bias
    }
  }

  if (allocate_weights) {
    // always need to allocate attn bias
    ParallelTensorShape attention_bias_shape = _input->get_shape();
    for (int i = 1; i < attention_bias_shape.num_dims - 1; i++) {
      attention_bias_shape.dims[i].size = 1;
    }

    int seed = std::rand();
    Initializer *attn_bias_initializer =
        new UniformInitializer(seed, 1.0f, 1.0f);

    weights[0] = model.create_parallel_weight_legion_ordering(
        attention_bias_shape.num_dims,
        attention_bias_shape.dims,
        _input->data_type,
        NULL /*owner_op*/,
        false /*create_grad*/,
        attn_bias_initializer,
        CHOSEN_SYNC_TYPE);

    if (numWeights > 1) {
      assert(elementwise_affine);

      ParallelTensorShape beta_gamma_shape = _input->get_shape();
      for (int i = axes.size(); i < beta_gamma_shape.num_dims - 1; i++) {
        beta_gamma_shape.dims[i].size = 1;
      }

      // weight
      Initializer *gamma_initializer = new UniformInitializer(seed, 1.0f, 1.0f);
      weights[1] = model.create_parallel_weight_legion_ordering(
          beta_gamma_shape.num_dims, // axes.size(),
          beta_gamma_shape.dims,
          _input->data_type,
          NULL /*owner_op*/,
          false /*create_grad*/,
          gamma_initializer,
          CHOSEN_SYNC_TYPE);

      // bias
      if (numWeights == 3) {
        assert(use_bias);
        Initializer *beta_initializer =
            new UniformInitializer(seed, 0.0f, 0.0f);
        weights[2] = model.create_parallel_weight_legion_ordering(
            beta_gamma_shape.num_dims, //.size(),
            beta_gamma_shape.dims,
            _input->data_type,
            NULL /*owner_op*/,
            false /*create_grad*/,
            beta_initializer,
            CHOSEN_SYNC_TYPE);
      }
    }
  }
}

void AddBiasResidualLayerNorm::init_inference(
    FFModel const &ff,
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
  IndexLauncher launcher(ADD_BIAS_RESIDUAL_LAYERNORM_INIT_TASK_ID,
                         parallel_is,
                         TaskArgument(this, sizeof(AddBiasResidualLayerNorm)),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         machine_view_hash);
  if (inplace_residual) {
    assert(batch_outputs[0]->part == batch_inputs[0]->part);
    assert(batch_outputs[0]->region == batch_inputs[0]->region);
  }
  // attn output
  // added: attn_output + attn final bias + residual
  int fid = 0;
  launcher.add_region_requirement(
      RegionRequirement(batch_inputs[0]->part,
                        0 /*projection id*/,
                        inplace_residual ? READ_WRITE : READ_ONLY,
                        EXCLUSIVE,
                        batch_inputs[0]->region));
  launcher.add_field(fid++, FID_DATA);
  // residual
  launcher.add_region_requirement(RegionRequirement(batch_inputs[1]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    batch_inputs[1]->region));
  launcher.add_field(fid++, FID_DATA);
  if (!inplace_residual) {
    launcher.add_region_requirement(
        RegionRequirement(batch_outputs[0]->part,
                          0 /*projection id*/,
                          WRITE_ONLY,
                          EXCLUSIVE,
                          batch_outputs[0]->region));
    launcher.add_field(fid++, FID_DATA);
  }
  // layer norm output
  launcher.add_region_requirement(RegionRequirement(batch_outputs[1]->part,
                                                    0 /*projection id*/,
                                                    WRITE_ONLY,
                                                    EXCLUSIVE,
                                                    batch_outputs[1]->region));
  launcher.add_field(fid++, FID_DATA);
  // attn final bias
  launcher.add_region_requirement(RegionRequirement(weights[0]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    weights[0]->region));
  launcher.add_field(fid++, FID_DATA);
  if (elementwise_affine) {
    launcher.add_region_requirement(RegionRequirement(weights[1]->part,
                                                      0 /*projection id*/,
                                                      READ_ONLY,
                                                      EXCLUSIVE,
                                                      weights[1]->region));
    launcher.add_field(fid++, FID_DATA);

    if (use_bias) {
      launcher.add_region_requirement(RegionRequirement(weights[2]->part,
                                                        0 /*projection id*/,
                                                        READ_ONLY,
                                                        EXCLUSIVE,
                                                        weights[2]->region));
      launcher.add_field(fid++, FID_DATA);
    }
  }
  FutureMap fm = runtime->execute_index_space(ctx, launcher);
  fm.wait_all_results();
  set_opmeta_from_futuremap_inference(ff, fm, batch_outputs[0]);
}

void AddBiasResidualLayerNorm::init(FFModel const &ff) {
  assert(check_output_input_weight_same_parallel_is());
  parallel_is = outputs[0]->parallel_is;
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  set_argumentmap_for_init(ff, argmap);
  IndexLauncher launcher(ADD_BIAS_RESIDUAL_LAYERNORM_INIT_TASK_ID,
                         parallel_is,
                         TaskArgument(this, sizeof(AddBiasResidualLayerNorm)),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         outputs[0]->machine_view.hash());
  if (inplace_residual) {
    assert(outputs[0]->part == inputs[0]->part);
    assert(outputs[0]->region == inputs[0]->region);
  }
  // input: attn output
  // added: attn_output + attn final bias + residual
  int fid = 0;
  launcher.add_region_requirement(
      RegionRequirement(inputs[0]->part,
                        0 /*projection id*/,
                        inplace_residual ? READ_WRITE : READ_ONLY,
                        EXCLUSIVE,
                        inputs[0]->region));
  launcher.add_field(fid++, FID_DATA);
  // residual
  launcher.add_region_requirement(RegionRequirement(inputs[1]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    inputs[1]->region));
  launcher.add_field(fid++, FID_DATA);
  if (!inplace_residual) {
    launcher.add_region_requirement(RegionRequirement(outputs[0]->part,
                                                      0 /*projection id*/,
                                                      WRITE_ONLY,
                                                      EXCLUSIVE,
                                                      outputs[0]->region));
    launcher.add_field(fid++, FID_DATA);
  }
  // layer norm output
  launcher.add_region_requirement(RegionRequirement(outputs[1]->part,
                                                    0 /*projection id*/,
                                                    WRITE_ONLY,
                                                    EXCLUSIVE,
                                                    outputs[1]->region));
  launcher.add_field(fid++, FID_DATA);
  // attn final bias
  launcher.add_region_requirement(RegionRequirement(weights[0]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    weights[0]->region));
  launcher.add_field(fid++, FID_DATA);
  if (elementwise_affine) {
    launcher.add_region_requirement(RegionRequirement(weights[1]->part,
                                                      0 /*projection id*/,
                                                      READ_ONLY,
                                                      EXCLUSIVE,
                                                      weights[1]->region));
    launcher.add_field(fid++, FID_DATA);

    if (use_bias) {
      launcher.add_region_requirement(RegionRequirement(weights[2]->part,
                                                        0 /*projection id*/,
                                                        READ_ONLY,
                                                        EXCLUSIVE,
                                                        weights[2]->region));
      launcher.add_field(fid++, FID_DATA);
    }
  }
  FutureMap fm = runtime->execute_index_space(ctx, launcher);
  fm.wait_all_results();
  set_opmeta_from_futuremap(ff, fm);
}

/*
  regions[0](I/O): attn output AND added output (attn output + final attn bias +
  residual) regions[1](I): residual regions[2](O): layer norm output
  regions[3](I): final attn bias
  regions[4](I): gamma
  regions[5](I): beta
*/
OpMeta *AddBiasResidualLayerNorm::init_task(
    Task const *task,
    std::vector<PhysicalRegion> const &regions,
    Context ctx,
    Runtime *runtime) {
  AddBiasResidualLayerNorm *ln = (AddBiasResidualLayerNorm *)task->args;
  FFHandler handle = *((FFHandler const *)task->local_args);
  Memory gpu_mem = get_proc_mem(Machine::get_machine(), task->target_proc);
  MemoryAllocator gpu_mem_allocator(gpu_mem);
  AddBiasResidualLayerNormMeta *meta =
      new AddBiasResidualLayerNormMeta(handle, ln, gpu_mem_allocator);
  meta->input_type[0] = ln->inputs[0]->data_type;
  meta->input_type[1] = ln->inputs[1]->data_type;
  meta->weight_type[0] = ln->weights[0]->data_type;
  if (ln->elementwise_affine) {
    meta->weight_type[1] = ln->weights[1]->data_type;
    if (ln->use_bias) {
      meta->weight_type[2] = ln->weights[2]->data_type;
    }
  }
  meta->output_type[0] = ln->outputs[0]->data_type;
  meta->output_type[1] = ln->outputs[1]->data_type;
  std::strcpy(meta->op_name, ln->name);
  meta->layer_guid = ln->layer_guid;
  return meta;
}

void AddBiasResidualLayerNorm::forward(FFModel const &ff) {
  assert(false);
}

FutureMap AddBiasResidualLayerNorm::inference(
    FFModel const &ff,
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
  /* std::cout << "AddBiasResidualLayerNorm op machine_view: " << *(MachineView
     const *)mv
            << std::endl; */
  IndexLauncher launcher(ADD_BIAS_RESIDUAL_LAYERNORM_INF_TASK_ID,
                         parallel_is,
                         TaskArgument(NULL, 0),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         machine_view_hash);
  launcher.add_future(bc);
  if (inplace_residual) {
    assert(batch_outputs[0]->part == batch_inputs[0]->part);
    assert(batch_outputs[0]->region == batch_inputs[0]->region);
  }
  int fid = 0;
  // input
  // added_output: input + attn bias + residual
  launcher.add_region_requirement(
      RegionRequirement(batch_inputs[0]->part,
                        0 /*projection id*/,
                        inplace_residual ? READ_WRITE : READ_ONLY,
                        EXCLUSIVE,
                        batch_inputs[0]->region));
  launcher.add_field(fid++, FID_DATA);
  // attn bias
  launcher.add_region_requirement(RegionRequirement(weights[0]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    weights[0]->region));
  launcher.add_field(fid++, FID_DATA);
  // residual
  launcher.add_region_requirement(RegionRequirement(batch_inputs[1]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    batch_inputs[1]->region));
  launcher.add_field(fid++, FID_DATA);
  if (!inplace_residual) {
    launcher.add_region_requirement(
        RegionRequirement(batch_outputs[0]->part,
                          0 /*projection id*/,
                          WRITE_ONLY,
                          EXCLUSIVE,
                          batch_outputs[0]->region));
    launcher.add_field(fid++, FID_DATA);
  }
  // output
  launcher.add_region_requirement(RegionRequirement(batch_outputs[1]->part,
                                                    0 /*projection id*/,
                                                    WRITE_ONLY,
                                                    EXCLUSIVE,
                                                    batch_outputs[1]->region));
  launcher.add_field(fid++, FID_DATA);
  if (elementwise_affine) {
    // gamma
    launcher.add_region_requirement(RegionRequirement(weights[1]->part,
                                                      0 /*projection id*/,
                                                      READ_ONLY,
                                                      EXCLUSIVE,
                                                      weights[1]->region));
    launcher.add_field(fid++, FID_DATA);
    if (use_bias) {
      // beta
      launcher.add_region_requirement(RegionRequirement(weights[2]->part,
                                                        0 /*projection id*/,
                                                        READ_ONLY,
                                                        EXCLUSIVE,
                                                        weights[2]->region));
      launcher.add_field(fid++, FID_DATA);
    }
  }
  return runtime->execute_index_space(ctx, launcher);
}

void AddBiasResidualLayerNorm::map_output_tensors(FFModel &ff) {
  assert(numOutputs == 2);
  assert(outputs[0]->get_volume() == inputs[0]->get_volume());
  if (inplace_residual) {
    outputs[0]->parallel_is = inputs[0]->parallel_is;
    outputs[0]->region = inputs[0]->region;
    outputs[0]->part = inputs[0]->part;
    outputs[0]->region_grad = inputs[0]->region_grad;
    outputs[0]->part_grad = inputs[0]->part_grad;
    // map output 1 to new region
    ff.map_tensor(outputs[1], this);
  } else {
    Op::map_output_tensors(ff);
  }
}

/*
  regions[0](I): input / added output
  regions[1](I): attn bias
  regions[2](I): residual
  regions[3](O): output
  regions[4](I): gamma
  regions[5](I): beta
*/
void AddBiasResidualLayerNorm::inference_task(
    Task const *task,
    std::vector<PhysicalRegion> const &regions,
    Context ctx,
    Runtime *runtime) {

  assert(task->regions.size() == regions.size());
  BatchConfig const *bc = BatchConfig::from_future(task->futures[0]);
  if (bc->num_tokens == 0) {
    return;
  }

  AddBiasResidualLayerNormMeta *m =
      *((AddBiasResidualLayerNormMeta **)task->local_args);

  assert(regions.size() ==
         4 + (m->elementwise_affine ? (m->use_bias ? 2 : 1) : 0));

  int rid = 0, tid = 0, did = 0;
  GenericTensorAccessorR input =
      helperGetGenericTensorAccessorRO(m->input_type[0],
                                       regions[rid++],
                                       task->regions[tid++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorR attn_bias =
      helperGetGenericTensorAccessorRO(m->weight_type[0],
                                       regions[rid++],
                                       task->regions[tid++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorR residual =
      helperGetGenericTensorAccessorRO(m->input_type[1],
                                       regions[rid++],
                                       task->regions[tid++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorW added_output;
  if (m->inplace_residual) {
    added_output = helperGetGenericTensorAccessorWO(m->output_type[0],
                                                    regions[0],
                                                    task->regions[0],
                                                    FID_DATA,
                                                    ctx,
                                                    runtime);
  } else {
    added_output = helperGetGenericTensorAccessorWO(m->output_type[0],
                                                    regions[rid++],
                                                    task->regions[tid++],
                                                    FID_DATA,
                                                    ctx,
                                                    runtime);
  }
  GenericTensorAccessorW output =
      helperGetGenericTensorAccessorWO(m->output_type[1],
                                       regions[rid++],
                                       task->regions[tid++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorR gamma, beta;

  Domain in_domain = runtime->get_index_space_domain(
      ctx, task->regions[did++].region.get_index_space());
  Domain attn_bias_domain = runtime->get_index_space_domain(
      ctx, task->regions[did++].region.get_index_space());
  Domain residual_domain = runtime->get_index_space_domain(
      ctx, task->regions[did++].region.get_index_space());
  Domain added_out_domain;
  if (m->inplace_residual) {
    added_out_domain = runtime->get_index_space_domain(
        ctx, task->regions[0].region.get_index_space());
  } else {
    added_out_domain = runtime->get_index_space_domain(
        ctx, task->regions[did++].region.get_index_space());
  }
  Domain out_domain = runtime->get_index_space_domain(
      ctx, task->regions[did++].region.get_index_space());

  Domain gamma_domain, beta_domain;

  assert(in_domain.get_volume() == out_domain.get_volume());
  assert(out_domain.get_volume() == added_out_domain.get_volume());
  assert(in_domain.get_volume() == residual_domain.get_volume());
  assert(in_domain == out_domain);
  assert(added_out_domain == out_domain);
  assert(residual_domain == in_domain);

  coord_t attn_bias_dim =
      attn_bias_domain.hi()[0] - attn_bias_domain.lo()[0] + 1;
  assert((in_domain.hi()[0] - in_domain.lo()[0] + 1) == attn_bias_dim);
  assert((residual_domain.hi()[0] - residual_domain.lo()[0] + 1) ==
         attn_bias_dim);
  assert((out_domain.hi()[0] - out_domain.lo()[0] + 1) == attn_bias_dim);
  assert((added_out_domain.hi()[0] - added_out_domain.lo()[0] + 1) ==
         attn_bias_dim);

  assert(in_domain.get_volume() ==
         m->effective_num_elements * m->effective_batch_size);

  if (m->elementwise_affine) {
    gamma = helperGetGenericTensorAccessorRO(m->weight_type[1],
                                             regions[rid++],
                                             task->regions[tid++],
                                             FID_DATA,
                                             ctx,
                                             runtime);
    gamma_domain = runtime->get_index_space_domain(
        ctx, task->regions[did++].region.get_index_space());

    if (m->use_bias) {
      beta = helperGetGenericTensorAccessorRO(m->weight_type[2],
                                              regions[rid++],
                                              task->regions[tid++],
                                              FID_DATA,
                                              ctx,
                                              runtime);
      beta_domain = runtime->get_index_space_domain(
          ctx, task->regions[did++].region.get_index_space());
      assert(gamma_domain == beta_domain);
    }

    assert(gamma_domain.get_volume() == m->effective_num_elements);
    int numdims = gamma_domain.get_dim();
    size_t vol = 1;
    int i = 0;
    while (vol < gamma_domain.get_volume()) {
      int g_d = gamma_domain.hi()[i] - gamma_domain.lo()[i] + 1;
      int in_d = in_domain.hi()[i] - in_domain.lo()[i] + 1;
      assert(g_d == in_d);
      vol *= g_d;
      i++;
    }
  }

  AddBiasResidualLayerNorm::inference_kernel_wrapper(
      m, bc, input, attn_bias, residual, added_output, output, gamma, beta);

  if (m->inference_debugging) {
    assert(task->index_point.get_dim() == 1);
    int shard_id = task->index_point.point_data[0];
    std::vector<GenericTensorAccessorR> weights_accessors;
    weights_accessors.push_back(attn_bias);
    if (m->elementwise_affine) {
      weights_accessors.push_back(gamma);
      if (m->use_bias) {
        weights_accessors.push_back(beta);
      }
    }
    AddBiasResidualLayerNorm::save_inference_tensors_to_file(
        m, shard_id, bc, {residual}, weights_accessors, {added_output, output});
  }
}

void AddBiasResidualLayerNorm::backward(FFModel const &ff) {
  ArgumentMap argmap;
  Context ctx = ff.config.lg_ctx;
  Runtime *runtime = ff.config.lg_hlr;
  set_argumentmap_for_backward(ff, argmap);
  IndexLauncher launcher(ADD_BIAS_RESIDUAL_LAYERNORM_BWD_TASK_ID,
                         parallel_is,
                         TaskArgument(NULL, 0),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         outputs[0]->machine_view.hash());
  int field_id = 0;
  // output_grad
  launcher.add_region_requirement(RegionRequirement(outputs[1]->part_grad,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    outputs[1]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  // added output
  launcher.add_region_requirement(RegionRequirement(outputs[0]->part,
                                                    0 /*projection id*/,
                                                    READ_ONLY,
                                                    EXCLUSIVE,
                                                    outputs[0]->region));
  launcher.add_field(field_id++, FID_DATA);
  // input grad
  launcher.add_region_requirement(RegionRequirement(inputs[0]->part_grad,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    inputs[0]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  // residual grad
  launcher.add_region_requirement(RegionRequirement(inputs[1]->part_grad,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    inputs[1]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  // attn bias
  launcher.add_region_requirement(RegionRequirement(weights[0]->part_grad,
                                                    0 /*projection id*/,
                                                    READ_WRITE,
                                                    EXCLUSIVE,
                                                    weights[0]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  if (elementwise_affine) {
    // gamma
    launcher.add_region_requirement(RegionRequirement(weights[1]->part,
                                                      0 /*projection id*/,
                                                      READ_ONLY,
                                                      EXCLUSIVE,
                                                      weights[1]->region));
    launcher.add_field(field_id++, FID_DATA);
    // gamma_grad
    launcher.add_region_requirement(RegionRequirement(weights[1]->part_grad,
                                                      0 /*projection id*/,
                                                      READ_WRITE,
                                                      EXCLUSIVE,
                                                      weights[1]->region_grad));
    launcher.add_field(field_id++, FID_DATA);
    if (use_bias) {
      // beta_grad
      launcher.add_region_requirement(
          RegionRequirement(weights[2]->part_grad,
                            0 /*projection id*/,
                            READ_WRITE,
                            EXCLUSIVE,
                            weights[2]->region_grad));
      launcher.add_field(field_id++, FID_DATA);
    }
  }
  runtime->execute_index_space(ctx, launcher);
}

void AddBiasResidualLayerNorm::backward_task(
    Task const *task,
    std::vector<PhysicalRegion> const &regions,
    Context ctx,
    Runtime *runtime) {
  assert(task->regions.size() == regions.size());
  AddBiasResidualLayerNormMeta *m =
      *((AddBiasResidualLayerNormMeta **)task->local_args);
  assert(regions.size() ==
         5 + (m->elementwise_affine ? (m->use_bias ? 3 : 2) : 0));

  int region_idx = 0, task_region_idx = 0;

  GenericTensorAccessorR output_grad =
      helperGetGenericTensorAccessorRO(m->output_type[1],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorR added_output =
      helperGetGenericTensorAccessorRO(m->output_type[0],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorW input_grad =
      helperGetGenericTensorAccessorRW(m->input_type[0],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorW residual_grad =
      helperGetGenericTensorAccessorRW(m->input_type[1],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorW attn_bias_grad =
      helperGetGenericTensorAccessorRW(m->input_type[2],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorR gamma;
  GenericTensorAccessorW gamma_grad, beta_grad;
  if (m->elementwise_affine) {
    assert(m->use_bias == (regions.size() == 6));
    gamma = helperGetGenericTensorAccessorRO(m->output_type[0],
                                             regions[region_idx++],
                                             task->regions[task_region_idx++],
                                             FID_DATA,
                                             ctx,
                                             runtime);
    gamma_grad =
        helperGetGenericTensorAccessorRW(m->output_type[0],
                                         regions[region_idx++],
                                         task->regions[task_region_idx++],
                                         FID_DATA,
                                         ctx,
                                         runtime);
    if (m->use_bias) {
      beta_grad =
          helperGetGenericTensorAccessorRW(m->output_type[0],
                                           regions[region_idx++],
                                           task->regions[task_region_idx++],
                                           FID_DATA,
                                           ctx,
                                           runtime);
    }
  }
  AddBiasResidualLayerNorm::backward_kernel_wrapper(m,
                                                    output_grad,
                                                    added_output,
                                                    input_grad,
                                                    residual_grad,
                                                    attn_bias_grad,
                                                    gamma,
                                                    gamma_grad,
                                                    beta_grad);
}

Legion::FutureMap AddBiasResidualLayerNorm::peft_bwd(
    FFModel const &ff,
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
  IndexLauncher launcher(ADD_BIAS_RESIDUAL_LAYERNORM_PEFT_BWD_TASK_ID,
                         parallel_is,
                         TaskArgument(NULL, 0),
                         argmap,
                         Predicate::TRUE_PRED,
                         false /*must*/,
                         0 /*mapper_id*/,
                         machine_view_hash);
  launcher.add_future(bc);
  int field_id = 0;
  // output_grad
  launcher.add_region_requirement(
      RegionRequirement(batch_outputs[1]->part_grad,
                        0 /*projection id*/,
                        READ_WRITE,
                        EXCLUSIVE,
                        batch_outputs[1]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  // input grad
  launcher.add_region_requirement(
      RegionRequirement(batch_inputs[0]->part_grad,
                        0 /*projection id*/,
                        reset_input_grads[0] ? WRITE_ONLY : READ_WRITE,
                        EXCLUSIVE,
                        batch_inputs[0]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  // residual grad
  launcher.add_region_requirement(
      RegionRequirement(batch_inputs[1]->part_grad,
                        0 /*projection id*/,
                        reset_input_grads[1] ? WRITE_ONLY : READ_WRITE,
                        EXCLUSIVE,
                        batch_inputs[1]->region_grad));
  launcher.add_field(field_id++, FID_DATA);
  if (elementwise_affine) {
    // gamma
    launcher.add_region_requirement(RegionRequirement(weights[1]->part,
                                                      0 /*projection id*/,
                                                      READ_ONLY,
                                                      EXCLUSIVE,
                                                      weights[1]->region));
    launcher.add_field(field_id++, FID_DATA);
  }
  return runtime->execute_index_space(ctx, launcher);
}

void AddBiasResidualLayerNorm::peft_bwd_task(
    Task const *task,
    std::vector<PhysicalRegion> const &regions,
    Context ctx,
    Runtime *runtime) {
  BatchConfig const *bc = BatchConfig::from_future(task->futures[0]);
  if (bc->num_active_peft_tokens() == 0) {
    return;
  }
  assert(task->regions.size() == regions.size());
  AddBiasResidualLayerNormMeta *m =
      *((AddBiasResidualLayerNormMeta **)task->local_args);
  assert(regions.size() == 3 + m->elementwise_affine);

  int region_idx = 0, task_region_idx = 0;

  GenericTensorAccessorR output_grad =
      helperGetGenericTensorAccessorRO(m->output_type[1],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorW input_grad =
      helperGetGenericTensorAccessorRW(m->input_type[0],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);
  GenericTensorAccessorW residual_grad =
      helperGetGenericTensorAccessorRW(m->input_type[1],
                                       regions[region_idx++],
                                       task->regions[task_region_idx++],
                                       FID_DATA,
                                       ctx,
                                       runtime);

  GenericTensorAccessorR gamma;
  if (m->elementwise_affine) {
    gamma = helperGetGenericTensorAccessorRO(m->output_type[0],
                                             regions[region_idx++],
                                             task->regions[task_region_idx++],
                                             FID_DATA,
                                             ctx,
                                             runtime);
  }
  AddBiasResidualLayerNorm::peft_bwd_kernel_wrapper(
      m, output_grad, input_grad, residual_grad, gamma);

  if (m->inference_debugging) {
    assert(task->index_point.get_dim() == 1);
    int shard_id = task->index_point.point_data[0];
    std::vector<GenericTensorAccessorR> weights_accessors;
    if (m->elementwise_affine) {
      weights_accessors.push_back(gamma);
    }
    AddBiasResidualLayerNorm::save_inference_tensors_to_file(
        m,
        shard_id,
        bc,
        {input_grad, residual_grad},
        weights_accessors,
        {output_grad},
        false /*fwd_pass*/);
  }
}

bool AddBiasResidualLayerNorm::measure_operator_cost(
    Simulator *sim, MachineView const &mv, CostMetrics &cost_metrics) const {
  return false;
}

void AddBiasResidualLayerNorm::serialize(Legion::Serializer &sez) const {
  sez.serialize(this->layer_guid.id);
  sez.serialize(this->layer_guid.transformer_layer_id);
  sez.serialize(this->layer_guid.model_id);
  sez.serialize(this->axes.size());
  for (size_t i = 0; i < this->axes.size(); i++) {
    sez.serialize(this->axes[i]);
  }
  sez.serialize(this->elementwise_affine);
  sez.serialize(this->eps);
  sez.serialize(this->use_bias);
  sez.serialize(this->inplace_residual);
  sez.serialize(strlen(this->name));
  sez.serialize(this->name, strlen(this->name));
}

using PCG::Node;
/*static*/
Node AddBiasResidualLayerNorm::deserialize(FFModel &ff,
                                           Legion::Deserializer &dez,
                                           ParallelTensor inputs[],
                                           int num_inputs) {
  assert(num_inputs == 2);
  size_t num_axes;
  std::vector<int> axes;
  bool elementwise_affine;
  bool use_bias;
  float eps;
  bool inplace_residual;
  size_t id, transformer_layer_id, deserialized_model_id;
  dez.deserialize(id);
  dez.deserialize(transformer_layer_id);
  dez.deserialize(deserialized_model_id);
  LayerID layer_guid(id, transformer_layer_id, deserialized_model_id);
  dez.deserialize(num_axes);
  for (size_t i = 0; i < num_axes; i++) {
    int axis_idx;
    dez.deserialize(axis_idx);
    axes.push_back(axis_idx);
  }
  dez.deserialize(elementwise_affine);
  dez.deserialize(eps);
  dez.deserialize(use_bias);
  dez.deserialize(inplace_residual);
  size_t name_len;
  char name[MAX_OPNAME] = {0};
  dez.deserialize(name_len);
  dez.deserialize(name, name_len);

  AddBiasResidualLayerNormParams params;
  params.layer_guid = layer_guid;
  params.axes = axes;
  params.elementwise_affine = elementwise_affine;
  params.eps = eps;
  params.use_bias = use_bias;
  params.inplace_residual = inplace_residual;
  strcpy(params.name, name);
  return ff.get_or_create_node<AddBiasResidualLayerNorm>({inputs[0], inputs[1]},
                                                         params);
}

}; // namespace FlexFlow

namespace std {
size_t hash<FlexFlow::AddBiasResidualLayerNormParams>::operator()(
    FlexFlow::AddBiasResidualLayerNormParams const &params) const {
  size_t key = 0;
  hash_combine(key, params.layer_guid.id);
  hash_combine(key, params.layer_guid.transformer_layer_id);
  hash_combine(key, params.layer_guid.model_id);
  hash_combine(key, params.axes.size());
  for (int n : params.axes) {
    hash_combine(key, n);
  }
  hash_combine(key, params.elementwise_affine);
  hash_combine(key, params.use_bias);
  hash_combine(key, params.inplace_residual);
  return key;
}
}; // namespace std
