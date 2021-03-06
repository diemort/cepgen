#include "TestProcess.h"

using namespace CepGen::Process;

TestProcess::TestProcess() : GenericProcess( "test", ".oO TEST PROCESS Oo.", false )
{}

double
TestProcess::computeWeight()
{
  return 1./( 1.-cos( x( 0 )*M_PI )*cos( x( 1 )*M_PI )*cos( x( 2 )*M_PI ) );
}
