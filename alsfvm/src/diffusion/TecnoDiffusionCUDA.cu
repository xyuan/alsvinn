#include "alsfvm/diffusion/TecnoDiffusionCUDA.hpp"
#include "alsfvm/equation/burgers/Burgers.hpp"
#include "alsfvm/diffusion/RoeMatrix.hpp"
#include "alsfvm/equation/equation_list.hpp"
#include <iostream>

namespace alsfvm {
    namespace diffusion {
        namespace {

            template<class Equation, int direction>
            __global__ void computeEntropyVariables(Equation equation, typename Equation::Views output, typename Equation::ConstViews input) {
                const size_t index = threadIdx.x + blockIdx.x * blockDim.x;

                const size_t nx = input.get(0).getNumberOfXCells();
                const size_t ny = input.get(0).getNumberOfYCells();
                const size_t nz = input.get(0).getNumberOfZCells();

                
                
                if (index >= nx*ny*nz) {
                    return;
                }

                auto in = equation.fetchConservedVariables(input, index);


                auto entropyVariables = equation.template computeEntropyVariablesMultipliedByEigenVectorMatrix<direction>(in);

                equation.setViewAt(output, index, entropyVariables);
            }

            template<class Equation, template<typename, int> class DiffusionMatrix, int direction>
            __global__ void multiplyDiffusionMatrix(Equation equation, typename Equation::Views output, 
                typename Equation::ConstViews leftView,
                typename Equation::ConstViews rightView,
                typename Equation::ConstViews conservedView,
                int numberOfXCells,
                int numberOfYCells,
                int numberOfZCells,
                ivec3 directionVector
                ) {


                const int index = threadIdx.x + blockIdx.x * blockDim.x;

                const size_t xInternalFormat = index % numberOfXCells;
                const size_t yInternalFormat = (index / numberOfXCells) % numberOfYCells;
                const size_t zInternalFormat = (index) / (numberOfXCells * numberOfYCells);

                if (xInternalFormat >= numberOfXCells || yInternalFormat >= numberOfYCells
                    || zInternalFormat >= numberOfZCells) {
                    return;
                }

                const int x = xInternalFormat + directionVector[0];
                const int y = yInternalFormat + directionVector[1];
                const int z = zInternalFormat + directionVector[2];

                const size_t middleIndex = output.index(x, y, z);

                const size_t rightIndex = output.index(x + directionVector[0], 
                    y + directionVector[1], 
                    z + directionVector[2]);

                const size_t leftIndex = output.index(x - directionVector[0],
                    y - directionVector[1],
                    z - directionVector[2]);

                auto diffusion = [&](size_t left, size_t right) {
                    auto leftValues = Equation::fetchConservedVariables(rightView, left);
                    auto rightValues = Equation::fetchConservedVariables(leftView, right);

                    auto conservedValues = Equation::fetchConservedVariables(conservedView, right);

                    DiffusionMatrix<Equation, direction> matrix(equation, conservedValues);

                    return 0.5*(equation.template computeEigenVectorMatrix<direction>(conservedValues) * (matrix * (leftValues - rightValues)));
                };


                equation.addToViewAt(output, middleIndex, diffusion(middleIndex, rightIndex) - diffusion(leftIndex, middleIndex));

            }

            template<class Equation, template<typename, int> class DiffusionMatrix, int direction>
            void applyDiffusionCUDA(Equation equation, volume::Volume& outputVolume, reconstruction::Reconstruction& reconstruction,
                volume::Volume& left, volume::Volume& right, 
                volume::Volume& entropyVariables, 
                const volume::Volume& conservedVolume) {
                typename Equation::ConstViews conservedView(conservedVolume);
                typename Equation::Views entropyVariablesView(entropyVariables);
                const size_t size = conservedVolume.getTotalNumberOfXCells()
                    * conservedVolume.getTotalNumberOfYCells()
                    * conservedVolume.getTotalNumberOfZCells();

                const size_t blockSize = 512;
                computeEntropyVariables<Equation, direction> << <(size + blockSize - 1) / blockSize, blockSize >> >(equation, entropyVariablesView, conservedView);


                const ivec3 directionVector = make_direction_vector(direction);
                for (size_t variable = 0; variable < outputVolume.getNumberOfVariables(); ++variable) {
                    volume::Volume variableVolume(entropyVariables, { variable });
                    reconstruction.performReconstruction(variableVolume, direction, 0, left, right);
                }

                typename Equation::ConstViews leftView(left);
                typename Equation::ConstViews rightView(right);
                
                typename Equation::Views outputView(outputVolume);
                const size_t ngc = reconstruction.getNumberOfGhostCells();

                const size_t nx = conservedVolume.getTotalNumberOfXCells();
                const size_t ny = conservedVolume.getTotalNumberOfYCells();
                const size_t nz = conservedVolume.getTotalNumberOfZCells();

                const ivec3 start = directionVector;
                const ivec3 end = ivec3(nx, ny, nz) - directionVector;




                const ivec3 numberOfCellsPerDimension = end - start;

                const size_t totalNumberOfCells = size_t(numberOfCellsPerDimension.x) *
                    size_t(numberOfCellsPerDimension.y) *
                    size_t(numberOfCellsPerDimension.z);


                multiplyDiffusionMatrix<Equation, DiffusionMatrix, direction>
                    << <(totalNumberOfCells + blockSize - 1) / blockSize, blockSize >> >(
                        equation, outputView,
                        leftView,
                        rightView,
                        conservedView,
                        numberOfCellsPerDimension.x,
                        numberOfCellsPerDimension.y,
                        numberOfCellsPerDimension.z,
                        directionVector
                        );
            }

            
        }


        template<class Equation, template<typename, int> class DiffusionMatrix>
        TecnoDiffusionCUDA<Equation, DiffusionMatrix>::TecnoDiffusionCUDA(volume::VolumeFactory& volumeFactory,
            alsfvm::shared_ptr<reconstruction::Reconstruction> reconstruction,
            const simulator::SimulatorParameters& simulatorParameters)

            :
            volumeFactory(volumeFactory),
            reconstruction(reconstruction),
            equation(static_cast<const typename Equation::Parameters&>(simulatorParameters.getEquationParameters()))
        {
            // empty
        }

        template<class Equation, template<typename, int> class DiffusionMatrix>
        void TecnoDiffusionCUDA<Equation, DiffusionMatrix>::applyDiffusion(volume::Volume& outputVolume,
            const volume::Volume& conservedVolume)
        {
            if (!left || left->getNumberOfXCells() != conservedVolume.getNumberOfXCells()) {
                size_t nx = conservedVolume.getNumberOfXCells();
                size_t ny = conservedVolume.getNumberOfYCells();
                size_t nz = conservedVolume.getNumberOfZCells();

                size_t gcx = conservedVolume.getNumberOfXGhostCells();
                size_t gcy = conservedVolume.getNumberOfYGhostCells();
                size_t gcz = conservedVolume.getNumberOfZGhostCells();



                left = volumeFactory.createConservedVolume(nx, ny, nz, gcx);
                right = volumeFactory.createConservedVolume(nx, ny, nz, gcx);
                entropyVariables = volumeFactory.createConservedVolume(nx, ny, nz, gcx);

            }
            


            for (int direction = 0; direction < outputVolume.getDimensions(); ++direction) {
                if (direction == 0) {
                    applyDiffusionCUDA<Equation, DiffusionMatrix, 0>(equation, outputVolume, *reconstruction,
                        *left, *right,
                        *entropyVariables,
                        conservedVolume);
                }
                else if (direction == 1) {
                    applyDiffusionCUDA<Equation, DiffusionMatrix, 1>(equation, outputVolume, *reconstruction,
                        *left, *right,
                        *entropyVariables,
                        conservedVolume);
                }
                else if (direction == 2) {
                    applyDiffusionCUDA<Equation, DiffusionMatrix, 2>(equation, outputVolume, *reconstruction,
                        *left, *right,
                        *entropyVariables,
                        conservedVolume);
                }
            }
        }

        template<class Equation, template<typename, int> class DiffusionMatrix>
        size_t TecnoDiffusionCUDA<Equation, DiffusionMatrix>::getNumberOfGhostCells() const {
            return reconstruction->getNumberOfGhostCells();
        }

        template class TecnoDiffusionCUDA<::alsfvm::equation::burgers::Burgers, ::alsfvm::diffusion::RoeMatrix >;
        template class TecnoDiffusionCUDA<::alsfvm::equation::euler::Euler, ::alsfvm::diffusion::RoeMatrix >;
    }
}