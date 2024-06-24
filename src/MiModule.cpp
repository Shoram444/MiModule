#include "MiModule.h"
#include "MiFilters.h"
#include "MiSDVisuHit.h"
using namespace std;

DPP_MODULE_REGISTRATION_IMPLEMENT(MiModule,"MiModule");

// Construct
MiModule::MiModule() : dpp::base_module()
{	
	eventNo = 0;
}

// Initialize 
void MiModule::initialize(const datatools::properties&  myConfig,
			      datatools::service_manager&   flServices,
			      dpp::module_handle_dict_type& /*moduleDict*/) 
{
	subor = new TFile("Default.root","NEW");
	strom = new TTree("Event","All data from event");
	strom->Branch("Eventdata", &E);	

	this->_set_initialized(true);
}

// Core method - process all data
dpp::base_module::process_status MiModule::process(datatools::things& workItem) 
{
	fillSD(workItem);
	fillCD(workItem);
	fillPTD(workItem);
	strom->Fill();	

	if (eventNo % 10000 == 0)
	{	
		buneve_end = clock();
		if (eventNo != 0)
		{
			cout << "MiModule: Spracoval som event c.: " << eventNo 
		     	     << ", posledna 10000 trvala " << (float)(buneve_end-buneve_st)/CLOCKS_PER_SEC 
			     << " s!" << endl;
		}
		else
		{
			cout << "MiModule: Zacal som zapis!" << endl;
		}
		buneve_st = clock();
	}

	eventNo++;

	// MUST return a status, see ref dpp::processing_status_flags_type
	return falaise::processing::status::PROCESS_OK;
}

// Destruct
MiModule::~MiModule() 
{ 
	subor->cd();
	strom->Write();
	subor->Close();	
	delete subor;

	// MUST reset module at destruction	
	if (this->is_initialized())
	{
		this->reset();
	}
}

// Reset
void MiModule::reset() 
{	
	this->_set_initialized(false);
}

//#############################################################################################################
// SD
//#############################################################################################################
void MiModule::fillSD(datatools::things& workI)
{		
	if(workI.has("SD"))
	{	
		// Get the SD data
		SD = workI.get<mctools::simulated_data>("SD");
		// Save data about primary event into primev
		primev = SD.get_primary_event();
		// Get the list with particle data into prcoll
		prcoll = primev.get_particles();

		MiSD* SDb = new MiSD();

		// Go through all the particles in SD bank using iterator it
		for (std::list<genbb::primary_particle>::const_iterator it = prcoll.begin() ; it != prcoll.end(); ++it)
		{
			//Save current particle data into the_particle so we can handle it 
  			the_particle = *it;

			MiSDParticle* p = new MiSDParticle();

			p->setname(the_particle.get_particle_label());
			p->setE   (the_particle.get_kinetic_energy() / CLHEP::keV);
			p->sett   (the_particle.get_time()           / CLHEP::ns);	
			p->setp   (the_particle.get_momentum().x(), 
				   the_particle.get_momentum().y(), 
				   the_particle.get_momentum().z());		
			p->setr   (SD.get_vertex().x(), 
				   SD.get_vertex().y(), 
				   SD.get_vertex().z());

			SDb->addpart(*p);

			delete p;
		}

		string caloname;
		MiSDVisuHit* visu_h;

		for (int i = 0; i < 4; i++)	
		{	
			switch (i)
			{
				case 0: caloname = "calo";
					break;
				case 1: caloname = "xcalo";
					break;
				case 2: caloname = "gveto";
					break;
				case 3: caloname = "__visu.tracks";
					break;
			}

			if (SD.has_step_hits(caloname))
			{
  				for (UInt_t ihit = 0; ihit < SD.get_number_of_step_hits(caloname); ihit++)
    				{			

					if(caloname != "__visu.tracks")
					{
						MiSDCaloHit* cal = new MiSDCaloHit();
						MiGID* gid = new MiGID();

      						the_scin_hit = SD.get_step_hit(caloname, ihit);

						switch (the_scin_hit.get_geom_id().get_depth()) 
						{
							// Get GID only for the proper depth, if depth is other than 1,2,3,4 or 5, write warning
  							case 5  : gid->setrow    (its(the_scin_hit.get_geom_id().get(4)));			
							case 4  : gid->setcolumn (its(the_scin_hit.get_geom_id().get(3))); 			
							case 3  : gid->setwall   (its(the_scin_hit.get_geom_id().get(2))); 			
							case 2  : gid->setside   (its(the_scin_hit.get_geom_id().get(1)));
							case 1  : gid->setmodule (its(the_scin_hit.get_geom_id().get(0))); 
					  		  	  gid->settype   (its(the_scin_hit.get_geom_id().get_type()));
					  	          	  break;
							default : cout << "WARNING: GID depth in event " << eventNo 
							       	       << " different from 1,2,3,4 or 5!!!/n";
						}

     						cal->setE(the_scin_hit.get_energy_deposit()/ CLHEP::keV);
     						cal->setGID(*gid);
     						cal->setname(the_scin_hit.get_particle_name());
						
						SDb->addcalohit(*cal);
 
						delete cal;
						delete gid;	
					}
					else
					{
						visu_h = new MiSDVisuHit();

						visu_h->setStepNo(ihit);

						// Get visualisation step particle name
						visu_h->setParticleName( SD.get_step_hit("__visu.tracks", ihit).get_particle_name() );	// Added 27.3.2018 19:33

						// Get visualisation step positions
						visu_h->setStart(	SD.get_step_hit("__visu.tracks", ihit).get_position_start().x(),
		                 		 	 		SD.get_step_hit("__visu.tracks", ihit).get_position_start().y(),
		                 		 	 		SD.get_step_hit("__visu.tracks", ihit).get_position_start().z());
						visu_h->setStop (	SD.get_step_hit("__visu.tracks", ihit).get_position_stop() .x(),
		                 		 	 		SD.get_step_hit("__visu.tracks", ihit).get_position_stop() .y(),
		                 	 	 	 		SD.get_step_hit("__visu.tracks", ihit).get_position_stop() .z());

						visu_h->setMomentumStart(	SD.get_step_hit("__visu.tracks", ihit).get_momentum_start().x(),
				                 		 	 		SD.get_step_hit("__visu.tracks", ihit).get_momentum_start().y(),
				                 		 	 		SD.get_step_hit("__visu.tracks", ihit).get_momentum_start().z());


						// Get visualisation step auxiliaries
						if ( SD.get_step_hit("__visu.tracks", ihit).has_time_start() )
						{
							visu_h->setTStart(   SD.get_step_hit("__visu.tracks", ihit).get_time_start() ); 
						}	
						if ( SD.get_step_hit("__visu.tracks", ihit).has_time_stop() )
						{
							visu_h->setTStop(    SD.get_step_hit("__visu.tracks", ihit).get_time_stop() ); 
						}	
						if ( SD.get_step_hit("__visu.tracks", ihit).has_material_name() )
						{
							visu_h->setMaterial( SD.get_step_hit("__visu.tracks", ihit).get_material_name() ); 
						}							
						if ( SD.get_step_hit("__visu.tracks", ihit).has_creator_process_name())
						{
							visu_h->setProcess(  SD.get_step_hit("__visu.tracks", ihit).get_creator_process_name() );
						}
						if ( SD.get_step_hit("__visu.tracks", ihit).has_track_id())
						{
							visu_h->setTrackID(  SD.get_step_hit("__visu.tracks", ihit).get_track_id() );
						}
						if ( SD.get_step_hit("__visu.tracks", ihit).has_parent_track_id())
						{
							visu_h->setParentID( SD.get_step_hit("__visu.tracks", ihit).get_parent_track_id() );
						}
						if ( SD.get_step_hit("__visu.tracks", ihit).has_energy_deposit())
						{
							visu_h->setEdep(     SD.get_step_hit("__visu.tracks", ihit).get_energy_deposit() );
						}
						if ( SD.get_step_hit("__visu.tracks", ihit).has_hit_id())
						{
							visu_h->setHitID(     SD.get_step_hit("__visu.tracks", ihit).get_hit_id() );
						}
						if ( SD.get_step_hit("__visu.tracks", ihit).has_leaving_volume())
						{
							visu_h->setLeftVolume( SD.get_step_hit("__visu.tracks", ihit).is_leaving_volume() );
						}
						if ( SD.get_step_hit("__visu.tracks", ihit).has_entering_volume())
						{
							visu_h->setEnteredVolume( SD.get_step_hit("__visu.tracks", ihit).is_entering_volume() );
						}
		
						SDb->addvisuhit(*visu_h);
						delete visu_h;
					}
    				}
			}
		}
	
		E.setSD(*SDb);

		delete SDb;
	} 
	else cout << "No SD Bank!!!\n";
}

//#############################################################################################################
// CD
//#############################################################################################################
void MiModule::fillCD(datatools::things& workI)
{
	if(workI.has("CD")) 
	{
		// Get the CD data
		CD = workI.get<snemo::datamodel::calibrated_data>("CD");
		// Save data about primary event into primev
		double time, stime, ene, sene;
		double tene = 0.0;

		MiCD* CDb = new MiCD();
	
		for (UInt_t ihit = 0; ihit < CD.calorimeter_hits().size(); ihit++)
		{
			MiCDCaloHit* cal = new MiCDCaloHit();

			MiGID* gid = new MiGID();
    			snemo::datamodel::calibrated_calorimeter_hit calib_hit = CD.calorimeter_hits().at(ihit).get();

			switch (calib_hit.get_geom_id().get_depth()) 
			{
				// Get GID only for the proper depth, if depth is other than 1,2,3,4 or 5, write warning
  				case 5  : gid->setrow    (its(calib_hit.get_geom_id().get(4)));			
				case 4  : gid->setcolumn (its(calib_hit.get_geom_id().get(3))); 			
				case 3  : gid->setwall   (its(calib_hit.get_geom_id().get(2))); 			
				case 2  : gid->setside   (its(calib_hit.get_geom_id().get(1)));
				case 1  : gid->setmodule (its(calib_hit.get_geom_id().get(0))); 
				  	  gid->settype   (its(calib_hit.get_geom_id().get_type()));
				       	  break;
				default : cout << "WARNING: GID depth in event " << eventNo 
				       	       << " different from 1,2,3,4 or 5!!!/n";
			}

     			cal->setGID(*gid);

			ene   = CD.calorimeter_hits().at(ihit).get().get_energy();
			sene  = CD.calorimeter_hits().at(ihit).get().get_sigma_energy();
			time  = CD.calorimeter_hits().at(ihit).get().get_time();
			stime = CD.calorimeter_hits().at(ihit).get().get_sigma_time();

			cal->setE(ene / CLHEP::keV);
			cal->setEs(sene / CLHEP::keV);
			cal->sett(time / CLHEP::ns);
			cal->setts(stime / CLHEP::ns);

			CDb->addcalohit(*cal);

			delete cal;
			tene += ene;
		}

		//// From Filip, adding tracker hits
		double X, Y, Z, R;
		for(int ihit = 0;ihit < CD.tracker_hits().size();ihit++)
		{
			MiCDTrackerHit* th = new MiCDTrackerHit();
			
			X = CD.tracker_hits().at(ihit).get().get_x();
			Y = CD.tracker_hits().at(ihit).get().get_y();
			Z = CD.tracker_hits().at(ihit).get().get_z();
			R = CD.tracker_hits().at(ihit).get().get_r();
			
			th->setX(X);
			th->setY(Y);
			th->setZ(Z);
			th->setR(R);
			
			CDb->addtrackerhit(*th);
			
			delete th;
			
		}

		E.setCD(*CDb);
		E.settotE(tene / CLHEP::keV);

		delete CDb;
	} 
	else cout << "No CD Bank!!!\n";	

		
}

//#############################################################################################################
// PTD
//#############################################################################################################
void MiModule::fillPTD(datatools::things& workI)
{
	if(workI.has("PTD"))
	{
		using namespace snemo::datamodel;	

		// Get the access to PTD bank
		PTD = workI.get<particle_track_data>("PTD");
		// Get the collection of particles
		particles_collection = PTD.particles();

		MiPTD* PTDb = new MiPTD();

		for(ParticleHdlCollection::const_iterator iparticle = particles_collection.begin(); 
		    iparticle!=particles_collection.end(); 
		    ++iparticle)
		{	
			MiCDParticle* p = new MiCDParticle();

			if      (particle_has_negative_charge (iparticle->get())) 
			{				
				p->setcharge(-1);
			}  			
			else if (particle_has_neutral_charge  (iparticle->get())) 
			{			
				p->setcharge(0);     // Added 9.10.2020
			}   				
			else if (particle_has_positive_charge (iparticle->get())) 
			{		
				p->setcharge(1);
			}  				
			else if (particle_has_undefined_charge(iparticle->get())) 
			{		
				p->setcharge(1000); // Changed 9.10.2020	
			}  			
			else 
			{
				p->setcharge(1001); // Added 9.10.2020
			}
			
			if (iparticle->get().has_associated_calorimeter_hits())
			{
				double time, stime, ene, sene;

      				for (CalorimeterHitHdlCollection::const_iterator icalo = 				            	iparticle->get().get_associated_calorimeter_hits().begin(); 
					icalo != iparticle->get().get_associated_calorimeter_hits().end(); 
			  	        ++icalo)  
				{				
					MiCDCaloHit* cal = new MiCDCaloHit();
				
					ene   = icalo->get().get_energy();
					sene  = icalo->get().get_sigma_energy();
					time  = icalo->get().get_time();
					stime = icalo->get().get_sigma_time();

					cal->setE(ene / CLHEP::keV);
					cal->setEs(sene / CLHEP::keV);
					cal->sett(time / CLHEP::ns);
					cal->setts(stime / CLHEP::ns);

					p->addcalohit(*cal);

					delete cal;
				}
			}

			const std::vector<datatools::handle<snemo::datamodel::vertex>> & particle_vertices = iparticle->get().get_vertices();

	  		for (datatools::handle<vertex> ivertex : particle_vertices)
	   		{
				snemo::datamodel::vertex vtx = ivertex.get(); // get the vertex
				MiVertex* ver = new MiVertex();
	      
	      			ver->setr(vtx.get_spot().get_position().x(),
	      				  vtx.get_spot().get_position().y(),
	      				  vtx.get_spot().get_position().z());

				if(vtx.is_on_source_foil())      
				{
		  			ver->setpos("source foil");
				}
	      			else if(vtx.is_on_wire())
				{
		  			ver->setpos("wire");
				}
	      			else if(vtx.is_on_main_calorimeter())
				{
		  			ver->setpos("calo");
				}
	     			else if(vtx.is_on_x_calorimeter())  
				{
		  			ver->setpos("xcalo");
				}
	      			else if(vtx.is_on_gamma_veto ()) 
				{
		  			ver->setpos("gveto");
				}
					else if(vtx.is_on_calibration_source ()) 
				{
		  			ver->setpos("calibration source");
        }
				else if(vtx.is_on_reference_source_plane()) 
				{
		  			ver->setpos("reference source plane");
				}
				else if(vtx.is_on_source_gap ()) 
				{
		  			ver->setpos("source gap");
				}
	      			else
				{
		  			ver->setpos("undefined");
				}

				p->addvertex(*ver);

				delete ver;
			}

			PTDb->addpart(*p);	
			
			delete p;
		}	

		E.setPTD(*PTDb);

		delete PTDb;
	}
	else cout << "No PTD Bank!!!\n";			
}

string MiModule::its(int in_i)
{
	ostringstream oss;

	oss << in_i;
	
	return oss.str();
}		

void MiModule::setInitialized(bool _init)
{
	init = _init;
}

bool MiModule::isInitialized()
{
	return init;
}
