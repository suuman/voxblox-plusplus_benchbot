#ifndef VOXBLOX_GSM_CONVERSIONS_H_
#define VOXBLOX_GSM_CONVERSIONS_H_

#include <vector>

#include <geometry_msgs/Transform.h>
#include <modelify_msgs/VoxelEvaluationDetails.h>
#include <pcl/point_types.h>
#include <voxblox/core/common.h>

namespace voxblox {
namespace voxblox_gsm {

inline void transformMsgs2Transformations(
    const std::vector<geometry_msgs::Transform>& transforms,
    std::vector<Transformation>* transformations) {
  CHECK_NOTNULL(transformations);
  for (const geometry_msgs::Transform& transform : transforms) {
    voxblox::Rotation quaternion(transform.rotation.w, transform.rotation.x,
                                 transform.rotation.y, transform.rotation.z);
    voxblox::Point translation(transform.translation.x, transform.translation.y,
                               transform.translation.z);
    transformations->emplace_back(quaternion, translation);
  }
}
inline void voxelEvaluationDetails2VoxelEvaluationDetailsMsg(
    const std::vector<voxblox::utils::VoxelEvaluationDetails>&
        voxel_evaluation_details_vector,
    std::vector<modelify_msgs::VoxelEvaluationDetails>*
        voxel_evaluation_details_msgs) {
  CHECK_NOTNULL(voxel_evaluation_details_msgs)->clear();

  for (const voxblox::utils::VoxelEvaluationDetails& voxel_evaluation_details :
       voxel_evaluation_details_vector) {
    // TODO(ff): Move this to voxblox_ros conversions.h.
    modelify_msgs::VoxelEvaluationDetails voxel_evaluation_details_msg;
    voxel_evaluation_details_msg.rmse = voxel_evaluation_details.rmse;
    voxel_evaluation_details_msg.max_error = voxel_evaluation_details.max_error;
    voxel_evaluation_details_msg.min_error = voxel_evaluation_details.min_error;
    voxel_evaluation_details_msg.num_evaluated_voxels =
        voxel_evaluation_details.num_evaluated_voxels;
    voxel_evaluation_details_msg.num_ignored_voxels =
        voxel_evaluation_details.num_ignored_voxels;
    voxel_evaluation_details_msg.num_overlapping_voxels =
        voxel_evaluation_details.num_overlapping_voxels;
    voxel_evaluation_details_msg.num_non_overlapping_voxels =
        voxel_evaluation_details.num_non_overlapping_voxels;
    voxel_evaluation_details_msgs->push_back(voxel_evaluation_details_msg);
  }
}

inline void convertVoxelGridToPointCloud(
    const voxblox::Layer<voxblox::TsdfVoxel>& tsdf_voxels,
    pcl::PointCloud<pcl::PointSurfel>* surfel_cloud) {
  CHECK_NOTNULL(surfel_cloud);

  voxblox::MeshIntegrator<voxblox::TsdfVoxel>::Config mesh_config;
  voxblox::MeshLayer::Ptr mesh_layer(
      new voxblox::MeshLayer(tsdf_voxels.block_size()));
  voxblox::MeshIntegrator<voxblox::TsdfVoxel> mesh_integrator(
      mesh_config, tsdf_voxels, mesh_layer.get());

  constexpr bool kOnlyMeshUpdatedBlocks = false;
  constexpr bool kClearUpdatedFlag = false;
  mesh_integrator.generateMesh(kOnlyMeshUpdatedBlocks, kClearUpdatedFlag);

  voxblox::Mesh::Ptr mesh = voxblox::aligned_shared<voxblox::Mesh>(
      mesh_layer->block_size(), voxblox::Point::Zero());
  mesh_layer->combineMesh(mesh);

  size_t vert_idx = 0u;
  for (const voxblox::Point& vert : mesh->vertices) {
    pcl::PointSurfel point;
    point.x = vert(0);
    point.y = vert(1);
    point.z = vert(2);

    if (mesh->hasColors()) {
      const voxblox::Color& color = mesh->colors[vert_idx];
      point.r = static_cast<int>(color.r);
      point.g = static_cast<int>(color.g);
      point.b = static_cast<int>(color.b);
    }

    if (mesh->hasNormals()) {
      const voxblox::Point& normal = mesh->normals[vert_idx];
      point.normal_x = normal(0);
      point.normal_y = normal(1);
      point.normal_z = normal(2);
    } else {
      LOG(FATAL) << "FATAAKK";
    }

    surfel_cloud->push_back(point);
    ++vert_idx;
  }

  surfel_cloud->is_dense = true;
  surfel_cloud->width = surfel_cloud->points.size();
  surfel_cloud->height = 1u;
}

}  // namespace voxblox_gsm
}  // namespace voxblox
#endif  // VOXBLOX_GSM_CONVERSIONS_H_