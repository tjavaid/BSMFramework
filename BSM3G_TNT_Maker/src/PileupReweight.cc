#include "BSMFramework/BSM3G_TNT_Maker/interface/PileupReweight.h"
PileupReweight::PileupReweight(std::string name, TTree* tree, bool debug, const pset& iConfig):baseTree(name,tree,debug)
{
  if(debug) std::cout<<"in PileupReweight constructor"<<std::endl;
  _MiniAODv2       = iConfig.getParameter<bool>("MiniAODv2");
  _is_data   = iConfig.getParameter<bool>("is_data");
  PUReweightfile_ = iConfig.getParameter<edm::FileInPath>("PUReweightfile");

  // Get data distribution from file
  const char *filePath = PUReweightfile_.fullPath().c_str();
  TFile file(filePath, "READ");
  TH1* h = NULL;
  file.GetObject("pileup",h);
  if( h == NULL ) {
    std::cerr << "\n\nERROR in PUWeight: Histogram 'pileup' does not exist in file \n.";
    throw std::exception();
  }
  h->SetDirectory(0);
  file.Close();

  // Computing weights
  // Store probabilites for each pu bin
  unsigned int nPUMax = 0;
  double *npuProbs = 0;
  
  nPUMax = 52;
  double npuWinter15_25ns[52] = {4.8551E-07,1.74806E-06,3.30868E-06,1.62972E-05,4.95667E-05,0.000606966,0.003307249,0.010340741,0.022852296,0.041948781,0.058609363,0.067475755,0.072817826,0.075931405,0.076782504,0.076202319,0.074502547,0.072355135,0.069642102,0.064920999,0.05725576,0.047289348,0.036528446,0.026376131,0.017806872,0.011249422,0.006643385,0.003662904,0.001899681,0.00095614,0.00050028,0.000297353,0.000208717,0.000165856,0.000139974,0.000120481,0.000103826,8.88868E-05,7.53323E-05,6.30863E-05,5.21356E-05,4.24754E-05,3.40876E-05,2.69282E-05,2.09267E-05,1.5989E-05,4.8551E-06,2.42755E-06,4.8551E-07,2.42755E-07,1.21378E-07,4.8551E-08};
  npuProbs = npuWinter15_25ns;

  // Check that binning of data-profile matches MC scenario
  if( nPUMax != static_cast<unsigned int>(h->GetNbinsX()) ) {
    std::cerr << "\n\nERROR number of bins (" << h->GetNbinsX() << ") in data PU-profile does not match number of bins (" << nPUMax << ") in MC scenario " << std::endl;
    throw std::exception();
  }

  std::vector<double> result(nPUMax,0.);
  double s = 0.;
  for(unsigned int npu = 0; npu < nPUMax; ++npu) {
    const double npu_estimated = h->GetBinContent(h->GetXaxis()->FindBin(npu));
    result[npu] = npu_estimated / npuProbs[npu];
    s += npu_estimated;
  }
  // normalize weights such that the total sum of weights over thw whole sample is 1.0, i.e., sum_i  result[i] * npu_probs[i] should be 1.0 (!)
  for(unsigned int npu = 0; npu < nPUMax; ++npu) {
    result[npu] /= s;
  }

  puWeigths_ = result;
  nPUMax_ = puWeigths_.size();

  // Clean up
  delete h;

  SetBranches();
}
PileupReweight::~PileupReweight(){
  delete tree_;
}

void PileupReweight::Fill(const edm::Event& iEvent){
  if(debug_) std::cout<<"getting PileupReweight info"<<std::endl;
  double w = 1.;
  if(!_is_data) {
    Handle<std::vector< PileupSummaryInfo > >  PupInfo;
    string pileupinfo;
    if(!_MiniAODv2) pileupinfo = "addPileupInfo";
    if(_MiniAODv2)  pileupinfo = "slimmedAddPileupInfo";
    iEvent.getByLabel(pileupinfo, PupInfo); 
    std::vector<PileupSummaryInfo>::const_iterator PVI;
    float nPU = -1;
    for(PVI = PupInfo->begin(); PVI != PupInfo->end(); ++PVI) {
      int BX = PVI->getBunchCrossing();
      if(BX == 0) { 
	nPU = PVI->getTrueNumInteractions();
	continue;
      }
    }
    if( nPU >= nPUMax_ ) {
      //std::cerr << "WARNING: Number of PU vertices = " << nPU << " out of histogram binning." << std::endl;
      // In case nPU is out-of data-profile binning,
      // use weight from last bin
      w = puWeigths_.back();
    } else {
      w = puWeigths_.at(nPU);
    }
  }
  PUWeight=w;

  if(debug_) std::cout<<"got PileupReweight info"<<std::endl;
}
void PileupReweight::SetBranches(){
  if(debug_) std::cout<<"setting branches: calling AddBranch of PileupReweight"<<std::endl;
  AddBranch(&PUWeight,"PUWeight");
}
