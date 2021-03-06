#pragma once

#include "cuda_includes.h"

#pragma warning(push, 0)
#include <gcem.hpp>
#pragma warning(pop)

#include "cuda_float_types.h"
#include "math_utilities.h"
#include "matrix.h"

namespace kae {

template <class PhysicalPropertiesT, class ElemT>
struct alignas(16) GasState
{
  using ElemType = ElemT;

  constexpr static ElemT kappa = PhysicalPropertiesT::kappa;
  constexpr static ElemT R     = PhysicalPropertiesT::R;
  ElemT rho;
  ElemT ux;
  ElemT uy;
  ElemT p;
};

struct IsValid
{
  template <class GasStateT>
  HOST_DEVICE bool operator()(const GasStateT & state)
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static bool get(const GasStateT & state)
  {
    return isfinite(state.rho) && isfinite(state.ux) && isfinite(state.uy) && isfinite(state.p) &&
          (state.rho > 0) && (state.p > 0);
  }
};

struct Rho
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.rho;
  }
};

struct P
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.p;
  }
};

struct VelocitySquared
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.ux * state.ux + state.uy * state.uy;
  }
};

struct Velocity
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return std::sqrt(VelocitySquared::get(state));
  }
};

struct MassFluxX
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.rho * state.ux;
  }
};

struct MassFluxY
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.rho * state.uy;
  }
};

struct MomentumFluxXx
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.rho * state.ux * state.ux + state.p;
  }
};

struct MomentumFluxXy
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.rho * state.ux * state.uy;
  }
};

struct MomentumFluxYy
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.rho * state.uy * state.uy + state.p;
  }
};

struct RhoEnergy
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    using ElemType = typename GasStateT::ElemType;
    constexpr ElemType multiplier = static_cast<ElemType>(1.0) / (GasStateT::kappa - static_cast<ElemType>(1.0));
    return multiplier * state.p + static_cast<ElemType>(0.5) * state.rho * VelocitySquared::get(state);
  }
};

struct Energy
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return RhoEnergy::get(state) / state.rho;
  }
};

struct EnthalpyFluxX
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return RhoEnergy::get(state) * state.ux + state.ux * state.p;
  }
};

struct EnthalpyFluxY
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return RhoEnergy::get(state) * state.uy + state.uy * state.p;
  }
};

struct SonicSpeedSquared
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return GasStateT::kappa * state.p / state.rho;
  }
};

struct SonicSpeed
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return std::sqrt(SonicSpeedSquared::get(state));
  }
};

struct Mach
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return std::sqrt(VelocitySquared::get(state) / SonicSpeedSquared::get(state));
  }
};

struct Temperature
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return state.p / state.rho / GasStateT::R;
  }
};

struct Rotate
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE GasStateT operator()(const GasStateT & state, ElemT nx, ElemT ny)
  {
    return get(state, nx, ny);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static GasStateT get(const GasStateT & state, ElemT nx, ElemT ny)
  {
    ElemT newUx = state.ux * nx + state.uy * ny;
    ElemT newUy = -state.ux * ny + state.uy * nx;
    return { state.rho, newUx, newUy, state.p };
  }
};

struct ReverseRotate
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE GasStateT operator()(const GasStateT & state, ElemT nx, ElemT ny)
  {
    return get(state, nx, ny);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static GasStateT get(const GasStateT & state, ElemT nx, ElemT ny)
  {
    ElemT newUx = state.ux * nx - state.uy * ny;
    ElemT newUy = state.ux * ny + state.uy * nx;
    return { state.rho, newUx, newUy, state.p };
  }
};

struct WaveSpeedX
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return SonicSpeed::get(state) + std::fabs(state.ux);
  }
};

struct WaveSpeedY
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return SonicSpeed::get(state) + std::fabs(state.uy);
  }
};

struct WaveSpeedXY
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> CudaFloat2T<ElemT>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> CudaFloat2T<ElemT>
  {
    return { WaveSpeedX::get(state), WaveSpeedY::get(state) };
  }
};

struct WaveSpeed
{
  template <class GasStateT>
  HOST_DEVICE auto operator()(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static auto get(const GasStateT & state) -> typename GasStateT::ElemType
  {
    return SonicSpeed::get(state) + Velocity::get(state);
  }
};

struct MirrorState
{
  template <class GasStateT>
  HOST_DEVICE GasStateT operator()(const GasStateT & state)
  {
    return get(state);
  }

  template <class GasStateT>
  HOST_DEVICE static GasStateT get(const GasStateT & state)
  {
    return { state.rho, -state.ux, state.uy, state.p };
  }
};

struct ConservativeVariables
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return { Rho::get(state), MassFluxX::get(state), MassFluxY::get(state), RhoEnergy::get(state) };
  }
};

struct XFluxes
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return { MassFluxX::get(state),
             MomentumFluxXx::get(state),
             MomentumFluxXy::get(state),
             EnthalpyFluxX::get(state) };
  }
};

struct YFluxes
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return { MassFluxY::get(state),
             MomentumFluxXy::get(state),
             MomentumFluxYy::get(state),
             EnthalpyFluxY::get(state) };
  }
};

struct SourceTerm
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return { MassFluxY::get(state),
             MomentumFluxXy::get(state),
             MassFluxY::get(state) * state.uy,
             EnthalpyFluxY::get(state) };
  }
};

struct ConservativeToGasState
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE GasStateT operator()(const CudaFloat4T<ElemT> & state)
  {
    return get<GasStateT>(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static GasStateT get(const CudaFloat4T<ElemT> & conservativeVariables)
  {
    const ElemT ux = conservativeVariables.y / conservativeVariables.x;
    const ElemT uy = conservativeVariables.z / conservativeVariables.x;
    const ElemT p = (GasStateT::kappa - static_cast<ElemT>(1.0)) *
                    (conservativeVariables.w - static_cast<ElemT>(0.5) * conservativeVariables.x * (ux * ux + uy * uy));
    return GasStateT{ conservativeVariables.x, ux, uy, p };
  }
};

struct EigenValuesX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> CudaFloat4T<ElemT>
  {
    const auto c = SonicSpeed::get(state);
    return { state.ux - c, state.ux, state.ux, state.ux + c };
  }
};

struct EigenValuesMatrixX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    const auto c = SonicSpeed::get(state);
    constexpr auto zero = static_cast<ElemT>(0);
    return kae::Matrix<ElemT, 4, 4>{ state.ux - c, zero,     zero,     zero,
                                     zero,         state.ux, zero,     zero,
                                     zero,         zero,     state.ux, zero,
                                     zero,         zero,     zero,     state.ux + c };
  }
};

template <bool uyIsZero>
struct LeftPrimitiveEigenVectorsX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto zero = static_cast<ElemT>(0.0);
    constexpr auto half = static_cast<ElemT>(0.5);

    const auto cReciprocal = 1 / SonicSpeed::get(state);
    const auto rhoReciprocal = 1 / state.rho;

    return kae::Matrix<ElemT, 4, 4>{
         zero,                          -half * cReciprocal, zero,  half * rhoReciprocal * cReciprocal * cReciprocal,
        -half * state.uy *rhoReciprocal, zero,               half,  half * state.uy * rhoReciprocal * cReciprocal * cReciprocal,
         half * state.uy *rhoReciprocal, zero,               half, -half * state.uy * rhoReciprocal * cReciprocal * cReciprocal,
         zero,                           half * cReciprocal, zero,  half * rhoReciprocal * cReciprocal * cReciprocal};
  }
};

template<>
struct LeftPrimitiveEigenVectorsX<true>
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto zero = static_cast<ElemT>(0.0);
    constexpr auto half = static_cast<ElemT>(0.5);

    const auto cRec = 1 / SonicSpeed::get(state);
    const auto cRecSqr = sqr(cRec);
    const auto uy = state.uy;
    const auto rho = state.rho;
    const auto mult = 1 / (1 - rho * uy);

    return kae::Matrix<ElemT, 4, 4>{
         zero,     -cRec / 2, zero,        cRecSqr / rho / 2,
         mult,      zero,    -rho * mult, -cRecSqr * mult,
        -uy * mult, zero,     mult,        uy * cRecSqr * mult,
         zero,      cRec / 2, zero,        cRecSqr / rho / 2 };
  }
};

struct DispatchedLeftPrimitiveEigenVectorsX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto eps = std::numeric_limits<ElemT>::epsilon();
    return std::fabs(state.uy) > eps ? LeftPrimitiveEigenVectorsX<false>::get(state) :
                                       LeftPrimitiveEigenVectorsX<true>::get(state);
  }
};

template <bool uyIsZero>
struct RightPrimitiveEigenVectorsX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto zero = static_cast<ElemT>(0.0);
    constexpr auto one = static_cast<ElemT>(1.0);

    const auto c = SonicSpeed::get(state);
    return kae::Matrix<ElemT, 4, 4>{
      state.rho,        -state.rho / state.uy, state.rho / state.uy, state.rho,
     -c,                 zero,                 zero,                  c,
      zero,              one,                  one,                   zero,
      state.rho * c * c, zero,                 zero,                  state.rho * c * c };
  }
};

template <>
struct RightPrimitiveEigenVectorsX<true>
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto zero = static_cast<ElemT>(0.0);
    constexpr auto one = static_cast<ElemT>(1.0);

    const auto c  = SonicSpeed::get(state);
    const auto uy = state.uy;
    return kae::Matrix<ElemT, 4, 4>{
      state.rho,         one,  state.rho, state.rho,
     -c,                 zero, zero,      c,
      zero,              uy,   one,       zero,
      state.rho * c * c, zero, zero,      state.rho * c * c };
  }
};

struct DispatchedRightPrimitiveEigenVectorsX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT& state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto eps = std::numeric_limits<ElemT>::epsilon();
    return std::fabs(state.uy) > eps ? RightPrimitiveEigenVectorsX<false>::get(state) :
                                       RightPrimitiveEigenVectorsX<true>::get(state);
  }
};

struct PrimitiveJacobianMatrixX
{
  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE auto operator()(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    return get(state);
  }

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const GasStateT & state) -> kae::Matrix<ElemT, 4, 4>
  {
    constexpr auto zero = static_cast<ElemT>(0.0);

    const auto c = SonicSpeed::get(state);
    return kae::Matrix<ElemT, 4, 4>{
      state.ux, state.rho,         zero,     zero,
      zero,     state.ux,          zero,     1 / state.rho,
      zero,     zero,              state.ux, zero,
      zero,     state.rho * c * c, zero,     state.ux};
  }
};

struct PrimitiveCharacteristicVariables
{

  template <class GasStateT, class ElemT = typename GasStateT::ElemType>
  HOST_DEVICE static auto get(const kae::Matrix<ElemT, 4, 4> & leftEigenVectors,
                              const GasStateT & state) -> kae::Matrix<ElemT, 4, 1>
  {
    kae::Matrix<ElemT, 4, 1> characteristicVariables{ state.rho, state.ux, state.uy, state.p };
    return leftEigenVectors * characteristicVariables;
  }
};

} // namespace kae
