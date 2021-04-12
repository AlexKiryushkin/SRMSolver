#pragma once

namespace kae {


template <class GpuGridT>
EBoundaryCondition SrmDualThrust<GpuGridT>::getBoundaryCondition(ElemType x, ElemType y)
{
  if (std::fabs(x - xRight) < static_cast<ElemType>(0.1) * GpuGridT::hx)
  {
    return EBoundaryCondition::ePressureOutlet;
  }

  if (isPointOnGrain(x, y))
  {
    return EBoundaryCondition::eMassFlowInlet;
  }

  return EBoundaryCondition::eWall;
}

template <class GpuGridT>
HOST_DEVICE auto SrmDualThrust<GpuGridT>::getRadius(unsigned i, unsigned j) -> ElemType
{
  return getRadius(i * GpuGridT::hx, j * GpuGridT::hy);
}

template <class GpuGridT>
HOST_DEVICE auto SrmDualThrust<GpuGridT>::getRadius(ElemType x, ElemType y) -> ElemType
{
  return y - yBottom;
}

template <class GpuGridT>
SrmDualThrust<GpuGridT>::SrmDualThrust()
  : m_distances{ GpuGridT::nx * GpuGridT::ny },
    m_linestring{}
{
  namespace bg = boost::geometry;

  for (unsigned i{}; i < nPoints; ++i)
  {
    bg::append(m_linestring, Point2d{ points[i][0U],  points[i][1U] });
  }

  std::for_each(std::begin(m_linestring), std::end(m_linestring), [](auto& point)
  {
    bg::set<0>(point, bg::get<0>(point) + xLeft);
    bg::set<1>(point, bg::get<1>(point) + yBottom);
  });

  Polygon2d polygon;
  std::copy(std::begin(m_linestring), std::end(m_linestring), std::back_inserter(polygon.outer()));

  for (unsigned i = 0U; i < GpuGridT::nx; ++i)
  {
    const auto x = i * GpuGridT::hx;
    for (unsigned j = 0U; j < GpuGridT::ny; ++j)
    {
      const auto y = j * GpuGridT::hy;
      const Point2d point{ x, y };
      const auto distance = static_cast<ElemType>(bg::distance(point, m_linestring));
      const auto isInside = bg::covered_by(point, polygon);

      const auto index = j * GpuGridT::nx + i;
      m_distances[index] = isInside ? -std::fabs(distance) : std::fabs(distance);
    }
  }
}

template <class GpuGridT>
constexpr auto SrmDualThrust<GpuGridT>::getInitialSBurn() -> ElemType
{
  ElemType initialSBurn{};
  initialSBurn += initialSBurnPart<0U>();
  initialSBurn += initialSBurnPart<1U>();
  initialSBurn += initialSBurnPart<2U>();
  initialSBurn += initialSBurnPart<3U>();
  initialSBurn += initialSBurnPart<4U>();
  initialSBurn += initialSBurnPart<5U>();
  initialSBurn += initialSBurnPart<6U>();
  initialSBurn += initialSBurnPart<7U>();
  initialSBurn += initialSBurnPart<8U>();
  initialSBurn += initialSBurnPart<9U>();
  initialSBurn += initialSBurnPart<10U>();
  initialSBurn += initialSBurnPart<11U>();
  initialSBurn += initialSBurnPart<12U>();
  initialSBurn += initialSBurnPart<13U>();
  initialSBurn += initialSBurnPart<14U>();
  initialSBurn += initialSBurnPart<15U>();
  initialSBurn += initialSBurnPart<16U>();
  initialSBurn += initialSBurnPart<17U>();
  initialSBurn += initialSBurnPart<18U>();
  initialSBurn += initialSBurnPart<19U>();
  initialSBurn += initialSBurnPart<20U>();
  return initialSBurn;
}

template <class GpuGridT>
constexpr auto SrmDualThrust<GpuGridT>::getFCritical() -> ElemType
{
  return static_cast<ElemType>(M_PI) * rkr * rkr;
}

template <class GpuGridT>
HOST_DEVICE auto SrmDualThrust<GpuGridT>::isChamber(ElemType x, ElemType y) -> ElemType
{
  return (chamberRight - x >= static_cast<ElemType>(0.1) * GpuGridT::hx) &&
    (x - xLeft >= static_cast<ElemType>(0.1) * GpuGridT::hx);
}

template <class GpuGridT>
HOST_DEVICE auto SrmDualThrust<GpuGridT>::isBurningSurface(ElemType x, ElemType y) -> ElemType
{
  constexpr auto eps = sqr(GpuGridT::hx);
  return (x- xLeft >= eps) &&
    (x - xLeft <= propellantRight + eps) &&
    (y - yBottom >= eps) &&
    (y - yBottom <= Rk + eps);
}

template <class GpuGridT>
bool SrmDualThrust<GpuGridT>::shouldApplyScheme(unsigned i, unsigned j)
{
  return true;
}

template <class GpuGridT>
bool SrmDualThrust<GpuGridT>::isPointOnGrain(ElemType x, ElemType y)
{
  return isBurningSurface(x, y);
}

template <class GpuGridT>
auto SrmDualThrust<GpuGridT>::values() const -> const thrust::host_vector<ElemType>&
{
  return m_distances;
}

} // namespace kae

