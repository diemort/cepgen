#include "herwig6hadroniser.h"

Herwig6Hadroniser::Herwig6Hadroniser()
{
  std::cout << "[Herwig6Hadroniser::Herwig6Hadroniser] [DEBUG] Constructor called" << std::endl;
  _name = "Herwig6";
}

Herwig6Hadroniser::~Herwig6Hadroniser()
{
#ifdef DEBUG
  std::cout << "[Herwig6Hadroniser::~Herwig6Hadroniser] [DEBUG] Destructor called" << std::endl;
#endif
}

bool
Herwig6Hadroniser::Hadronise(Event *ev_)
{
  int i;
  ParticlesRef pp;
  ParticlesRef::iterator p;

  std::cout << "[Herwig6Hadroniser::Hadronise]" << std::endl;
  ev_->Dump();
  
  pp = ev_->GetParticles();
  for (p=pp.begin(), i=0; p!=pp.end() && i<NMXHEP; p++, i++) {
    if ((*p)->status==3) (*p)->status = 193; //FIXME workaround for cluster fragmentation
  
    hepevt_.idhep[i] = (*p)->pdgId;
    hepevt_.isthep[i] = (*p)->status;
    hepevt_.phep[i][0] = (*p)->Px();
    hepevt_.phep[i][1] = (*p)->Py();
    hepevt_.phep[i][2] = (*p)->Pz();
    hepevt_.phep[i][3] = (*p)->E();
    hepevt_.phep[i][4] = (*p)->M();
    for (int j=0; j<4; j++) {
      hepevt_.vhep[i][j] = 0.; // Particle produced at the detector center
    }
    std::cout << "(" << i << ")--> " << (*p)->pdgId << std::endl;
  }
  hepevt_.nhep = i;
  
  this->hwdhad();
  
  std::cout << "after hadronisation" << std::endl;
  for (i=0; i<hepevt_.nhep; i++) {
    std::cout << "--> " << hepevt_.idhep[i] << std::endl;
  }
  
  return true;
}
