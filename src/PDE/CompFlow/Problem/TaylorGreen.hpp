// *****************************************************************************
/*!
  \file      src/PDE/CompFlow/Problem/TaylorGreen.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Problem configuration for the compressible flow equations
  \details   This file defines a policy class for the compressible flow
    equations, defined in PDE/CompFlow/CompFlow.h. See PDE/CompFlow/Problem.h
    for general requirements on Problem policy classes for CompFlow.
*/
// *****************************************************************************
#ifndef CompFlowProblemTaylorGreen_h
#define CompFlowProblemTaylorGreen_h

#include <string>
#include <unordered_set>

#include "Types.hpp"
#include "Fields.hpp"
#include "FunctionPrototypes.hpp"
#include "SystemComponents.hpp"
#include "Inciter/Options/Problem.hpp"

namespace inciter {

//! CompFlow system of PDEs problem: Taylor-Green
//! \see G.I. Taylor, A.E. Green, "Mechanism of the Production of Small Eddies
//!   from Large Ones", Proc. R. Soc. Lond. A 1937 158 499-521; DOI:
//!   10.1098/rspa.1937.0036. Published 3 February 1937
//! \see Waltz, et. al, "Verification of a three-dimensional unstructured finite
//!   element method using analytic and manufactured solutions", Computers and
//!   Fluids, 2013, Vol.81, pp.57-67.
class CompFlowProblemTaylorGreen {

  private:
    using ncomp_t = tk::ctr::ncomp_t;
    using eq = tag::compflow;
    static constexpr ncomp_t m_ncomp = 5;    //!< Number of scalar components

  public:
    //! Evaluate analytical solution at (x,y,0) for all components
    static tk::SolutionFn::result_type
    solution( ncomp_t system, ncomp_t ncomp,
              tk::real x, tk::real y, tk::real, tk::real, int& );

    //! Compute and return source term for Rayleigh-Taylor manufactured solution
    //! \param[in] x X coordinate where to evaluate the source
    //! \param[in] y Y coordinate where to evaluate the source
    //! \param[in,out] r Density source
    //! \param[in,out] ru X momentum source
    //! \param[in,out] rv Y momentum source
    //! \param[in,out] rw Z momentum source
    //! \param[in,out] re Specific total energy source
    //! \note The function signature must follow tk::SrcFn
    static tk::CompFlowSrcFn::result_type
    src( ncomp_t, tk::real x, tk::real y, tk::real, tk::real,
         tk::real& r, tk::real& ru, tk::real& rv, tk::real& rw, tk::real& re )
    {
      r = ru = rv = rw = 0.0;
      re = 3.0*M_PI/8.0*( cos(3.0*M_PI*x)*cos(M_PI*y)
                        - cos(3.0*M_PI*y)*cos(M_PI*x) );
    }

    //! Return field names to be output to file
    std::vector< std::string > fieldNames( ncomp_t ) const;

    //! Return field output going to file
    std::vector< std::vector< tk::real > >
    fieldOutput( ncomp_t system,
                 ncomp_t,
                 ncomp_t offset,
                 std::size_t nunk,
                 tk::real,
                 tk::real V,
                 const std::vector< tk::real >& vol,
                 const std::array< std::vector< tk::real >, 3 >& coord,
                 tk::Fields& U ) const;

    //! Return names of integral variables to be output to diagnostics file
    std::vector< std::string > names( ncomp_t ) const;

    //! Return problem type
    static ctr::ProblemType type() noexcept
    { return ctr::ProblemType::TAYLOR_GREEN; }
};

} // inciter::

#endif // CompFlowProblemTaylorGreen_h
