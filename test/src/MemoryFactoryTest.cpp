
#include "gtest/gtest.h"
#include <vector>

#include "alsfvm/memory/MemoryFactory.hpp"
#ifdef ALSVINN_HAVE_CUDA
#include "alsfvm/cuda/CudaMemory.hpp"
TEST(CudaMemoryFactoryTest, CreateCudaMemoryArea) {
	auto deviceConfiguration = std::make_shared<alsfvm::DeviceConfiguration>();
	const std::string memoryName = "CudaMemory";
	alsfvm::memory::MemoryFactory factory(memoryName, deviceConfiguration);


    size_t nx = 10;
    size_t ny = 10;
    size_t nz = 10;
    auto memory = factory.createScalarMemory(nx, ny, nz);
    ASSERT_EQ(nx*ny*nz, memory->getSize());

	auto cudaMemory = std::dynamic_pointer_cast<alsfvm::cuda::CudaMemory <alsfvm::real >> (memory);
	ASSERT_TRUE(!!cudaMemory);
}

#endif

#include "alsfvm/memory/HostMemory.hpp"
TEST(HostMemoryFactoryTest, CreateHostMemoryArea) {
	auto deviceConfiguration = std::make_shared<alsfvm::DeviceConfiguration>();
	const std::string memoryName = "HostMemory";
	alsfvm::memory::MemoryFactory factory(memoryName, deviceConfiguration);


    size_t nx = 10;
    size_t ny = 10;
    size_t nz = 10;
    auto memory = factory.createScalarMemory(nx, ny, nz);
    ASSERT_EQ(nx*ny*nz, memory->getSize());

    ASSERT_EQ(nx, memory->getSizeX());
    ASSERT_EQ(ny, memory->getSizeY());
    ASSERT_EQ(nz, memory->getSizeZ());

	auto hostMemory = std::dynamic_pointer_cast<alsfvm::memory::HostMemory<alsfvm::real>>(memory);
	ASSERT_TRUE(!!hostMemory);
}