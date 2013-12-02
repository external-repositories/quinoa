//******************************************************************************
/*!
  \file      src/Base/RNGTestPrint.h
  \author    J. Bakosi
  \date      Mon 02 Dec 2013 09:42:15 PM MST
  \copyright Copyright 2005-2012, Jozsef Bakosi, All rights reserved.
  \brief     RNGTest's printer
  \details   RNGTest's printer
*/
//******************************************************************************
#ifndef RNGTestPrint_h
#define RNGTestPrint_h

extern "C" {
  #include <gofw.h>
}

#include <Macro.h>
#include <Print.h>
#include <RNGTest/InputDeck/InputDeck.h>

namespace rngtest {

//! RNGTestPrint : Print
class RNGTestPrint : public tk::Print {

  public:
    //! Bring vanilla overloads from base into scope in case local overloads fail
    using Print::section;
    using Print::item;

    //! Constructor
    explicit RNGTestPrint(const std::unique_ptr< ctr::InputDeck >& control) :
      m_ctr(*control) {}

    //! Destructor
    ~RNGTestPrint() override = default;

    //! Print control option: 'group : option' only if differs from its default
    template<typename OptionType, typename... tags>
    void Section() const {
      if (m_ctr.get<tags...>() != ctr::InputDeckDefaults.get<tags...>()) {
        tk::Option<OptionType> opt;
        auto& group = opt.group();
        auto& value = opt.name(m_ctr.get<tags...>());
        std::cout << m_section_title_value_fmt % m_section_indent
                                               % m_section_bullet
                                               % group
                                               % value;
        std::cout << m_section_underline_fmt
                     % m_section_indent
                     % std::string(m_section_indent_size + 3 +
                                   group.size() + value.size(), '-');
      }
    }

    //! Print control option: 'group : option' only if differs from its default
    template<typename OptionType, typename... tags>
    void Item() const {
      if (m_ctr.get<tags...>() != ctr::InputDeckDefaults.get<tags...>()) {
        tk::Option<OptionType> opt;
        std::cout << m_item_name_value_fmt % m_item_indent
                                           % opt.group()
                                           % opt.name(m_ctr.get<tags...>());
      }
    }

    //! Print all fields of MKL RNG parameters
    template< typename RNG, typename UniformMethod, typename GaussianMethod,
              typename MapType >
    void Mklparams( const MapType& map ) const {
      tk::Option< RNG > rng;
      tk::Option< UniformMethod > um;
      tk::Option< GaussianMethod > gm;
      for (auto& m : map) {
        subsection( rng.name(m.first) );
        std::cout << m_item_name_value_fmt
                     % m_item_indent
                     % "seed"
                     % m.second.template get<quinoa::ctr::seed>();
        std::cout << m_item_name_value_fmt
                     % m_item_indent
                     % um.group()
                     % um.name(
                         m.second.template get<quinoa::ctr::uniform_method>() );
        std::cout << m_item_name_value_fmt
                     % m_item_indent
                     % gm.group()
                     % gm.name(
                         m.second.template get<quinoa::ctr::gaussian_method>() );
        endsubsection();
      }
    }

    //! Print statistical test names
    template< class StatTest, class TestContainer >
    void names( const TestContainer& tests,
                const typename TestContainer::size_type& ntest ) const {
      using Psize = typename StatTest::Pvals::size_type;
      using Tsize = typename TestContainer::size_type;
      for (Tsize i=0; i<ntest; ++i) {
        Psize npval = tests[i]->nresult();
        for (Psize p=0; p<npval; ++p) {
          std::string name( tests[i]->name(p) );
          if (p>0) name += " *";
          std::cout << m_list_item_fmt % m_item_indent % name;
        }
      }
      raw( "\n" );
      raw( m_item_indent + "Note: Tests followed by an asterisk (*) are\n" +
           m_item_indent + "statistics computed from the preceding test.\n" );
    }

    //! Return humand-readable p-value (ala TestU01::bbattery.c::WritePval)
    std::string pval( double p ) const {
      std::stringstream ss;
      if (p < gofw_Suspectp) {

        if ((p >= 0.01) && (p <= 0.99))
          ss << p;
        else if (p < gofw_Epsilonp)
          ss << "eps";
        else if (p < 0.01)
          ss << p;
        else if (p >= 1.0 - gofw_Epsilonp1)
          ss << "1 - eps1";
        else if (p < 1.0 - 1.0e-4)
          ss << p;
        else
          ss << 1.0 - p;

      } else if (p > 1.0 - gofw_Suspectp) {

        if (p >= 1.0 - gofw_Epsilonp1)
          ss << "1 - eps1";
        else if (p >= 1.0 - 1.0e-4)
          ss << "1 - " << 1.0 - p;
        else
          ss << p;

      }
      return ss.str();
    }

    //! Print failed statistical test names, RNGs, and p-values
    template< class StatTest, class TestContainer >
    void failed(
      const std::string& name,
      const typename std::vector< typename StatTest::Pvals >::size_type& total,
      const typename std::vector< typename StatTest::Pvals >::size_type& failed,
      const std::vector< typename StatTest::Pvals >& pvals,
      const TestContainer& tests ) const
    {
      using Psize = typename StatTest::Pvals::size_type;
      using Pval = typename StatTest::Pvals::value_type;
      using Tsize = typename TestContainer::size_type;
      std::stringstream ss;
      ss << name << " (" << failed << "/" << total << ")";
      section( ss.str() );
      raw( m_item_indent + "Legend: Test, RNG : p-value\n" +
           m_item_indent + "Note: List groupped by RNG, and in the order given "
                           "in the input file.\n\n" );
      Tsize ntest = tests.size();
      std::string oldname;
      for (Tsize i=0; i<ntest; ++i) {
        Psize npval = pvals[i].size();
        for (Psize p=0; p<npval; ++p) {
          if ( fabs(pvals[i][p]+1.0) > std::numeric_limits<Pval>::epsilon() ) {
            tk::Option< quinoa::ctr::RNG > rng;
            std::string newname( rng.name( tests[i]->rng() ) );
            std::string rngname( newname == oldname ? "" : (", " + newname) );
            oldname = newname;
            std::cout << m_item_widename_value_fmt
                         % m_item_indent
                         % (tests[i]->name(p) + rngname)
                         % pval( pvals[i][p] );
          }
        }
      }
    }

  private:
    //! Don't permit copy constructor
    RNGTestPrint(const RNGTestPrint&) = delete;
    //! Don't permit copy assigment
    RNGTestPrint& operator=(const RNGTestPrint&) = delete;
    //! Don't permit move constructor
    RNGTestPrint(RNGTestPrint&&) = delete;
    //! Don't permit move assigment
    RNGTestPrint& operator=(RNGTestPrint&&) = delete;

    const ctr::InputDeck& m_ctr;         //!< Parsed control
};

} // rngtest::

#endif // RNGTestPrint_h
