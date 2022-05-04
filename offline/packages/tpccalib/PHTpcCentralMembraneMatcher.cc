/**
 * \file PHTpcCentralMembraneMatcher.cc
 * \brief match reconstructed CM clusters to CM pads, calculate differences, store on the node tree and compute distortion reconstruction maps
 * \author Tony Frawley <frawley@fsunuc.physics.fsu.edu>, Hugo Pereira Da Costa <hugo.pereira-da-costa@cea.fr>
 */

#include "PHTpcCentralMembraneMatcher.h"

#include <fun4all/Fun4AllReturnCodes.h>

#include <phool/PHCompositeNode.h>
#include <phool/getClass.h>
#include <phool/phool.h>
#include <TNtuple.h>
#include <TFile.h>
#include <TH1F.h>
#include <TH2F.h>
#include <string>
#include <TVector3.h>
#include <TCanvas.h>
#include <TGraph.h>
#include <cmath>

/// Tracking includes
#include <trackbase/CMFlashClusterv1.h>
#include <trackbase/CMFlashClusterContainerv1.h>
#include <trackbase/CMFlashDifferencev1.h>
#include <trackbase/CMFlashDifferenceContainerv1.h>

using namespace std;

//____________________________________________________________________________..
PHTpcCentralMembraneMatcher::PHTpcCentralMembraneMatcher(const std::string &name):
  SubsysReco(name)
{

  // Make some histograms on an output file for diagnostics
  char temp[500];
  sprintf(temp,
	  "./eval_output/Matcher_Histograms_%i.root", _process);
  fout = new TFile(temp,"RECREATE");
  
  hxy_reco = new TH2F("hxy_reco","reco cluster x:y",800,-100,100,800,-80,80);
  hxy_truth = new TH2F("hxy_truth","truth cluster x:y",800,-100,100,800,-80,80);
  hdrdphi = new TH2F("hdrdphi","dr vs dphi",800,-0.5,0.5,800,-0.001,0.001);
  hdrdphi->GetXaxis()->SetTitle("dr");  
  hdrdphi->GetYaxis()->SetTitle("dphi");  
  hrdr = new TH2F("hrdr","dr vs r",800,0.0,80.0,800,-0.5,0.5);
  hrdr->GetXaxis()->SetTitle("r");  
  hrdr->GetYaxis()->SetTitle("dr");  
  hrdphi = new TH2F("hrdphi","dphi vs r",800,0.0,80.0,800,-0.001,0.001);
  hrdphi->GetXaxis()->SetTitle("r");  
  hrdphi->GetYaxis()->SetTitle("dphi");  
  hdr1_single = new TH1F("hdr1_single", "innner dr single", 200,-0.2, 0.2);
  hdr2_single = new TH1F("hdr2_single", "mid dr single", 200,-0.2, 0.2);
  hdr3_single = new TH1F("hdr3_single", "outer dr single", 200,-0.2, 0.2);
  hdr1_double = new TH1F("hdr1_double", "innner dr double", 200,-0.2, 0.2);
  hdr2_double = new TH1F("hdr2_double", "mid dr double", 200,-0.2, 0.2);
  hdr3_double = new TH1F("hdr3_double", "outer dr double", 200,-0.2, 0.2);
  hdrphi = new TH1F("hdrphi","r * dphi", 200, -0.05, 0.05);
  hnclus = new TH1F("hnclus", " nclusters ", 100, 0., 3.);
  
  // Get truth cluster positions
  //=====================
  
  CalculateCenters(nPads_R1, R1_e, nGoodStripes_R1_e, keepUntil_R1_e, nStripesIn_R1_e, nStripesBefore_R1_e, cx1_e, cy1_e);
  CalculateCenters(nPads_R1, R1, nGoodStripes_R1, keepUntil_R1, nStripesIn_R1, nStripesBefore_R1, cx1, cy1);
  CalculateCenters(nPads_R2, R2, nGoodStripes_R2, keepUntil_R2, nStripesIn_R2, nStripesBefore_R2, cx2, cy2);
  CalculateCenters(nPads_R3, R3, nGoodStripes_R3, keepUntil_R3, nStripesIn_R3, nStripesBefore_R3, cx3, cy3);
  
  TVector3 dummyPos;
  const double phi_petal = M_PI / 9.0;  // angle span of one petal
  
  // k is the petal ID, rotate the stripe center to this petal	      
  
  // inner region extended is the 8 layers inside 30 cm    
  for(int j = 0; j < nRadii; ++j)
    for(int i=0; i < nGoodStripes_R1_e[j]; ++i)
      for(int k =0; k<18; ++k)
	{
	  dummyPos.SetXYZ(cx1_e[i][j], cy1_e[i][j], 0.0);
	  dummyPos.RotateZ(k * phi_petal);
	  
	  truth_pos.push_back(dummyPos);

	  if(Verbosity() > 2)	  
	    std::cout << " i " << i << " j " << j << " k " << k << " x1 " << dummyPos.X() << " y1 " << dummyPos.Y()
		      <<  " theta " << atan2(dummyPos.Y(), dummyPos.X())
		      << " radius " << sqrt(pow(dummyPos.X(),2)+pow(dummyPos.Y(),2)) << std::endl; 
	  if(_histos) hxy_truth->Fill(dummyPos.X(),dummyPos.Y());      	  
	}  
  
  // inner region is the 8 layers outside 30 cm  
  for(int j = 0; j < nRadii; ++j)
    for(int i=0; i < nGoodStripes_R1[j]; ++i)
      for(int k =0; k<18; ++k)
	{
	  dummyPos.SetXYZ(cx1[i][j], cy1[i][j], 0.0);
	  dummyPos.RotateZ(k * phi_petal);
	  
	  truth_pos.push_back(dummyPos);

	  if(Verbosity() > 2)	  
	    std::cout << " i " << i << " j " << j << " k " << k << " x1 " << dummyPos.X() << " y1 " << dummyPos.Y()
		      <<  " theta " << atan2(dummyPos.Y(), dummyPos.X())
		      << " radius " << sqrt(pow(dummyPos.X(),2)+pow(dummyPos.Y(),2)) << std::endl; 
	  if(_histos) hxy_truth->Fill(dummyPos.X(),dummyPos.Y());      	  
	}  
  
  for(int j = 0; j < nRadii; ++j)
    for(int i=0; i < nGoodStripes_R2[j]; ++i)
      for(int k =0; k<18; ++k)
	{
	  dummyPos.SetXYZ(cx2[i][j], cy2[i][j], 0.0);
	  dummyPos.RotateZ(k * phi_petal);
	  
	  truth_pos.push_back(dummyPos);

	  if(Verbosity() > 2)	  
	    std::cout << " i " << i << " j " << j << " k " << k << " x1 " << dummyPos.X() << " y1 " << dummyPos.Y()
		      <<  " theta " << atan2(dummyPos.Y(), dummyPos.X())
		      << " radius " << sqrt(pow(dummyPos.X(),2)+pow(dummyPos.Y(),2)) << std::endl; 
	  if(_histos) hxy_truth->Fill(dummyPos.X(),dummyPos.Y());      	  
	}      	  
  
  for(int j = 0; j < nRadii; ++j)
    for(int i=0; i < nGoodStripes_R3[j]; ++i)
      for(int k =0; k<18; ++k)
	{
	  dummyPos.SetXYZ(cx3[i][j], cy3[i][j], 0.0);
	  dummyPos.RotateZ(k * phi_petal);

	  truth_pos.push_back(dummyPos);
	  
	  if(Verbosity() > 2)
	    std::cout << " i " << i << " j " << j << " k " << k << " x1 " << dummyPos.X() << " y1 " << dummyPos.Y()
		      <<  " theta " << atan2(dummyPos.Y(), dummyPos.X())
		      << " radius " << sqrt(pow(dummyPos.X(),2)+pow(dummyPos.Y(),2)) << std::endl; 
	  if(_histos) hxy_truth->Fill(dummyPos.X(),dummyPos.Y());      	  
	}      	  
}


//___________________________________________________________
void PHTpcCentralMembraneMatcher::set_grid_dimensions( int phibins, int rbins )
{
  m_phibins = phibins;
  m_rbins = rbins;
}

//____________________________________________________________________________..
int PHTpcCentralMembraneMatcher::InitRun(PHCompositeNode *topNode)
{
  int ret = GetNodes(topNode);
  return ret;
}

//____________________________________________________________________________..
int PHTpcCentralMembraneMatcher::process_event(PHCompositeNode * /*topNode*/)
{
  std::vector<TVector3> reco_pos;
  std::vector<unsigned int> reco_nclusters;
 
  // read the reconstructed CM clusters
  auto clusrange = m_corrected_CMcluster_map->getClusters();
  for (auto cmitr = clusrange.first;
       cmitr !=clusrange.second;
       ++cmitr)
    {
      auto cmkey = cmitr->first;
      auto cmclus = cmitr->second;
      unsigned int nclus = cmclus->getNclusters();

      // Do the static + average distortion corrections if the container was found
      Acts::Vector3 pos(cmclus->getX(), cmclus->getY(), cmclus->getZ());
      if( m_dcc_in) pos = m_distortionCorrection.get_corrected_position( pos, m_dcc_in ); 

      TVector3 tmp_pos(pos[0], pos[1], pos[2]);
      reco_pos.push_back(tmp_pos);      
      reco_nclusters.push_back(nclus);

      if(Verbosity() > 0)
	{
	  double raw_rad = sqrt( cmclus->getX()*cmclus->getX() + cmclus->getY()*cmclus->getY() );
	  double corr_rad = sqrt( tmp_pos.X()*tmp_pos.X() + tmp_pos.Y()*tmp_pos.Y() );
	  std::cout << "found raw cluster " << cmkey << " with x " << cmclus->getX() << " y " << cmclus->getY() << " z " << cmclus->getZ()   << " radius " << raw_rad << std::endl; 
	  std::cout << "                --- corrected positions: " << tmp_pos.X() << "  " << tmp_pos.Y() << "  " << tmp_pos.Z() << " radius " << corr_rad << std::endl; 
	}

      if(_histos)
	{      
	  hxy_reco->Fill(tmp_pos.X(), tmp_pos.Y());
	}
    }

  // Match reco and truth positions
  //std::map<unsigned int, unsigned int> matched_pair;
  std::vector<std::pair<unsigned int, unsigned int>> matched_pair;
  std::vector<unsigned int> matched_nclus;

  // loop over truth positions
  for(unsigned int i=0; i<truth_pos.size(); ++i)
    {
      double rad1=sqrt(truth_pos[i].X() * truth_pos[i].X() + truth_pos[i].Y() * truth_pos[i].Y());
      double phi1 = truth_pos[i].Phi();
      for(unsigned int j = 0; j < reco_pos.size(); ++j)
	{
	  unsigned int nclus = reco_nclusters[j];

	  double rad2=sqrt(reco_pos[j].X() * reco_pos[j].X() + reco_pos[j].Y() * reco_pos[j].Y());
	  double phi2 = reco_pos[j].Phi();

	  if(fabs(rad1-rad2) < m_rad_cut && fabs(phi1-phi2) < m_phi_cut)
	    {
	      //matched_pair.insert(std::make_pair(i,j));
	      matched_pair.push_back(std::make_pair(i,j));
	      matched_nclus.push_back(nclus);
	      break;
	    }
	}      
    }
  
    for(unsigned int ip = 0; ip < matched_pair.size(); ++ip)
    {
      std::pair<unsigned int, unsigned int> p = matched_pair[ip];
      unsigned int nclus = matched_nclus[ip];
      
      if(_histos)
	{
	  double rad1 = sqrt(reco_pos[p.second].X() * reco_pos[p.second].X() + reco_pos[p.second].Y() * reco_pos[p.second].Y());
	  double rad2 = sqrt(truth_pos[p.first].X() * truth_pos[p.first].X() + truth_pos[p.first].Y() * truth_pos[p.first].Y());

	  double dr =  rad1-rad2;
	  double dphi = reco_pos[p.second].Phi() - truth_pos[p.first].Phi();
	  double r =  rad2;

	  hdrphi->Fill(r * dphi);
	  hdrdphi->Fill(dr, dphi);
	  hrdr->Fill(r,dr);
	  hrdphi->Fill(r,dphi);
	  hnclus->Fill( (float) nclus);

	  if(nclus==1)
	    {
	      if(r < 40.0) hdr1_single->Fill(dr); 
	      if(r >= 40.0 && r < 58.0) hdr2_single->Fill(dr); 
	      if(r >= 58.0) hdr3_single->Fill(dr); 	  
	    }
	  else
	    {
	      if(r < 40.0) hdr1_double->Fill(dr); 
	      if(r >= 40.0 && r < 58.0) hdr2_double->Fill(dr); 
	      if(r >= 58.0) hdr3_double->Fill(dr); 	  
	    }
	}

      // add to node tree
      unsigned int key = p.first;
     auto cmdiff = new CMFlashDifferencev1();
     cmdiff->setTruthPhi(truth_pos[p.first].Phi());
     cmdiff->setTruthR(truth_pos[p.first].Perp());
     cmdiff->setTruthZ(truth_pos[p.first].Z());

     cmdiff->setRecoPhi(reco_pos[p.second].Phi());
     cmdiff->setRecoR(reco_pos[p.second].Perp());
     cmdiff->setRecoZ(reco_pos[p.second].Z());

     cmdiff->setNclusters(nclus);

     m_cm_flash_diffs->addDifferenceSpecifyKey(key, cmdiff);
    } 
  
  if(Verbosity() > 0)
    {	
      // read back differences from node tree as a check
      auto diffrange = m_cm_flash_diffs->getDifferences();
      for (auto cmitr = diffrange.first;
	   cmitr !=diffrange.second;
	   ++cmitr)
	{
	  auto key = cmitr->first;
	  auto cmreco = cmitr->second;
	  
	  std::cout << " key " << key 
		    << " nclus " << cmreco->getNclusters() 
		    << " truth Phi " << cmreco->getTruthPhi() << " reco Phi " << cmreco->getRecoPhi()
		    << " truth R " << cmreco->getTruthR() << " reco R " << cmreco->getRecoR() 
		    << " truth Z " << cmreco->getTruthZ() << " reco Z " << cmreco->getRecoZ() 
		    << std::endl;
	}
    }
  
  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..
int PHTpcCentralMembraneMatcher::End(PHCompositeNode * /*topNode*/ )
{

  if(_histos)
    {
      fout->cd();
      
      hxy_reco->Write();
      hxy_truth->Write();
      hdrdphi->Write();
      hrdr->Write();
      hrdphi->Write();
      hdrphi->Write();
      hdr1_single->Write();
      hdr2_single->Write();
      hdr3_single->Write();
      hdr1_double->Write();
      hdr2_double->Write();
      hdr3_double->Write();
      hnclus->Write();
            
      fout->Close();
    }

  return Fun4AllReturnCodes::EVENT_OK;
}

//____________________________________________________________________________..

int  PHTpcCentralMembraneMatcher::GetNodes(PHCompositeNode* topNode)
{
  //---------------------------------
  // Get Objects off of the Node Tree
  //---------------------------------

  m_corrected_CMcluster_map  = findNode::getClass<CMFlashClusterContainer>(topNode, "CORRECTED_CM_CLUSTER");
  if(!m_corrected_CMcluster_map)
    {
      std::cout << PHWHERE << "CORRECTED_CM_CLUSTER Node missing, abort." << std::endl;
      return Fun4AllReturnCodes::ABORTRUN;
    }      

  // input tpc distortion correction
  m_dcc_in = findNode::getClass<TpcDistortionCorrectionContainer>(topNode,"TpcDistortionCorrectionContainer");
  if( m_dcc_in )
    { 
      std::cout << "PHTpcCentralMembraneMatcher:   found TPC distortion correction container" << std::endl; 
    }

  // create node for results of matching
  std::cout << "Creating node CM_FLASH_DIFFERENCES" << std::endl;  
  PHNodeIterator iter(topNode);
  
  // Looking for the DST node
  PHCompositeNode *dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
  if (!dstNode)
    {
      std::cout << PHWHERE << "DST Node missing, doing nothing." << std::endl;
      return Fun4AllReturnCodes::ABORTRUN;
    }      
  PHNodeIterator dstiter(dstNode);
  PHCompositeNode *DetNode =
    dynamic_cast<PHCompositeNode *>(dstiter.findFirst("PHCompositeNode", "TRKR"));
  if (!DetNode)
    {
      DetNode = new PHCompositeNode("TRKR");
      dstNode->addNode(DetNode);
    }
  
  m_cm_flash_diffs = new CMFlashDifferenceContainerv1;
  PHIODataNode<PHObject> *CMFlashDifferenceNode =
    new PHIODataNode<PHObject>(m_cm_flash_diffs, "CM_FLASH_DIFFERENCES", "PHObject");
  DetNode->addNode(CMFlashDifferenceNode);
  
  // output tpc fluctuation distortion container
  // this one is filled on the fly on a per-CM-event basis, and applied in the tracking chain
  const std::string dcc_out_node_name = "TpcDistortionCorrectionContainerFluctuation";
  m_dcc_out = findNode::getClass<TpcDistortionCorrectionContainer>(topNode,dcc_out_node_name);
  if( !m_dcc_out )
  { 
  
    /// Get the DST node and check
    auto dstNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "DST"));
    if (!dstNode)
    {
      std::cout << "PHTpcCentralMembraneMatcher::InitRun - DST Node missing, quitting" << std::endl;
      return Fun4AllReturnCodes::ABORTRUN;
    }
    
    // Get the tracking subnode and create if not found
    auto svtxNode = dynamic_cast<PHCompositeNode *>(iter.findFirst("PHCompositeNode", "SVTX"));
    if (!svtxNode)
    {
      svtxNode = new PHCompositeNode("SVTX");
      dstNode->addNode(svtxNode);
    }

    std::cout << "PHTpcCentralMembraneMatcher::GetNodes - creating TpcDistortionCorrectionContainer in node " << dcc_out_node_name << std::endl;
    m_dcc_out = new TpcDistortionCorrectionContainer;
    auto node = new PHDataNode<TpcDistortionCorrectionContainer>(m_dcc_out, dcc_out_node_name);
    svtxNode->addNode(node);
  }

  // also prepare the local distortion container, used to aggregate multple events 
  m_dcc_out_internal.reset( new TpcDistortionCorrectionContainer );

  // compute axis limits to include guarding bins, needed for TH2::Interpolate to work
  const float phiMin = m_phiMin - (m_phiMin-m_phiMax)/m_phibins;
  const float phiMax = m_phiMax + (m_phiMin-m_phiMax)/m_phibins;
  
  const float rMin = m_rMin - (m_rMin-m_rMax)/m_rbins;
  const float rMax = m_rMax + (m_rMin-m_rMax)/m_rbins;

  // reset all output distortion container so that they match the requested grid size
  const std::array<const std::string,2> extension = {{ "_negz", "_posz" }};
  for( auto&& dcc:{m_dcc_out, m_dcc_out_internal.get()} )
  {
    for( int i =0; i < 2; ++i )
    {
      delete dcc->m_hDPint[i]; dcc->m_hDPint[i] = new TH2F( Form("hIntDistortionP%s", extension[i].c_str()), Form("hIntDistortionP%s", extension[i].c_str()), m_phibins+2, phiMin, phiMax, m_rbins, rMin, rMax );
      delete dcc->m_hDRint[i]; dcc->m_hDRint[i] = new TH2F( Form("hIntDistortionR%s", extension[i].c_str()), Form("hIntDistortionR%s", extension[i].c_str()), m_phibins+2, phiMin, phiMax, m_rbins, rMin, rMax );
      delete dcc->m_hDZint[i]; dcc->m_hDZint[i] = new TH2F( Form("hIntDistortionZ%s", extension[i].c_str()), Form("hIntDistortionZ%s", extension[i].c_str()), m_phibins+2, phiMin, phiMax, m_rbins, rMin, rMax );
      delete dcc->m_hentries[i]; dcc->m_hentries[i] = new TH2I( Form("hEntries%s", extension[i].c_str()), Form("hEntries%s", extension[i].c_str()), m_phibins+2, phiMin, phiMax, m_rbins, rMin, rMax );
    }
  }
  
  return Fun4AllReturnCodes::EVENT_OK;
}

//_____________________________________________________________
void PHTpcCentralMembraneMatcher::CalculateCenters(
    int nPads,
    const std::array<double, nRadii>& R,
    std::array<int, nRadii>& nGoodStripes,
    const std::array<int, nRadii>& keepUntil,
    std::array<int, nRadii>& nStripesIn,
    std::array<int, nRadii>& nStripesBefore,
    double cx[][nRadii], double cy[][nRadii])
{
  const double phi_module = M_PI / 6.0;  // angle span of a module
  const int pr_mult = 3;                 // multiples of intrinsic resolution of pads
  const int dw_mult = 8;                 // multiples of diffusion width
  const double diffwidth = 0.6 * mm;     // diffusion width
  const double adjust = 0.015;           //arbitrary angle to center the pattern in a petal

  double theta = 0.0;

  //center coords

  //calculate spacing first:
  std::array<double, nRadii> spacing;
  for (int i = 0; i < nRadii; i++)
  {
    spacing[i] = 2.0 * ((dw_mult * diffwidth / R[i]) + (pr_mult * phi_module / nPads));
  }

  //center calculation
  for (int j = 0; j < nRadii; j++)
  {
    int i_out = 0;
    for (int i = keepThisAndAfter[j]; i < keepUntil[j]; i++)
    {
      if (j % 2 == 0)
      {
        theta = i * spacing[j] + (spacing[j] / 2) - adjust;
        cx[i_out][j] = R[j] * cos(theta) / cm;
        cy[i_out][j] = R[j] * sin(theta) / cm;
      }
      else
      {
        theta = (i + 1) * spacing[j] - adjust;
        cx[i_out][j] = R[j] * cos(theta) / cm;
        cy[i_out][j] = R[j] * sin(theta) / cm;
      }

      std::cout << " j " << j << " i " << i << " i_out " << i_out << " theta " << theta << " cx " << cx[i_out][j] << " cy " << cy[i_out][j] 
		<< " radius " << sqrt(pow(cx[i_out][j],2)+pow(cy[i_out][j],2)) << std::endl; 

      i_out++;

      nStripesBefore_R1_e[0] = 0;

      nStripesIn[j] = keepUntil[j] - keepThisAndAfter[j];
      if (j==0)
      {
	nStripesBefore[j] = 0;
      }
      else
      {
        nStripesBefore[j] = nStripesIn[j - 1] + nStripesBefore[j - 1];
      }
      nStripesBefore_R1_e[0] = 0;
    }
    nGoodStripes[j] = i_out;
  }
}


