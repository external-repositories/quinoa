// *****************************************************************************
/*!
  \file      src/DiffEq/MixMassFractionBetaCoeffPolicy.h
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Mix mass-fraction beta SDE coefficients policies
  \details   This file defines coefficients policy classes for the mix
    mass-fraction beta SDE, defined in DiffEq/MixMassFractionBeta.h.

    General requirements on mix mass-fraction beta SDE coefficients policy
    classes:

    - Must define a _constructor_, which is used to initialize the SDE
      coefficients, b, S, kappa, rho2, and r. Required signature:
      \code{.cpp}
        CoeffPolicyName(
          tk::ctr::ncomp_type ncomp,
          const std::vector< kw::sde_bprime::info::expect::type >& bprime_,
          const std::vector< kw::sde_S::info::expect::type >& S_,
          const std::vector< kw::sde_kappaprime::info::expect::type >& kprime_,
          const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
          const std::vector< kw::sde_r::info::expect::type >& r_,
          std::vector< kw::sde_bprime::info::expect::type  >& bprime,
          std::vector< kw::sde_S::info::expect::type >& S,
          std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
          std::vector< kw::sde_rho2::info::expect::type >& rho2_,
          std::vector< kw::sde_r::info::expect::type >& r_ );
      \endcode
      where
      - ncomp denotes the number of scalar components of the system of
        mix mass-fraction beta SDEs.
      - Constant references to bprime_, S_, kprime_, rho2_, and r_, which
        denote vectors of real values used to initialize the parameter
        vectors of the system of mix mass-fraction beta SDEs. The length of
        the vectors must be equal to the number of components given by ncomp.
      - References to bprime, S, kprime, rho2_, and r, which denote the
        parameter vectors to be initialized based on bprime_, S_, kprime_,
        rho2_, and r_.

    - Must define the static function _type()_, returning the enum value of the
      policy option. Example:
      \code{.cpp}
        static ctr::CoeffPolicyType type() noexcept {
          return ctr::CoeffPolicyType::DECAY;
        }
      \endcode
      which returns the enum value of the option from the underlying option
      class, collecting all possible options for coefficients policies.

    - Must define the function _update()_, called from
      MixMassFractionBeta::advance(), updating the model coefficients.
      Required signature:
      \code{.cpp}
        void update(
          char depvar,
          char dissipation_depvar,
          char velocity_depvar,
          ctr::DepvarType velocity_solve,
          ncomp_t ncomp,
          const std::map< tk::ctr::Product, tk::real >& moments,
          const std::vector< kw::sde_bprime::info::expect::type  >& bprime,
          const std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
          const std::vector< kw::sde_rho2::info::expect::type >& rho2,
          const std::vector< kw::sde_r::info::expect::type >& r,
          const std::vector< tk::Table >& hts,
          const std::vector< tk::Table >& hp,
          std::vector< kw::sde_b::info::expect::type  >& b,
          std::vector< kw::sde_kappa::info::expect::type >& k,
          std::vector< kw::sde_S::info::expect::type >& S ) const {}
      \endcode
      where _depvar_ is the dependent variable associated with the mix
      mass-fraction beta SDE, specified in the control file by the user, _ncomp_
      is the number of components in the system, _moments_ is the map
      associating moment IDs (tk::ctr::vector< tk::ctr::Term >) to values of
      statistical moments, _bprime_, _kprime_, rho2, r, are user-defined
      parameters, and _b_, _k_, _S_, are the SDE parameters computed, see
      DiffEq/MixMassFractionBeta.h.

      The constant reference to hts, denotes a vector of y=f(x) functions (see
      src/DiffEq/HydroTimeScales.h and
      src/Control/Walker/Options/HydroTimescales.h) used to configure the
      inverse hydrodynamics time scales (extracted from direct numerical
      simulations) of the system of mix mass-fraction beta SDEs if the
      MixMassFracBetaCoeffHydroTimeScaleHomDecay coefficients policy is
      selected. The length of this vector must be equal to the number of
      components given by ncomp. Note that hts is only used by
      MixMassFracBetaCoeffHydroTimeScaleHomDecay.

      The constant reference to hp, denotes a vector of y=f(x) functions (see
      src/DiffEq/HydroProductions.h and
      src/Control/Walker/Options/HydroProductions.h) used to configure the
      turbulent kinetic energy production divided by the dissipation rate, P/e,
      a measure of the non-eqilibrium nature of the turbulent flow (extracted
      from direct numerical simulations) of the system of mix mass-fraction beta
      SDEs if the MixMassFracBetaCoeffHydroTimeScaleHomDecay
      coefficients policy is selected. The length of this vector must be equal
      to the number of components given by ncomp. Note that hp is only used by
      MixMassFracBetaCoeffHydroTimeScaleHomDecay.
*/
// *****************************************************************************
#ifndef MixMassFractionBetaCoeffPolicy_h
#define MixMassFractionBetaCoeffPolicy_h

#include <brigand/sequences/list.hpp>

#include "Types.h"
#include "Table.h"
#include "Walker/Options/CoeffPolicy.h"
#include "Langevin.h"

namespace walker {

//! \brief Mix mass-fraction beta SDE decay coefficients policy
//! \details User-defined parameters b' and kappa' are constants in time and
//!   ensure decay in the evolution of <y^2>.
class MixMassFracBetaCoeffDecay {

    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor: initialize coefficients
    MixMassFracBetaCoeffDecay(
      ncomp_t ncomp,
      const std::vector< kw::sde_bprime::info::expect::type >& bprime_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
      const std::vector< kw::sde_r::info::expect::type >& r_,
      std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      std::vector< kw::sde_rho2::info::expect::type >& rho2,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k )
    {
      ErrChk( bprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'b'");
      ErrChk( S_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'S'");
      ErrChk( kprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'kappa'");
      ErrChk( rho2_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'rho2'");
      ErrChk( r_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'r'");

      bprime = bprime_;
      S = S_;
      kprime = kprime_;
      rho2 = rho2_;
      r = r_;

      b.resize( bprime.size() );
      k.resize( kprime.size() );
    }

    //! Coefficients policy type accessor
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::DECAY; }

    //! \brief Update coefficients using constant coefficients for b' and kappa'
    //! \details This where the mix mass-fraction beta SDE is made consistent
    //!   with the no-mix and fully mixed limits by specifying the SDE
    //!   coefficients, b and kappa as functions of b' and kappa'. We leave S
    //!   unchanged.
    void update(
      char depvar,
      char,
      char,
      ctr::DepvarType,
      ncomp_t ncomp,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      const std::vector< kw::sde_rho2::info::expect::type >&,
      const std::vector< kw::sde_r::info::expect::type >&,
      const std::vector< tk::Table >&,
      const std::vector< tk::Table >&,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_S::info::expect::type >&,
      tk::real ) const
    {
      for (ncomp_t c=0; c<ncomp; ++c) {
        tk::real m = tk::ctr::lookup( tk::ctr::mean(depvar,c), moments );
        tk::real v = tk::ctr::lookup( tk::ctr::variance(depvar,c), moments );

        if (m<1.0e-8 || m>1.0-1.0e-8) m = 0.5;
        if (v<1.0e-8 || v>1.0-1.0e-8) v = 0.5;

        b[c] = bprime[c] * (1.0 - v / m / ( 1.0 - m ));
        k[c] = kprime[c] * v;
      }
    }
};

//! \brief Mix mass-fraction beta SDE homogneous decay coefficients policy
//! \details User-defined parameters b' and kappa' are constants in time and
//!   ensure decay in the evolution of \<y^2\>. Additionally, S is constrained
//!   to make d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY).
class MixMassFracBetaCoeffHomDecay {

    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor: initialize coefficients
    MixMassFracBetaCoeffHomDecay(
      ncomp_t ncomp,
      const std::vector< kw::sde_bprime::info::expect::type >& bprime_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
      const std::vector< kw::sde_r::info::expect::type >& r_,
      std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      std::vector< kw::sde_rho2::info::expect::type >& rho2,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k )
    {
      ErrChk( bprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'b''");
      ErrChk( S_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'S'");
      ErrChk( kprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'kappa''");
      ErrChk( rho2_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'rho2'");
      ErrChk( r_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'r'");

      bprime = bprime_;
      S = S_;
      kprime = kprime_;
      rho2 = rho2_;
      r = r_;

      b.resize( bprime.size() );
      k.resize( kprime.size() );
    }

    //! Coefficients policy type accessor
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::HOMOGENEOUS_DECAY; }

    //! \brief Update coefficients b', kappa', and S
    //! \details This where the mix mass-fraction beta SDE is made consistent
    //!   with the no-mix and fully mixed limits by specifying the SDE
    //!   coefficients, b and kappa as functions of b' and kappa'. We also
    //!   specify S to force d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY).
    void update(
      char depvar,
      char,
      char,
      ctr::DepvarType,
      ncomp_t ncomp,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< tk::Table >&,
      const std::vector< tk::Table >&,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_S::info::expect::type >& S,
      tk::real ) const
    {
      using tk::ctr::lookup;
      using tk::ctr::mean;
      using tk::ctr::variance;
      using tk::ctr::cen3;
      // statistics nomenclature:
      //   Y = instantaneous mass fraction,
      //   R = instantaneous density,
      //   y = Y - <Y>, mass fraction fluctuation about its mean,
      //   r = R - <R>, density fluctuation about its mean,
      // <Y> = mean mass fraction,
      // <R> = mean density,
      for (ncomp_t c=0; c<ncomp; ++c) {
        tk::real m = lookup( mean(depvar,c), moments );            // <Y>
        tk::real v = lookup( variance(depvar,c), moments );        // <y^2>
        tk::real d = lookup( mean(depvar,c+ncomp), moments );      // <R>
        tk::real d2 = lookup( variance(depvar,c+ncomp), moments ); // <r^2>
        tk::real d3 = lookup( cen3(depvar,c+ncomp), moments );     // <r^3>

        if (m<1.0e-8 || m>1.0-1.0e-8) m = 0.5;
        if (v<1.0e-8 || v>1.0-1.0e-8) v = 0.5;
        b[c] = bprime[c] * (1.0 - v/m/(1.0-m));
        //b[c] = bprime[c] * (1.0 - v/M[c]/(1.0-M[c]));
        k[c] = kprime[c] * v;
        //b[c] = 1.0;
        //k[c] = 0.5*v/(m*(1.0-m));

        if (d < 1.0e-8) {
          std::cout << "d:" << d << " ";
          d = 0.5;
        }
        tk::real R = 1.0 + d2/d/d;
        tk::real B = -1.0/r[c]/r[c];
        tk::real C = (2.0+r[c])/r[c]/r[c];
        tk::real D = -(1.0+r[c])/r[c]/r[c];
        tk::real diff =
          B*d/rho2[c] +
          C*d*d*R/rho2[c]/rho2[c] +
          D*d*d*d*(1.0 + 3.0*d2/d/d + d3/d/d/d)/rho2[c]/rho2[c]/rho2[c];
        S[c] = (rho2[c]/d/R +
                2.0*k[c]/b[c]*rho2[c]*rho2[c]/d/d*r[c]*r[c]/R*diff - 1.0) / r[c];
        if (S[c] < 0.0 || S[c] > 1.0) {
          std::cout << S[c] << " ";
          S[c] = 0.5;
        }
      }
    }
};

//! \brief Mix mass-fraction beta SDE Monte Carlo homogenous decay coefficients
//!   policy
//! \details User-defined parameters b' and kappa' are constants in time and
//!   ensure decay in the evolution of \<y^2\>. Additionally, S is constrained
//!   to make d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY). This is the same as
//!   the specification in MixMassFracBetaCoeffHomDecay, but uses more advanced
//!   statistics, available from the Monte Carlo simulation, which yield a
//!   simpler formula for the coefficient S.
class MixMassFracBetaCoeffMonteCarloHomDecay {

    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor: initialize coefficients
    MixMassFracBetaCoeffMonteCarloHomDecay(
      ncomp_t ncomp,
      const std::vector< kw::sde_bprime::info::expect::type >& bprime_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
      const std::vector< kw::sde_r::info::expect::type >& r_,
      std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      std::vector< kw::sde_rho2::info::expect::type >& rho2,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k )
    {
      ErrChk( bprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'b''");
      ErrChk( S_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'S'");
      ErrChk( kprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'kappa''");
      ErrChk( rho2_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'rho2'");
      ErrChk( r_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'r'");

      bprime = bprime_;
      S = S_;
      kprime = kprime_;
      rho2 = rho2_;
      r = r_;

      b.resize( bprime.size() );
      k.resize( kprime.size() );
    }

    //! Coefficients policy type accessor
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::MONTE_CARLO_HOMOGENEOUS_DECAY; }

    //! \brief Update coefficients b', kappa', and S
    //! \details This where the mix mass-fraction beta SDE is made consistent
    //!   with the no-mix and fully mixed limits by specifying the SDE
    //!   coefficients, b and kappa as functions of b' and kappa'. We also
    //!   specify S to force d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY).
    void update(
      char depvar,
      char,
      char,
      ctr::DepvarType,
      ncomp_t ncomp,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< tk::Table >&,
      const std::vector< tk::Table >&,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_S::info::expect::type >& S,
      tk::real) const
    {
      using tk::ctr::lookup;
      using tk::ctr::mean;
      using tk::ctr::variance;
      using tk::ctr::ord2;
      // statistics nomenclature:
      //   Y = instantaneous mass fraction,
      //   R = instantaneous density,
      //   y = Y - <Y>, mass fraction fluctuation about its mean,
      //   r = R - <R>, density fluctuation about its mean,
      // <Y> = mean mass fraction,
      // <R> = mean density,
      for (ncomp_t c=0; c<ncomp; ++c) {
        tk::real m = lookup( mean(depvar,c), moments );            // <Y>
        tk::real v = lookup( variance(depvar,c), moments );        // <y^2>
        tk::real r2 = lookup( ord2(depvar,c+ncomp), moments );     // <R^2>

        const tk::ctr::Term Y( static_cast<char>(std::toupper(depvar)),
                               c,
                               tk::ctr::Moment::ORDINARY );
        const tk::ctr::Term R( static_cast<char>(std::toupper(depvar)),
                               c+ncomp,
                               tk::ctr::Moment::ORDINARY );
        const tk::ctr::Term OneMinusY( static_cast<char>(std::toupper(depvar)),
                                       c+3*ncomp,
                                       tk::ctr::Moment::ORDINARY );

        const auto YR2 = tk::ctr::Product( { Y, R, R } );
        const auto Y1MYR3 = tk::ctr::Product( { Y, OneMinusY, R, R, R } );

        tk::real yr2 = lookup( YR2, moments );          // <RY^2>
        tk::real y1myr3 = lookup( Y1MYR3, moments );    // <Y(1-Y)R^3>

        if (m<1.0e-8 || m>1.0-1.0e-8) m = 0.5;
        if (v<1.0e-8 || v>1.0-1.0e-8) v = 0.5;
        b[c] = bprime[c] * (1.0 - v/m/(1.0-m));
        k[c] = kprime[c] * v;
        //b[c] = 1.0;
        //k[c] = 0.5*v/(m*(1.0-m));

        if (r2 < 1.0e-8) {
          std::cout << "r2:" << r2 << " ";
          r2 = 0.5;
        }
        S[c] = (yr2 + 2.0*k[c]/b[c]*r[c]/rho2[c]*y1myr3) / r2;
        if (S[c] < 0.0 || S[c] > 1.0) {
          std::cout << "S:" << S[c] << " ";
          S[c] = 0.5;
        }
      }
    }
};

//! \brief Mix mass-fraction beta SDE coefficients policy with DNS hydrodynamics
//!   time scale
//! \details User-defined parameters b' and kappa' are functions of an
//!   externally, e.g., DNS-, provided hydrodynamics time scale ensuring decay
//!   in the evolution of \<y^2\>. Additionally, S is constrained to
//!   make d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY). Additionally,
//!   we pull in a hydrodynamic timescale from an external function.
//! \see kw::hydrotimescale_info
class MixMassFracBetaCoeffHydroTimeScale {

    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor: initialize coefficients
    MixMassFracBetaCoeffHydroTimeScale(
      ncomp_t ncomp,
      const std::vector< kw::sde_bprime::info::expect::type >& bprime_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
      const std::vector< kw::sde_r::info::expect::type >& r_,
      std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      std::vector< kw::sde_rho2::info::expect::type >& rho2,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k )
    {
      ErrChk( bprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'b''");
      ErrChk( S_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'S'");
      ErrChk( kprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'kappa''");
      ErrChk( rho2_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'rho2'");
      ErrChk( r_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'r'");

      bprime = bprime_;
      S = S_;
      kprime = kprime_;
      rho2 = rho2_;
      r = r_;

      b.resize( bprime.size() );
      k.resize( kprime.size() );
    }

    //! Coefficients policy type accessor
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::HYDROTIMESCALE; }

    //! \brief Update coefficients b', kappa', and S
    //! \details This where the mix mass-fraction beta SDE is made consistent
    //!   with the no-mix and fully mixed limits by specifying the SDE
    //!   coefficients, b and kappa as functions of b' and kappa'. Additionally,
    //!   we pull in a hydrodynamic timescale from an external function. We also
    //!   specify S to force d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY).
    void update(
      char depvar,
      char,
      char,
      ctr::DepvarType,
      ncomp_t ncomp,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< tk::Table >& hts,
      const std::vector< tk::Table >& hp,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_S::info::expect::type >& S,
      tk::real t ) const
    {
      using tk::ctr::lookup;
      using tk::ctr::mean;
      using tk::ctr::variance;
      using tk::ctr::cen3;

      if (m_it == 0)
        for (ncomp_t c=0; c<ncomp; ++c)
           m_s.push_back( S[c] );

      // statistics nomenclature:
      //   Y = instantaneous mass fraction,
      //   R = instantaneous density,
      //   y = Y - <Y>, mass fraction fluctuation about its mean,
      //   r = R - <R>, density fluctuation about its mean,
      // <Y> = mean mass fraction,
      // <R> = mean density,
      for (ncomp_t c=0; c<ncomp; ++c) {

        const tk::ctr::Term Y( static_cast<char>(std::toupper(depvar)),
                               c,
                               tk::ctr::Moment::ORDINARY );
        const tk::ctr::Term dens( static_cast<char>(std::toupper(depvar)),
                                  c+ncomp,
                                  tk::ctr::Moment::ORDINARY );
        const tk::ctr::Term s1( static_cast<char>(std::tolower(depvar)),
                                c+ncomp,
                                tk::ctr::Moment::CENTRAL );
        const tk::ctr::Term s2( static_cast<char>(std::tolower(depvar)),
                                c+ncomp*2,
                                tk::ctr::Moment::CENTRAL );

        const auto RY = tk::ctr::Product( { dens, Y } );
        tk::real ry = lookup( RY, moments );                       // <RY>
        const auto dscorr = tk::ctr::Product( { s1, s2 } );
        tk::real ds = -lookup( dscorr, moments );                  // b = -<rv>
        tk::real d = lookup( mean(depvar,c+ncomp), moments );      // <R>
        tk::real d2 = lookup( variance(depvar,c+ncomp), moments ); // <r^2>
        tk::real d3 = lookup( cen3(depvar,c+ncomp), moments );     // <r^3>
        tk::real yt = ry/d;

        // Sample hydrodynamics timescale and prod/diss at time t
        auto ts = hydrotimescale( t, hts[c] );  // eps/k
        auto pe = hydroproduction( t, hp[c] );  // P/eps = (dk/dt+eps)/eps

        tk::real a = r[c]/(1.0+r[c]*yt);
        tk::real bnm = a*a*yt*(1.0-yt);
        tk::real thetab = 1.0 - ds/bnm;
        tk::real f2 =
          1.0 / std::pow(1.0 + std::pow(pe-1.0,2.0)*std::pow(ds,0.25),0.5);
        tk::real b1 = m_s[0];
        tk::real b2 = m_s[1];
        tk::real b3 = m_s[2];
        tk::real eta = d2/d/d/ds;
        tk::real beta2 = b2*(1.0+eta*ds);
        tk::real Thetap = thetab*0.5*(1.0+eta/(1.0+eta*ds));
        tk::real beta3 = b3*(1.0+eta*ds);
        tk::real beta10 = b1 * (1.0+ds)/(1.0+eta*ds);
        tk::real beta1 = bprime[c] * 2.0/(1.0+eta+eta*ds) *
                      (beta10 + beta2*Thetap*f2 + beta3*Thetap*(1.0-Thetap)*f2);
        b[c] = beta1 * ts;
        k[c] = kprime[c] * beta1 * ts * ds * ds;

        tk::real R = 1.0 + d2/d/d;
        tk::real B = -1.0/r[c]/r[c];
        tk::real C = (2.0+r[c])/r[c]/r[c];
        tk::real D = -(1.0+r[c])/r[c]/r[c];
        tk::real diff =
          B*d/rho2[c] +
          C*d*d*R/rho2[c]/rho2[c] +
          D*d*d*d*(1.0 + 3.0*d2/d/d + d3/d/d/d)/rho2[c]/rho2[c]/rho2[c];
        S[c] = (rho2[c]/d/R +
               2.0*k[c]/b[c]*rho2[c]*rho2[c]/d/d*r[c]*r[c]/R*diff - 1.0) / r[c];
      }

      ++m_it;
    }

    //! Sample the inverse hydrodynamics time scale at time t
    //! \param[in] t Time at which to sample inverse hydrodynamics time scale
    //! \param[in] ts Hydro time scale table to sample
    //! \return Sampled value from discrete table of inverse hydro time scale
    tk::real hydrotimescale( tk::real t, const tk::Table& ts ) const
    { return tk::sample( t, ts ); }

    //! Sample the hydrodynamics production/dissipation rate (P/e) at time t
    //! \param[in] t Time at which to sample hydrodynamics P/e
    //! \param[in] p P/e table to sample
    //! \return Sampled value from discrete table of P/e
    tk::real hydroproduction( tk::real t, const tk::Table& p ) const
    { return tk::sample( t, p ); }

    mutable std::size_t m_it = 0;
    mutable std::vector< tk::real > m_s;
};

//! \brief Mix mass-fraction beta SDE coefficients policy coupled to velocity
//! \details User-defined parameters b' and kappa' are functions of P/eps and
//!    k/eps from a coupled velocity model. Additionally, S is constrained to
//!   make d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY).
class MixMassFracBetaCoeffInstVel {

    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor: initialize coefficients
    MixMassFracBetaCoeffInstVel(
      ncomp_t ncomp,
      const std::vector< kw::sde_bprime::info::expect::type >& bprime_,
      const std::vector< kw::sde_S::info::expect::type >& S_,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime_,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2_,
      const std::vector< kw::sde_r::info::expect::type >& r_,
      std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      std::vector< kw::sde_S::info::expect::type >& S,
      std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      std::vector< kw::sde_rho2::info::expect::type >& rho2,
      std::vector< kw::sde_r::info::expect::type >& r,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k )
    {
      ErrChk( bprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'b''");
      ErrChk( S_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'S'");
      ErrChk( kprime_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'kappa''");
      ErrChk( rho2_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'rho2'");
      ErrChk( r_.size() == ncomp,
        "Wrong number of mix mass-fraction beta SDE parameters 'r'");

      bprime = bprime_;
      S = S_;
      kprime = kprime_;
      rho2 = rho2_;
      r = r_;

      b.resize( bprime.size() );
      k.resize( kprime.size() );
    }

    //! Coefficients policy type accessor
    static ctr::CoeffPolicyType type() noexcept
    { return ctr::CoeffPolicyType::INSTANTANEOUS_VELOCITY; }

    //! \brief Update coefficients b', kappa', and S
    //! \details This where the mix mass-fraction beta SDE is made consistent
    //!   with the no-mix and fully mixed limits by specifying the SDE
    //!   coefficients, b and kappa as functions of b' and kappa'. Additionally,
    //!   we pull in a hydrodynamic timescale from an external function. We also
    //!   specify S to force d\<rho\>/dt = 0, where \<rho\> = rho_2/(1+rY).
    void update(
      char depvar,
      char dissipation_depvar,
      char /*velocity_depvar*/,
      ctr::DepvarType /*velocity_solve*/,
      ncomp_t ncomp,
      const std::map< tk::ctr::Product, tk::real >& moments,
      const std::vector< kw::sde_bprime::info::expect::type  >& bprime,
      const std::vector< kw::sde_kappaprime::info::expect::type >& kprime,
      const std::vector< kw::sde_rho2::info::expect::type >& rho2,
      const std::vector< kw::sde_r::info::expect::type >& r,
      const std::vector< tk::Table >&,
      const std::vector< tk::Table >&,
      std::vector< kw::sde_b::info::expect::type  >& b,
      std::vector< kw::sde_kappa::info::expect::type >& k,
      std::vector< kw::sde_S::info::expect::type >& S,
      tk::real ) const
    {
      using tk::ctr::lookup;
      using tk::ctr::mean;
      using tk::ctr::variance;
      using tk::ctr::cen3;

      if (m_it == 0)
        for (ncomp_t c=0; c<ncomp; ++c)
           m_s.push_back( S[c] );

      // statistics nomenclature:
      //   Y = instantaneous mass fraction,
      //   R = instantaneous density,
      //   y = Y - <Y>, mass fraction fluctuation about its mean,
      //   r = R - <R>, density fluctuation about its mean,
      // <Y> = mean mass fraction,
      // <R> = mean density,
      for (ncomp_t c=0; c<ncomp; ++c) {

        const tk::ctr::Term Y( static_cast<char>(std::toupper(depvar)),
                               c,
                               tk::ctr::Moment::ORDINARY );
        const tk::ctr::Term dens( static_cast<char>(std::toupper(depvar)),
                                  c+ncomp,
                                  tk::ctr::Moment::ORDINARY );
        const tk::ctr::Term s1( static_cast<char>(std::tolower(depvar)),
                                c+ncomp,
                                tk::ctr::Moment::CENTRAL );
        const tk::ctr::Term s2( static_cast<char>(std::tolower(depvar)),
                                c+ncomp*2,
                                tk::ctr::Moment::CENTRAL );

        const auto RY = tk::ctr::Product( { dens, Y } );
        tk::real ry = lookup( RY, moments );                       // <RY>
        const auto dscorr = tk::ctr::Product( { s1, s2 } );
        tk::real ds = -lookup( dscorr, moments );                  // b = -<rv>
        tk::real d = lookup( mean(depvar,c+ncomp), moments );      // <R>
        tk::real d2 = lookup( variance(depvar,c+ncomp), moments ); // <r^2>
        tk::real d3 = lookup( cen3(depvar,c+ncomp), moments );     // <r^3>
        tk::real yt = ry/d;

        // Compute turbulent kinetic energy
        // auto K = tke( velocity_depvar, velocity_solve, moments );
        // Access mean turbulence frequency from coupled dissipation model
        // hydroptimescale: eps/k = <O>
        tk::real ts = lookup( mean(dissipation_depvar,0), moments );

        auto pe = 1.0; // hydroproductions: P/eps = 1.0 (equilibrium)

        // tk::real a = r[c]/(1.0+r[c]*yt);
        // tk::real bnm = a*a*yt*(1.0-yt);
        // tk::real thetab = 1.0 - ds/bnm;
        // tk::real f2 =
        //   1.0 / std::pow(1.0 + std::pow(pe-1.0,2.0)*std::pow(ds,0.25),0.5);
        // tk::real b1 = m_s[0];
        // tk::real b2 = m_s[1];
        // tk::real b3 = m_s[2];
        // tk::real eta = d2/d/d/ds;
        // tk::real beta2 = b2*(1.0+eta*ds);
        // tk::real Thetap = thetab*0.5*(1.0+eta/(1.0+eta*ds));
        // tk::real beta3 = b3*(1.0+eta*ds);
        // tk::real beta10 = b1 * (1.0+ds)/(1.0+eta*ds);
        tk::real beta1 = 2.0;//bprime[c] * 2.0/(1.0+eta+eta*ds) *
                      //(beta10 + beta2*Thetap*f2 + beta3*Thetap*(1.0-Thetap)*f2);
        b[c] = beta1 * ts;
        k[c] = kprime[c] * beta1 * ts * ds * ds;

        tk::real R = 1.0 + d2/d/d;
        tk::real B = -1.0/r[c]/r[c];
        tk::real C = (2.0+r[c])/r[c]/r[c];
        tk::real D = -(1.0+r[c])/r[c]/r[c];
        tk::real diff =
          B*d/rho2[c] +
          C*d*d*R/rho2[c]/rho2[c] +
          D*d*d*d*(1.0 + 3.0*d2/d/d + d3/d/d/d)/rho2[c]/rho2[c]/rho2[c];
        S[c] = (rho2[c]/d/R +
               2.0*k[c]/b[c]*rho2[c]*rho2[c]/d/d*r[c]*r[c]/R*diff - 1.0) / r[c];
      }

      ++m_it;
    }

    mutable std::size_t m_it = 0;
    mutable std::vector< tk::real > m_s;
};

//! List of all mix mass-fraction beta's coefficients policies
using MixMassFracBetaCoeffPolicies =
  brigand::list< MixMassFracBetaCoeffDecay
               , MixMassFracBetaCoeffHomDecay
               , MixMassFracBetaCoeffMonteCarloHomDecay
               , MixMassFracBetaCoeffHydroTimeScale
               , MixMassFracBetaCoeffInstVel
               >;

} // walker::

#endif // MixMassFractionBetaCoeffPolicy_h
