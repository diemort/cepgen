#ifndef CepGen_Cards_FunctionBuilder_h
#define CepGen_Cards_FunctionBuilder_h

#include <vector>
#include <array>
#include "CepGen/Core/utils.h"

#ifdef MATHEX
#include <mathex.h>
#endif

using std::string;

namespace CepGen
{
  template<std::size_t N>
  class FunctionBuilder
  {
#ifdef MATHEX
    public:
      FunctionBuilder() {}
      FunctionBuilder( const std::string& expr, const std::array<std::string,N>& vars ) :
        vars_( vars ) {
        parser_.expression( expr );
        for ( unsigned short i = 0; i < vars_.size(); ++i ) {
          parser_.addvar( vars_[i], &values_[i] );
        }
      }
      double eval( double x ) {
        static_assert( N==1, "This function only works with single-dimensional functions" );
        values_[0] = x;
        double ret = 1.0;
        try { ret = parser_.eval(); } catch ( const smlib::mathex::error& e ) {
          InWarning( Form( "Failed to evaluate the function: %s", e.what() ) );
        }
        return ret;
      }
      double eval( const std::array<double,N>& x ) {
        values_ = x;
        double ret = 1.0;
        try { ret = parser_.eval(); } catch ( const smlib::mathex::error& e ) {
          InWarning( Form( "Failed to evaluate the function: %s", e.what() ) );
        }
        return ret;
      }
      std::string& expression() { return parser_.expression(); }

    private:
      smlib::mathex parser_;
      std::array<std::string,N> vars_;
      std::array<double,N> values_;
#else
    public:
      FunctionBuilder( const std::string&, std::array<std::string,N>& ) {
        InError( "MathEx is not linked to this program! the math evaluator is hence disabled!" );
      }
      double eval( double ) { return 1.0; }
#endif
  };
}

#endif
