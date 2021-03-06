#pragma once

#include "cuda_includes.h"

#include "cuda_float_types.h"
#include "gas_dynamic_flux.h"
#include "gas_state.h"

constexpr unsigned maxSizeX = 120U;
constexpr unsigned maxSizeY = 200U;
__constant__ int8_t calculateBlockMatrix[maxSizeX * maxSizeY];

namespace kae {

namespace detail {

template <class GpuGridT, class ShapeT, class GasStateT, class ElemT = typename GasStateT::ElemType>
__global__ void
/*__launch_bounds__ (256, 5)*/
gasDynamicIntegrateTVDSubStep(const GasStateT * __restrict__ pPrevValue,
                                              const GasStateT * __restrict__ pFirstValue,
                                              GasStateT *       __restrict__ pCurrValue,
                                              const ElemT *     __restrict__ pCurrPhi,
                                              ElemT dt, CudaFloat2T<ElemT> lambda, ElemT prevWeight)
{
  constexpr auto     hx             = GpuGridT::hx;
  constexpr auto     levelThreshold = 4 * hx;
  constexpr unsigned smx            = GpuGridT::sharedMemory.x;
  constexpr unsigned nx             = GpuGridT::nx;
  constexpr unsigned ny             = GpuGridT::ny;
  constexpr unsigned smExtension    = GpuGridT::smExtension;
  constexpr unsigned smSize         = smx * GpuGridT::sharedMemory.y;

  constexpr unsigned fluxSmx        = GpuGridT::blockSize.x + 1U;
  constexpr unsigned fluxSmy        = GpuGridT::blockSize.y + 1U;
  constexpr unsigned fluxSmSize     = fluxSmx * fluxSmy;

  const unsigned ti = threadIdx.x;
  const unsigned ai = ti + smExtension;

  const unsigned tj = threadIdx.y;
  const unsigned aj = tj + smExtension;

  const unsigned i = ti + blockDim.x * blockIdx.x;
  const unsigned j = tj + blockDim.y * blockIdx.y;
  if ((i >= nx) || (j >= ny) || (calculateBlockMatrix[blockIdx.y * maxSizeX + blockIdx.x] == 0))
  {
    return;
  }

  const unsigned sharedIdx = aj * smx + ai;
  const unsigned globalIdx = j * nx + i;

  __shared__ GasStateT prevMatrix[smSize];
  __shared__ ElemT xFlux1[fluxSmSize];
  __shared__ ElemT xFlux2[fluxSmSize];
  __shared__ ElemT xFlux3[fluxSmSize];
  __shared__ ElemT xFlux4[fluxSmSize];
  __shared__ ElemT yFlux1[fluxSmSize];
  __shared__ ElemT yFlux2[fluxSmSize];
  __shared__ ElemT yFlux3[fluxSmSize];
  __shared__ ElemT yFlux4[fluxSmSize];

  const auto levelValue = __ldg(&pCurrPhi[globalIdx]);

  if ((ti < smExtension) && (i >= smExtension))
  {
    prevMatrix[sharedIdx - smExtension] = pPrevValue[globalIdx - smExtension];
  }

  if ((tj < smExtension) && (j >= smExtension))
  {
    prevMatrix[sharedIdx - smExtension * smx] = pPrevValue[globalIdx - smExtension * nx];
  }

  if (levelValue < levelThreshold)
  {
    prevMatrix[sharedIdx] = pPrevValue[globalIdx];
  }

  if ((ti >= GpuGridT::blockSize.x - smExtension) && (i + smExtension < nx))
  {
    prevMatrix[sharedIdx + smExtension] = pPrevValue[globalIdx + smExtension];
  }

  if ((tj >= GpuGridT::blockSize.y - smExtension) && (j + smExtension < ny))
  {
    prevMatrix[sharedIdx + smExtension * smx] = pPrevValue[globalIdx + smExtension * nx];
  }

  __syncthreads();

  if ((tj == 1U) && (ti < GpuGridT::blockSize.y))
  {
    const auto transposedSharedIdx = ai * smx + aj - 1U;
    {
      const auto transFluxSharedIdx = (ti + 1U) * fluxSmx + tj;
      xFlux1[transFluxSharedIdx - 1U] = getFlux<Rho, MassFluxX, 1U, GpuGridT>(prevMatrix, transposedSharedIdx - 1U, lambda.x);
      xFlux2[transFluxSharedIdx - 1U] = getFlux<MassFluxX, MomentumFluxXx, 1U, GpuGridT>(prevMatrix, transposedSharedIdx - 1U, lambda.x);
      xFlux3[transFluxSharedIdx - 1U] = getFlux<MassFluxY, MomentumFluxXy, 1U, GpuGridT>(prevMatrix, transposedSharedIdx - 1U, lambda.x);
      xFlux4[transFluxSharedIdx - 1U] = getFlux<RhoEnergy, EnthalpyFluxX, 1U, GpuGridT>(prevMatrix, transposedSharedIdx - 1U, lambda.x);
    }
  }

  const unsigned fluxSharedIdx = (tj + 1U) * fluxSmx + ti + 1U;
  const bool fluxShouldBeCalculated = (levelValue <= hx + static_cast<ElemT>(1e-6));
  if (fluxShouldBeCalculated)
  {
    if (tj == 0U)
    {
      yFlux1[fluxSharedIdx - fluxSmx] = getFlux<Rho, MassFluxY, smx, GpuGridT>(prevMatrix, sharedIdx - smx, lambda.y);
      yFlux2[fluxSharedIdx - fluxSmx] = getFlux<MassFluxX, MomentumFluxXy, smx, GpuGridT>(prevMatrix, sharedIdx - smx, lambda.y);
      yFlux3[fluxSharedIdx - fluxSmx] = getFlux<MassFluxY, MomentumFluxYy, smx, GpuGridT>(prevMatrix, sharedIdx - smx, lambda.y);
      yFlux4[fluxSharedIdx - fluxSmx] = getFlux<RhoEnergy, EnthalpyFluxY, smx, GpuGridT>(prevMatrix, sharedIdx - smx, lambda.y);
    }

    yFlux1[fluxSharedIdx] = getFlux<Rho, MassFluxY, smx, GpuGridT>(prevMatrix, sharedIdx, lambda.y);
    yFlux2[fluxSharedIdx] = getFlux<MassFluxX, MomentumFluxXy, smx, GpuGridT>(prevMatrix, sharedIdx, lambda.y);
    yFlux3[fluxSharedIdx] = getFlux<MassFluxY, MomentumFluxYy, smx, GpuGridT>(prevMatrix, sharedIdx, lambda.y);
    yFlux4[fluxSharedIdx] = getFlux<RhoEnergy, EnthalpyFluxY, smx, GpuGridT>(prevMatrix, sharedIdx, lambda.y);

    xFlux1[fluxSharedIdx] = getFlux<Rho, MassFluxX, 1U, GpuGridT>(prevMatrix, sharedIdx, lambda.x);
    xFlux2[fluxSharedIdx] = getFlux<MassFluxX, MomentumFluxXx, 1U, GpuGridT>(prevMatrix, sharedIdx, lambda.x);
    xFlux3[fluxSharedIdx] = getFlux<MassFluxY, MomentumFluxXy, 1U, GpuGridT>(prevMatrix, sharedIdx, lambda.x);
    xFlux4[fluxSharedIdx] = getFlux<RhoEnergy, EnthalpyFluxX, 1U, GpuGridT>(prevMatrix, sharedIdx, lambda.x);
  }

  __syncthreads();

  const bool schemeShouldBeApplied = (levelValue < 0);
  if (schemeShouldBeApplied)
  {
    const ElemT rReciprocal = 1 / ShapeT::getRadius(i, j);

    GasStateT calculatedGasState = prevMatrix[sharedIdx];
    CudaFloat4T<ElemT> newConservativeVariables =
      {
        Rho::get(calculatedGasState) -
          dt * GpuGridT::hxReciprocal * (xFlux1[fluxSharedIdx] - xFlux1[fluxSharedIdx - 1U]) -
          dt * GpuGridT::hyReciprocal * (yFlux1[fluxSharedIdx] - yFlux1[fluxSharedIdx - fluxSmx]) -
          dt * rReciprocal * MassFluxY::get(calculatedGasState),
        MassFluxX::get(calculatedGasState) -
          dt * GpuGridT::hxReciprocal * (xFlux2[fluxSharedIdx] - xFlux2[fluxSharedIdx - 1U]) -
          dt * GpuGridT::hyReciprocal * (yFlux2[fluxSharedIdx] - yFlux2[fluxSharedIdx - fluxSmx]) -
          dt * rReciprocal * MomentumFluxXy::get(calculatedGasState),
        MassFluxY::get(calculatedGasState) -
          dt * GpuGridT::hxReciprocal * (xFlux3[fluxSharedIdx] - xFlux3[fluxSharedIdx - 1U]) -
          dt * GpuGridT::hyReciprocal * (yFlux3[fluxSharedIdx] - yFlux3[fluxSharedIdx - fluxSmx]) -
          dt * rReciprocal * MassFluxY::get(calculatedGasState) * calculatedGasState.uy,
        RhoEnergy::get(calculatedGasState) -
          dt * GpuGridT::hxReciprocal * (xFlux4[fluxSharedIdx] - xFlux4[fluxSharedIdx - 1U]) -
          dt * GpuGridT::hyReciprocal * (yFlux4[fluxSharedIdx] - yFlux4[fluxSharedIdx - fluxSmx]) -
          dt * rReciprocal * EnthalpyFluxY::get(calculatedGasState)
      };
    if (prevWeight != 1)
    {
      const GasStateT firstGasState = pFirstValue[globalIdx];
      newConservativeVariables.x = prevWeight * newConservativeVariables.x + (1 - prevWeight) * Rho::get(firstGasState);
      newConservativeVariables.y = prevWeight * newConservativeVariables.y + (1 - prevWeight) * MassFluxX::get(firstGasState);
      newConservativeVariables.z = prevWeight * newConservativeVariables.z + (1 - prevWeight) * MassFluxY::get(firstGasState);
      newConservativeVariables.w = prevWeight * newConservativeVariables.w + (1 - prevWeight) * RhoEnergy::get(firstGasState);
    }
    calculatedGasState = ConservativeToGasState::get<GasStateT>(newConservativeVariables);
    pCurrValue[globalIdx] = calculatedGasState;
  }
}

template <class GpuGridT, class ShapeT, class GasStateT, class ElemT>
void gasDynamicIntegrateTVDSubStepWrapper(thrust::device_ptr<const GasStateT> pPrevValue,
                                          thrust::device_ptr<const GasStateT> pFirstValue,
                                          thrust::device_ptr<GasStateT> pCurrValue,
                                          thrust::device_ptr<const ElemT> pCurrPhi,
                                          ElemT dt, CudaFloat2T<ElemT> lambda, ElemT pPrevWeight)
{
  gasDynamicIntegrateTVDSubStep<GpuGridT, ShapeT, GasStateT> << <GpuGridT::gridSize, GpuGridT::blockSize >> >
    (pPrevValue.get(), pFirstValue.get(), pCurrValue.get(), pCurrPhi.get(), dt, lambda, pPrevWeight);
}

} // namespace detail

} // namespace kae
