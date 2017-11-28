#include "LHEFHandler.h"

#ifdef HEPMC_VERSION3

#include "CepGen/Version.h"

namespace CepGen
{
  namespace OutputHandler
  {
    LHEFHandler::LHEFHandler( const char* filename ) :
      ExportHandler( ExportHandler::LHE ),
      lhe_output_( new LHEF::Writer( filename ) )
    {}

    void
    LHEFHandler::initialise( const Parameters& params )
    {
      lhe_output_->headerBlock()
        << "<!--\n"
        << "***** Sample generated with CepGen v" << version() << " *****\n"
        << "* process: " << params.processName() << " (" << params.kinematics.mode << ")\n"
        << "* structure functions: " << params.remnant_mode << "\n"
        << "*--- incoming state\n"
        << "* Q² range (GeV²): " << params.kinematics.q2_min << " - " << params.kinematics.q2_max << "\n"
        << "* remnants mass range (GeV): " << params.kinematics.mx_min << " - " << params.kinematics.mx_max << "\n"
        << "*--- central system\n"
        << "* single particle pT (GeV): " << params.kinematics.pt_min << " - " << params.kinematics.pt_max << "\n"
        << "* single particle energy (GeV): " << params.kinematics.e_min << " - " << params.kinematics.e_max << "\n"
        << "* single particle eta: " << params.kinematics.eta_min << " - " << params.kinematics.eta_max << "\n"
        << "**************************************************\n"
        << "-->";
      //params.dump( lhe_output_->initComments(), false );
      LHEF::HEPRUP run = lhe_output_->heprup;
      run.IDBMUP = { params.kinematics.in1pdg, params.kinematics.in2pdg };
      run.EBMUP = { params.kinematics.in1p, params.kinematics.in2p };
      run.NPRUP = 1;
      run.resize();
      run.XSECUP[0] = cross_sect_;
      run.XERRUP[0] = cross_sect_err_;
      run.XMAXUP[0] = 1.;
      run.LPRUP[0] = 1;
      lhe_output_->heprup = run;
      lhe_output_->init();
    }

    void
    LHEFHandler::operator<<( const Event* ev )
    {
      LHEF::HEPEUP out;
      out.heprup = &lhe_output_->heprup;
      out.XWGTUP = 1.;
      out.XPDWUP = std::pair<double,double>( 0., 0. );
      out.SCALUP = 0.;
      out.AQEDUP = Constants::alphaEM;
      out.AQCDUP = Constants::alphaQCD;
      out.NUP = ev->numParticles();
      out.resize();
      for ( unsigned short ip=0; ip<ev->numParticles(); ip++ ) {
        const Particle part = ev->getConstById( ip );
        out.IDUP[ip] = part.integerPdgId(); // PDG id
        out.ISTUP[ip] = part.status(); // status code
        out.MOTHUP[ip] = std::pair<int,int>( ( part.mothersIds().size() > 0 ) ? *part.mothersIds().begin()+1 : 0, ( part.mothersIds().size() > 1 ) ? *part.mothersIds().rbegin()+1 : 0 ); // mothers
        out.ICOLUP[ip] = std::pair<int,int>( 0, 0 );
        out.PUP[ip] = std::vector<double>( { { part.momentum().px(), part.momentum().py(), part.momentum().pz(), part.energy(), part.mass() } } ); // momentum
        out.VTIMUP[ip] = 0.; // invariant lifetime
        out.SPINUP[ip] = 0.;
      }
      lhe_output_->eventComments() << "haha";
      lhe_output_->hepeup = out;
      lhe_output_->writeEvent();
    }
  }
}

#endif
