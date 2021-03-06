
#include <gtest/gtest.h>

#include <SrmSolver/cuda_float_types.h>
#include <SrmSolver/get_coordinates_matrix.h>
#include <SrmSolver/get_stencil_indices.h>
#include <SrmSolver/gpu_grid.h>
#include <SrmSolver/matrix.h>
#include <SrmSolver/matrix_operations.h>

namespace kae_tests {

template <class T>
class get_coordinate_matrix : public testing::Test
{
public:

  using ElemType = T;
  using Real2Type = kae::CudaFloat2T<T>;

  constexpr static unsigned nx{ 201U };
  constexpr static unsigned ny{ 101U };
  constexpr static unsigned smExtension{ 3U };
  using LxToType = std::ratio<2, 1>;
  using LyToType = std::ratio<1, 1>;
  using GpuGridType = kae::GpuGrid<nx, ny, LxToType, LyToType, smExtension, ElemType>;

};

using TypeParams = testing::Types<float, double>;
TYPED_TEST_SUITE(get_coordinate_matrix, TypeParams);

TYPED_TEST(get_coordinate_matrix, get_coordinate_matrix_1)
{
  using kae::detail::getStencilIndices;
  using kae::detail::getCoordinatesMatrix;

  using tf              = TestFixture;
  using ElemType        = typename tf::ElemType;
  using Real2Type       = typename tf::Real2Type;
  using GpuGridType     = typename tf::GpuGridType;

  const auto negativeValue = static_cast<ElemType>(-0.1);
  std::vector<ElemType> phiValues(GpuGridType::nx * GpuGridType::ny, negativeValue);

  const Real2Type surfacePoint{ static_cast<ElemType>(0.45642), static_cast<ElemType>(0.33522) };
  const Real2Type normal{ static_cast<ElemType>(0.8), static_cast<ElemType>(0.6) };
  const auto indexMatrix = getStencilIndices<GpuGridType, 1U>(phiValues.data(), surfacePoint, normal);
  const auto coordinateMatrix = getCoordinatesMatrix<GpuGridType>(surfacePoint, normal, indexMatrix);
  const decltype(coordinateMatrix) goldCoordinateMatrix{ static_cast<ElemType>(1) };
  const decltype(coordinateMatrix) thresholdMatrix = coordinateMatrix - goldCoordinateMatrix;
  constexpr ElemType threshold{ std::is_same<ElemType, float>::value ? static_cast<ElemType>(1e-6) :
                                                                       static_cast<ElemType>(1e-14) };
  EXPECT_LE(maxCoeff(cwiseAbs(thresholdMatrix)), threshold);
}

TYPED_TEST(get_coordinate_matrix, get_coordinate_matrix_2_1)
{
  using kae::detail::getStencilIndices;
  using kae::detail::getCoordinatesMatrix;

  using tf          = TestFixture;
  using ElemType    = typename tf::ElemType;
  using Real2Type   = typename tf::Real2Type;
  using GpuGridType = typename tf::GpuGridType;

  constexpr unsigned order = 2U;

  const auto negativeValue = static_cast<ElemType>(-0.1);
  std::vector<ElemType> phiValues(GpuGridType::nx * GpuGridType::ny, negativeValue);

  const Real2Type surfacePoint{ static_cast<ElemType>(0.453), static_cast<ElemType>(0.332) };
  const Real2Type normal{ static_cast<ElemType>(1.0), static_cast<ElemType>(0.0) };
  const auto indexMatrix = getStencilIndices<GpuGridType, order>(phiValues.data(), surfacePoint, normal);
  const auto coordinateMatrix = getCoordinatesMatrix<GpuGridType>(surfacePoint, normal, indexMatrix);

  const decltype(coordinateMatrix) goldCoordinateMatrix{
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.003), static_cast<ElemType>(-0.002) },
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.003), static_cast<ElemType>(0.008) },
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.013), static_cast<ElemType>(-0.002) },
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.013), static_cast<ElemType>(0.008) } };
  constexpr ElemType threshold{ std::is_same<ElemType, float>::value ? static_cast<ElemType>(1e-6) :
                                                                       static_cast<ElemType>(1e-14) };
  const decltype(coordinateMatrix) thresholdMatrix = coordinateMatrix - goldCoordinateMatrix;
  EXPECT_LE(maxCoeff(cwiseAbs(thresholdMatrix)), threshold);
}

TYPED_TEST(get_coordinate_matrix, get_coordinate_matrix_2_2)
{
  using kae::detail::getStencilIndices;
  using kae::detail::getCoordinatesMatrix;

  using tf = TestFixture;
  using ElemType = typename tf::ElemType;
  using Real2Type = typename tf::Real2Type;
  using GpuGridType = typename tf::GpuGridType;

  constexpr unsigned order = 2U;

  const auto negativeValue = static_cast<ElemType>(-0.1);
  std::vector<ElemType> phiValues(GpuGridType::nx * GpuGridType::ny, negativeValue);

  const Real2Type surfacePoint{ static_cast<ElemType>(0.453), static_cast<ElemType>(0.332) };
  const Real2Type normal{ static_cast<ElemType>(0.0), static_cast<ElemType>(1.0) };
  const auto indexMatrix = getStencilIndices<GpuGridType, order>(phiValues.data(), surfacePoint, normal);
  const auto coordinateMatrix = getCoordinatesMatrix<GpuGridType>(surfacePoint, normal, indexMatrix);

  const decltype(coordinateMatrix) goldCoordinateMatrix{
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.002), static_cast<ElemType>(0.003) },
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.002), static_cast<ElemType>(-0.007) },
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.012), static_cast<ElemType>(0.003) },
    { static_cast<ElemType>(1.0), static_cast<ElemType>(-0.012), static_cast<ElemType>(-0.007) } };
  constexpr ElemType threshold{ std::is_same<ElemType, float>::value ? static_cast<ElemType>(1e-6) :
                                                                       static_cast<ElemType>(1e-14) };
  const decltype(coordinateMatrix) thresholdMatrix = coordinateMatrix - goldCoordinateMatrix;
  EXPECT_LE(maxCoeff(cwiseAbs(thresholdMatrix)), threshold);
}

} // namespace kae_tests