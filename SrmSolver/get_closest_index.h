#pragma once

#include "math_utilities.h"

namespace kae {

namespace detail {

template <class GpuGridT, class ElemT>
HOST_DEVICE unsigned getClosestIndex(const ElemT * pCurrPhi,
                                     unsigned      i,
                                     unsigned      j,
                                     ElemT         nx,
                                     ElemT         ny)
{
  const unsigned globalIdx = j * GpuGridT::nx + i;
  const ElemT level        = pCurrPhi[globalIdx];

  const ElemT xSurface    = i * GpuGridT::hx - nx * level;
  const ElemT ySurface    = j * GpuGridT::hy - ny * level;

  const unsigned iSurface = std::round(xSurface * GpuGridT::hxReciprocal);
  const unsigned jSurface = std::round(ySurface * GpuGridT::hyReciprocal);

  ElemT minDistanceSquared = GpuGridT::lx * GpuGridT::lx + GpuGridT::ly * GpuGridT::ly;
  unsigned iClosest   = GpuGridT::n;
  unsigned jClosest   = GpuGridT::n;
  for (unsigned iCl = iSurface - 3; iCl <= iSurface + 3; ++iCl)
  {
    for (unsigned jCl = jSurface - 3; jCl <= jSurface + 3; ++jCl)
    {
      if (pCurrPhi[jCl * GpuGridT::nx + iCl] >= 0)
      {
        continue;
      }

      ElemT distanceSquared = sqr(iCl * GpuGridT::hx - xSurface) + sqr(jCl * GpuGridT::hy - ySurface);
      if (minDistanceSquared < distanceSquared)
      {
        continue;
      }

      iClosest = iCl;
      jClosest = jCl;
      minDistanceSquared = distanceSquared;
    }
  }
  return jClosest * GpuGridT::nx + iClosest;
}

} // namespace detail

} // namespace kae