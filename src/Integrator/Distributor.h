//******************************************************************************
/*!
  \file      src/Integrator/Distributor.h
  \author    J. Bakosi
  \date      Fri 12 Sep 2014 07:23:43 AM MDT
  \copyright 2005-2014, Jozsef Bakosi.
  \brief     Distributor drives the time integration of differential equations
  \details   Distributor drives the time integration of differential equations
*/
//******************************************************************************
#ifndef Distributor_h
#define Distributor_h

#include <TaggedTuple.h>
#include <QuinoaPrint.h>
#include <Quinoa/CmdLine/CmdLine.h>
#include <distributor.decl.h>

namespace quinoa {

//! Distributor drives the time integration of differential equations
class Distributor : public CBase_Distributor {

  public:
    //! Constructor
    explicit Distributor( const ctr::CmdLine& cmdline );

    //! Finish initialization
    void init();

    //! Finish estimation of ordinary moments
    void estimateOrd( const std::vector< tk::real >& ord );

    //! Finish estimation of central moments
    void estimateCen( const std::vector< tk::real >& ctr );

    //! Finish estimation of PDFs
    void estimatePDF( const std::vector< PDF >& pdf );

  private:
    using CProxyInt = CProxy_Integrator< CProxy_Distributor >;

    //! Compute load distribution for given total work and virtualization
    void computeLoadDistribution( uint64_t& chunksize, uint64_t& remainder );

    //! compute size of next time step
    tk::real computedt();

    //! Print out time integration header
    void header() const;

    //! Evaluate time step
    void evaluateTime();

    //! Print out one-liner report on time step
    void report();

    //! Pretty printer
    QuinoaPrint m_print;

    //! Counters of integrator chares completing a function
    tk::tuple::tagged_tuple< tag::init,     uint64_t,
                             tag::ordinary, uint64_t,
                             tag::central,  uint64_t,
                             tag::pdf,      uint64_t,
                             tag::chare,    uint64_t > m_count;

    //! Output indicators
    tk::tuple::tagged_tuple< tag::stat, bool,
                             tag::pdf,  bool > m_output;

    uint64_t m_it;                              //!< Iteration count
    tk::real m_t;                               //!< Physical time
    std::vector< CProxyInt > m_proxy;           //!< Integrator proxies
    std::vector< tk::Timer > m_timer;           //!< Timers
    std::vector< std::string > m_nameOrdinary;  //!< Ordinary moment names
    std::vector< std::string > m_nameCentral;   //!< Central moment names
    std::vector< tk::real > m_ordinary;         //!< Ordinary moments
    std::vector< tk::real > m_central;          //!< Central moments
    std::vector< PDF > m_pdf;                   //!< PDFs
};

} // quinoa::

#endif // Distributor_h