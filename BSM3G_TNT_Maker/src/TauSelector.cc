#include "BSMFramework/BSM3G_TNT_Maker/interface/TauSelector.h"
TauSelector::TauSelector(std::string name, TTree* tree, bool debug, const pset& iConfig):
  baseTree(name,tree,debug){
  tauToken_       	  = iConfig.getParameter<edm::InputTag>("taus");
  packedPFCandidateToken_ = iConfig.getParameter<edm::InputTag>("packedPFCandidates");
  _vertexInputTag 	  = iConfig.getParameter<edm::InputTag>("vertices");
  _beamSpot               = iConfig.getParameter<edm::InputTag>("beamSpot");
  _Tau_pt_min     	  = iConfig.getParameter<double>("Tau_pt_min");
  _Tau_eta_max    	  = iConfig.getParameter<double>("Tau_eta_max");
  _Tau_vtx_ndof_min       = iConfig.getParameter<int>("Tau_vtx_ndof_min");
  _Tau_vtx_rho_max        = iConfig.getParameter<int>("Tau_vtx_rho_max");
  _Tau_vtx_position_z_max = iConfig.getParameter<double>("Tau_vtx_position_z_max");
  _super_TNT      	  = iConfig.getParameter<bool>("super_TNT");
  _MiniAODv2      	  = iConfig.getParameter<bool>("MiniAODv2");
  SetBranches();
}
TauSelector::~TauSelector(){
  delete tree_;
}
void TauSelector::Fill(const edm::Event& iEvent, const edm::EventSetup& iSetup){
  Clear();
  /////
  //   Recall collections
  /////
  edm::Handle<edm::View<pat::Tau> > taus;
  iEvent.getByLabel(tauToken_, taus);
  edm::Handle<pat::PackedCandidateCollection> pfs;
  iEvent.getByLabel(packedPFCandidateToken_, pfs);
  reco::BeamSpot beamSpot;
  edm::Handle<reco::BeamSpot> beamSpotHandle;
  iEvent.getByLabel(_beamSpot, beamSpotHandle);
  edm::ESHandle<TransientTrackBuilder> theB;
  iSetup.get<TransientTrackRecord>().get("TransientTrackBuilder",theB);
  edm::Handle<reco::VertexCollection> vtx_h;
  iEvent.getByLabel(_vertexInputTag, vtx_h);
  /////
  //   Require a good vertex 
  /////
  //reco::VertexCollection::const_iterator firstGoodVertex = vtx->end();
  //for(reco::VertexCollection::const_iterator it = vtx->begin(); it != vtx->end(); it++){
  //  if(isGoodVertex(*it)){
  //    firstGoodVertex = it;
  //    break;
  //  }
  //}
  //if(firstGoodVertex == vtx->end()) return; // skip event if there are no good PVs
  if(vtx_h->empty()) return; // skip the event if no PV found
  const reco::Vertex &firstGoodVertex = vtx_h->front();
  bool isgoodvtx = isGoodVertex(firstGoodVertex);
  if(!isgoodvtx) return;
  GlobalPoint thepv(firstGoodVertex.position().x(),firstGoodVertex.position().y(),firstGoodVertex.position().z());
  /////
  //   Get tau information 
  /////
  for(edm::View<pat::Tau>::const_iterator tau = taus->begin(); tau != taus->end(); tau++){
    //Acceptance 
    if(tau->pt() < _Tau_pt_min) continue;
    if(fabs(tau->eta()) > _Tau_eta_max) continue;
    //if(!(tau->leadChargedHadrCand().isNonnull())) continue;
    //Kinematic
    Tau_pt.push_back(tau->pt());
    Tau_eta.push_back(tau->eta());
    Tau_phi.push_back(tau->phi());
    Tau_energy.push_back(tau->energy());
    const reco::Track *leadTrack = 0;
    bool isBestTrackNonNull = false;
    bool leadPackedCandidateExists = false;
    if(tau->leadChargedHadrCand().isNonnull()){
      const reco::CandidatePtr hadTauLeadChargedCand = tau->leadChargedHadrCand();                                                                   
      Tau_leadChargedCandPt.push_back(hadTauLeadChargedCand.isNonnull()  ? hadTauLeadChargedCand->pt()     : -999);      
      Tau_leadChargedCandEta.push_back(hadTauLeadChargedCand.isNonnull() ? hadTauLeadChargedCand->eta()    : -999);
      Tau_leadChargedCandPhi.push_back(hadTauLeadChargedCand.isNonnull() ? hadTauLeadChargedCand->phi()    : -999);
      Tau_leadChargedCandE.push_back(hadTauLeadChargedCand.isNonnull()   ? hadTauLeadChargedCand->energy() : -999);
      //loop over packed PF candidates and find the one which matches the embedded packed candidate within the pat::Tau
      for(unsigned int iPF = 0, nPF = pfs->size(); iPF < nPF; ++iPF){
        const pat::PackedCandidate &pfCandidate = (*pfs)[iPF];
        if((hadTauLeadChargedCand->pt()  == pfCandidate.pt())  &&
           (hadTauLeadChargedCand->eta() == pfCandidate.eta()) && 
           (hadTauLeadChargedCand->phi() == pfCandidate.phi())){ // the packed PF candidate and embedded lead candidate within the pat::Tau should be the same
          leadPackedCandidateExists = true; // if there is a match between the packed PF candidate and embedded lead candidate within the pat::Tau
          if(pfCandidate.bestTrack() != NULL){isBestTrackNonNull = true; leadTrack = pfCandidate.bestTrack();} // grab the associated CTF track (if it exists)
        }
        if(leadPackedCandidateExists && isBestTrackNonNull) break;
      }
      //if(!(leadPackedCandidateExists)) continue; // throw away the tau if there was no matching packed PF candidate to the embedded lead candidate within the pat::Tau
      //if(!(isBestTrackNonNull)) continue; // throw away the tau if it's lead charged hadron has no associated CTF track
      if(isBestTrackNonNull && leadPackedCandidateExists){  
        Tau_leadChargedCandTrack_pt.push_back(leadTrack->pt());
        Tau_leadChargedCandTrack_ptError.push_back(leadTrack->ptError());
      }else{
        Tau_leadChargedCandTrack_pt.push_back(-998);
        Tau_leadChargedCandTrack_ptError.push_back(-998);
      }
    }else{
      Tau_leadChargedCandPt.push_back(-999);  
      Tau_leadChargedCandEta.push_back(-999);  
      Tau_leadChargedCandPhi.push_back(-999);  
      Tau_leadChargedCandE.push_back(-999);  
      Tau_leadChargedCandTrack_pt.push_back(-999);  
      Tau_leadChargedCandTrack_ptError.push_back(-999);  
    }
    //Charge
    Tau_charge.push_back(tau->charge());
    Tau_leadChargedCandCharge.push_back(tau->leadChargedHadrCand().isNonnull() ? tau->leadChargedHadrCand()->charge() : -999);   
    //Decay mode finding
    //Tau_decayModeFindingOldDMs.push_back(tau->tauID("decayModeFindingOldDMs"));
    Tau_decayModeFinding.push_back(tau->tauID("decayModeFinding"));
    Tau_decayModeFindingNewDMs.push_back(tau->tauID("decayModeFindingNewDMs"));
    //Against Muon
    if(!_MiniAODv2){
      Tau_againstMuonLoose2.push_back(tau->tauID("againstMuonLoose2"));
      Tau_againstMuonTight2.push_back(tau->tauID("againstMuonTight2"));
    }
    Tau_againstMuonLoose3.push_back(tau->tauID("againstMuonLoose3"));
    Tau_againstMuonTight3.push_back(tau->tauID("againstMuonTight3"));
    //Against Electron
    if(!_MiniAODv2){
      Tau_againstElectronLoose.push_back(tau->tauID("againstElectronLoose"));
      Tau_againstElectronMedium.push_back(tau->tauID("againstElectronMedium"));
      Tau_againstElectronTight.push_back(tau->tauID("againstElectronTight"));
    }
    Tau_againstElectronVLooseMVA5.push_back(tau->tauID("againstElectronVLooseMVA5"));
    Tau_againstElectronLooseMVA5.push_back(tau->tauID("againstElectronLooseMVA5"));
    Tau_againstElectronMediumMVA5.push_back(tau->tauID("againstElectronMediumMVA5"));
    Tau_againstElectronTightMVA5.push_back(tau->tauID("againstElectronTightMVA5"));
    Tau_againstElectronVTightMVA5.push_back(tau->tauID("againstElectronVTightMVA5"));
    Tau_againstElectronMVA5category.push_back(tau->tauID("againstElectronMVA5category"));
    Tau_againstElectronMVA5raw.push_back(tau->tauID("againstElectronMVA5raw"));
    //Isolation
    if(!_MiniAODv2){
      Tau_byLooseIsolationMVA3newDMwoLT.push_back(tau->tauID("byLooseIsolationMVA3newDMwoLT"));
      Tau_byLooseIsolationMVA3oldDMwoLT.push_back(tau->tauID("byLooseIsolationMVA3oldDMwoLT"));
      Tau_byMediumIsolationMVA3newDMwoLT.push_back(tau->tauID("byMediumIsolationMVA3newDMwoLT"));
      Tau_byMediumIsolationMVA3oldDMwoLT.push_back(tau->tauID("byMediumIsolationMVA3oldDMwoLT"));
      Tau_byTightIsolationMVA3newDMwoLT.push_back(tau->tauID("byTightIsolationMVA3newDMwoLT"));
      Tau_byTightIsolationMVA3oldDMwoLT.push_back(tau->tauID("byTightIsolationMVA3oldDMwoLT"));
    }
    Tau_byVLooseIsolationMVA3newDMwLT.push_back(tau->tauID("byVLooseIsolationMVA3newDMwLT"));
    Tau_byVLooseIsolationMVA3oldDMwLT.push_back(tau->tauID("byVLooseIsolationMVA3oldDMwLT"));
    Tau_byLooseCombinedIsolationDeltaBetaCorr3Hits.push_back(tau->tauID("byLooseCombinedIsolationDeltaBetaCorr3Hits"));
    Tau_byLooseIsolationMVA3newDMwLT.push_back(tau->tauID("byLooseIsolationMVA3newDMwLT"));
    Tau_byLooseIsolationMVA3oldDMwLT.push_back(tau->tauID("byLooseIsolationMVA3oldDMwLT"));
    Tau_byMediumCombinedIsolationDeltaBetaCorr3Hits.push_back(tau->tauID("byMediumCombinedIsolationDeltaBetaCorr3Hits"));
    Tau_byMediumIsolationMVA3newDMwLT.push_back(tau->tauID("byMediumIsolationMVA3newDMwLT"));
    Tau_byMediumIsolationMVA3oldDMwLT.push_back(tau->tauID("byMediumIsolationMVA3oldDMwLT"));
    Tau_byTightCombinedIsolationDeltaBetaCorr3Hits.push_back(tau->tauID("byTightCombinedIsolationDeltaBetaCorr3Hits"));
    Tau_byTightIsolationMVA3newDMwLT.push_back(tau->tauID("byTightIsolationMVA3newDMwLT"));
    Tau_byTightIsolationMVA3oldDMwLT.push_back(tau->tauID("byTightIsolationMVA3oldDMwLT"));
    Tau_byVTightIsolationMVA3newDMwLT.push_back(tau->tauID("byVTightIsolationMVA3newDMwLT"));
    Tau_byVTightIsolationMVA3oldDMwLT.push_back(tau->tauID("byVTightIsolationMVA3oldDMwLT"));
    Tau_byVVTightIsolationMVA3newDMwLT.push_back(tau->tauID("byVVTightIsolationMVA3newDMwLT"));
    Tau_byVVTightIsolationMVA3oldDMwLT.push_back(tau->tauID("byVVTightIsolationMVA3oldDMwLT"));
    Tau_byCombinedIsolationDeltaBetaCorrRaw3Hits.push_back(tau->tauID("byCombinedIsolationDeltaBetaCorrRaw3Hits"));
    Tau_byIsolationMVA3newDMwLTraw.push_back(tau->tauID("byIsolationMVA3newDMwLTraw"));
    Tau_byIsolationMVA3oldDMwLTraw.push_back(tau->tauID("byIsolationMVA3oldDMwLTraw"));
    Tau_chargedIsoPtSum.push_back(tau->tauID("chargedIsoPtSum"));
    Tau_neutralIsoPtSum.push_back(tau->tauID("neutralIsoPtSum"));
    Tau_puCorrPtSum.push_back(tau->tauID("puCorrPtSum"));
    if(_MiniAODv2){
      Tau_byLoosePileupWeightedIsolation3Hits.push_back(tau->tauID("byLoosePileupWeightedIsolation3Hits"));
      Tau_byMediumPileupWeightedIsolation3Hits.push_back(tau->tauID("byMediumPileupWeightedIsolation3Hits"));
      Tau_byTightPileupWeightedIsolation3Hits.push_back(tau->tauID("byTightPileupWeightedIsolation3Hits"));
      Tau_byPhotonPtSumOutsideSignalCone.push_back(tau->tauID("byPhotonPtSumOutsideSignalCone"));
      Tau_byPileupWeightedIsolationRaw3Hits.push_back(tau->tauID("byPileupWeightedIsolationRaw3Hits"));
      Tau_footprintCorrection.push_back(tau->tauID("footprintCorrection"));
      Tau_neutralIsoPtSumWeight.push_back(tau->tauID("neutralIsoPtSumWeight"));
      Tau_photonPtSumOutsideSignalCone.push_back(tau->tauID("photonPtSumOutsideSignalCone"));
    }
    //Other prop and Track related variables
    Tau_nProngs.push_back(tau->signalChargedHadrCands().size());
    if(isBestTrackNonNull && leadPackedCandidateExists){
      Tau_leadChargedCandNdof.push_back(leadTrack->ndof());
      Tau_leadChargedCandChi2.push_back(leadTrack->chi2());
      Tau_leadChargedCandValidHits.push_back(leadTrack->numberOfValidHits());
    }else{
      Tau_leadChargedCandNdof.push_back(-998);
      Tau_leadChargedCandChi2.push_back(-998);
      Tau_leadChargedCandValidHits.push_back(-998);
    }
    //IP
    //default tau POG lifetime variables
    Tau_defaultDxy.push_back(tau->dxy());
    Tau_defaultDxyError.push_back(tau->dxy_error());
    Tau_defaultDxySig.push_back(tau->dxy_Sig());
    Tau_defaultFlightLengthX.push_back(tau->flightLength().x());
    Tau_defaultFlightLengthY.push_back(tau->flightLength().y());
    Tau_defaultFlightLengthZ.push_back(tau->flightLength().z());
    Tau_defaultFlightLengthSig.push_back(tau->flightLengthSig());
    Tau_default_PCAx_pv.push_back(tau->dxy_PCA().x());
    Tau_default_PCAy_pv.push_back(tau->dxy_PCA().y());
    Tau_default_PCAz_pv.push_back(tau->dxy_PCA().z());
    //tau lead track point of closest approach (PCA) to the beamspot and primary vertex
    //AJ vars
    if(beamSpotHandle.isValid() && isBestTrackNonNull && leadPackedCandidateExists){
      TransientTrack tauTransTkPtr = theB->build(leadTrack);
      beamSpot = *beamSpotHandle;
      math::XYZPoint point(beamSpot.x0(),beamSpot.y0(), beamSpot.z0());
      GlobalPoint thebs(beamSpot.x0(),beamSpot.y0(),beamSpot.z0());
      GlobalPoint tauLeadTrack_pca_bs = tauTransTkPtr.trajectoryStateClosestToPoint(thebs).position();
      GlobalPoint tauLeadTrack_pca_pv = tauTransTkPtr.trajectoryStateClosestToPoint(thepv).position();
      Tau_leadChargedCandDz_pv.push_back(leadTrack->dz(firstGoodVertex.position()));
      Tau_leadChargedCandDxy_pv.push_back(leadTrack->dxy(firstGoodVertex.position()));
      Tau_leadChargedCandDz_bs.push_back(leadTrack->dz(point));
      Tau_leadChargedCandDxy_bs.push_back(-1.*(leadTrack->dxy(point)));
      Tau_leadChargedCandDzError.push_back(leadTrack->dzError());
      Tau_leadChargedCandDxyError.push_back(leadTrack->d0Error());
      Tau_leadChargedCandVtx.push_back(leadTrack->vx());
      Tau_leadChargedCandVty.push_back(leadTrack->vy());
      Tau_leadChargedCandVtz.push_back(leadTrack->vz());
      Tau_leadChargedCandTrack_PCAx_bs.push_back(tauLeadTrack_pca_bs.x());
      Tau_leadChargedCandTrack_PCAy_bs.push_back(tauLeadTrack_pca_bs.y());
      Tau_leadChargedCandTrack_PCAz_bs.push_back(tauLeadTrack_pca_bs.z());
      Tau_leadChargedCandTrack_PCAx_pv.push_back(tauLeadTrack_pca_pv.x());
      Tau_leadChargedCandTrack_PCAy_pv.push_back(tauLeadTrack_pca_pv.y());
      Tau_leadChargedCandTrack_PCAz_pv.push_back(tauLeadTrack_pca_pv.z());
      const float pionMass = 0.139570;
      float pionSigma = pionMass*1e-6;
      float chi2 = 0.0;
      float ndf = 0.0;
      KinematicParticleFactoryFromTransientTrack pFactory;
      RefCountedKinematicParticle tauParticle = pFactory.particle(tauTransTkPtr, pionMass, chi2, ndf, pionSigma);
      Tau_leadChargedCandTrackFitErrorMatrix_00.push_back(tauParticle->stateAtPoint(tauLeadTrack_pca_bs).kinematicParametersError().matrix()(0,0));
      Tau_leadChargedCandTrackFitErrorMatrix_01.push_back(tauParticle->stateAtPoint(tauLeadTrack_pca_bs).kinematicParametersError().matrix()(0,1));
      Tau_leadChargedCandTrackFitErrorMatrix_02.push_back(tauParticle->stateAtPoint(tauLeadTrack_pca_bs).kinematicParametersError().matrix()(0,2));
      Tau_leadChargedCandTrackFitErrorMatrix_11.push_back(tauParticle->stateAtPoint(tauLeadTrack_pca_bs).kinematicParametersError().matrix()(1,1));
      Tau_leadChargedCandTrackFitErrorMatrix_12.push_back(tauParticle->stateAtPoint(tauLeadTrack_pca_bs).kinematicParametersError().matrix()(1,2));
      Tau_leadChargedCandTrackFitErrorMatrix_22.push_back(tauParticle->stateAtPoint(tauLeadTrack_pca_bs).kinematicParametersError().matrix()(2,2));
    }else{
      edm::LogInfo("MyAnalyzer") << "No beam spot available from EventSetup \n";
      Tau_leadChargedCandDz_pv.push_back(-999);
      Tau_leadChargedCandDxy_pv.push_back(-999);
      Tau_leadChargedCandDz_bs.push_back(-999);
      Tau_leadChargedCandDxy_bs.push_back(-999);
      Tau_leadChargedCandDzError.push_back(-999);
      Tau_leadChargedCandDxyError.push_back(-999);
      Tau_leadChargedCandVtx.push_back(-999);
      Tau_leadChargedCandVty.push_back(-999);
      Tau_leadChargedCandVtz.push_back(-999);
      Tau_leadChargedCandTrack_PCAx_bs.push_back(-999);
      Tau_leadChargedCandTrack_PCAy_bs.push_back(-999);
      Tau_leadChargedCandTrack_PCAz_bs.push_back(-999);
      Tau_leadChargedCandTrack_PCAx_pv.push_back(-999);
      Tau_leadChargedCandTrack_PCAy_pv.push_back(-999);
      Tau_leadChargedCandTrack_PCAz_pv.push_back(-999);
      Tau_leadChargedCandTrackFitErrorMatrix_00.push_back(-999);
      Tau_leadChargedCandTrackFitErrorMatrix_01.push_back(-999);
      Tau_leadChargedCandTrackFitErrorMatrix_02.push_back(-999);
      Tau_leadChargedCandTrackFitErrorMatrix_11.push_back(-999);
      Tau_leadChargedCandTrackFitErrorMatrix_12.push_back(-999);
      Tau_leadChargedCandTrackFitErrorMatrix_22.push_back(-999);
    }
  }
}

void TauSelector::SetBranches(){
  if(debug_) std::cout<<"setting branches: calling AddBranch of baseTree"<<std::endl;
  //Kinematic
  AddBranch(&Tau_pt                           ,"Tau_pt");
  AddBranch(&Tau_eta                          ,"Tau_eta");
  AddBranch(&Tau_phi                          ,"Tau_phi");
  AddBranch(&Tau_energy                       ,"Tau_energy");
  AddBranch(&Tau_leadChargedCandPt            ,"Tau_leadChargedCandPt");
  AddBranch(&Tau_leadChargedCandEta           ,"Tau_leadChargedCandEta");
  AddBranch(&Tau_leadChargedCandPhi           ,"Tau_leadChargedCandPhi");
  AddBranch(&Tau_leadChargedCandE             ,"Tau_leadChargedCandE");
  AddBranch(&Tau_leadChargedCandTrack_pt      ,"Tau_leadChargedCandTrack_pt");
  AddBranch(&Tau_leadChargedCandTrack_ptError ,"Tau_leadChargedCandTrack_ptError");
  //Charge
  AddBranch(&Tau_charge                ,"Tau_charge");
  AddBranch(&Tau_leadChargedCandCharge ,"Tau_leadChargedCandCharge");
  //Decay mode finding
  //AddBranch(&Tau_decayModeFindingOldDMs ,"Tau_decayModeFindingOldDMs");
  AddBranch(&Tau_decayModeFinding       ,"Tau_decayModeFinding");
  AddBranch(&Tau_decayModeFindingNewDMs ,"Tau_decayModeFindingNewDMs");
  //Against Muon
  if(!_MiniAODv2){
    AddBranch(&Tau_againstMuonLoose2 ,"Tau_againstMuonLoose2");
    AddBranch(&Tau_againstMuonTight2 ,"Tau_againstMuonTight2");
  }
  AddBranch(&Tau_againstMuonLoose3 ,"Tau_againstMuonLoose3");
  AddBranch(&Tau_againstMuonTight3 ,"Tau_againstMuonTight3");
  //Against Electron
  if(!_MiniAODv2){
    AddBranch(&Tau_againstElectronLoose      ,"Tau_againstElectronLoose");
    AddBranch(&Tau_againstElectronMedium     ,"Tau_againstElectronMedium");
    AddBranch(&Tau_againstElectronTight      ,"Tau_againstElectronTight");
  }
  AddBranch(&Tau_againstElectronVLooseMVA5 ,"Tau_againstElectronVLooseMVA5");
  AddBranch(&Tau_againstElectronLooseMVA5  ,"Tau_againstElectronLooseMVA5");
  AddBranch(&Tau_againstElectronMediumMVA5 ,"Tau_againstElectronMediumMVA5");
  AddBranch(&Tau_againstElectronTightMVA5  ,"Tau_againstElectronTightMVA5");
  //Isolation
  //MiniAODv1
  if(!_MiniAODv2){
    AddBranch(&Tau_byLooseIsolationMVA3newDMwoLT  ,"Tau_byLooseIsolationMVA3newDMwoLT");
    AddBranch(&Tau_byLooseIsolationMVA3oldDMwoLT  ,"Tau_byLooseIsolationMVA3oldDMwoLT");
    AddBranch(&Tau_byMediumIsolationMVA3newDMwoLT ,"Tau_byMediumIsolationMVA3newDMwoLT");
    AddBranch(&Tau_byMediumIsolationMVA3oldDMwoLT ,"Tau_byMediumIsolationMVA3oldDMwoLT");
    AddBranch(&Tau_byTightIsolationMVA3newDMwoLT  ,"Tau_byTightIsolationMVA3newDMwoLT");
    AddBranch(&Tau_byTightIsolationMVA3oldDMwoLT  ,"Tau_byTightIsolationMVA3oldDMwoLT");
  }
  //MiniADOv1v2
  AddBranch(&Tau_byVLooseIsolationMVA3newDMwLT               ,"Tau_byVLooseIsolationMVA3newDMwLT");
  AddBranch(&Tau_byVLooseIsolationMVA3oldDMwLT               ,"Tau_byVLooseIsolationMVA3oldDMwLT");
  AddBranch(&Tau_byLooseCombinedIsolationDeltaBetaCorr3Hits  ,"Tau_byLooseCombinedIsolationDeltaBetaCorr3Hits");
  AddBranch(&Tau_byLooseIsolationMVA3newDMwLT                ,"Tau_byLooseIsolationMVA3newDMwLT");
  AddBranch(&Tau_byLooseIsolationMVA3oldDMwLT                ,"Tau_byLooseIsolationMVA3oldDMwLT");
  AddBranch(&Tau_byMediumCombinedIsolationDeltaBetaCorr3Hits ,"Tau_byMediumCombinedIsolationDeltaBetaCorr3Hits");
  AddBranch(&Tau_byMediumIsolationMVA3newDMwLT               ,"Tau_byMediumIsolationMVA3newDMwLT");
  AddBranch(&Tau_byMediumIsolationMVA3oldDMwLT               ,"Tau_byMediumIsolationMVA3oldDMwLT");
  AddBranch(&Tau_byTightCombinedIsolationDeltaBetaCorr3Hits  ,"Tau_byTightCombinedIsolationDeltaBetaCorr3Hits");
  AddBranch(&Tau_byTightIsolationMVA3newDMwLT                ,"Tau_byTightIsolationMVA3newDMwLT");
  AddBranch(&Tau_byTightIsolationMVA3oldDMwLT                ,"Tau_byTightIsolationMVA3oldDMwLT");
  AddBranch(&Tau_byVTightIsolationMVA3newDMwLT               ,"Tau_byVTightIsolationMVA3newDMwLT");
  AddBranch(&Tau_byVTightIsolationMVA3oldDMwLT               ,"Tau_byVTightIsolationMVA3oldDMwLT");
  AddBranch(&Tau_byVVTightIsolationMVA3newDMwLT              ,"Tau_byVVTightIsolationMVA3newDMwLT");
  AddBranch(&Tau_byVVTightIsolationMVA3oldDMwLT              ,"Tau_byVVTightIsolationMVA3oldDMwLT");
  AddBranch(&Tau_byCombinedIsolationDeltaBetaCorrRaw3Hits    ,"Tau_byCombinedIsolationDeltaBetaCorrRaw3Hits");
  AddBranch(&Tau_byIsolationMVA3newDMwLTraw                  ,"Tau_byIsolationMVA3newDMwLTraw");
  AddBranch(&Tau_byIsolationMVA3oldDMwLTraw                  ,"Tau_byIsolationMVA3oldDMwLTraw");
  AddBranch(&Tau_chargedIsoPtSum                             ,"Tau_chargedIsoPtSum");
  AddBranch(&Tau_neutralIsoPtSum                             ,"Tau_neutralIsoPtSum");
  AddBranch(&Tau_puCorrPtSum                                 ,"Tau_puCorrPtSum");
  //MiniAODv2
  if(!_MiniAODv2){
    AddBranch(&Tau_byLoosePileupWeightedIsolation3Hits  ,"Tau_byLoosePileupWeightedIsolation3Hits");
    AddBranch(&Tau_byMediumPileupWeightedIsolation3Hits ,"Tau_byMediumPileupWeightedIsolation3Hits");
    AddBranch(&Tau_byTightPileupWeightedIsolation3Hits  ,"Tau_byTightPileupWeightedIsolation3Hits");
    AddBranch(&Tau_byPhotonPtSumOutsideSignalCone       ,"Tau_byPhotonPtSumOutsideSignalCone");
    AddBranch(&Tau_byPileupWeightedIsolationRaw3Hits    ,"Tau_byPileupWeightedIsolationRaw3Hits");
    AddBranch(&Tau_footprintCorrection                  ,"Tau_footprintCorrection");
    AddBranch(&Tau_neutralIsoPtSumWeight                ,"Tau_neutralIsoPtSumWeight");
    AddBranch(&Tau_photonPtSumOutsideSignalCone         ,"Tau_photonPtSumOutsideSignalCone");
  }
  //Other prop and Track related variables
  AddBranch(&Tau_nProngs                  ,"Tau_nProngs");
  AddBranch(&Tau_leadChargedCandNdof      ,"Tau_leadChargedCandNdof");
  AddBranch(&Tau_leadChargedCandChi2      ,"Tau_leadChargedCandChi2");
  AddBranch(&Tau_leadChargedCandValidHits ,"Tau_leadChargedCandValidHits");
  //IP
  AddBranch(&Tau_defaultDxy                            ,"Tau_defaultDxy");
  AddBranch(&Tau_defaultDxyError                       ,"Tau_defaultDxyError");
  AddBranch(&Tau_defaultDxySig                         ,"Tau_defaultDxySig");
  AddBranch(&Tau_defaultFlightLengthX                  ,"Tau_defaultFlightLengthX");
  AddBranch(&Tau_defaultFlightLengthY                  ,"Tau_defaultFlightLengthY");
  AddBranch(&Tau_defaultFlightLengthZ                  ,"Tau_defaultFlightLengthZ");
  AddBranch(&Tau_defaultFlightLengthSig                ,"Tau_defaultFlightLengthSig");
  AddBranch(&Tau_default_PCAx_pv                       ,"Tau_default_PCAx_pv");
  AddBranch(&Tau_default_PCAy_pv                       ,"Tau_default_PCAy_pv");
  AddBranch(&Tau_default_PCAz_pv                       ,"Tau_default_PCAz_pv");
  AddBranch(&Tau_leadChargedCandDz_pv                  ,"Tau_leadChargedCandDz_pv");
  AddBranch(&Tau_leadChargedCandDxy_pv                 ,"Tau_leadChargedCandDxy_pv");
  AddBranch(&Tau_leadChargedCandDz_bs                  ,"Tau_leadChargedCandDz_bs");
  AddBranch(&Tau_leadChargedCandDxy_bs                 ,"Tau_leadChargedCandDxy_bs");
  AddBranch(&Tau_leadChargedCandDzError                ,"Tau_leadChargedCandDzError");
  AddBranch(&Tau_leadChargedCandDxyError               ,"Tau_leadChargedCandDxyError");
  AddBranch(&Tau_leadChargedCandVtx                    ,"Tau_leadChargedCandVtx");
  AddBranch(&Tau_leadChargedCandVty                    ,"Tau_leadChargedCandVty");
  AddBranch(&Tau_leadChargedCandVtz                    ,"Tau_leadChargedCandVtz");
  AddBranch(&Tau_leadChargedCandTrack_PCAx_bs          ,"Tau_leadChargedCandTrack_PCAx_bs");
  AddBranch(&Tau_leadChargedCandTrack_PCAy_bs          ,"Tau_leadChargedCandTrack_PCAy_bs");
  AddBranch(&Tau_leadChargedCandTrack_PCAz_bs          ,"Tau_leadChargedCandTrack_PCAz_bs");
  AddBranch(&Tau_leadChargedCandTrack_PCAx_pv          ,"Tau_leadChargedCandTrack_PCAx_pv");
  AddBranch(&Tau_leadChargedCandTrack_PCAy_pv          ,"Tau_leadChargedCandTrack_PCAy_pv");
  AddBranch(&Tau_leadChargedCandTrack_PCAz_pv          ,"Tau_leadChargedCandTrack_PCAz_pv");
  AddBranch(&Tau_leadChargedCandTrackFitErrorMatrix_00 ,"Tau_leadChargedCandTrackFitErrorMatrix_00");
  AddBranch(&Tau_leadChargedCandTrackFitErrorMatrix_01 ,"Tau_leadChargedCandTrackFitErrorMatrix_01");
  AddBranch(&Tau_leadChargedCandTrackFitErrorMatrix_02 ,"Tau_leadChargedCandTrackFitErrorMatrix_02");
  AddBranch(&Tau_leadChargedCandTrackFitErrorMatrix_11 ,"Tau_leadChargedCandTrackFitErrorMatrix_11");
  AddBranch(&Tau_leadChargedCandTrackFitErrorMatrix_12 ,"Tau_leadChargedCandTrackFitErrorMatrix_12");
  AddBranch(&Tau_leadChargedCandTrackFitErrorMatrix_22 ,"Tau_leadChargedCandTrackFitErrorMatrix_22");
}

void TauSelector::Clear(){
  //Kinematic
  Tau_pt.clear();
  Tau_eta.clear();
  Tau_phi.clear();
  Tau_energy.clear();
  Tau_leadChargedCandPt.clear();
  Tau_leadChargedCandEta.clear();
  Tau_leadChargedCandPhi.clear();
  Tau_leadChargedCandE.clear();
  Tau_leadChargedCandTrack_pt.clear();
  Tau_leadChargedCandTrack_ptError.clear();
  //Charge
  Tau_charge.clear();
  Tau_leadChargedCandCharge.clear();
  //Decay mode finding
  //Tau_decayModeFindingOldDMs.clear();
  Tau_decayModeFinding.clear();
  Tau_decayModeFindingNewDMs.clear();
  //Against Muon
  if(!_MiniAODv2){
    Tau_againstMuonLoose2.clear();
    Tau_againstMuonTight2.clear();
  }
  Tau_againstMuonLoose3.clear();
  Tau_againstMuonTight3.clear();
  //Against Electron
  if(!_MiniAODv2){
    Tau_againstElectronLoose.clear();
    Tau_againstElectronMedium.clear();
    Tau_againstElectronTight.clear();
  }
  Tau_againstElectronVLooseMVA5.clear();
  Tau_againstElectronLooseMVA5.clear();
  Tau_againstElectronMediumMVA5.clear();
  Tau_againstElectronTightMVA5.clear();
  //Isolation
  //MiniAODv1
  if(!_MiniAODv2){
    Tau_byLooseIsolationMVA3newDMwoLT.clear();
    Tau_byLooseIsolationMVA3oldDMwoLT.clear();
    Tau_byMediumIsolationMVA3newDMwoLT.clear();
    Tau_byMediumIsolationMVA3oldDMwoLT.clear();
    Tau_byTightIsolationMVA3newDMwoLT.clear();
    Tau_byTightIsolationMVA3oldDMwoLT.clear();  
  }
  //MiniAODv1v2
  Tau_byVLooseIsolationMVA3newDMwLT.clear();
  Tau_byVLooseIsolationMVA3oldDMwLT.clear();
  Tau_byLooseCombinedIsolationDeltaBetaCorr3Hits.clear();
  Tau_byLooseIsolationMVA3newDMwLT.clear();
  Tau_byLooseIsolationMVA3oldDMwLT.clear();
  Tau_byMediumCombinedIsolationDeltaBetaCorr3Hits.clear();
  Tau_byMediumIsolationMVA3newDMwLT.clear();
  Tau_byMediumIsolationMVA3oldDMwLT.clear();
  Tau_byTightCombinedIsolationDeltaBetaCorr3Hits.clear();
  Tau_byTightIsolationMVA3newDMwLT.clear();
  Tau_byTightIsolationMVA3oldDMwLT.clear();
  Tau_byVTightIsolationMVA3newDMwLT.clear();
  Tau_byVTightIsolationMVA3oldDMwLT.clear();
  Tau_byVVTightIsolationMVA3newDMwLT.clear();
  Tau_byVVTightIsolationMVA3oldDMwLT.clear();
  Tau_byCombinedIsolationDeltaBetaCorrRaw3Hits.clear();
  Tau_byIsolationMVA3newDMwLTraw.clear();
  Tau_byIsolationMVA3oldDMwLTraw.clear();
  Tau_chargedIsoPtSum.clear();
  Tau_neutralIsoPtSum.clear();
  Tau_puCorrPtSum.clear();
  if(_MiniAODv2){
    Tau_byLoosePileupWeightedIsolation3Hits.clear();
    Tau_byMediumPileupWeightedIsolation3Hits.clear();
    Tau_byTightPileupWeightedIsolation3Hits.clear();
    Tau_byPhotonPtSumOutsideSignalCone.clear();
    Tau_byPileupWeightedIsolationRaw3Hits.clear();
    Tau_footprintCorrection.clear();
    Tau_neutralIsoPtSumWeight.clear();
    Tau_photonPtSumOutsideSignalCone.clear();
  }
  //Other prop and Track related variables
  Tau_nProngs.clear();
  Tau_leadChargedCandNdof.clear();
  Tau_leadChargedCandChi2.clear();
  Tau_leadChargedCandValidHits.clear();
  //IP
  Tau_defaultDxy.clear();
  Tau_defaultDxyError.clear();
  Tau_defaultDxySig.clear();
  Tau_defaultFlightLengthX.clear();
  Tau_defaultFlightLengthY.clear();
  Tau_defaultFlightLengthZ.clear();
  Tau_defaultFlightLengthSig.clear();
  Tau_default_PCAx_pv.clear();
  Tau_default_PCAy_pv.clear();
  Tau_default_PCAz_pv.clear();
  Tau_leadChargedCandDz_pv.clear();
  Tau_leadChargedCandDxy_pv.clear();
  Tau_leadChargedCandDz_bs.clear();
  Tau_leadChargedCandDxy_bs.clear();
  Tau_leadChargedCandDzError.clear();
  Tau_leadChargedCandDxyError.clear();
  Tau_leadChargedCandVtx.clear();
  Tau_leadChargedCandVty.clear();
  Tau_leadChargedCandVtz.clear();
  Tau_leadChargedCandTrack_PCAx_bs.clear();
  Tau_leadChargedCandTrack_PCAy_bs.clear();
  Tau_leadChargedCandTrack_PCAz_bs.clear();
  Tau_leadChargedCandTrack_PCAx_pv.clear();
  Tau_leadChargedCandTrack_PCAy_pv.clear();
  Tau_leadChargedCandTrack_PCAz_pv.clear();
  Tau_leadChargedCandTrackFitErrorMatrix_00.clear();
  Tau_leadChargedCandTrackFitErrorMatrix_01.clear();
  Tau_leadChargedCandTrackFitErrorMatrix_02.clear();
  Tau_leadChargedCandTrackFitErrorMatrix_11.clear();
  Tau_leadChargedCandTrackFitErrorMatrix_12.clear();
  Tau_leadChargedCandTrackFitErrorMatrix_22.clear();
}
bool TauSelector::isGoodVertex(const reco::Vertex& vtxxx) {
  if (vtxxx.isFake()) return false;
  if (vtxxx.ndof() < _Tau_vtx_ndof_min) return false;
  if (vtxxx.position().Rho() > _Tau_vtx_rho_max) return false;
  if (fabs(vtxxx.position().Z()) > _Tau_vtx_position_z_max) return false;
  return true;
}
