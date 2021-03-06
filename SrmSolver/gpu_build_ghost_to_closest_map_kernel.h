#pragma once

#include "cuda_includes.h"

#include "boundary_condition.h"
#include "cuda_float_types.h"
#include "get_closest_index.h"
#include "get_extrapolated_ghost_value.h"
#include "level_set_derivatives.h"

namespace kae {

namespace detail {

template <class GpuGridT, class ShapeT, class ElemT>
__global__ void findClosestIndices(thrust::device_ptr<const ElemT>                      pCurrPhi,
                                   thrust::device_ptr<thrust::pair<unsigned, unsigned>> pClosestIndices,
                                   thrust::device_ptr<EBoundaryCondition>               pBoundaryConditions,
                                   thrust::device_ptr<CudaFloat2T<ElemT>>               pNormals)
{
  const unsigned i         = threadIdx.x + blockDim.x * blockIdx.x;
  const unsigned j         = threadIdx.y + blockDim.y * blockIdx.y;
  const unsigned globalIdx = j * GpuGridT::nx + i;
  if ((i >= GpuGridT::nx) || (j >= GpuGridT::ny))
  {
    return;
  }

  if ((i < 10U) || (j < 10U) || (i >= GpuGridT::nx - 10) || (j >= GpuGridT::ny - 10))
  {
    return;
  }

  ElemT nx = getLevelSetDerivative<GpuGridT, 1U>(pCurrPhi.get(), globalIdx, true);
  ElemT ny = getLevelSetDerivative<GpuGridT, GpuGridT::nx>(pCurrPhi.get(), globalIdx, true);
  const ElemT length = std::hypot(nx, ny);
  nx /= length;
  ny /= length;
  pNormals[globalIdx] = { nx, ny };

  const ElemT level = pCurrPhi[globalIdx];
  const bool pointIsGhost = (level >= 0) && (std::fabs(level) < 5 * GpuGridT::hx);
  if (!pointIsGhost)
  {
    return;
  }
  const EBoundaryCondition boundaryCondition = ShapeT::getBoundaryCondition(i * GpuGridT::hx - nx * level, 
                                                                            j * GpuGridT::hy - ny * level);
  if (boundaryCondition == EBoundaryCondition::eWall)
  {
    const ElemT iMirror  = i - 2 * nx * level * GpuGridT::hxReciprocal;
    const ElemT jMirror  = j - 2 * ny * level * GpuGridT::hxReciprocal;

    const int iMirrorInt = std::round(iMirror);
    const int jMirrorInt = std::round(jMirror);

    const ElemT sum      = std::fabs(iMirror - iMirrorInt) + std::fabs(jMirror - jMirrorInt);
    if ((sum < static_cast<ElemT>(0.01) * GpuGridT::hx) && (pCurrPhi[jMirrorInt * GpuGridT::nx + iMirrorInt] < 0))
    {
      const unsigned mirrorGlobalIdx = jMirrorInt * GpuGridT::nx + iMirrorInt;
      pClosestIndices[globalIdx]     = thrust::make_pair(globalIdx, mirrorGlobalIdx);
      pBoundaryConditions[globalIdx] = EBoundaryCondition::eMirror;
      return;
    }
  }

  const unsigned closestGlobalIdx = getClosestIndex<GpuGridT>(pCurrPhi.get(), i, j, nx, ny);
  pClosestIndices[globalIdx]      = thrust::make_pair(globalIdx, closestGlobalIdx);
  pBoundaryConditions[globalIdx]  = boundaryCondition;
}

template <class GpuGridT, class ShapeT, class ElemT>
void findClosestIndicesWrapper(thrust::device_ptr<const ElemT>                      pCurrPhi,
                               thrust::device_ptr<thrust::pair<unsigned, unsigned>> pClosestIndices,
                               thrust::device_ptr<EBoundaryCondition>               pBoundaryConditions,
                               thrust::device_ptr<CudaFloat2T<ElemT>>               pNormals)
{
  findClosestIndices<GpuGridT, ShapeT><<<GpuGridT::gridSize, GpuGridT::blockSize>>>
    (pCurrPhi, pClosestIndices, pBoundaryConditions, pNormals);
  cudaDeviceSynchronize();
}

} // namespace detail

} // namespace kae
