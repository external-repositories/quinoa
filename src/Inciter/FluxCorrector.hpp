// *****************************************************************************
/*!
  \file      src/Inciter/FluxCorrector.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     FluxCorrector performs limiting for transport equations
  \details   FluxCorrector performs limiting for transport equations. Each
    FluxCorrector object performs the limiting procedure, according to a
    flux-corrected transport algorithm, on a chunk of the full load (part of the
    mesh).
*/
// *****************************************************************************
#ifndef FluxCorrector_h
#define FluxCorrector_h

#include <utility>
#include <unordered_map>

#include "NoWarning/pup.hpp"

#include "Keywords.hpp"
#include "Fields.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

//! FluxCorrector is used to perform flux-corrected transport
//! \see Löhner, R., Morgan, K., Peraire, J. and Vahdati, M. (1987), Finite
//!   element flux-corrected transport (FEM–FCT) for the Euler and Navier–Stokes
//!   equations. Int. J. Numer. Meth. Fluids, 7: 1093–1109.
//!   doi:10.1002/fld.1650071007
class FluxCorrector {

  private:
    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! Constructor
    //! \param[in] is Size of the mesh element connectivity vector (inpoel size)
    explicit FluxCorrector( std::size_t is = 0 ) :
      m_aec( is, g_inputdeck.get< tag::component >().nprop() ),
      m_sys( findsys< tag::compflow >() ),
      m_vel( findvel< tag::compflow >() ) {}

    //! Collect scalar comonent indices for equation systems
    //! \tparam Eq Equation types to consider as equation systems
    //! \return List of component indices to treat as a system
    //! \warning At this point multiple CompFlow eq systems will be limited as
    //!    one single coupled system!
    template< class... Eq >
    std::vector< ncomp_t >
    findsys() {
      std::vector< ncomp_t > s;
      if (g_inputdeck.get< tag::discr, tag::sysfct >()) {
        ( ... , [&](){
          const auto& ncompv = g_inputdeck.get< tag::component >().get< Eq >();
          for (std::size_t e=0; e<ncompv.size(); ++e) {
            auto offset = g_inputdeck.get< tag::component >().offset< Eq >( e );
            for (std::size_t c=0; c<ncompv[e]; ++c) s.push_back( offset + c );
          }
          }() );
      }
      Assert( std::all_of( begin(s), end(s), [&]( std::size_t i ){
                return i < g_inputdeck.get< tag::component >().nprop(); } ),
              "Eq system index larger than total number of components" );
      return s;
    }

    //! Find components of a velocity for equation systems
    //! \tparam Eq Equation types to consider as equation systems
    //! \return List of 3 component indices to treat as a velocity
    //! \warning Currently, this is only a punt for single-material flow: we
    //!   simply take the components 1,2,3 as the velocity.
    template< class... Eq >
    std::array< ncomp_t, 3 >
    findvel() {
      return { 1, 2, 3 };
    }

    //! Resize state (e.g., after mesh refinement)
    void resize( std::size_t is ) {
      m_aec.resize( is, g_inputdeck.get< tag::component >().nprop() );
    }

    //! Compute antidiffusive element contributions (AEC)
    void aec(
      const std::array< std::vector< tk::real >, 3 >& coord,
      const std::vector< std::size_t >& inpoel,
      const std::vector< tk::real >& vol,
      const std::unordered_map< std::size_t,
              std::vector< std::pair< bool, tk::real > > >& bc,
      const std::unordered_map< std::size_t, std::array< tk::real, 4 > >& bnorm,
      const tk::Fields& Un,
      tk::Fields& P );

    //! Verify the assembled antidiffusive element contributions
    bool verify( std::size_t nchare,
                 const std::vector< std::size_t >& inpoel,
                 const tk::Fields& dUh,
                 const tk::Fields& dUl ) const;

    //! Compute lumped mass matrix lhs for low order system
    tk::Fields lump( const std::array< std::vector< tk::real >, 3 >& coord,
                     const std::vector< std::size_t >& inpoel ) const;

    //! Compute mass diffusion contribution to the rhs of the low order system
    tk::Fields diff( const std::array< std::vector< tk::real >, 3 >& coord,
                     const std::vector< std::size_t >& inpoel,
                     const tk::Fields& Un ) const;

    //! \brief Compute the maximum and minimum unknowns of all elements
    //!   surrounding nodes
    void alw( const std::vector< std::size_t >& inpoel,
              const tk::Fields& Un,
              const tk::Fields& Ul,
              tk::Fields& Q ) const;

    //! Compute limited antiffusive element contributions and apply to mesh nodes
    void lim( const std::vector< std::size_t >& inpoel,
              const std::unordered_map< std::size_t,
                      std::vector< std::pair< bool, tk::real > > >& bcdir,
              const tk::Fields& P,
              const tk::Fields& Ul,
              tk::Fields& Q,
              tk::Fields& A ) const;

    // Collect mesh output fields from FCT
    std::tuple< std::vector< std::string >,
                std::vector< std::vector< tk::real > > >
    fields( const std::vector< std::size_t >& inpoel ) const;

    /** @name Charm++ pack/unpack serializer member functions */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er& p ) {
      p | m_aec;
      p | m_sys;
      p | m_vel;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i FluxCorrector object reference
    friend void operator|( PUP::er& p, FluxCorrector& i ) { i.pup(p); }
    //@}

  private:
   //! Antidiffusive element contributions for all scalar components
   tk::Fields m_aec;
   //! Component indices to treat as a system
   std::vector< ncomp_t > m_sys;
   //! Component indices to treat as a velocity vector
   std::array< ncomp_t, 3 > m_vel;
};

} // inciter::

#endif // FluxCorrector_h
