// *****************************************************************************
/*!
  \file      src/Inciter/Scheme.hpp
  \copyright 2012-2015 J. Bakosi,
             2016-2018 Los Alamos National Security, LLC.,
             2019 Triad National Security, LLC.
             All rights reserved. See the LICENSE file for details.
  \brief     Generic forwarding interface to discretization proxies
  \details   This file defines a generic interface to discretization proxies.

    The purpose of this class is to hide, behind a single type, different
    Charm++  proxy types that model a single concept, i.e., define some common
    functions as Charm++ entry methods that can be used in either a broadcast
    and/or in a way of addressing a single array element. As a result, member
    functions can be invoked by client code without knowing the underlying type
    or any specifics to the underlying differences of the classes that model the
    same concept, i.e., expose the same member functions. The idea is very
    similar to inheritance and runtime polymorphism with base classes and
    virtual functions: some member functions and data are common to all types
    modeled (and thus are not repeated and/or copied), while some are specific.
    A difference is that the "base" and "child" classes are Charm++ proxies.
    Note that while Charm++ does support inheritance and runtime polymorphism
    with chare arrays, we still prefer the implementation below because it uses
    entirely value semantics (inside and in client code) and thus it keeps the
    complexity of the dispatch behind this class and does not expose it client
    code.

    The advantages of this class over traditional runtime polymorphism are (1)
    value semantics (both internally and to client code), (2) not templated,
    and (3) PUPable, i.e., an instance of Scheme can be sent across the network
    using Charm++'s pup framework. Also, since the class only holds a couple of
    chare proxies, it is extremely lightweight.

    Example usage from client code:

    \code{.cpp}
      // Instantiate a Scheme object
      Scheme s( ctr::SchemeType::DG );  // see Control/Inciter/Options/Scheme.h

      // Call a member function entry method in broadcast fashion
      s.coord< tag::bcast >( ... );     // equivalent to proxy.coord( ... );

      // Call a member function entry method in addressing a single array
      // element
      s.coord< tag::elem >( 0, ... );   // equivalent to proxy[0].coord( ... );

      // Broadcast to a member function with optinoal CkEntryOptions object
      CkEntryOptions opt;
      s.coord< tag::bcast >( ..., opt );     // proxy.coord( ..., opt );

      // Address array element with optinoal CkEntryOptions object
      s.coord< tag::elem >( 0, ..., opt );   // proxy[0].coord( ..., opt );
    \endcode

    Organization, implementation details, end extension of the class:

    Scheme, via inheriting from SchemeBase, contains two Charm++ proxies:
    discproxy and proxy. The former contains data and functionality common to
    all discretizations, and this can be considered as an equivalent to a base
    class in the OOP sense. The latter, proxy, contains data and functionality
    specific to a particular discretization. When instantiated, Scheme is
    configured for a single specific discretization which must be selected from
    the list of types in SchemeBase::Proxy.

    The underlying type of proxy is a variant, which allows storing exactly one
    object. A variant is a type-safe union. An instance of a variant at any
    given time either holds a value of one of its alternative types. Read more
    on std::variant or boost::variant on how they work.

    All new member functions that comprise of the concept of the underlying
    proxies, i.e., the interface, must be defined in Scheme. Whereas common
    data, functionality, as well as the list of the proxy types that can be
    configured are defined in SchemeBase. Adding a new forwarding function
    either as a broadcast or addressing a particular chare array element can be
    done by simply copying an existing (similar) one and modifying what
    underlying function (entry method) it calls. The ones that forward to
    discproxy are grouped first, while the ones that forward to the specific
    proxy are listed as second. Using SFINAE, multiple overloads are (and can
    be) defined for a single function, depending on (1) whether it is a
    broadcast or addressing an array element, (2) whether it takes an optional
    (default) last argument, usually used for passing a CkEntryOptions object.
    You can see the Charm++-generated .decl.h file to see what (if any) default
    arguments a particular entry method may take.

    Currently, forwarding functions are defined for two types entry method
    calls: broadcasts, i.e., proxy.fn(), and addressing a particular element,
    i.e., proxy[x].fn(). Another third might be useful to add in the future and
    that is addressing an entry method behind a section proxy. As we have not
    used section proxies so far, this is not yet implemented, but if necessary,
    it should be relatively straightforward to do.

    Extending this class to other discretization schemes is done entirely in
    SchemeBase. Adding a new discretization scheme amounts to, at the minimum:
    (1) Adding a new type of Charm++ chare array proxy to SchemeBase::Proxy,
    (2) Adding a new type of Charm++ chare array element proxy to
        SchemeBase::ProxyElem, and
    (3) Adding a new branch to the if test in SchemeBase's constructor.

    Implementation details: All forwarding calls are implemented taking a
    variadic parameter pack, which can take any number of arguments (including
    zero) and use perfect forwarding to pass the arguments to the entry method
    calls. This way the code remains generic, easy to modify, and the compiler
    automatically adjusts the generated forwarding calls if the types and/or
    number of the arguments to any of the entry methods change. One exception to
    this is those forwarding calls that deal with default arguments, allowing
    for passing CkEntryOptions objects. There the number of arguments are
    hard-coded in the SFINAE construct, but should also be straightforward to
    modify if necessary.

    Note that another way of doing the dispatch could have been implemented
    using a (compile-, or runtime) associative container storing std::function
    objects which would store pre-bound function arguments. That would work, but
    there are three problems with such an approach: (1) std::function is not
    obvious how to pup, i.e., send across the network, (2) std::bind cannot
    currently be used to bind a variadic number arguments and thus the bind
    calls would not be very generic, and (3) a runtime associative container
    would take additional state. (Note that problem (2) above could probably be
    solved with variadic lambdas, but the (1) and (3) remain.)

  \see Talk on [Concept-based runtime polymorphism with Charm++ chare arrays
    using value semantics](http://charm.cs.illinois.edu/charmWorkshop/slides/CharmWorkshop2018_bakosi.pdf) at the 16th Annual Workshop on Charm++ and its
    Applications, April 2018.
*/
// *****************************************************************************
#ifndef Scheme_h
#define Scheme_h

#include "Tags.hpp"
#include "SchemeBase.hpp"

namespace inciter {

//! Generic forwarding interface to discretization proxies
class Scheme : public SchemeBase {

  public:
    // Inherit base constructors
    using SchemeBase::SchemeBase;

    // Calls to proxy, specific to a particular discretization

    //////  proxy.setup(...)
    //! Function to call the setup entry method of an array proxy (broadcast)
    //! \param[in] args Arguments to member function (entry method) to be called
    //! \details This function calls the setup member function of a chare array
    //!   proxy and thus equivalent to proxy.setup(...), using the last argument
    //!   as default.
    template< typename... Args >
    void setup( Args&&... args ) {
      std::visit( [&]( auto& p ){ p.setup(std::forward<Args>(args)...); },
                  proxy );
    }

    //////  proxy.resizeComm(...)
    //! Function to call the resizeComm entry method of an array proxy
    //! \param[in] args Arguments to member function (entry method) to be called
    //! \details This function calls the resizeComm member function of a chare
    //!   array proxy and thus equivalent to proxy.resizeComm(...), using the
    //!   last argument as default.
    template< typename... Args >
    void resizeComm( Args&&... args ) {
      std::visit( [&]( auto& p ){ p.resizeComm(std::forward<Args>(args)...); },
                  proxy );
    }

    //////  proxy.lhs(...)
    //! Function to call the lhs entry method of an array proxy (broadcast)
    //! \param[in] args Arguments to member function (entry method) to be called
    //! \details This function calls the lhs member function of a chare array
    //!   proxy and thus equivalent to proxy.lhs(...), using the last argument
    //!   as default.
    template< typename... Args >
    void lhs( Args&&... args ) {
      std::visit( [&]( auto& p ){ p.lhs(std::forward<Args>(args)...); },
                  proxy );
    }

    //////  proxy.resized(...)
    //! Function to call the resized entry method of an array proxy (broadcast)
    //! \param[in] args Arguments to member function (entry method) to be called
    //! \details This function calls the resized member function of a chare
    //!   array proxy and thus equivalent to proxy.resized(...), using the last
    //!   argument as default.
    template< typename... Args >
    void resized( Args&&... args ) {
      std::visit( [&]( auto& p ){ p.resized(std::forward<Args>(args)...); },
                  proxy );
    }

    //////  proxy.advance(...)
    //! Function to call the advance entry method of an array proxy (broadcast)
    //! \param[in] args Arguments to member function entry method to be called
    //! \details This function calls the advance member function of a chare
    //!    array proxy and thus equivalent to proxy.advance(...), using the last
    //!    argument as default.
    template< typename... Args >
    void advance( Args&&... args ) {
      std::visit( [&]( auto& p ){ p.advance(std::forward<Args>(args)...); },
                  proxy );
    }

    //////  proxy.diag(...)
    //! function to call the diag entry method of an array proxy (broadcast)
    //! \param[in] args arguments to member function (entry method) to be called
    //! \details this function calls the diag member function of a chare array
    //!   proxy and thus equivalent to proxy.diag(...), specifying a
    //!   non-default last argument.
    template< typename... Args >
    void diag( Args&&... args ) {
      std::visit( [&]( auto& p ){ p.diag(std::forward<Args>(args)...); },
                  proxy );
    }

    //////  proxy[x].insert(...)
    //! Function to call the insert entry method of an element proxy (p2p)
    //! \param[in] x Chare array element index
    //! \param[in] args Arguments to member function (entry method) to be called
    //! \details This function calls the insert member function of a chare array
    //!   element proxy and thus equivalent to proxy[x].insert(...), using the
    //!   last argument as default.
    template< typename... Args >
    void insert( const CkArrayIndex1D& x, Args&&... args ) {
      auto e = tk::element< ProxyElem >( proxy, x );
      std::visit( [&]( auto& p ){ p.insert(std::forward<Args>(args)...); }, e );
    }

    //////  proxy.doneInserting(...)
    //! \brief Function to call the doneInserting entry method of an array proxy
    //!   (broadcast)
    //! \details This function calls the doneInserting member function of a
    //!   chare array proxy and thus equivalent to proxy.doneInserting(...).
    void doneInserting() {
      std::visit( [&]( auto& p ){ p.doneInserting(); }, proxy );
    }

    ///@{
    //! \brief Pack/Unpack serialize member function
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    void pup( PUP::er &p ) {
      SchemeBase::pup( p );
    }
    //! \brief Pack/Unpack serialize operator|
    //! \param[in,out] p Charm++'s PUP::er serializer object reference
    //! \param[in,out] s Scheme object reference
    friend void operator|( PUP::er& p, Scheme& s ) { s.pup(p); }
    //@}
};

} // inciter::

#endif // Scheme_h
