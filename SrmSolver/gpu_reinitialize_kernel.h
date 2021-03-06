#pragma once

#include "cuda_includes.h"

#include "level_set_derivatives.h"

namespace kae {

namespace detail {

template <class GpuGridT, class ShapeT, class ElemT>
__global__ void reinitializeTVDSubStep(thrust::device_ptr<const ElemT> pPrevValue,
                                       thrust::device_ptr<const ElemT> pFirstValue,
                                       thrust::device_ptr<ElemT>       pCurrValue,
                                       ElemT dt, ElemT prevWeight)
{
  const unsigned ti        = threadIdx.x;
  const unsigned ai        = ti + GpuGridT::smExtension;

  const unsigned tj        = threadIdx.y;
  const unsigned aj        = tj + GpuGridT::smExtension;

  const unsigned i         = ti + blockDim.x * blockIdx.x;
  const unsigned j         = tj + blockDim.y * blockIdx.y;
  if ((i >= GpuGridT::nx) || (j >= GpuGridT::ny))
  {
    return;
  }

  constexpr auto smx = GpuGridT::sharedMemory.x;
  const unsigned sharedIdx = aj * smx + ai;
  const unsigned globalIdx = j * GpuGridT::nx + i;

  __shared__ ElemT prevMatrix[GpuGridT::smSize];

  if ((ti < GpuGridT::smExtension) && (i >= GpuGridT::smExtension))
  {
    prevMatrix[sharedIdx - GpuGridT::smExtension] = pPrevValue[globalIdx - GpuGridT::smExtension];
  }

  if ((tj < GpuGridT::smExtension) && (j >= GpuGridT::smExtension))
  {
    prevMatrix[(aj - GpuGridT::smExtension) * smx + ai] = pPrevValue[(j - GpuGridT::smExtension) * GpuGridT::nx + i];
  }

  prevMatrix[sharedIdx] = pPrevValue[globalIdx];

  if ((ti >= blockDim.x - GpuGridT::smExtension) && (i + GpuGridT::smExtension < GpuGridT::nx))
  {
    prevMatrix[sharedIdx + GpuGridT::smExtension] = pPrevValue[globalIdx + GpuGridT::smExtension];
  }

  if ((tj >= blockDim.y - GpuGridT::smExtension) && (j + GpuGridT::smExtension < GpuGridT::ny))
  {
    prevMatrix[(aj + GpuGridT::smExtension) * smx + ai] = pPrevValue[(j + GpuGridT::smExtension) * GpuGridT::nx + i];
  }

  __syncthreads();

  const bool schemeShouldBeApplied = (i > GpuGridT::smExtension + 1) && 
                                     (i < GpuGridT::nx - GpuGridT::smExtension - 2) && 
                                     (j > GpuGridT::smExtension + 1) && 
                                     (j < GpuGridT::ny - GpuGridT::smExtension - 2) && 
                                      ShapeT::shouldApplyScheme(i, j);

  if (schemeShouldBeApplied)
  {
    const ElemT sgdValue = prevMatrix[sharedIdx];
    const ElemT grad     = getLevelSetAbsGradient<GpuGridT, smx>(prevMatrix, sharedIdx, (sgdValue > 0));
    const ElemT sgn      = sgdValue / std::hypot(sgdValue, grad * GpuGridT::hx);
    const ElemT val      = sgdValue - dt * sgn * (grad - static_cast<ElemT>(1.0));

    if (prevWeight != static_cast<ElemT>(1.0))
    {
      pCurrValue[globalIdx] = (1 - prevWeight) * pFirstValue[globalIdx] + prevWeight * val;
    }
    else
    {
      pCurrValue[globalIdx] = val;
    }
  }
}

template <class GpuGridT, class ShapeT, class ElemT>
void reinitializeTVDSubStepWrapper(thrust::device_ptr<const ElemT> pPrevValue,
                                   thrust::device_ptr<const ElemT> pFirstValue,
                                   thrust::device_ptr<ElemT>       pCurrValue,
                                   ElemT dt, ElemT prevWeight)
{
  reinitializeTVDSubStep<GpuGridT, ShapeT><<<GpuGridT::gridSize, GpuGridT::blockSize>>>
  (pPrevValue, pFirstValue, pCurrValue, dt, prevWeight);
  cudaDeviceSynchronize();
}

} // namespace detail

} // namespace kae
