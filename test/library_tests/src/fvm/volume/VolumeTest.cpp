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

#include "alsfvm/volume/Volume.hpp"
#include "alsfvm/memory/HostMemory.hpp"
#include "gtest/gtest.h"
#include "alsfvm/volume/VolumeFactory.hpp"

using namespace alsfvm::volume;

TEST(VolumeTest, SizeTest) {
    std::vector<std::string> variableNames = {"a", "b"};

    const size_t nx = 10;
    const size_t ny = 10;
    const size_t nz = 10;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");
    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);
    alsfvm::volume::Volume volume(variableNames, factory, nx, ny, nz);
    ASSERT_EQ(variableNames.size(), volume.getNumberOfVariables());
}

TEST(VolumeTest, GetVariableIndex) {
    std::vector<std::string> variableNames = { "alpha", "beta" };

    const size_t nx = 10;
    const size_t ny = 10;
    const size_t nz = 10;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");

    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);
    alsfvm::volume::Volume volume(variableNames, factory, nx, ny, nz);

    ASSERT_EQ(0, volume.getIndexFromName("alpha"));
    ASSERT_EQ(1, volume.getIndexFromName("beta"));
    ASSERT_EQ(std::string("alpha"), volume.getName(0));
    ASSERT_EQ(std::string("beta"), volume.getName(1));

}

TEST(VolumeTest, WriteToMemoryArea) {
    std::vector<std::string> variableNames = { "alpha", "beta" };

    const size_t nx = 10;
    const size_t ny = 10;
    const size_t nz = 10;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");

    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);
    alsfvm::volume::Volume volume(variableNames, factory, nx, ny, nz);

    auto memory0 = volume.getScalarMemoryArea("alpha");

    ASSERT_EQ(nx, memory0->getSizeX());
    ASSERT_EQ(ny, memory0->getSizeY());
    ASSERT_EQ(nz, memory0->getSizeZ());

    alsfvm::dynamic_pointer_cast<alsfvm::memory::HostMemory<alsfvm::real> >
    (memory0)->at(0, 0, 0) = 10;


}

TEST(VolumeTest, FactoryTestEuler3) {
    const size_t nx = 10;
    const size_t ny = 10;
    const size_t nz = 10;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");

    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);

    VolumeFactory volumeFactory("euler3", factory);

    auto eulerConserved = volumeFactory.createConservedVolume(nx, ny, nz);

    ASSERT_EQ(5, eulerConserved->getNumberOfVariables());

    ASSERT_EQ(nx, eulerConserved->getNumberOfXCells());

    ASSERT_EQ(ny, eulerConserved->getNumberOfYCells());
    ASSERT_EQ(nz, eulerConserved->getNumberOfZCells());

    ASSERT_EQ(0, eulerConserved->getIndexFromName("rho"));
    ASSERT_EQ(1, eulerConserved->getIndexFromName("mx"));
    ASSERT_EQ(2, eulerConserved->getIndexFromName("my"));
    ASSERT_EQ(3, eulerConserved->getIndexFromName("mz"));
    ASSERT_EQ(4, eulerConserved->getIndexFromName("E"));

    auto eulerExtra = volumeFactory.createExtraVolume(nx, ny, nz);

    ASSERT_EQ(4, eulerExtra->getNumberOfVariables());

    ASSERT_EQ(nx, eulerExtra->getNumberOfXCells());

    ASSERT_EQ(ny, eulerExtra->getNumberOfYCells());
    ASSERT_EQ(nz, eulerExtra->getNumberOfZCells());

    ASSERT_EQ(0, eulerExtra->getIndexFromName("p"));
    ASSERT_EQ(1, eulerExtra->getIndexFromName("ux"));
    ASSERT_EQ(2, eulerExtra->getIndexFromName("uy"));
    ASSERT_EQ(3, eulerExtra->getIndexFromName("uz"));

}

TEST(VolumeTest, FactoryTestEuler2) {
    const size_t nx = 10;
    const size_t ny = 10;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");

    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);

    VolumeFactory volumeFactory("euler2", factory);

    auto eulerConserved = volumeFactory.createConservedVolume(nx, ny, 1);

    ASSERT_EQ(4, eulerConserved->getNumberOfVariables());

    ASSERT_EQ(nx, eulerConserved->getNumberOfXCells());

    ASSERT_EQ(ny, eulerConserved->getNumberOfYCells());

    ASSERT_EQ(0, eulerConserved->getIndexFromName("rho"));
    ASSERT_EQ(1, eulerConserved->getIndexFromName("mx"));
    ASSERT_EQ(2, eulerConserved->getIndexFromName("my"));
    ASSERT_EQ(3, eulerConserved->getIndexFromName("E"));

    auto eulerExtra = volumeFactory.createExtraVolume(nx, ny, 1);

    ASSERT_EQ(3, eulerExtra->getNumberOfVariables());

    ASSERT_EQ(nx, eulerExtra->getNumberOfXCells());

    ASSERT_EQ(ny, eulerExtra->getNumberOfYCells());

    ASSERT_EQ(0, eulerExtra->getIndexFromName("p"));
    ASSERT_EQ(1, eulerExtra->getIndexFromName("ux"));
    ASSERT_EQ(2, eulerExtra->getIndexFromName("uy"));
}


TEST(VolumeTest, FactoryTestEuler1) {
    const size_t nx = 10;

    auto configuration = alsfvm::make_shared<alsfvm::DeviceConfiguration>("cpu");

    auto factory = alsfvm::make_shared<alsfvm::memory::MemoryFactory>
        (configuration);

    VolumeFactory volumeFactory("euler1", factory);

    auto eulerConserved = volumeFactory.createConservedVolume(nx, 1, 1);

    ASSERT_EQ(3, eulerConserved->getNumberOfVariables());

    ASSERT_EQ(nx, eulerConserved->getNumberOfXCells());


    ASSERT_EQ(0, eulerConserved->getIndexFromName("rho"));
    ASSERT_EQ(1, eulerConserved->getIndexFromName("mx"));
    ASSERT_EQ(2, eulerConserved->getIndexFromName("E"));

    auto eulerExtra = volumeFactory.createExtraVolume(nx, 1, 1);

    ASSERT_EQ(2, eulerExtra->getNumberOfVariables());

    ASSERT_EQ(nx, eulerExtra->getNumberOfXCells());

    ASSERT_EQ(0, eulerExtra->getIndexFromName("p"));
    ASSERT_EQ(1, eulerExtra->getIndexFromName("ux"));
}
