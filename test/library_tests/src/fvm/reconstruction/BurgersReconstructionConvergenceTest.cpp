/* Copyright (c) 2018 ETH Zurich, Kjetil Olsen Lye
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "alsutils/config.hpp"
#include <gtest/gtest.h>
#include "alsfvm/reconstruction/ReconstructionFactory.hpp"
#include "alsfvm/volume/VolumeFactory.hpp"
#include "alsfvm/volume/volume_foreach.hpp"
#include "alsfvm/equation/burgers/Burgers.hpp"
#include "alsfvm/reconstruction/WENOCoefficients.hpp"
#include "alsfvm/boundary/BoundaryFactory.hpp"
#include <array>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/lu.hpp>
#include "utils/polyfit.hpp"
#include "alsutils/config.hpp"
using namespace alsfvm;
using namespace alsfvm::memory;
using namespace alsfvm::volume;
using namespace alsfvm::reconstruction;
using namespace alsfvm::grid;



namespace {
struct ReconstructionParameters {
    const real expectedConvergenceRate = 1.0;
    const real expectedLInftyConvergenceRate = 1.0;
    const std::string name;
    const std::string platform;

    ReconstructionParameters(real expectedConvergenceRate_,
        real expectedLInftyConvergenceRate_,
        const std::string& name_,
        const std::string& platform_
    )
        :
        expectedConvergenceRate(expectedConvergenceRate_),
        expectedLInftyConvergenceRate(expectedLInftyConvergenceRate_),
        name(name_),
#ifdef ALSVINN_HAVE_CUDA
        platform(platform_)
#else
        platform("cpu")
#endif
    {

    }

};


std::ostream& operator<<(std::ostream& os,
    const ReconstructionParameters& parameters) {
    os << "\n{\n\texpectedConvergenceRate = " << parameters.expectedConvergenceRate
        << "\n\texpectedLInftyConvergenceRate = " <<
        parameters.expectedLInftyConvergenceRate
        << "\n\tname = " << parameters.name
        << "\n\tplatform = " << parameters.platform << std::endl << "}" << std::endl;
    return os;
}
}

class BurgersReconstructionConvergenceTest : public ::testing::TestWithParam
    <ReconstructionParameters> {
public:
    ReconstructionParameters parameters;
    size_t nx = 10;
    size_t ny = 10;
    size_t nz = 1;

    Grid grid;

    alsfvm::shared_ptr<DeviceConfiguration> deviceConfiguration;
    alsfvm::shared_ptr<MemoryFactory> memoryFactory;
    ReconstructionFactory reconstructionFactory;
    VolumeFactory volumeFactory;

    alsfvm::shared_ptr<DeviceConfiguration> deviceConfigurationCPU;
    alsfvm::shared_ptr<MemoryFactory> memoryFactoryCPU;
    VolumeFactory volumeFactoryCPU;

    simulator::SimulatorParameters simulatorParameters;

    alsfvm::shared_ptr<Reconstruction> wenoCUDA;

    alsfvm::shared_ptr<Volume> conserved;
    alsfvm::shared_ptr<Volume> left;
    alsfvm::shared_ptr<Volume> right;

    alsfvm::shared_ptr<Volume> conservedCPU;
    alsfvm::shared_ptr<Volume> leftCPU;
    alsfvm::shared_ptr<Volume> rightCPU;

    alsfvm::shared_ptr<boundary::Boundary> boundary;

    BurgersReconstructionConvergenceTest()

        :
        parameters(GetParam()),
        grid({ 0, 0, 0 }, {
        1, 1, 0
    }, ivec3(nx, ny, nz)),
    deviceConfiguration(new DeviceConfiguration(parameters.platform)),
    memoryFactory(new MemoryFactory(deviceConfiguration)),
    volumeFactory("burgers", memoryFactory),
    deviceConfigurationCPU(new DeviceConfiguration("cpu")),
    memoryFactoryCPU(new MemoryFactory(deviceConfigurationCPU)),
    volumeFactoryCPU("burgers", memoryFactoryCPU)

    {
        auto burgersParameters = alsfvm::make_shared<equation::EquationParameters>();

        simulatorParameters.setEquationParameters(burgersParameters);


    }

    void makeReconstruction(const std::string name, size_t newNx) {
        nx = newNx;
        nz = 1;
        ny = 1;

        grid = Grid({ 0, 0, 0 }, { 1, 1, 0 }, ivec3(nx, ny, nz));

        makeReconstruction(name);
    }

    void makeReconstruction(const std::string& name) {
        wenoCUDA = reconstructionFactory.createReconstruction(name, "burgers",
                simulatorParameters, memoryFactory, grid, deviceConfiguration);

        conserved = volumeFactory.createConservedVolume(nx, ny, nz,
                wenoCUDA->getNumberOfGhostCells());
        left = volumeFactory.createConservedVolume(nx, ny, nz,
                wenoCUDA->getNumberOfGhostCells());
        right = volumeFactory.createConservedVolume(nx, ny, nz,
                wenoCUDA->getNumberOfGhostCells());

        conservedCPU = volumeFactoryCPU.createConservedVolume(nx, ny, nz,
                wenoCUDA->getNumberOfGhostCells());
        rightCPU = volumeFactoryCPU.createConservedVolume(nx, ny, nz,
                wenoCUDA->getNumberOfGhostCells());
        leftCPU = volumeFactoryCPU.createConservedVolume(nx, ny, nz,
                wenoCUDA->getNumberOfGhostCells());

        conserved->makeZero();

        boundary::BoundaryFactory boundaryFactory("periodic", deviceConfiguration);

        boundary = boundaryFactory.createBoundary(wenoCUDA->getNumberOfGhostCells());
    }

};

TEST_P(BurgersReconstructionConvergenceTest, ReconstructionTest) {
    // We test that we actually get second order convergence of
    // WENO2. That is, we create a 1 dimensional grid
    // with values u[x] = sin(x) + 2
    // (note: we add the + 2 to make sure we always have positive density)

    auto f = [](real x) {
        return sin(2 * M_PI * x) + 2;
    };

    // Integral of f / dx
    // where dx = b - a
    auto averageIntegralF = [](real a, real b) {
        return (-cos(2 * M_PI * b) + cos(2 * M_PI * a)) / (2 * M_PI * (b - a)) + 2;
    };

    const size_t startK = std::is_same<real, float>::value ? 3 : 5;
    const size_t endK = std::is_same<real, float>::value ? 9 : 15;

    const real expectedConvergenceRate = parameters.expectedConvergenceRate;
    const real expectedLInftyConvergenceRate =
        parameters.expectedLInftyConvergenceRate;



    std::vector<double> L1Left;
    std::vector<double> L1Right;
    std::vector<double> LInftyLeft;
    std::vector<double> LInftyRight;
    std::vector<double> resolutions;

    for (size_t k = startK; k < endK; ++k) {

        const size_t n = 1 << k;
        resolutions.push_back(std::log2(n));
        makeReconstruction(parameters.name, n);
        const size_t numberOfGhostCells = wenoCUDA->getNumberOfGhostCells();
        const real dx = grid.getCellLengths().x;
        auto conservedView = conservedCPU->getScalarMemoryArea("u")->getView();

        for (int x = 0; x < int(nx); ++x) {
            const real a = x * dx;
            const real b = (x + 1) * dx;

            const size_t index = conservedView.index(x + numberOfGhostCells, 0, 0);
            equation::burgers::PrimitiveVariables primitiveVariables;

            primitiveVariables.u = averageIntegralF(a, b);


            equation::EquationParameters burgersParameters;
            equation::burgers::Burgers eq(burgersParameters);
            auto conservedVariables = eq.computeConserved(primitiveVariables);


            conservedCPU->getScalarMemoryArea("u")->getPointer()[index] =
                conservedVariables.u;

        }

        conservedCPU->copyTo(*conserved);

        boundary->applyBoundaryConditions(*conserved, grid);
        wenoCUDA->performReconstruction(*conserved, 0, 0, *left, *right);

        left->copyTo(*leftCPU);
        right->copyTo(*rightCPU);

        double L1DifferenceLeft = 0.0;
        double L1DifferenceRight = 0.0;
        double LInftyDifferenceLeft = 0.0;
        double LInftyDifferenceRight = 0.0;

        for (int x = 0; x < int(nx); ++x) {
            const double a = x * dx;
            const double b = (x + 1) * dx;

            const size_t index = conservedView.index(x + numberOfGhostCells, 0, 0);

            const double leftValue = leftCPU->getScalarMemoryArea("u")->getPointer()[index];
            const double rightValue =
                rightCPU->getScalarMemoryArea("u")->getPointer()[index];
            const double differenceLeft = std::abs(leftValue - f(a));

            L1DifferenceLeft += differenceLeft;
            LInftyDifferenceLeft = std::max(LInftyDifferenceLeft, differenceLeft);

            const double differenceRight = std::abs(rightValue - f(b));
            L1DifferenceRight += differenceRight;
            LInftyDifferenceRight = std::max(LInftyDifferenceRight, differenceRight);


        }

        L1Left.push_back(std::log2(L1DifferenceLeft / n));
        L1Right.push_back(std::log2(L1DifferenceRight / n));
        LInftyLeft.push_back(std::log2(LInftyDifferenceLeft));
        LInftyRight.push_back(std::log2(LInftyDifferenceRight));

    }

    ASSERT_LE(expectedConvergenceRate, -linearFit(resolutions, L1Left)[0]);
    ASSERT_LE(expectedConvergenceRate, -linearFit(resolutions, L1Right)[0]);

    ASSERT_LE(expectedLInftyConvergenceRate, -linearFit(resolutions,
            LInftyLeft)[0]);
    ASSERT_LE(expectedLInftyConvergenceRate, -linearFit(resolutions,
            LInftyRight)[0]);
}

#ifdef ALSVINN_USE_FLOAT

INSTANTIATE_TEST_CASE_P(ReconstructionTests,
    BurgersReconstructionConvergenceTest,
    ::testing::Values(
        ReconstructionParameters(2.5,  1.9, "weno2", "cuda"),
        ReconstructionParameters(2.5,  1.9, "weno2", "cpu"),
        ReconstructionParameters(1.9,  1.9, "eno2", "cpu"),
        ReconstructionParameters(2.9,  2.7, "eno3", "cpu"),
        ReconstructionParameters(1.9,  1.9, "eno2", "cuda"),
        ReconstructionParameters(2.9,  2.7, "eno3", "cuda"),
        ReconstructionParameters(0.9, .9, "none", "cpu"),
        ReconstructionParameters(0.9, .9, "none", "cuda")
    ));

#else
INSTANTIATE_TEST_CASE_P(ReconstructionTests,
    BurgersReconstructionConvergenceTest,
    ::testing::Values(
        ReconstructionParameters(2.99,  2.99, "weno2", "cuda"),
        ReconstructionParameters(2.99,  2.99, "weno2", "cpu"),
        ReconstructionParameters(1.99,  1.99, "eno2", "cpu"),
        ReconstructionParameters(2.99,  2.99, "eno3", "cpu"),
        ReconstructionParameters(1.99,  1.99, "eno2", "cuda"),
        ReconstructionParameters(2.99,  2.99, "eno3", "cuda"),
        ReconstructionParameters(0.99, .99, "none", "cpu"),
        ReconstructionParameters(0.99, .99, "none", "cuda")
    ));
#endif
