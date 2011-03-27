/***************************************************************************
*               Generated by StarUML(tm) C++ Add-In                        *
***************************************************************************/
/***************************************************************************
*                                                                          *
*           Institute of Industrial and Control Engineering                *
*                 Technical University of Catalunya                        *
*                        Barcelona, Spain                                  *
*                                                                          *
*                Project Name:       Kautham Planner                       *
*                                                                          *
*     Copyright (C) 2007 - 2009 by Alexander P�rez and Jan Rosell          *
*            alexander.perez@upc.edu and jan.rosell@upc.edu                *
*                                                                          *
*             This is a motion planning tool to be used into               *
*             academic environment and it's provided without               *
*                     any warranty by the authors.                         *
*                                                                          *
*          Alexander P�rez is also with the Escuela Colombiana             *
*          de Ingenier�a "Julio Garavito" placed in Bogot� D.C.            *
*             Colombia.  alexander.perez@escuelaing.edu.co                 *
*                                                                          *
***************************************************************************/
/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
 
 

#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include <libsampling/robconf.h>
#include "localplanner.h"
#include "guibroplanner.h"

using namespace libSampling;

namespace libPlanner {
   namespace GUIBRO{
	//! Constructor
    GUIBROPlanner::GUIBROPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize):
              Planner(stype, init, goal, samples, sampler, ws, lcPlan, ssize)
	{
		//set intial values
		_alpha = 0.1;
		_xi = 0.1;
		_deltaZ = 50;
	    _gen = new LCPRNG();

		//set intial values from parent class data
		_speedFactor = 1;
		_solved = false;
		setStepSize(ssize);//also changes stpssize of localplanner
	  
		_guiName = "GUIBRO Planner";
		addParameter("Step Size", ssize);
      addParameter("Max. Samples", _maxNumSamples);
		addParameter("Speed Factor", _speedFactor);
		addParameter("1- Rotation (alpha)", _alpha);
		addParameter("2- Bending (xi)", _xi);
		addParameter("3- Advance (delta_z)", _deltaZ);

		//removeParameter("Neigh Thresshold");
		//removeParameter("Drawn Path Link");
		//removeParameter("Max. Neighs");

    }

	//! void destructor
	GUIBROPlanner::~GUIBROPlanner(){
			
	}
	
	//! setParameters sets the parameters of the planner
    bool GUIBROPlanner::setParameters(){
      try{
        HASH_S_K::iterator it = _parameters.find("Step Size");
		if(it != _parameters.end())
			setStepSize(it->second);//also changes stpssize of localplanner
        else
          return false;

        it = _parameters.find("Speed Factor");
        if(it != _parameters.end())
          _speedFactor = it->second;
        else
          return false;

        it = _parameters.find("Max. Samples");
        if(it != _parameters.end()){
          _maxNumSamples = it->second;
		    }else
          return false;

		it = _parameters.find("1- Rotation (alpha)");
        if(it != _parameters.end())
			_alpha = it->second;
        else
          return false;

        it = _parameters.find("2- Bending (xi)");
        if(it != _parameters.end())
          _xi = it->second;
        else
          return false;


        it = _parameters.find("3- Advance (delta_z)");
        if(it != _parameters.end())
          _deltaZ = it->second;
        else
          return false;


      }catch(...){
        return false;
      }
      return true;
    }

	
	//! function to find a solution path
	/*
		bool GUIBROPlanner::applyRandControl(Sample *currSmp, Sample *newSmp)
		{
			RobConf *currentconf;
			//move to currSmp and return configMap
			currentconf = _wkSpace->getConfigMapping(currSmp).at(0);
			//compute a control u
			vector<KthReal> u;
			u.push_back( (_gen->d_rand()-0.5)*_alpha );
			u.push_back( (_gen->d_rand()-0.5)*_xi );
			u.push_back( - _deltaZ );//backwards

			//apply control u until collision:
			Sample *tmpSmp;
			tmpSmp = new Sample(_wkSpace->getDimension());
			//a) advance a deltaZ step
			vector<RobConf*> newconf;
			newconf.push_back( &_wkSpace->getRobot(0)->ConstrainedKinematics(u) );
			tmpSmp->setMappedConf(newconf);
			
			//b) keed advancing until collision
			bool advanced = false;
			while( ! _wkSpace->collisionCheck(tmpSmp))
			{
				advanced = true;
				currentconf = _wkSpace->getConfigMapping(tmpSmp).at(0);//move and update
				newconf[0] = &_wkSpace->getRobot(0)->ConstrainedKinematics(u);
				tmpSmp->setMappedConf(newconf);
			}
			//c) return the sample previous to the collision
			if(advanced)
			{
				//go forward one step
				u[2] = -u[2];
				currentconf = _wkSpace->getConfigMapping(tmpSmp).at(0);//move and update
				newconf[0] = &_wkSpace->getRobot(0)->ConstrainedKinematics(u);
				newSmp->setMappedConf(newconf);
				return true; //no collision: newSmp now contains the sample to be added to sample set
			}
			else return false; //collision - discard sample
			
	  	}
		*/

	//! From a collision sample (currGsmp), it goes backward a random step using the alpha and xi
	//! angles of the parent, stores the new sample, computes new alpha and xi angles and 
	//! advances until collision, and stores the sample
		bool GUIBROPlanner::applyRandControl(guibroSample *currGsmp)
		{
			int advanceStep=50;
			RobConf *currentconf;
			Sample *nextSmp;
			vector<RobConf*> newconf; newconf.resize(1);
			vector<KthReal> u; u.resize(3);
			vector<KthReal> uold; uold.resize(3);
			vector<KthReal> uapp; uapp.resize(3);

			//move to currSmp and return configMap
			currentconf = _wkSpace->getConfigMapping(currGsmp->smpPtr).at(0);
			
			//1-go backwards u[2] units mantianing alpha and xi of currGsmp
			u[0] = currGsmp->u[0];
			u[1] = currGsmp->u[1];
			KthReal u2 = (_gen->d_rand()*0.6+0.4)*advanceStep*3;
			if(u2<currGsmp->u[2]) u[2] = -u2 ;//backwards
			else u[2] = -currGsmp->u[2]*0.9;
			

			//1a: apply control
			nextSmp = new Sample(_wkSpace->getDimension());
			newconf[0] =  &_wkSpace->getRobot(0)->ConstrainedKinematics(u);
			nextSmp->setMappedConf(newconf);

			//1b: store the sample
			if( _wkSpace->collisionCheck(nextSmp))
			{
				cout<<"Error: no collision should have been detected!"<<endl;
				delete nextSmp;
				return false; //collision - sample discarded 
			}
			else
			{
				_samples->add(nextSmp);
				guibroSample *gSmp = new guibroSample;
				gSmp->smpPtr = nextSmp;
				gSmp->parent = currGsmp->parent;//the parent is same as the current (collision) sample
				gSmp->length = 0; 
				gSmp->curvature = 0;
				gSmp->steps = 0;
				//same control as currGsmp but discounting the deltaZ used to go backwards
				gSmp->u[0] = currGsmp->u[0];
				gSmp->u[1] = currGsmp->u[1];
				gSmp->u[2] = currGsmp->u[2]-u[2];
				_guibroSet.push_back(gSmp);
			}
			currentconf = _wkSpace->getConfigMapping(nextSmp).at(0);//move and update

			//2-go forward step by step using a new alpha and xi, until colision
			uold[0] = u[0];
			uold[1] = u[1];
			uold[2] = u[2];
			uapp[0] = u[0];
			uapp[1] = u[1];
			uapp[2] = u[2];

			u[0] = currGsmp->u[0] + (_gen->d_rand()-0.5)*_alpha;
			u[1] = currGsmp->u[1] + (_gen->d_rand()-0.5)*_xi;
			u[2] = _deltaZ;//forward
			
			KthReal deltau[3];
			int stepsu = 10;
			deltau[0] = (u[0]-uold[0])/stepsu;
			deltau[1] = (u[1]-uold[1])/stepsu;

			int advanced = 0;
			uapp[0] = uold[0]+advanced*deltau[0];
			uapp[1] = uold[1]+advanced*deltau[1];
			uapp[2] = u[2];

			//2a) advance a deltaZ step
			nextSmp = new Sample(_wkSpace->getDimension());
			newconf[0] = &_wkSpace->getRobot(0)->ConstrainedKinematics(uapp);
			nextSmp->setMappedConf(newconf);
			
			//2b) keed advancing until collision

			while( ! _wkSpace->collisionCheck(nextSmp))
			{
				advanced++;
				if(advanced==10*advanceStep) break;

				if(advanced<=stepsu)
				{
					uapp[0] = uold[0]+advanced*deltau[0];
					uapp[1] = uold[1]+advanced*deltau[1];
					uapp[2] = u[2];
				}
				currentconf = _wkSpace->getConfigMapping(nextSmp).at(0);//move and update
				_samples->add(nextSmp);
				nextSmp = new Sample(_wkSpace->getDimension());
				newconf[0] = &_wkSpace->getRobot(0)->ConstrainedKinematics(uapp);
				nextSmp->setMappedConf(newconf);
			};
			//2c) store the sample if advanced at least 50 steps
			if(advanced>advanceStep)
			{
				_samples->add(nextSmp);
				guibroSample *gSmp = new guibroSample;
				gSmp->smpPtr = nextSmp;
				gSmp->parent = _guibroSet[_guibroSet.size()-1];
				gSmp->length = 0; 
				gSmp->curvature = 0;
				gSmp->steps = 0;
				gSmp->u[0] = u[0];
				gSmp->u[1] = u[1];
				gSmp->u[2] = advanced;
				gSmp->leave = true;
				gSmp->parent->leave = false;
				_guibroSet.push_back(gSmp);
				return true; //no collision - sample added to samnpleset
			}
			else 
			{
				delete nextSmp;
				return false; //collision - sample discarded 
			}
			
	  	}
		
		bool GUIBROPlanner::applyRandControlFirsTime(guibroSample *currGsmp)
		{
			int advanceStep=50;
			RobConf *currentconf;
			Sample *nextSmp;
			vector<RobConf*> newconf; newconf.resize(1);
			vector<KthReal> u; u.resize(3);
			vector<KthReal> uold; uold.resize(3);
			vector<KthReal> uapp; uapp.resize(3);

			//move to currSmp and return configMap
			currentconf = _wkSpace->getConfigMapping(currGsmp->smpPtr).at(0);
			
			//2-go forward step by step using a new alpha and xi, until colision

			u[0] = currGsmp->u[0] + (_gen->d_rand()-0.5)*_alpha;
			u[1] = currGsmp->u[1] + (_gen->d_rand()-0.5)*_xi;
			u[2] = _deltaZ;//forward

			uold[0] = 0.0;//Now assuming goal has xi=0 and apha=0
			uold[1] = 0.0;
			uold[2] = u[2];

			KthReal deltau[3];
			int stepsu = 10;
			deltau[0] = (u[0]-uold[0])/stepsu;
			deltau[1] = (u[1]-uold[1])/stepsu;

			int advanced = 0;
			uapp[0] = uold[0]+advanced*deltau[0];
			uapp[1] = uold[1]+advanced*deltau[1];
			uapp[2] = u[2];

			//2a) advance a deltaZ step
			
			nextSmp = new Sample(_wkSpace->getDimension());
			newconf[0] = &_wkSpace->getRobot(0)->ConstrainedKinematics(uapp);
			nextSmp->setMappedConf(newconf);
			
			//2b) keed advancing until collision
						

			while( ! _wkSpace->collisionCheck(nextSmp))
			{
				advanced++;
				if(advanced==10*advanceStep) break;		
				
				if(advanced<=stepsu)
				{
					uapp[0] = uold[0]+advanced*deltau[0];
					uapp[1] = uold[1]+advanced*deltau[1];
					uapp[2] = u[2];
				}
				currentconf = _wkSpace->getConfigMapping(nextSmp).at(0);//move and update
				_samples->add(nextSmp);
				nextSmp = new Sample(_wkSpace->getDimension());
				newconf[0] = &_wkSpace->getRobot(0)->ConstrainedKinematics(uapp);
				nextSmp->setMappedConf(newconf);
			};
			//2c) store the sample if advanced at least 50 steps
			if(advanced>advanceStep)
			{
				//_samples->add(nextSmp);
				guibroSample *gSmp = new guibroSample;
				gSmp->smpPtr = nextSmp;
				gSmp->parent = _guibroSet[_guibroSet.size()-1];
				gSmp->length = 0; 
				gSmp->curvature = 0;
				gSmp->steps = 0;
				gSmp->u[0] = u[0];
				gSmp->u[1] = u[1];
				gSmp->u[2] = advanced;
				gSmp->leave = true;
				_guibroSet.push_back(gSmp);
				return true; //no collision - sample added to samnpleset
			}
			else 
			{
				delete nextSmp;
				return false; //collision - sample discarded 
			}
			
	  	}
		


	//! function to find a solution path
		bool GUIBROPlanner::trySolve()
		{
			_solved = false;
			char sampleDIM = _wkSpace->getDimension();

			//Set _samples only with the initial sample
			if( _samples->getSize() > 0){
			    Sample *SmpInit;
			    Sample *SmpGoal;
				SmpInit = new Sample(sampleDIM);
				SmpInit->setCoords(_init->getCoords());
				_wkSpace->collisionCheck(SmpInit);
				SmpInit->setMappedConf(_wkSpace->getConfigMapping(_init));
				SmpGoal = new Sample(sampleDIM);
				SmpGoal->setCoords(_goal->getCoords());
				_wkSpace->collisionCheck(SmpGoal);
				SmpGoal->setMappedConf(_wkSpace->getConfigMapping(_goal));
				_samples->clear();
				_init=SmpInit;
				_goal=SmpGoal;
				
				_samples->add(_init);

				//Define the first guibroSample and add to guibroSet
				_guibroSet.clear();
				guibroSample *gSmp = new guibroSample;
				gSmp->smpPtr = _init;
				gSmp->parent = NULL;
				gSmp->length = 0; 
				gSmp->curvature = 0;
				gSmp->steps = 0;
				gSmp->u[0] = 0;
				gSmp->u[1] = 0;
				gSmp->u[2] = 0;
				_guibroSet.push_back(gSmp);
			}
			else
			{
				//no query have been set
				cout<<"Trysolve did nothing - no query have been set"<<endl;
				return false;
			}


			//Loop by growing the tree
			guibroSample *curr = _guibroSet.at(0);
			int count=1;
			applyRandControlFirsTime(curr);
			if(_guibroSet.size()==1) 
			{
				_solved = false;
				return _solved;
			}

			int currentNumSamples = 10;//_guibroSet->getSize();
			for(int i=1; i<currentNumSamples; i++)
			{
				if(_guibroSet.size()==i) {
					i=0;
					continue;
				}
				curr = _guibroSet.at(i);
				if(curr->leave==false) continue;
				if(applyRandControl(curr))
				{
					count++;
				}
			}
			return _solved;
		}
	  }
}
