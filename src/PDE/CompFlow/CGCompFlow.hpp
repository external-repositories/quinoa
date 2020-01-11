// *****************************************************************************
/*!
  \file      src/PDE/CompFlow/CGCompFlow.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019-2020 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Compressible single-material flow using continuous Galerkin
  \details   This file implements the physics operators governing compressible
    single-material flow using continuous Galerkin discretization.
*/
// *****************************************************************************
#ifndef CGCompFlow_h
#define CGCompFlow_h

#include <cmath>
#include <algorithm>
#include <unordered_set>
#include <unordered_map>

#include "Macro.hpp"
#include "Exception.hpp"
#include "Vector.hpp"
#include "EoS/EoS.hpp"
#include "ProblemCommon.hpp"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

namespace cg {

//! \brief CompFlow used polymorphically with tk::CGPDE
//! \details The template arguments specify policies and are used to configure
//!   the behavior of the class. The policies are:
//!   - Physics - physics configuration, see PDE/CompFlow/Physics.h
//!   - Problem - problem configuration, see PDE/CompFlow/Problems.h
//! \note The default physics is Euler, set in inciter::deck::check_compflow()
template< class Physics, class Problem >
class CompFlow {

  private:
    using ncomp_t = kw::ncomp::info::expect::type;

  public:
    //! \brief Constructor
    //! \param[in] c Equation system index (among multiple systems configured)
    explicit CompFlow( ncomp_t c ) :
      m_physics(),
      m_problem(),
      m_system( c ),
      m_ncomp(
        g_inputdeck.get< tag::component >().get< tag::compflow >().at(c) ),
      m_offset(
        g_inputdeck.get< tag::component >().offset< tag::compflow >(c) )
    {
       Assert( m_ncomp == 5, "Number of CompFlow PDE components must be 5" );
    }

    //! Initalize the compressible flow equations, prepare for time integration
    //! \param[in] coord Mesh node coordinates
    //! \param[in,out] unk Array of unknowns
    //! \param[in] t Physical time
    void initialize( const std::array< std::vector< tk::real >, 3 >& coord,
                     tk::Fields& unk,
                     tk::real t ) const
    {
      Assert( coord[0].size() == unk.nunk(), "Size mismatch" );
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];
      // set initial and boundary conditions using problem policy
      for (ncomp_t i=0; i<coord[0].size(); ++i) {
        const auto s =
          Problem::solution( m_system, m_ncomp, x[i], y[i], z[i], t );
        unk(i,0,m_offset) = s[0]; // rho
        unk(i,1,m_offset) = s[1]; // rho * u
        unk(i,2,m_offset) = s[2]; // rho * v
        unk(i,3,m_offset) = s[3]; // rho * w
        unk(i,4,m_offset) = s[4]; // rho * e, e: total = kinetic + internal
      }
    }

    //! Return analytic solution (if defined by Problem) at xi, yi, zi, t
    //! \param[in] xi X-coordinate
    //! \param[in] yi Y-coordinate
    //! \param[in] zi Z-coordinate
    //! \param[in] t Physical time
    //! \return Vector of analytic solution at given location and time
    std::vector< tk::real >
    analyticSolution( tk::real xi, tk::real yi, tk::real zi, tk::real t ) const
    {
      auto s = Problem::solution( m_system, m_ncomp, xi, yi, zi, t );
      return std::vector< tk::real >( begin(s), end(s) );
    }

    //! Gather terms not dependent of dt
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bndel List of elements contributing to chare-boundary nodes
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    void gather( const std::array< std::vector< tk::real >, 3 >& coord,
                 const std::vector< std::size_t >& inpoel,
                 const std::vector< std::size_t >& bndel,
                 const std::unordered_map< std::size_t, std::size_t >& bid,
                 const tk::Fields& U,
                 tk::Fields& Ue ) const
    {
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );

      //for (auto e : bndel)
      //  gather_adv( e, coord, inpoel, bid, U, Ue );
      for (std::size_t e=0; e<inpoel.size()/4; ++e)
        gather_adv( e, coord, inpoel, bid, U, Ue );
    }

    //! Scatter terms not dependent dt
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bndel List of elements contributing to chare-boundary nodes
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    //! \param[in,out] R Right-hand side vector computed
    void scatter( const std::array< std::vector< tk::real >, 3 >& coord,
                  const std::vector< std::size_t >& inpoel,
                  const std::vector< std::size_t >& bndel,
                  const std::unordered_map< std::size_t, std::size_t >& bid,
                  const tk::Fields&,
                  const tk::Fields& Ue,
                  tk::Fields& R ) const
    {
      Assert( R.nunk() == coord[0].size(),
              "Number of unknowns and/or number of components in right-hand "
              "side vector incorrect" );

      // zero right hand side for all components
      for (ncomp_t c=0; c<5; ++c) R.fill( c, m_offset, 0.0 );

      //for (auto e : bndel)
      //  scatter_adv( e, coord, inpoel, bid, Ue, R );
      for (std::size_t e=0; e<inpoel.size()/4; ++e)
        scatter_adv( e, coord, inpoel, bid, Ue, R );
    }

    //! Gather terms dependent of dt
    //! \param[in] t Physical time
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bndel List of elements contributing to chare-boundary nodes
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    void gatherdt( tk::real t,
                   const std::array< std::vector< tk::real >, 3 >& coord,
                   const std::vector< std::size_t >& inpoel,
                   const std::vector< std::size_t >& bndel,
                   const std::unordered_map< std::size_t, std::size_t >& bid,
                   const tk::Fields& U,
                   tk::Fields& Ue ) const
    {
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );

      //for (auto e : bndel)
      //  gather_src( e, t, coord, inpoel, bid, U, Ue );
      for (std::size_t e=0; e<inpoel.size()/4; ++e)
        gather_src( e, t, coord, inpoel, bid, U, Ue );
    }

    //! Gather terms dependent of dt
    //! \param[in] t Physical time
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bndel List of elements contributing to chare-boundary nodes
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in,out] R Right-hand side vector to contribute to
    void scatterdt( tk::real t,
                    const std::array< std::vector< tk::real >, 3 >& coord,
                    const std::vector< std::size_t >& inpoel,
                    const std::vector< std::size_t >& bndel,
                    const std::unordered_map< std::size_t, std::size_t >& bid,
                    tk::Fields& R ) const
    {
      Assert( R.nunk() == coord[0].size(),
              "Number of unknowns and/or number of components in right-hand "
              "side vector incorrect" );

      // 2nd stage: contribute to chare-boundary only
      //for (auto e : bndel)
      //  scatter_src( e, t, coord, inpoel, bid, R );
      // 2nd stage: internal nodes only
      for (std::size_t e=0; e<inpoel.size()/4; ++e)
        scatter_src( e, t, coord, inpoel, bid, R );
    }

    //! Compute the minimum time step size
    //! \param[in] U Solution vector at recent time step
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \return Minimum time step size
    tk::real dt( const std::array< std::vector< tk::real >, 3 >& coord,
                 const std::vector< std::size_t >& inpoel,
                 const tk::Fields& U ) const
    {
      Assert( U.nunk() == coord[0].size(), "Number of unknowns in solution "
              "vector at recent time step incorrect" );
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];
      // ratio of specific heats
      auto g = g_inputdeck.get< tag::param, tag::compflow, tag::gamma >()[0][0];
      // compute the minimum dt across all elements we own
      tk::real mindt = std::numeric_limits< tk::real >::max();
      for (std::size_t e=0; e<inpoel.size()/4; ++e) {
        const std::array< std::size_t, 4 > N{{ inpoel[e*4+0], inpoel[e*4+1],
                                               inpoel[e*4+2], inpoel[e*4+3] }};
        // compute cubic root of element volume as the characteristic length
        const std::array< tk::real, 3 >
          ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
          ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
          da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
        const auto L = std::cbrt( tk::triple( ba, ca, da ) / 6.0 );
        // access solution at element nodes at recent time step
        std::array< std::array< tk::real, 4 >, 5 > u;
        for (ncomp_t c=0; c<5; ++c) u[c] = U.extract( c, m_offset, N );
        // compute the maximum length of the characteristic velocity (fluid
        // velocity + sound velocity) across the four element nodes
        tk::real maxvel = 0.0;
        for (std::size_t j=0; j<4; ++j) {
          auto& r  = u[0][j];    // rho
          auto& ru = u[1][j];    // rho * u
          auto& rv = u[2][j];    // rho * v
          auto& rw = u[3][j];    // rho * w
          auto& re = u[4][j];    // rho * e
          auto p = eos_pressure< tag::compflow >
                     ( m_system, r, ru/r, rv/r, rw/r, re );
          if (p < 0) p = 0.0;
          auto c = eos_soundspeed< tag::compflow >( m_system, r, p );
          auto v = std::sqrt((ru*ru + rv*rv + rw*rw)/r/r) + c; // char. velocity
          if (v > maxvel) maxvel = v;
        }
        // compute element dt for the Euler equations
        auto euler_dt = L / maxvel;
        // compute element dt based on the viscous force
        auto viscous_dt = m_physics.viscous_dt( L, u );
        // compute element dt based on thermal diffusion
        auto conduct_dt = m_physics.conduct_dt( L, g, u );
        // compute minimum element dt
        auto elemdt = std::min( euler_dt, std::min( viscous_dt, conduct_dt ) );
        // find minimum dt across all elements
        if (elemdt < mindt) mindt = elemdt;
      }
      return mindt;
    }

    //! Extract the velocity field at cell nodes. Currently unused.
    //! \param[in] U Solution vector at recent time step
    //! \param[in] N Element node indices    
    //! \return Array of the four values of the velocity field
    std::array< std::array< tk::real, 4 >, 3 >
    velocity( const tk::Fields& U,
              const std::array< std::vector< tk::real >, 3 >&,
              const std::array< std::size_t, 4 >& N ) const
    {
      std::array< std::array< tk::real, 4 >, 3 > v;
      v[0] = U.extract( 1, m_offset, N );
      v[1] = U.extract( 2, m_offset, N );
      v[2] = U.extract( 3, m_offset, N );
      auto r = U.extract( 0, m_offset, N );
      std::transform( r.begin(), r.end(), v[0].begin(), v[0].begin(),
                      []( tk::real s, tk::real& d ){ return d /= s; } );
      std::transform( r.begin(), r.end(), v[1].begin(), v[1].begin(),
                      []( tk::real s, tk::real& d ){ return d /= s; } );
      std::transform( r.begin(), r.end(), v[2].begin(), v[2].begin(),
                      []( tk::real s, tk::real& d ){ return d /= s; } );
      return v;
    }

    //! \brief Query Dirichlet boundary condition value on a given side set for
    //!    all components in this PDE system
    //! \param[in] t Physical time
    //! \param[in] deltat Time step size
    //! \param[in] ss Pair of side set ID and (local) node IDs on the side set
    //! \param[in] coord Mesh node coordinates
    //! \return Vector of pairs of bool and boundary condition value associated
    //!   to mesh node IDs at which Dirichlet boundary conditions are set. Note
    //!   that instead of the actual boundary condition value, we return the
    //!   increment between t+dt and t, since that is what the solution requires
    //!   as we solve for the soution increments and not the solution itself.
    std::map< std::size_t, std::vector< std::pair<bool,tk::real> > >
    dirbc( tk::real t,
           tk::real deltat,
           const std::pair< const int, std::vector< std::size_t > >& ss,
           const std::array< std::vector< tk::real >, 3 >& coord ) const
    {
      using tag::param; using tag::compflow; using tag::bcdir;
      using NodeBC = std::vector< std::pair< bool, tk::real > >;
      std::map< std::size_t, NodeBC > bc;
      const auto& ubc = g_inputdeck.get< param, compflow, tag::bc, bcdir >();
      if (!ubc.empty()) {
        Assert( ubc.size() > 0, "Indexing out of Dirichlet BC eq-vector" );
        const auto& x = coord[0];
        const auto& y = coord[1];
        const auto& z = coord[2];
        for (const auto& b : ubc[0])
          if (std::stoi(b) == ss.first)
            for (auto n : ss.second) {
              Assert( x.size() > n, "Indexing out of coordinate array" );
              auto s = solinc( m_system, m_ncomp, x[n], y[n], z[n],
                               t, deltat, Problem::solution );
              bc[n] = {{ {true,s[0]}, {true,s[1]}, {true,s[2]}, {true,s[3]},
                         {true,s[4]} }};
            }
      }
      return bc;
    }

    //! Set symmetry boundary conditions at nodes
    //! \param[in] U Solution vector at recent time step
    //! \param[in] bnorm Face normals in boundary points: key global node id,
    //!    value: unit normal
    void
    symbc( tk::Fields& U,
           const std::unordered_map<std::size_t,std::array<tk::real,4>>& bnorm )
    const {
      for (const auto& [ i, nr ] : bnorm ) {
        std::array< tk::real, 3 >
          n{ nr[0], nr[1], nr[2] },
          v{ U(i,1,m_offset), U(i,2,m_offset), U(i,3,m_offset) };
        auto v_dot_n = tk::dot( v, n );
        U(i,1,m_offset) -= v_dot_n * n[0];
        U(i,2,m_offset) -= v_dot_n * n[1];
        U(i,3,m_offset) -= v_dot_n * n[2];
      }
    }

    //! Query nodes at which symmetry boundary conditions are set
    //! \param[in] bface Boundary-faces mapped to side set ids
    //! \param[in] triinpoel Boundary-face connectivity
    //! \param[in,out] nodes Node ids at which symmetry BCs are set
    void
    symbcnodes( const std::map< int, std::vector< std::size_t > >& bface,
                const std::vector< std::size_t >& triinpoel,
                std::unordered_set< std::size_t >& nodes ) const
    {
      using tag::param; using tag::compflow; using tag::bcsym;
      const auto& bc = g_inputdeck.get< param, compflow, tag::bc, bcsym >();
      if (!bc.empty() && bc.size() > m_system) {
        const auto& ss = bc[ m_system ];// side sets with sym bcs specified
        for (const auto& s : ss) {
          auto k = bface.find( std::stoi(s) );
          if (k != end(bface)) {
            for (auto f : k->second) {  // face ids on symbc side set
              nodes.insert( triinpoel[f*3+0] );
              nodes.insert( triinpoel[f*3+1] );
              nodes.insert( triinpoel[f*3+2] );
            }
          }
        }
      }
    }

    //! Return field names to be output to file
    //! \return Vector of strings labelling fields output in file
    std::vector< std::string > fieldNames() const
    { return m_problem.fieldNames( m_ncomp ); }

    //! Return field output going to file
    //! \param[in] t Physical time
    //! \param[in] V Total mesh volume
    //! \param[in] coord Mesh node coordinates
    //! \param[in] v Nodal mesh volumes
    //! \param[in,out] U Solution vector at recent time step
    //! \return Vector of vectors to be output to file
    std::vector< std::vector< tk::real > >
    fieldOutput( tk::real t,
                 tk::real V,
                 const std::array< std::vector< tk::real >, 3 >& coord,
                 const std::vector< tk::real >& v,
                 tk::Fields& U ) const
    {
      return
        m_problem.fieldOutput( m_system, m_ncomp, m_offset, t, V, v, coord, U );
    }

    //! Return names of integral variables to be output to diagnostics file
    //! \return Vector of strings labelling integral variables output
    std::vector< std::string > names() const
    { return m_problem.names( m_ncomp ); }

  private:
    const Physics m_physics;            //!< Physics policy
    const Problem m_problem;            //!< Problem policy
    const ncomp_t m_system;             //!< Equation system index
    const ncomp_t m_ncomp;              //!< Number of components in this PDE
    const ncomp_t m_offset;             //!< Offset PDE operates from

    //! Gather advetcion
    //! \details Update element values from node values (gather-add)
    //! \param[in] e Element to compute
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    void gather_adv( std::size_t e,
                     const std::array< std::vector< tk::real >, 3 >& coord,
                     const std::vector< std::size_t >& inpoel,
                     const std::unordered_map< std::size_t, std::size_t >& bid,
                     const tk::Fields& U,
                     tk::Fields& Ue ) const
    {
      const std::array< std::size_t, 4 > N{{ inpoel[e*4+0], inpoel[e*4+1],
                                             inpoel[e*4+2], inpoel[e*4+3] }};
      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // compute element Jacobi determinant
      std::array< tk::real, 3 >
        ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
        ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
        da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
      auto J = tk::triple( ba, ca, da );        // J = 6V
      Assert( J > 0, "Element Jacobian non-positive" );

      // shape function derivatives, nnode*ndim [4][3]
      std::array< std::array< tk::real, 3 >, 4 > grad;
      grad[1] = tk::crossdiv( ca, da, J );
      grad[2] = tk::crossdiv( da, ba, J );
      grad[3] = tk::crossdiv( ba, ca, J );
      for (std::size_t i=0; i<3; ++i)
        grad[0][i] = -grad[1][i]-grad[2][i]-grad[3][i];

      // access solution at element nodes
      std::array< std::array< tk::real, 4 >, 5 > u;
      for (ncomp_t c=0; c<5; ++c) u[c] = U.extract( c, m_offset, N );
      // access pointer to solution at elements
      std::array< const tk::real*, 5 > ue;
      for (ncomp_t c=0; c<5; ++c) ue[c] = Ue.cptr( c, m_offset );

      // pressure
      std::array< tk::real, 4 > p;
      for (std::size_t a=0; a<4; ++a)
        p[a] = eos_pressure< tag::compflow >
                 ( m_system, u[0][a], u[1][a]/u[0][a], u[2][a]/u[0][a],
                   u[3][a]/u[0][a], u[4][a] );

      // sum flux contributions to element
      tk::real d = 0.5;
      for (std::size_t j=0; j<3; ++j)
        for (std::size_t a=0; a<4; ++a) {
          // mass: advection
          Ue.var(ue[0],e) -= d * grad[a][j] * u[j+1][a];
          // momentum: advection
          for (std::size_t i=0; i<3; ++i)
            Ue.var(ue[i+1],e) -= d * grad[a][j] * u[j+1][a]*u[i+1][a]/u[0][a];
          // momentum: pressure
          Ue.var(ue[j+1],e) -= d * grad[a][j] * p[a];
          // energy: advection and pressure
          Ue.var(ue[4],e) -= d * grad[a][j] *
                            (u[4][a] + p[a]) * u[j+1][a]/u[0][a];
        }
    }

    //! Scatter advection
    //! \details Form rhs from element values (scatter-add)
    //! \param[in] e Element to compute
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    //! \param[in,out] R Right-hand side vector computed
    void scatter_adv( std::size_t e,
                      const std::array< std::vector< tk::real >, 3 >& coord,
                      const std::vector< std::size_t >& inpoel,
                      const std::unordered_map< std::size_t, std::size_t >& bid,
                      const tk::Fields& Ue,
                      tk::Fields& R ) const
    {
      const std::array< std::size_t, 4 >
        N{{ inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] }};

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // compute element Jacobi determinant
      std::array< tk::real, 3 >
        ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
        ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
        da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
      auto J = tk::triple( ba, ca, da );        // J = 6V
      Assert( J > 0, "Element Jacobian non-positive" );

      // shape function derivatives, nnode*ndim [4][3]
      std::array< std::array< tk::real, 3 >, 4 > grad;
      grad[1] = tk::crossdiv( ca, da, J );
      grad[2] = tk::crossdiv( da, ba, J );
      grad[3] = tk::crossdiv( ba, ca, J );
      for (std::size_t i=0; i<3; ++i)
        grad[0][i] = -grad[1][i]-grad[2][i]-grad[3][i];

      // access solution at elements
      std::array< tk::real, 5 > ue;
      for (ncomp_t c=0; c<5; ++c) ue[c] = Ue( e, c, m_offset );
      // access pointer to right hand side at component and offset
      std::array< const tk::real*, 5 > r;
      for (ncomp_t c=0; c<5; ++c) r[c] = R.cptr( c, m_offset );

      // pressure
      auto p = eos_pressure< tag::compflow >
                 ( m_system, ue[0], ue[1]/ue[0], ue[2]/ue[0], ue[3]/ue[0],
                   ue[4] );

      // scatter-add flux contributions to rhs at nodes
      tk::real d = J/6.0;
      for (std::size_t j=0; j<3; ++j)
        for (std::size_t a=0; a<4; ++a) {
          // mass: advection
          R.var(r[0],N[a]) += d * grad[a][j] * ue[j+1];
          // momentum: advection
          for (std::size_t i=0; i<3; ++i)
            R.var(r[i+1],N[a]) += d * grad[a][j] * ue[j+1]*ue[i+1]/ue[0];
          // momentum: pressure
          R.var(r[j+1],N[a]) += d * grad[a][j] * p;
          // energy: advection and pressure
          R.var(r[4],N[a]) += d * grad[a][j] * (ue[4] + p) * ue[j+1]/ue[0];
        }
    }

    //! Gather source
    //! \details Update element values from node values (gather-add)
    //! \param[in] e Element to compute
    //! \param[in] t Physical time at which to evaluate the source
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in] U Solution vector at recent time step
    //! \param[in,out] Ue Element-centered solution vector at intermediate step
    //!    (used here internally as a scratch array)
    void gather_src( std::size_t e,
                    tk::real t,
                    const std::array< std::vector< tk::real >, 3 >& coord,
                    const std::vector< std::size_t >& inpoel,
                    const std::unordered_map< std::size_t, std::size_t >& bid,
                    const tk::Fields& U,
                    tk::Fields& Ue ) const
    {
      const std::array< std::size_t, 4 >
        N{{ inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] }};

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // access solution at element nodes
      std::array< std::array< tk::real, 4 >, 5 > u;
      for (ncomp_t c=0; c<5; ++c) u[c] = U.extract( c, m_offset, N );
      // access pointer to solution at elements
      std::array< const tk::real*, 5 > ue;
      for (ncomp_t c=0; c<5; ++c) ue[c] = Ue.cptr( c, m_offset );

      // add (optional) source to all equations
      std::array< std::vector< tk::real >, 4 > s{{
        Problem::src( m_system, m_ncomp, x[N[0]], y[N[0]], z[N[0]], t ),
        Problem::src( m_system, m_ncomp, x[N[1]], y[N[1]], z[N[1]], t ),
        Problem::src( m_system, m_ncomp, x[N[2]], y[N[2]], z[N[2]], t ),
        Problem::src( m_system, m_ncomp, x[N[3]], y[N[3]], z[N[3]], t ) }};
      for (std::size_t c=0; c<5; ++c)
        for (std::size_t a=0; a<4; ++a)
          Ue.var(ue[c],e) += s[a][c] / 8.0;
    }

    //! Scatter source
    //! \details Form rhs from element values (scatter-add)
    //! \param[in] e Element to compute
    //! \param[in] t Physical time at which to evaluate the source
    //! \param[in] coord Mesh node coordinates
    //! \param[in] inpoel Mesh element connectivity
    //! \param[in] bid Local chare-boundary node ids (value) associated to
    //!    global node ids (key)
    //! \param[in,out] R Right-hand side vector computed
    void scatter_src( std::size_t e,
                      tk::real t,
                      const std::array< std::vector< tk::real >, 3 >& coord,
                      const std::vector< std::size_t >& inpoel,
                      const std::unordered_map< std::size_t, std::size_t >& bid,
                      tk::Fields& R ) const
    {
      const std::array< std::size_t, 4 >
        N{{ inpoel[e*4+0], inpoel[e*4+1], inpoel[e*4+2], inpoel[e*4+3] }};

      const auto& x = coord[0];
      const auto& y = coord[1];
      const auto& z = coord[2];

      // compute element Jacobi determinant
      std::array< tk::real, 3 >
        ba{{ x[N[1]]-x[N[0]], y[N[1]]-y[N[0]], z[N[1]]-z[N[0]] }},
        ca{{ x[N[2]]-x[N[0]], y[N[2]]-y[N[0]], z[N[2]]-z[N[0]] }},
        da{{ x[N[3]]-x[N[0]], y[N[3]]-y[N[0]], z[N[3]]-z[N[0]] }};
      auto J = tk::triple( ba, ca, da );        // J = 6V
      Assert( J > 0, "Element Jacobian non-positive" );

      // shape function derivatives, nnode*ndim [4][3]
      std::array< std::array< tk::real, 3 >, 4 > grad;
      grad[1] = tk::crossdiv( ca, da, J );
      grad[2] = tk::crossdiv( da, ba, J );
      grad[3] = tk::crossdiv( ba, ca, J );
      for (std::size_t i=0; i<3; ++i)
        grad[0][i] = -grad[1][i]-grad[2][i]-grad[3][i];

      // access pointer to right hand side at component and offset
      std::array< const tk::real*, 5 > r;
      for (ncomp_t c=0; c<5; ++c) r[c] = R.cptr( c, m_offset );

      // add (optional) source to all equations
      tk::real d = J/24.0;
      auto xc = (x[N[0]] + x[N[1]] + x[N[2]] + x[N[3]]) / 4.0;
      auto yc = (y[N[0]] + y[N[1]] + y[N[2]] + y[N[3]]) / 4.0;
      auto zc = (z[N[0]] + z[N[1]] + z[N[2]] + z[N[3]]) / 4.0;
      auto s = Problem::src( m_system, m_ncomp, xc, yc, zc, t );
      for (std::size_t c=0; c<5; ++c)
        for (std::size_t a=0; a<4; ++a)
          R.var(r[c],N[a]) += d * s[c];
    }
};

} // cg::

} // inciter::

#endif // CGCompFlow_h
