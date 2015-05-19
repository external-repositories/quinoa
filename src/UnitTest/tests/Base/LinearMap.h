//******************************************************************************
/*!
  \file      src/UnitTest/tests/Base/LinearMap.h
  \author    J. Bakosi
  \date      Tue 19 May 2015 02:17:06 PM MDT
  \copyright 2012-2015, Jozsef Bakosi.
  \brief     Unit tests for Base/LinearMap
  \details   Unit tests for Base/LinearMap
*/
//******************************************************************************
#ifndef test_LinearMap_h
#define test_LinearMap_h

#include <tut/tut.hpp>
#include <LinearMap.h>
#include <linearmap.decl.h>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wconversion"
#endif

#include <testarray.decl.h>

#if defined(__clang__) || defined(__GNUC__)
  #pragma GCC diagnostic pop
#endif

namespace unittest {

extern std::string g_executable;

} // unittest::

namespace tut {

//! All tests in group inherited from this base
struct LinearMap_common {};

//! Test group shortcuts
using LinearMap_group =
  test_group< LinearMap_common, MAX_TESTS_IN_GROUP >;
using LinearMap_object = LinearMap_group::object;

//! Define test group
LinearMap_group LinearMap( "Base/LinearMap" );

//! Test definitions for group

//! Test if constructor does not throw on positive number of elements
template<> template<>
void LinearMap_object::test< 1 >() {
  set_test_name( "ctor doesn't throw on positive nelem" );

  tk::CProxy_LinearMap::ckNew( 2 );
}

//! Charm++ chare array definition for testing arrays
struct TestArray : CBase_TestArray {
  explicit TestArray() {}
  explicit TestArray( CkMigrateMessage* ) {}
};

//! Test use of LinearMap creating an array with nchare < numpes
template<> template<>
void LinearMap_object::test< 2 >() {
  int nchare = CkNumPes() > 1 ? CkNumPes()/2 : 1;
  set_test_name( "use with nchare (" + std::to_string(nchare) + ") <= numpes ("
                 + std::to_string(CkNumPes()) + ")" );

  // Create linear map chare
  tk::CProxy_LinearMap map = tk::CProxy_LinearMap( nchare );

  // Create array options object for use with linear map chare
  CkArrayOptions opts( nchare );
  opts.setMap( map );

  // Create chare array using linear map
  CProxy_TestArray arrayproxy = CProxy_TestArray::ckNew( opts );
  arrayproxy.doneInserting();

  // If this test fails it will spew errors on the screen...
}

//! Test use of LinearMap creating an array with nchare > numpes
template<> template<>
void LinearMap_object::test< 3 >() {
  int nchare = 2 * CkNumPes();
  set_test_name( "use with nchare (" + std::to_string(nchare) + ") > numpes ("
                 + std::to_string(CkNumPes()) + ")" );

  // Create linear map chare
  tk::CProxy_LinearMap map = tk::CProxy_LinearMap( nchare );

  // Create array options object for use with linear map chare
  CkArrayOptions opts( nchare );
  opts.setMap( map );

  // Create chare array using linear map
  CProxy_TestArray arrayproxy = CProxy_TestArray::ckNew( opts );
  arrayproxy.doneInserting();

  // If this test fails it will spew errors on the screen...
}

} // tut::

#endif // test_LinearMap_h
