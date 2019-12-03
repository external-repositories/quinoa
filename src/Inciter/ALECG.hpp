// *****************************************************************************
/*!
  \file      src/Inciter/ALECG.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     ALECG for a PDE system with continuous Galerkin + ALE + RK
  \details   ALECG advances a system of partial differential equations (PDEs)
    using a continuous Galerkin (CG) finite element (FE) spatial discretization
    (using linear shapefunctions on tetrahedron elements) combined with a
    Runge-Kutta (RK) time stepping scheme in the arbitrary Eulerian-Lagrangian
    reference frame.

    There are a potentially large number of ALECG Charm++ chares created by
    Transporter. Each ALECG gets a chunk of the full load (part of the mesh)
    and does the same: initializes and advances a number of PDE systems in time.

    The implementation uses the Charm++ runtime system and is fully
    asynchronous, overlapping computation and communication. The algorithm
    utilizes the structured dagger (SDAG) Charm++ functionality.
*/
// *****************************************************************************
#ifndef ALECG_h
#define ALECG_h

#include <vector>
#include <map>

#include "QuinoaConfig.hpp"
#include "Types.hpp"
#include "Fields.hpp"
#include "DerivedData.hpp"
#include "FluxCorrector.hpp"
#include "NodeDiagnostics.hpp"
#include "Inciter/InputDeck/InputDeck.hpp"

#include "NoWarning/alecg.decl.h"

namespace inciter {

extern ctr::InputDeck g_inputdeck;

//! ALECG Charm++ chare array used to advance PDEs in time with ALECG+RK
class ALECG : public CBase_ALECG {

  public:
    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wunused-parameter"
      #pragma clang diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic push
      #pragma GCC diagnostic ignored "-Wunused-parameter"
      #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    #elif defined(__INTEL_COMPILER)
      #pragma warning( push )
      #pragma warning( disable: 1478 )
    #endif
    // Include Charm++ SDAG code. See http://charm.cs.illinois.edu/manuals/html/
    // charm++/manual.html, Sec. "Structured Control Flow: Structured Dagger".
    ALECG_SDAG_CODE
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #elif defined(STRICT_GNUC)
      #pragma GCC diagnostic pop
    #elif defined(__INTEL_COMPILER)
      #pragma warning( pop )
    #endif

    //! Constructor
    explicit ALECG( const CProxy_Discretization& disc,
                    const std::map< int, std::vector< std::size_t > >& bface,
                    const std::map< int, std::vector< std::size_t > >& bnode,
                    const std::vector< std::size_t >& triinpoel );

    #if defined(__clang__)
      #pragma clang diagnostic push
      #pragma clang diagnostic ignored "-Wundefined-func-template"
    #endif
    //! Migrate constructor
    // cppcheck-suppress uninitMemberVar
    explicit ALECG( CkMigrateMessage* msg ) : CBase_ALECG( msg ) {}
    #if defined(__clang__)
      #pragma clang diagnostic pop
    #endif

    //! Configure Charm++ custom reduction types initiated from this chare array
    static void registerReducers();

    //! Return from migration
    void ResumeFromSync() override;

    //! Size communication buffers (no-op)
    void resizeComm() {}

    //! Setup: query boundary conditions, output mesh, etc.
    void setup();

    // Initially compute left hand side diagonal matrix
    void init();

    //! Advance equations to next time step
    void advance( tk::real newdt );

    //! Compute left-hand side of transport equations
    void lhs();

    //! Receive boundary point normals on chare-boundaries
    void comnorm(
      const std::unordered_map< std::size_t, std::array<tk::real,4> >& innorm );

    //! Receive contributions to left-hand side matrix on chare-boundaries
    void comlhs( const std::vector< std::size_t >& gid,
                 const std::vector< std::vector< tk::real > >& L );

    //! Receive contributions to right-hand side vector on chare-boundaries
    void comrhs( const std::vector< std::size_t >& gid,
                 const std::vector< std::vector< tk::real > >& R );

    //! Update solution at the end of time step
    void update( const tk::Fields& a );

    //! Optionally refine/derefine mesh
    void refine();

    //! Receive new mesh from refiner
    void resizePostAMR(
      const std::vector< std::size_t >& ginpoel,
      const tk::UnsMesh::Chunk& chunk,
      const tk::UnsMesh::Coords& coord,
      const std::unordered_map< std::size_t, tk::UnsMesh::Edge >& addedNodes,
      const std::unordered_map< std::size_t, std::size_t >& addedTets,
      const std::unordered_map< int, std::vector< std::size_t > >& msum,
      const std::map< int, std::vector< std::size_t > >& /* bface */,
      const std::map< int, std::vector< std::size_t > >& bnode,
      const std::vector< std::size_t >& triinpoel );

    //! Const-ref access to current solution
    //! \return Const-ref to current solution
    const tk::Fields& solution() const { return m_u; }

    //! Resizing data sutrctures after mesh refinement has been completed
    void resized();

    //! Evaluate whether to continue with next time step
    void step();

    // Evaluate whether to do load balancing
    void evalLB();

    //! Continue to next time step
    void next();

    /** @name Charm++ pack/unpack serializer member functions */
    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) override {
      p | m_disc;
      p | m_initial;
      p | m_nsol;
      p | m_nlhs;
      p | m_nrhs;
      p | m_nnorm;
      p | m_bnode;
      p | m_triinpoel;
      p | m_esued;
      p | m_psup;
      p | m_u;
      p | m_un;
      p | m_lhs;
      p | m_rhs;
      p | m_bcdir;
      p | m_lhsc;
      p | m_rhsc;
      p | m_diag;
      p | m_bnorm;
      p | m_bnormc;
      p | m_stage;
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] i ALECG object reference
    friend void operator|( PUP::er& p, ALECG& i ) { i.pup(p); }
    //@}

  private:
    using ncomp_t = kw::ncomp::info::expect::type;

    //! Discretization proxy
    CProxy_Discretization m_disc;
    //! 1 if starting time stepping, 0 if during time stepping
    int m_initial;
    //! Counter for high order solution vector nodes updated
    std::size_t m_nsol;
    //! Counter for left-hand side matrix (vector) nodes updated
    std::size_t m_nlhs;
    //! Counter for right-hand side vector nodes updated
    std::size_t m_nrhs;
    //! Counter for receiving boundary point normals
    std::size_t m_nnorm;
    //! Boundary node lists mapped to side set ids where BCs are set by user
    std::map< int, std::vector< std::size_t > > m_bnode;
    //! Boundary triangle face connecitivity independent of BCs
    std::vector< std::size_t > m_triinpoel;
    //! Elements surrounding edges
    std::unordered_map< tk::UnsMesh::Edge, std::vector< std::size_t >,
                        tk::UnsMesh::Hash<2>, tk::UnsMesh::Eq<2> > m_esued;
    //! Points surrounding points
    std::pair< std::vector< std::size_t >, std::vector< std::size_t > > m_psup;
    //! Unknown/solution vector at mesh nodes
    tk::Fields m_u;
    //! Unknown/solution vector at mesh nodes at previous time
    tk::Fields m_un;
    //! Lumped lhs mass matrix
    tk::Fields m_lhs;
    //! Right-hand side vector (for the high order system)
    tk::Fields m_rhs;
    //! Boundary conditions evaluated and assigned to local mesh node IDs
    //! \details Vector of pairs of bool and boundary condition value associated
    //!   to local mesh node IDs at which the user has set Dirichlet boundary
    //!   conditions for all PDEs integrated. The bool indicates whether the BC
    //!   is set at the node for that component the if true, the real value is
    //!   the increment (from t to dt) in the BC specified for a component.
    std::unordered_map< std::size_t,
      std::vector< std::pair< bool, tk::real > > > m_bcdir;
    //! Receive buffer for communication of the left hand side
    //! \details Key: chare id, value: lhs for all scalar components per node
    std::unordered_map< std::size_t, std::vector< tk::real > > m_lhsc;
    //! Receive buffer for communication of the right hand side
    //! \details Key: chare id, value: rhs for all scalar components per node
    std::unordered_map< std::size_t, std::vector< tk::real > > m_rhsc;
    //! Diagnostics object
    NodeDiagnostics m_diag;
    //! Face normals in boundary points
    //! \details Key: local node id, value: unit normal and inverse distance
    //!   square between face centroids and points
    std::unordered_map< std::size_t, std::array< tk::real, 4 > > m_bnorm;
    //! Receive buffer for communication of the boundary point normals
    //! \details Key: global node id, value: normals (first 3 components),
    //!   inverse distance squared (4th component)
    std::unordered_map< std::size_t, std::array< tk::real, 4 > > m_bnormc;
    //! Runge-Kutta stage counter
    std::size_t m_stage;

    //! Access bound Discretization class pointer
    Discretization* Disc() const {
      Assert( m_disc[ thisIndex ].ckLocal() != nullptr, "ckLocal() null" );
      return m_disc[ thisIndex ].ckLocal();
    }

    //! Generate boundary points (independent of BCs set)
    std::vector< std::size_t > triinp();

    //! Compute boundary point normals
    void
    bnorm( const std::map< int, std::vector< std::size_t > >& bface,
           const std::vector< std::size_t >& triinpoel,
           std::unordered_set< std::size_t >&& symbcnodes );

    //! Finish setting up communication maps (norms, etc.)
    void normfinal();

    //! Output mesh and particle fields to files
    void out();

    //! Output mesh-based fields to file
    void writeFields( CkCallback c ) const;

    //! Combine own and communicated contributions to left hand side
    void lhsmerge();

    //! Compute righ-hand side vector of transport equations
    void rhs();

    //! Start time stepping
    void start();

    //! Solve low and high order diagonal systems
    void solve();

    //! Compute time step size
    void dt();

    //! Evaluate whether to continue with next time step stage
    void stage();

    //! Evaluate whether to save checkpoint/restart
    void evalRestart();
};

} // inciter::

#endif // ALECG_h
