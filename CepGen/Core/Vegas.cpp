#include "Vegas.h"

namespace CepGen
{
  Vegas::Vegas( const unsigned int dim, double f_( double*, size_t, void* ), Parameters* param ) :
    vegas_bin_( 0 ), correc_( 0. ), correc2_( 0. ),
    input_params_( param ),
    grid_prepared_( false ), gen_prepared_( false ),
    f_max2_( 0. ), f_max_diff_( 0. ), f_max_old_( 0. ), f_max_global_( 0. ),
    function_( std::unique_ptr<gsl_monte_function>( new gsl_monte_function ) )
  {
    //--- function to be integrated
    function_->f = f_;
    function_->dim = dim;
    function_->params = (void*)param;
    num_converg_ = param->vegas.ncvg;
    num_iter_ = param->vegas.itvg;

    //--- initialise the random number generator
    gsl_rng_env_setup();
    rng_ = gsl_rng_alloc( gsl_rng_default );
    gsl_rng_set( rng_, time( nullptr ) ); // seed with time

    Debugging( Form( "Number of integration dimensions: %d\n\t"
                     "Number of iterations:             %d\n\t"
                     "Number of function calls:         %d", dim, num_iter_, num_converg_ ) );
  }

  Vegas::~Vegas()
  {
    if ( rng_ ) gsl_rng_free( rng_ );
  }

  int
  Vegas::integrate( double& result, double& abserr )
  {
    //--- prepare Vegas
    gsl_monte_vegas_state* state = gsl_monte_vegas_alloc( function_->dim );

    //--- integration bounds
    std::vector<double> x_low( function_->dim, 0. ), x_up( function_->dim, 1. );

    //--- launch Vegas
    int veg_res;

    //----- warmup (prepare the grid)
    if ( !grid_prepared_ ) {
      veg_res = gsl_monte_vegas_integrate( function_.get(), &x_low[0], &x_up[0], function_->dim, 10000, rng_, state, &result, &abserr );
      grid_prepared_ = true;
    }
    //----- integration
    for ( unsigned int i=0; i<num_iter_; i++ ) {
      veg_res = gsl_monte_vegas_integrate( function_.get(), &x_low[0], &x_up[0], function_->dim, 0.2*num_converg_, rng_, state, &result, &abserr );
      PrintMessage( Form( ">> Iteration %2d: average = %10.6f   sigma = %10.6f   chi2 = %4.3f", i+1, result, abserr, gsl_monte_vegas_chisq( state ) ) );
    }

    //--- clean Vegas
    gsl_monte_vegas_free( state );

    return veg_res;
  }

  void
  Vegas::generate()
  {
    std::ofstream of;
    std::string fn;

    if ( !gen_prepared_ ) setGen();

    Information( Form( "%d events will be generated", input_params_->generation.maxgen ) );

    unsigned int i = 0;
    while ( i < input_params_->generation.maxgen ) {
      if ( generateOneEvent() ) i++;
    }
    Information( Form( "%d events generated", i ) );
  }

  bool
  Vegas::generateOneEvent()
  {
    if ( !gen_prepared_ ) setGen();

    const unsigned int ndim = function_->dim, max = pow( mbin_, ndim );

    std::vector<double> x( ndim, 0. );

    //--- correction cycles
    
    if ( vegas_bin_ != 0 ) {
      bool has_correction = false;
      while ( !correctionCycle( x, has_correction ) ) {}
      if ( has_correction ) return storeEvent( x );
    }

    double weight;
    double y = -1.;

    //--- normal generation cycle

    //----- select a Vegas bin and reject if fmax is too small
    do {
      do {
        // ...
        vegas_bin_ = uniform() * max;
        y = uniform() * f_max_global_;
        nm_[vegas_bin_] += 1;
      } while ( y > f_max_[vegas_bin_] );
      // Select x values in this Vegas bin
      int jj = vegas_bin_;
      for ( unsigned int i=0; i<ndim; i++ ) {
        int jjj = jj / mbin_;
        n_[i] = jj - jjj * mbin_;
        x[i] = ( uniform() + n_[i] ) / mbin_;
        jj = jjj;
      }

      // Get weight for selected x value
      weight = F( x );
    } while ( y > weight );

    if ( weight <= f_max_[vegas_bin_] ) vegas_bin_ = 0;
    // Init correction cycle if weight is higher than fmax or ffmax
    else if ( weight <= f_max_global_ ) {
      f_max_old_ = f_max_[vegas_bin_];
      f_max_[vegas_bin_] = weight;
      f_max_diff_ = weight-f_max_old_;
      correc_ = ( nm_[vegas_bin_] - 1. ) * f_max_diff_ / f_max_global_ - 1.;
    }
    else {
      f_max_old_ = f_max_[vegas_bin_];
      f_max_[vegas_bin_] = weight;
      f_max_diff_ = weight-f_max_old_;
      f_max_global_ = weight;
      correc_ = ( nm_[vegas_bin_] - 1. ) * f_max_diff_ / f_max_global_ * weight / f_max_global_ - 1.;
    }

    Debugging( Form( "Correction applied: %f, Vegas bin = %d", correc_, vegas_bin_ ) );

    // Return with an accepted event
    if ( weight > 0. ) return storeEvent( x );
    return false;
  }

  bool
  Vegas::correctionCycle( std::vector<double>& x, bool& has_correction )
  {
    double weight;
    const unsigned int ndim = function_->dim;

    Debugging( Form( "Correction cycles are started.\n\t"
                     "j = %f"
                     "correc = %f"
                     "corre2 = %f", vegas_bin_, correc2_ ) );

    if ( correc_ >= 1. ) correc_ -= 1.;
    if ( uniform() < correc_ ) {
      correc_ = -1.;
      std::vector<double> xtmp( ndim, 0. );
      // Select x values in Vegas bin
      for ( unsigned int k=0; k<ndim; k++ ) {
        xtmp[k] = ( uniform() + n_[k] ) * inv_mbin_;
      }
      // Compute weight for x value
      weight = F( xtmp );
      // Parameter for correction of correction
      if ( weight > f_max_[vegas_bin_] ) {
        if ( weight > f_max2_ ) f_max2_ = weight;
        correc2_ -= 1.;
        correc_ += 1.;
      }
      // Accept event
      if ( weight >= f_max_diff_*uniform() + f_max_old_ ) { // FIXME!!!!
        //Error("Accepting event!!!");
        //return storeEvent(x);
        x = xtmp;
        has_correction = true;
        return true;
      }
      return false;
    }
    // Correction if too big weight is found while correction
    // (All your bases are belong to us...)
    if ( f_max2_ > f_max_[vegas_bin_] ) {
      f_max_old_ = f_max_[vegas_bin_];
      f_max_[vegas_bin_] = f_max2_;
      f_max_diff_ = f_max2_-f_max_old_;
      if ( f_max2_ < f_max_global_ ) {
        correc_ = ( nm_[vegas_bin_] - 1. ) * f_max_diff_ / f_max_global_ - correc2_;
      }
      else {
        f_max_global_ = f_max2_;
        correc_ = ( nm_[vegas_bin_] - 1. ) * f_max_diff_ / f_max_global_ * f_max2_ / f_max_global_ - correc2_;
      }
      correc2_ = 0.;
      f_max2_ = 0.;
      return false;
    }
    return true;
  }

  bool
  Vegas::storeEvent( const std::vector<double>& x )
  {
    input_params_->setStorage( true );
    F( x );
    input_params_->generation.ngen += 1;
    input_params_->setStorage( false );
    if ( input_params_->generation.ngen % input_params_->generation.gen_print_every == 0 ) {
      Debugging( Form( "Generated events: %d", input_params_->generation.ngen ) );
      input_params_->generation.last_event->dump();
    }
    return true;
  }

  void
  Vegas::setGen()
  {
    Information( Form( "Preparing the grid for the generation of unweighted events: %d points", input_params_->vegas.npoints ) );
    // Variables for debugging
    std::ostringstream os;
    if ( Logger::get().level >= Logger::Debug ) {
      Debugging( Form( "MaxGen = %d", input_params_->generation.maxgen ) );
    }

    const unsigned int ndim = function_->dim,
                       max = pow( mbin_, ndim ),
                       npoin = input_params_->vegas.npoints;
    const double inv_npoin = 1./npoin;

    if ( ndim > max_dimensions_ ) {
      FatalError( Form( "Number of dimensions to integrate exceed the maximum number, %d", max_dimensions_ ) );
    }

    nm_ = std::vector<int>( max, 0 );
    f_max_ = std::vector<double>( max, 0. );
    n_ = std::vector<int>( ndim, 0 );

    std::vector<double> x( ndim, 0. );

    input_params_->generation.ngen = 0;

    // ...
    double sum = 0., sum2 = 0., sum2p = 0.;

    //--- main loop
    for ( unsigned int i=0; i<max; i++ ) {
      int jj = i;
      for ( unsigned int j=0; j<ndim; j++ ) {
        int jjj = jj*inv_mbin_;
        n_[j] = jj-jjj*mbin_;
        jj = jjj;
      }
      double fsum = 0., fsum2 = 0.;
      for ( unsigned int j=0; j<npoin; j++ ) {
        for ( unsigned int k=0; k<ndim; k++ ) {
          x[k] = ( uniform() + n_[k] ) * inv_mbin_;
        }
        const double z = F( x );
        f_max_[i] = std::max( f_max_[i], z );
        fsum += z;
        fsum2 += z*z;
      }
      const double av = fsum*inv_npoin, av2 = fsum2*inv_npoin, sig2 = av2 - av*av;
      sum += av;
      sum2 += av2;
      sum2p += sig2;
      f_max_global_ = std::max( f_max_global_, f_max_[i] );

      if ( Logger::get().level >= Logger::DebugInsideLoop ) {
        const double sig = sqrt( sig2 );
        const double eff = ( f_max_[i] != 0. ) ? f_max_[i]/av : 1.e4;
        os.str(""); for ( unsigned int j=0; j<ndim; j++ ) { os << n_[j]; if ( j != ndim-1 ) os << ", "; }
        DebuggingInsideLoop( Form( "In iteration #%d:\n\t"
                                   "av   = %f\n\t"
                                   "sig  = %f\n\t"
                                   "fmax = %f\n\t"
                                   "eff  = %f\n\t"
                                   "n = (%s)",
                                   i, av, sig, f_max_[i], eff, os.str().c_str() ) );
      }
    } // end of main loop

    sum = sum/max;
    sum2 = sum2/max;
    sum2p = sum2p/max;

    if ( Logger::get().level >= Logger::Debug ) {
      const double sig = sqrt( sum2-sum*sum ), sigp = sqrt( sum2p );

      double eff1 = 0.;
      for ( unsigned int i=0; i<max; i++ ) eff1 += ( f_max_[i] / ( max*sum ) );
      const double eff2 = f_max_global_/sum;

      Debugging( Form( "Average function value     =  sum   = %f\n\t"
                       "Average function value**2  =  sum2  = %f\n\t"
                       "Overall standard deviation =  sig   = %f\n\t"
                       "Average standard deviation =  sigp  = %f\n\t"
                       "Maximum function value     = ffmax  = %f\n\t"
                       "Average inefficiency       =  eff1  = %f\n\t"
                       "Overall inefficiency       =  eff2  = %f\n\t",
                       sum, sum2, sig, sigp, f_max_global_, eff1, eff2 ) );
    }
    gen_prepared_ = true;
    Information( "Grid prepared! Now launching the production." );
  }
}

