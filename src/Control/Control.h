//******************************************************************************
/*!
  \file      src/Control/Control.h
  \author    J. Bakosi
  \date      Fri Oct 18 09:36:08 2013
  \copyright Copyright 2005-2012, Jozsef Bakosi, All rights reserved.
  \brief     Control base
  \details   Control base
*/
//******************************************************************************
#ifndef Control_h
#define Control_h

#include <string>
#include <sstream>

#include <TaggedTuple.h>

namespace tk {

//! Control : tagged_tuple
template<typename... Ts>
class Control : public tuple::tagged_tuple<Ts...> {

  private:
    //! Short-hand to innherited tagged tuple
    using Tuple = tuple::tagged_tuple<Ts...>;

  public:
    //! Constructor: set defaults
    explicit Control() = default;

    //! Destructor
    virtual ~Control() noexcept = default;

    //! Const-ref accessor
    //! TODO: Replace the overloads below with a variadic one
    //! Const-ref accessor to single element at 1st level
    template< typename tag >
    constexpr const typename Tuple::template nT<tag>&
    get() const noexcept {
      return Tuple::template get<tag>();
    }
    //! Const-ref accessor to single element at 2nd level
    template< typename tag, typename subtag >
    constexpr const typename Tuple::template nT<tag>::template nT<subtag>&
    get() const noexcept {
      return Tuple::template get<tag>().template get<subtag>();
    }
    //! Const-ref accessor to single element at 3rd level
    template< typename tag, typename subtag, typename subsubtag >
    constexpr const typename Tuple::template nT<tag>
                                  ::template nT<subtag>
                                  ::template nT<subsubtag>&
    get() const noexcept {
      return Tuple::template get<tag>().
                    template get<subtag>().
                    template get<subsubtag>();
    }

    //! Rvalue accessor
    //! TODO: Replace the overloads below with a variadic one
    //! Rvalue accessor to single element at 1st level
    template< typename tag >
    typename Tuple::template nT<tag>& get() noexcept {
      return Tuple::template get<tag>();
    }
    //! Rvalue accessor to single element at 2nd level
    template< typename tag, typename subtag >
    typename Tuple::template nT<tag>::template nT<subtag>& get() noexcept {
      return Tuple::template get<tag>().template get<subtag>();
    }
    //! Rvalue accessor to single element at 3rd level
    template< typename tag, typename subtag, typename subsubtag >
    typename Tuple::template nT<tag>
                  ::template nT<subtag>
                  ::template nT<subsubtag>& get() noexcept {
      return Tuple::template get<tag>().
                    template get<subtag>().
                    template get<subsubtag>();
    }

    //! Move value to slot
    //! TODO: Replace the overloads below with a variadic one
    //! Move value to slot at tag at 1st level
    template< typename tag >
    void set(const typename Tuple::template nT<tag>& value) noexcept {
      Tuple::template get<tag>() = std::move(value);
    }
    //! Move value to slot at tag at 2nd level
    template< typename tag, typename subtag >
    void set(const typename Tuple::template nT<tag>
                                 ::template nT<subtag>& value) noexcept {
      Tuple::template get<tag>().
             template get<subtag>() = std::move(value);
    }
    //! Move value to slot at tag at 3rd level
    template< typename tag, typename subtag, typename subsubtag >
    void set(const typename Tuple::template nT<tag>
                                 ::template nT<subtag>
                                 ::template nT<subsubtag>& value) noexcept {
      Tuple::template get<tag>().
             template get<subtag>().
             template get<subsubtag>() = std::move(value);
    }

    //! Convert and move value to slot
    //! TODO: Replace the overloads below with a variadic one
    //! Convert and move value to slot at tag at 1st level
    template< typename tag >
    void store(const std::string& value) noexcept {
      Tuple::template get<tag>() =
        convert<typename Tuple::template nT<tag>>( std::move(value) );
    }
    //! Convert and move value to slot at tag at 2nd level
    template< typename tag, typename subtag >
    void store(const std::string& value) noexcept {
      Tuple::template get<tag>().
             template get<subtag>() =
        convert<typename Tuple::template nT<tag>
                              ::template nT<subtag>>( std::move(value) );
    }
    //! Convert and move value to slot at tag at 3rd level
    template< typename tag, typename subtag, typename subsubtag >
    void store(const std::string& value) noexcept {
      Tuple::template get<tag>().
             template get<subtag>().
             template get<subsubtag>() =
        convert<typename Tuple::template nT<tag>
                              ::template nT<subtag>
                              ::template nT<subsubtag>>( std::move(value) );
    }

    //! Push back value to vector at slot
    //! TODO: Replace the overloads below with a variadic one
    //! Push back value to vector at tag at 1st level without conversion
    template< typename tag >
    void push_back(const typename Tuple::template nT<tag>::value_type& value) {
      Tuple::template get<tag>().push_back( std::move(value) );
    }
    //! Push back value to vector at tag at 2nd level without conversion
    template< typename tag, typename subtag >
    void push_back(const typename Tuple::template nT<tag>
                                       ::template nT<subtag>
                                       ::value_type& value) {
      Tuple::template get<tag>().
             template get<subtag>().push_back( std::move(value) );
    }
    //! Push back value to vector at tag at 3rd level without conversion
    template< typename tag, typename subtag, typename subsubtag >
    void push_back(const typename Tuple::template nT<tag>
                                       ::template nT<subtag>
                                       ::template nT<subsubtag>
                                       ::value_type& value) {
      Tuple::template get<tag>().
             template get<subtag>().
             template get<subsubtag>().push_back( std::move(value) );
    }

    //! Convert and push back value to vector at slot
    //! TODO: Replace the overloads below with a variadic one
    //! Convert and push back value to vector at tag at 1st level
    template< typename tag >
    void store_back(const std::string& value) {
      Tuple::template get<tag>().push_back(
        convert<typename Tuple::template nT<tag>
                              ::value_type>( std::move(value) ));
    }
    //! Convert and move value to slot at tag at 2nd level
    template< typename tag, typename subtag >
    void store_back(const std::string& value) {
      Tuple::template get<tag>().
             template get<subtag>().push_back(
        convert<typename Tuple::template nT<tag>
                              ::template nT<subtag>
                              ::value_type>( std::move(value) ));
    }
    //! Convert and move value to slot at tag at 3rd level
    template< typename tag, typename subtag, typename subsubtag >
    void store_back(const std::string& value) {
      Tuple::template get<tag>().
             template get<subtag>().
             template get<subsubtag>().push_back(
        convert<typename Tuple::template nT<tag>
                              ::template nT<subtag>
                              ::template nT<subsubtag>
                              ::value_type>( std::move(value) ));
    }

    // convert string to 'type' via std::stringstream
    template< typename type >
    type convert(const std::string& str) {
      std::stringstream ss(str);
      type num;
      ss >> num;
      return num;
    }

    // convert 'type' to string via std::stringstream
    template< typename type >
    std::string convert(const type& val) {
      std::stringstream ss;
      ss << val;
      return ss.str();
    }

  private:
    //! Don't permit copy constructor
    Control(const Control&) = delete;
    //! Don't permit copy assigment
    Control& operator=(const Control&) = delete;
    //! Don't permit move constructor
    Control(Control&&) = delete;
    //! Don't permit move assigment
    Control& operator=(Control&&) = delete;
};

} // tk::

#endif // Control_h
