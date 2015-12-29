#ifndef MCGen_h
#define MCGen_h

#include <fstream>
#include <sstream>
#include <string>
#include <ctime>

#include "Vegas.h"
#include "Physics.h"

////////////////////////////////////////////////////////////////////////////////

/**
 * @image latex lpair_logo.pdf
 * @mainpage Principles
 * This Monte Carlo generator, based on the LPAIR code developed in the
 * early 1990s by J. Vermaseren *et al*@cite Vermaseren1983347, allows to compute
 * the cross-section and to generate events for the \f$\gamma\gamma\to\ell^{+}\ell^{-}\f$
 * process in high energy physics.
 * 
 * The main operation is the integration of the matrix element (given as a 
 * subset of a Process object) performed by *Vegas*, an importance sampling
 * algorithm written in 1972 by G. P. Lepage@cite PeterLepage1978192. 
 *
 */

////////////////////////////////////////////////////////////////////////////////

/**
 * The function to be integrated, which returns the value of the weight of an
 * event, including the matrix element of the process, all the kinematic
 * factors, and the cut restrictions. \f$x\f$ is an array of random numbers used
 * to select a random point inside the phase space.
 */
double f(double*,size_t,void*);

////////////////////////////////////////////////////////////////////////////////

/**
 * This object represents the core of this Monte Carlo generator, with its
 * allowance to generate the events (using the embedded Vegas object) and to
 * study the phase space in term of the variation of resulting cross section
 * while scanning the various parameters (point \f$\textbf{x}\f$ in the
 * DIM-dimensional phase space).
 *
 * The phase space is constrained using the Parameters object given as an
 * argument to the constructor, and the differential cross-sections for each
 * value of the array \f$\textbf{x}\f$ are computed in the f-function defined
 * outside (but populated inside) this object.
 *
 * This f-function embeds a Process object which defines all the methods to
 * obtain this differential cross-section as well as the in- and outgoing
 * kinematics associated to each particle.
 *
 * @author Laurent Forthomme <laurent.forthomme@uclouvain.be>
 * @date February 2013
 * @brief Core of the Monte-Carlo generator
 *
 */
class MCGen {
 public:
  /// Core of the Monte Carlo integrator and events generator
  MCGen();
  /// Core of the Monte Carlo integrator and events generator
  /// \param[in] ip_ List of input parameters defining the phase space on which to perform the integration
  MCGen(Parameters *ip_);
  ~MCGen();
  /// Dump this program's header into the standard output stream
  void PrintHeader();
  /**
   * Compute the cross section for the run parameters defined by this object.
   * This returns the cross section as well as the absolute error computed along.
   * @brief Compute the cross-section for the given process
   * @param[out] xsec_ The computed cross-section, in pb
   * @param[out] err_ The absolute integration error on the computed cross-section, in pb
   */
  void ComputeXsection(double* xsec_,double* err_);
  /**
   * Generate one single event given the phase space computed by Vegas in the integration step
   * @return A pointer to the Event object generated in this run
   */
  Event* GenerateOneEvent();
  /// Number of dimensions on which the integration is performed
  inline size_t GetNdim() {
    if (!parameters->process) return 0;
    return parameters->process->GetNdim(parameters->process_mode);
  }
  inline double ComputePoint(double* x_) {
    PrepareFunction();
    double res = f(x_, GetNdim(), (void*)parameters);
    std::ostringstream os;
    for (unsigned int i=0; i<GetNdim(); i++) { os << x_[i] << " "; }
    Debug(Form("Result for x[%zu] = ( %s):\n\t%10.6f", GetNdim(), os.str().c_str(), res));
    return res;
  }
  /// Physical Parameters used in the events generation and cross-section computation
  Parameters* parameters;
  /// Last event generated in this run
  Event *last_event;
 private:
  void PrepareFunction();
  /// Call the Vegas constructor (once, just before the first integration attempt)
  void BuildVegas();
  /// Vegas instance which will integrate the function
  Vegas *fVegas;
  /// Cross section value computed at the last integration
  double fCrossSection;
  /// Error on the cross section as computed in the last integration
  double fCrossSectionError;
  /// Has a first integration beed already performed?
  bool fHasCrossSection;
};

#endif
