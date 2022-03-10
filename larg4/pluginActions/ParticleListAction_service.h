////////////////////////////////////////////////////////////////////////
/// \file  ParticleListAction_service.h
/// \brief Use Geant4's user "hooks" to maintain a list of particles generated by Geant4.
///
/// \author  seligman@nevis.columbia.edu
////////////////////////////////////////////////////////////////////////
//
// accumulate a list of particles modeled by Geant4.
//

// Include guard
#ifndef PARTICLELISTACTION_SERVICE_H
#define PARTICLELISTACTION_SERVICE_H


#include "art/Framework/Principal/Handle.h"
#include "art/Framework/Services/Registry/ServiceDeclarationMacros.h"
#include "canvas/Persistency/Common/Assns.h"
#include "messagefacility/MessageLogger/MessageLogger.h"

#include "nug4/ParticleNavigation/ParticleList.h"
#include "nusimdata/SimulationBase/MCParticle.h"
#include "nusimdata/SimulationBase/MCTruth.h"
#include "nusimdata/SimulationBase/simb.h" // simb::GeneratedParticleIndex_t

#include "artg4tk/actionBase/EventActionBase.hh"
#include "artg4tk/actionBase/SteppingActionBase.hh"
#include "artg4tk/actionBase/TrackingActionBase.hh"

#include "lardataobj/Simulation/GeneratedParticleInfo.h"

#include "Geant4/globals.hh"
#include <map>
#include <set>

// Forward declarations.
class G4Event;
class G4Track;
class G4Step;

namespace sim {
  class ParticleList;
}

namespace larg4 {

  class ParticleListActionService : public artg4tk::EventActionBase,
                                    public artg4tk::TrackingActionBase,
                                    public artg4tk::SteppingActionBase {
  public:
    // Standard constructors and destructors;
    explicit ParticleListActionService(fhicl::ParameterSet const&);

    // UserActions method that we'll override, to obtain access to
    // Geant4's particle tracks and trajectories.
    void preUserTrackingAction(const G4Track*) override;
    void postUserTrackingAction(const G4Track*) override;
    void userSteppingAction(const G4Step*) override;

    /// Returns the index of primary truth (`sim::NoGeneratorIndex` if none).
    simb::GeneratedParticleIndex_t GetPrimaryTruthIndex(int trackId) const;

    // Called at the beginning of each event (note that this is after the
    // primaries have been generated and sent to the event manager)
    void beginOfEventAction(const G4Event*) override;

    // Called at the end of each event, right before GEANT's state switches
    // out of event processing and into closed geometry (last chance to access
    // the current event).
    void endOfEventAction(const G4Event*) override;

    // Set/get the current Art event
    void
    setInputCollections(std::vector<art::Handle<std::vector<simb::MCTruth>>> const& mclists)
    {
      fMCLists = &mclists;
    }

    void
    setPtrInfo(art::ProductID pid,
               art::EDProductGetter const* productGetter)
    {
      pid_ = pid;
      productGetter_ = productGetter;
    }

    std::unique_ptr<std::vector<simb::MCParticle>>
    ParticleCollection()
    {
      return std::move(partCol_);
    }
    std::unique_ptr<std::set<int>>

    DroppedTracksCollection()
    {
      return std::move(droppedCol_);
    }

    std::unique_ptr<art::Assns<simb::MCTruth, simb::MCParticle, sim::GeneratedParticleInfo>>
    AssnsMCTruthToMCParticle()
    {
      return std::move(tpassn_);
    }

  private:
    struct ParticleInfo_t {
      simb::MCParticle* particle = nullptr; ///< simple structure representing particle
      bool keepFullTrajectory = false;      ///< if there was decision to keep

      /// Index of the particle in the original generator truth record.
      simb::GeneratedParticleIndex_t truthIndex = simb::NoGeneratedParticleIndex;

      /// Resets the information (does not release memory it does not own)
      void
      clear()
      {
        particle = nullptr;
        keepFullTrajectory = false;
        truthIndex = simb::NoGeneratedParticleIndex;
      }

      /// Returns whether there is a particle
      bool
      hasParticle() const
      {
        return particle;
      }

      /// Returns whether there is a particle
      bool
      isPrimary() const
      {
        return simb::isGeneratedParticleIndex(truthIndex);
      }

      /// Returns the index of the particle in the generator truth record.
      simb::GeneratedParticleIndex_t
      truthInfoIndex() const
      {
        return truthIndex;
      }

    }; // ParticleInfo_t

    // Yields the ParticleList accumulated during the current event.
    sim::ParticleList&& YieldList();

    // this method will loop over the fParentIDMap to get the
    // parentage of the provided trackid
    int GetParentage(int trackid) const;

    G4double fenergyCut;             ///< The minimum energy for a particle to
                                     ///< be included in the list.
    ParticleInfo_t fCurrentParticle; ///< information about the particle currently being simulated
                                     ///< for a single particle.
    sim::ParticleList fParticleList; ///< The accumulated particle information for
                                     ///< all particles in the event.
    G4bool fstoreTrajectories;       ///< Whether to store particle trajectories with each particle.
    std::vector<std::string>
      fkeepGenTrajectories;          ///< List of generators for which fstoreTrajectories applies.
                                     ///  if not provided and storeTrajectories is true, then all
                                     ///  trajectories for all generators will be stored. If
                                     ///  storeTrajectories is set to false, this list is ignored
                                     ///  and all additional trajectory points are not stored.
    std::map<int, int> fParentIDMap; ///< key is current track ID, value is parent ID
    int fCurrentTrackID;             ///< track ID of the current particle, set to eve ID
                                     ///< for EM shower particles
    mutable int fTrackIDOffset;      ///< offset added to track ids when running over
                                     ///< multiple MCTruth objects.
    bool fKeepEMShowerDaughters;     ///< whether to keep EM shower secondaries, tertiaries, etc
    std::vector<std::string> fNotStoredPhysics; ///< Physics processes that will not be stored
    bool fkeepOnlyPrimaryFullTraj; ///< Whether to store trajectories only for primaries and
                                   ///  their descendants with MCTruth process = "primary"
    bool fSparsifyTrajectories;    ///< help reduce the number of trajectory points.
    double fSparsifyMargin;        ///< set the sparsification margin
    bool fKeepTransportation;      ///< tell whether or not to keep the transportation process
    bool fKeepSecondToLast; ///< tell whether or not to force keeping the second to last point

    std::vector<art::Handle<std::vector<simb::MCTruth>>> const*
      fMCLists; ///< MCTruthCollection input lists

    /// Map: particle track ID -> index of primary information in MC truth.
    std::map<int, simb::GeneratedParticleIndex_t> fPrimaryTruthMap;

    /// Map: particle track ID -> index of primary parent in std::vector<simb::MCTruth> object
    std::map<int, size_t> fMCTIndexMap;

    /// Map: particle trakc ID -> boolean decision to keep or not full trajectory points
    std::map<int, bool> fMCTPrimProcessKeepMap;

    /// Map: MCTruthIndex -> generator, input label of generator and keepGenerator decision
    std::map<size_t, std::pair<std::string, G4bool>> fMCTIndexToGeneratorMap;

    /// Map: not stored process and counter
    std::unordered_map<std::string, int> fNotStoredCounterUMap;

    /// set: list of track ids for which no MCParticle was created 
    std::set<int> fdroppedTracksSet;

    std::unique_ptr<std::vector<simb::MCParticle>> partCol_;
    std::unique_ptr<std::set<int>> droppedCol_;
    std::unique_ptr<art::Assns<simb::MCTruth, simb::MCParticle, sim::GeneratedParticleInfo>>
      tpassn_;
    art::ProductID pid_{art::ProductID::invalid()};
    art::EDProductGetter const* productGetter_{nullptr};
    /// Adds a trajectory point to the current particle, and runs the filter
    void AddPointToCurrentParticle(TLorentzVector const& pos,
                                   TLorentzVector const& mom,
                                   std::string const& process);
  };

} // namespace larg4

DECLARE_ART_SERVICE(larg4::ParticleListActionService, LEGACY)

#endif // PARTICLELISTACTION_SERVICE_H
