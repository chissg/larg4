//
//=============================================================================
// SimEnergyDepositSD: Class representing a liquid Ar TPC
// Author: Hans Wenzel (Fermilab)
//=============================================================================

#ifndef LARG4_SERVICES_SIMENERGYDEPOSITSD_H
#define LARG4_SERVICES_SIMENERGYDEPOSITSD_H
#include "Geant4/G4VSensitiveDetector.hh"
#include "lardataobj/Simulation/SimEnergyDeposit.h"

class G4Step;
class G4HCofThisEvent;
//class SimEnergyDepositCollection;

//....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
namespace larg4 {

    class SimEnergyDepositSD : public G4VSensitiveDetector {
    public:
        SimEnergyDepositSD(G4String);
        ~SimEnergyDepositSD();
        void Initialize(G4HCofThisEvent*);
        G4bool ProcessHits(G4Step*, G4TouchableHistory*);
	const sim::SimEnergyDepositCollection& GetHits() const { return hitCollection; }
    private:
      sim::SimEnergyDepositCollection hitCollection;
    };

    //....oooOO0OOooo........oooOO0OOooo........oooOO0OOooo........oooOO0OOooo......
}

#endif //  LARG4_SERVICES_SIMENERGYDEPOSITSD_H
