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

#include <gtest/gtest.h>
#include "alsfvm/integrator/RungeKutta2.hpp"
#include <cmath>
#define TOLERANCE (std::is_same<alsfvm::real, float>::value ? 0.1 : 5e-2)
using namespace alsfvm;
using namespace alsfvm::integrator;
using namespace alsfvm::numflux;

namespace {

// Represents the system du/dt = u;
class ODESystem : public System {
public:
    real dt;
    ODESystem(real dt) : dt(dt) {}
    size_t getNumberOfGhostCells() {
        return 0;
    }

    void operator()( volume::Volume& conservedVariables,
        rvec3& waveSpeeds, bool computeWaveSpeeds,
        volume::Volume& output) override {
        output.getScalarMemoryArea(0)->getPointer()[0] = dt *
            conservedVariables.getScalarMemoryArea(0)->getPointer()[0];
        waveSpeeds = rvec3(1, 0, 0);
    }
};
}
TEST(RungeKutta2Test, ConvergenceTest) {

    // We test that if we integrate the system
    // du/dt = u
    // u(0)=  1
    // we will get an approximation to exp(1) at u(1)


    std::vector<std::string> variableNames = { "u" };

    const size_t nx = 1;
    const size_t ny = 1;
    const size_t nz = 1;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");

    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);


    std::vector<double> errors;
    std::vector<int> resolutions;
    const size_t startK = std::is_same<real, float>::value ? 3 : 5;
    const size_t endK = std::is_same<real, float>::value ? 7 : 14;

    for (size_t k = startK; k < endK; ++k) {


        const size_t N = (2 << k);
        const double dt = double(1) / real(N);
        alsfvm::shared_ptr<System> flux(new ODESystem(dt));

        RungeKutta2 integrator(flux);
        std::vector<alsfvm::shared_ptr<alsfvm::volume::Volume> >
        volumes(integrator.getNumberOfSubsteps() + 1);

        for (auto& volume : volumes) {
            volume.reset(new alsfvm::volume::Volume(variableNames, factory, nx, ny, nz));
            volume->getScalarMemoryArea(0)->getPointer()[0] = 1;
        }

        double t = 0;
        simulator::TimestepInformation timestepInformation;
        const double cfl = 1; // To keep timestep constant

        for (size_t i = 0; i < N; i++) {
            for (size_t substep = 0; substep < integrator.getNumberOfSubsteps();
                ++substep) {
                auto& currentVolume = volumes[substep + 1];

                // Note that we do not care about spatial resolution here
                integrator.performSubstep(volumes, rvec3(1, 1, 1), 1, cfl, *currentVolume,
                    substep, timestepInformation);

            }

            timestepInformation.incrementTime(dt);
            volumes.back().swap(volumes.front());
            t += dt;
        }

        ASSERT_EQ(t, 1);
        const double error = std::abs(std::exp(1) -
                volumes.front()->getScalarMemoryArea(0)->getPointer()[0]);


        errors.push_back(error);
        resolutions.push_back(N);


    }



    for (size_t i = 1; i < errors.size(); ++i) {
        EXPECT_NEAR((std::log(errors[i]) - std::log(errors[i - 1])) / (std::log(
                    resolutions[i]) - std::log(resolutions[i - 1])), -2, TOLERANCE);
    }


}
