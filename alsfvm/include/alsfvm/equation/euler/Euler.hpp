#pragma once
#include "alsfvm/equation/euler/ConservedVariables.hpp"
#include "alsfvm/equation/euler/ExtraVariables.hpp"
#include "alsfvm/equation/euler/AllVariables.hpp"
#include "alsfvm/equation/euler/PrimitiveVariables.hpp"

#include "alsfvm/equation/euler/Views.hpp"
#include "alsfvm/volume/Volume.hpp"
#include "alsfvm/equation/euler/ViewsExtra.hpp"

///
/// Gamma constant
/// \note This will be moved into a paramter struct soon! 
///
#define GAMMA (5.0/3.0)


namespace alsfvm {
	namespace equation {
		namespace euler {

            class Euler {
			public:
				typedef euler::ConservedVariables ConservedVariables;
				typedef euler::ExtraVariables ExtraVariables;
                typedef euler::PrimitiveVariables PrimitiveVariables;
				typedef euler::AllVariables AllVariables;
				///
				/// Defaults to "euler".
				///
				static const std::string name;

				///
				/// Gives the number of conserved variables used (5)
				///
				static const size_t  numberOfConservedVariables = 5;

				__device__ __host__ static size_t getNumberOfConservedVariables() {
					return 5;
				}

				typedef equation::euler::Views<volume::Volume, memory::View<real> > Views;
				typedef equation::euler::Views<const volume::Volume, const memory::View<const real> > ConstViews;

				typedef equation::euler::ViewsExtra<volume::Volume, memory::View<real> > ViewsExtra;
				typedef equation::euler::ViewsExtra<const volume::Volume, const memory::View<const real> > ConstViewsExtra;


				///
				/// Fetches and computes the all variables from memory
				///
				__device__ __host__ static AllVariables fetchAllVariables(ConstViews& views, size_t index) {
					return makeAllVariables(views.rho.at(index),
						views.mx.at(index), 
						views.my.at(index), 
						views.mz.at(index), 
						views.E.at(index));
				}
				
                template<class T, class S>
                __device__ __host__ static ConservedVariables fetchConservedVariables(euler::Views<T, S>& views, size_t index) {
					return ConservedVariables(views.rho.at(index),
						views.mx.at(index),
						views.my.at(index),
						views.mz.at(index),
						views.E.at(index));
				}

				__device__ __host__ static ExtraVariables fetchExtraVariables(ConstViewsExtra& views, size_t index) {
					return ExtraVariables(views.p.at(index),
						views.ux.at(index),
						views.uy.at(index),
						views.uz.at(index));
				}

				///
				/// Writes the ConservedVariable struct back to memory
				///
				__device__ __host__ static void setViewAt(Views& output, size_t index, const ConservedVariables& input) {
					output.rho.at(index) = input.rho;
					output.mx.at(index) = input.m.x;
					output.my.at(index) = input.m.y;
					output.mz.at(index) = input.m.z;
					output.E.at(index) = input.E;

				}

				///
				/// Writes the ExtraVariable struct back to memory
				///
				__device__ __host__ static void setExtraViewAt(ViewsExtra& output, size_t index, const ExtraVariables& input) {
					output.p.at(index) = input.p;
					output.ux.at(index) = input.u.x;
					output.uy.at(index) = input.u.y;
					output.uz.at(index) = input.u.z;

				}

				///
				/// Adds the conserved variables to the view at the given index
				/// 
				/// Basically sets output[index] += input
				///
				__device__ __host__ static void addToViewAt(Views& output, size_t index, const ConservedVariables& input) {
					output.rho.at(index) += input.rho;
					output.mx.at(index) += input.m.x;
					output.my.at(index) += input.m.y;
					output.mz.at(index) += input.m.z;
					output.E.at(index) += input.E;

				}

				///
				/// Computes the point flux. 
				///
				/// Here we view the Euler equation as the following hyperbolic system				
				/// \f[\vec{U}_t+F(\vec{U})_x+G(\vec{U})_y+H(\vec{U})_z = 0\f]
				/// where 
				/// \f[\vec{U}=\left(\begin{array}{l}\rho\\ \vec{m}\\E\end{array}\right)=\left(\begin{array}{c}\rho\\ \rho u\\ \rho v\\ \rho w \\E\end{array}\right)\f]
				/// and
				/// \f[F(\vec{U})=\left(\begin{array}{c}\rho u\\ \rho u^2+p\\ \rho u v\\ \rho u w\\ u(E+p)\end{array}\right)\qquad
				///	   G(\vec{U})=\left(\begin{array}{c}\rho v\\ \rho uv\\ \rho v^2+p\\ \rho v w\\ v(E+p)\end{array}\right)\qquad
				///    H(\vec{U})=\left(\begin{array}{c}\rho w\\ \rho uw\\ \rho w v\\ \rho w^2+p\\ w(E+p)\end{array}\right)
				/// \f]
				/// \returns \f[\left\{\begin{array}{lr}F(\vec{U})&\mathrm{if}\;\mathrm{direction}=0\\
				///										G(\vec{U})&\mathrm{if}\;\mathrm{direction}=1\\
				///                                     H(\vec{U})&\mathrm{if}\;\mathrm{direction}=2
				///           \end{array}\right.\f]
				///
				/// \param[in] u the variables to use
				/// \param[out] F the resulting flux
				///

				template<size_t direction>
				__device__ __host__ static void computePointFlux(const AllVariables& u, ConservedVariables& F) {
					static_assert(direction < 3, "We only support up to three dimensions");

					F.rho = u.m[direction];
					F.m = u.u[direction] * u.m;
					F.m[direction] += u.p;
					F.E = (u.E + u.p) * u.u[direction];
				}



				///
				/// Computes the extra variables \f$ p \f$ and \f$u\f$. 
				/// Here we compute
				/// \f[u =\frac{1}{\rho} m\f]
				/// and
				/// \f[p = (1-\gamma)(E-\frac{1}{2\rho}m^2)\f]
				///
				__device__ __host__ static ExtraVariables computeExtra(const ConservedVariables& u) {
                    ExtraVariables v;
                    real ie = u.E - 0.5*u.m.dot(u.m)/u.rho;
                    v.u = u.m / u.rho;
                    v.p = (GAMMA-1)*ie;
                    return v;
                }

                ///
                /// \brief computes the extra variables from the primitive ones
                ///
                /// \param primitiveVariables the primtive variables
                /// \return the computed all variables
                /// \note This implementation is not made for speed! Should only be
                /// used sparsely (eg. for initialization).
                ///
				__device__ __host__ static ExtraVariables computeExtra(const PrimitiveVariables& primitiveVariables) {
                    return ExtraVariables(primitiveVariables.p,
                                          primitiveVariables.u.x,
                                          primitiveVariables.u.y,
                                          primitiveVariables.u.z
                                          );
                }

                ///
                /// \brief computes the extra variables from the primitive ones
                ///
                /// \param primitiveVariables the primtive variables
                /// \return the computed all variables
                /// \note This implementation is not made for speed! Should only be
                /// used sparsely (eg. for initialization).
                ///
				__device__ __host__ static ConservedVariables computeConserved(const PrimitiveVariables& primitiveVariables) {
                    const rvec3 m = primitiveVariables.rho * primitiveVariables.u;

                    const real E =
                            primitiveVariables.p / (GAMMA - 1)
                            + 0.5*primitiveVariables.rho*primitiveVariables.u.dot(primitiveVariables.u);

                    return ConservedVariables(primitiveVariables.rho, m.x, m.y, m.z, E);
                }

				///
				/// Computes the wave speed in the given direction
				/// (absolute value of wave speed)
				///
				template<int direction>
				__device__ __host__ static real computeWaveSpeed(const ConservedVariables& u,
					const ExtraVariables& v) {
					static_assert(direction >= 0, "Direction can not be negative");
					static_assert(direction < 3, "We only support dimension up to and inclusive 3");

                    return abs(v.u[direction]) +sqrt(GAMMA * v.p / u.rho);
				}

				/// 
				/// Checks to see if the variables obeys the constraint.
				/// In this case it checks that
				/// \f[\rho > 0\f]
				/// and
				/// \f[p\geq 0\f]
				/// 
				/// \returns true if the inequalities are fulfilled, false otherwise
				///
				__device__ __host__ static bool obeysConstraints(const ConservedVariables& u,
					const ExtraVariables& v) 
				{

                    return u.rho < INFINITY && (u.rho == u.rho) && (u.rho > 0) && (v.p > 0);
				}

				__device__ __host__ static AllVariables makeAllVariables(real rho, real mx, real my, real mz, real E) {

                    ConservedVariables conserved(rho, mx, my, mz, E);
                    return AllVariables(conserved, computeExtra(conserved));
                }

                __device__ __host__ static real getWeight(const ConstViews& in, size_t index) {
                    return in.rho.at(index);
                }

                __device__ __host__ static PrimitiveVariables computePrimitiveVariables(const ConservedVariables& conserved) {
                    rvec3 u = conserved.m / conserved.rho;
                    real ie = conserved.E - 0.5*conserved.m.dot(conserved.m)/conserved.rho;

                    real p = (GAMMA-1)*ie;
                    return PrimitiveVariables(conserved.rho, u.x, u.y, u.z, p);
                }


			};
		}
} // namespace alsfvm
} // namespace equation

