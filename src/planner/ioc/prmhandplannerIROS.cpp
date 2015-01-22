/*************************************************************************\
   Copyright 2014 Institute of Industrial and Control Engineering (IOC)
                 Universitat Politecnica de Catalunya
                 BarcelonaTech
    All Rights Reserved.

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the
    Free Software Foundation, Inc.,
    59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 \*************************************************************************/

/* Author: Alexander Perez, Jan Rosell */


//FIXME: this planner is done for a single TREE robot (associtated to wkSpace->robots[0])


#include "prmhandplannerIROS.h"
 
 namespace Kautham {

  namespace IOC{
		
	PRMHandPlannerIROS::PRMHandPlannerIROS(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler,
        WorkSpace *ws, int cloudSize, KthReal cloudRad)
      :PRMHandPlanner(stype, init, goal, samples, sampler, ws, cloudSize,  cloudRad)
	{	
			_testSamples.clear();
      _idName = "PRM Hand IROS";
	}

	PRMHandPlannerIROS::~PRMHandPlannerIROS()
	{
			
	}

    bool PRMHandPlannerIROS::setParameters(){
      return PRMHandPlanner::setParameters();

	  /*
      PRMPlanner::setParameters();
      try{
        HASH_S_K::iterator it = _parameters.find("Cloud Size");
        if(it != _parameters.end())
          _cloudSize = it->second;
        else
          return false;

        it = _parameters.find("Cloud Radius");
        if(it != _parameters.end())
          _cloudRadius = it->second;
        else
          return false;
      }catch(...){
        return false;
      }
      return true;
	  */
    }

    
	
	bool PRMHandPlannerIROS::trySolve()
	{
		if(_samples->changed())
		{
			PRMPlanner::clearGraph(); //also puts _solved to false;
		}

		if(_solved) {
			cout << "PATH ALREADY SOLVED"<<endl;
			return true;
		}
		
		cout<<"ENTERING TRYSOLVE!!!"<<endl;
	    clock_t entertime = clock();

		//Create graph with initial and goal sample
		if( !_isGraphSet )
		{
			_samples->findBFNeighs(_neighThress, _kNeighs);
			connectSamples();
			PRMPlanner::loadGraph();
			if(PRMPlanner::findPath()) return true;
		}


	    
		//First create the cloud of samples.
		//It is a vector (_testSamples) of samples with the robotjoint coordinates set to zero.

		if(_testSamples.size() > 0 ){
			for(unsigned int i = 0; i < _testSamples.size(); i++)
			delete _testSamples[i];  
			_testSamples.clear();
		}

		KthReal dist = 0;
    std::vector<KthReal> stps(wkSpace()->getNumRobControls());
    std::vector<KthReal> coord(wkSpace()->getNumRobControls());
		Sample *tmpSample;

		//compute distance between cini and cgoal in robotjoint coordinates
        for(unsigned i = _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); i < _wkSpace->getNumRobControls(); i ++)
		{
			stps[i] = goalSamp()->getCoords()[i] - initSamp()->getCoords()[i];
			dist += (stps[i]*stps[i]);
		}
		dist = sqrt(dist);

		//compute the number of steps to interpolate robot motion
		// int maxsteps = (dist/(2*_cloudRadius))+2; //the 2 is necessary to always reduce the distance...
		//int maxsteps = (dist/(_cloudRadius))+2; //the 2 is necessary to always reduce the distance...
        int maxsteps = (dist/(5*_locPlanner->stepSize()))+2; //the 2 is necessary to always reduce the distance...
		//int maxsteps = 5; //the 2 is necessary to always reduce the distance...
      
		//compute the stepsize in each robotjoint
        for(unsigned k =_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getNumRobControls(); k++)
			stps[k] = stps[k]/maxsteps; //this is the delta step for each dimension
			
		//Sampling the clouds...
		//Iterate through all the steps of the robot, centering a cloud of hand configurations
		//around each robot configuration.
		//Iterate backwards from goal to ini since cloud of hand joints is initialized at goal sample
                tmpSample = new Sample(_wkSpace->getNumRobControls());

		//FIXME-prepared for a max of 5 PMDs. This should ot be hardcoded...
        for(unsigned pmd=1;pmd<=5;pmd++)
		{

			cout <<"Trying with "<<pmd<<" PMDs"<<endl;
			createCloudInConfig(_init, pmd);
			createCloudInConfig(_goal, pmd);

			for(int i=0; i <= maxsteps; i++)	{
			//for(int i=maxsteps; i >= 0; i--)	{	
				
				//Set the coordinates of the hand at those stored in the _testSamples vector
				vector<Sample*>::iterator itSam;
				int newsamples=0;
				int samplesincloud=0;
				for(itSam=_testSamples.begin(); itSam!=_testSamples.end();++itSam)
				{		
					//Set the coordinates of the robot joints 
                    for(unsigned k =_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getNumRobControls(); k++)
						coord[k] = initSamp()->getCoords()[k] + i*stps[k];

                    for(unsigned k = 0; k < _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k++){
						coord[k] = (*itSam)->getCoords()[k];
					}
					//Set the new sample with the hand-arm coorinates and collision-check.
					tmpSample->setCoords(coord);
					if( !_wkSpace->collisionCheck(tmpSample)){ 
						//Free sample. Add to the sampleset _samples.
						_samples->add(tmpSample);
                                                tmpSample = new Sample(_wkSpace->getNumRobControls());
						//add to graph
						if(i==0) PRMPlanner::connectLastSample( initSamp() );
						else if (i==maxsteps) PRMPlanner::connectLastSample( goalSamp() );
						else PRMPlanner::connectLastSample( );
				
						samplesincloud++;
					}
					else 
					{
					//Collision-sample. Change the coordinates of the hand until a free hand-arm configuration
					//is found. Store the new hand cooridnates to the _testSamples vector for propagation to the 
					//next arm configuration.
					//The new sample must be set around the iterpolated hand-pose between ini and goal hand poses.
						/*
                        for(int k = 0; k < _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k++){
							double advancedratio = (double)i/(double)maxsteps;
							coord[k]=_init->getCoords()[k] + advancedratio*(_goal->getCoords()[k]-_init->getCoords()[k]);
						}
						*/
						//add random noise around it
						int numsamples=2;//for each collision sample add two free samples
						int countnew=0;
						for(int r=0; r<numsamples*100; r++)
						{
                            unsigned k;
							for(k=0;k < pmd;k++)
							{
								coord[k] = (KthReal)_gen->d_rand();
							}
                            for(;k < _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk();k++)
							{
								//coord[k] = 0.5;
								coord[k] = (KthReal)_gen->d_rand();
							}
							
                            for(k=_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk();k < _wkSpace->getNumRobControls();k++)
							{
								coord[k] = initSamp()->getCoords()[k] + i*stps[k] + _cloudRadius*(2*(KthReal)_gen->d_rand()-1);
								//coord[k] = (KthReal)_gen->d_rand();
								if(coord[k] > 1) coord[k]=1;
								if(coord[k] < 0) coord[k]=0;
							}
						
							//try the new sample (arm joint unchanged)
							tmpSample->setCoords(coord);
							if( ! _wkSpace->collisionCheck(tmpSample))
							{
								//Free sample. Add to the sampleset _samples.
								_samples->add(tmpSample);
                                                                tmpSample = new Sample(_wkSpace->getNumRobControls());

								//add to graph
								if(i==0) PRMPlanner::connectLastSample( initSamp() );
								else if (i==maxsteps) PRMPlanner::connectLastSample( goalSamp() );
								else PRMPlanner::connectLastSample( );

								//substitue its value in the _testSamples vector for further propagation
								/*
                                KthReal *newcoord = new KthReal[_wkSpace->getNumRobControls()];
                                for(int k =_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getNumRobControls(); k++)
									newcoord[k] = 0;
                                for(int k = 0; k < _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k++)
									newcoord[k] = coord[k];
								(*itSam)->setCoords(newcoord);
								*/
								samplesincloud++;
								newsamples++;//cloud with new sample substituting old one
								countnew++;
								//break; //exit for r-loop
							}
							if(countnew >= numsamples) break;//exit for r-loop
						}//endfor - trials for free new random sample
					}//endelse - generation of new random sample
				}//endfor - loop for generation cloud around an arm configuration

				cout << "Cloud with " << samplesincloud << " samples (" << newsamples << " of them are new)" <<endl;
			}//endfor steps i

			cout << "PRM Free Nodes = " << _samples->getSize() << endl;


			if(PRMPlanner::findPath())
			{
				cout << "PRM Free Nodes = " << _samples->getSize() << endl;
				cout<<"PATH POUND using "<<pmd<< "PMDs"<<endl;
				printConnectedComponents();
				
				clock_t finaltime = clock();
				cout<<"TIME TO COMPUTE THE PATH = "<<(double)(finaltime-entertime)/CLOCKS_PER_SEC<<endl;
				PRMPlanner::smoothPath();

				clock_t finalsmoothtime = clock();
				cout<<"TIME TO SMOOTH THE PATH = "<<(double)(finalsmoothtime - finaltime)/CLOCKS_PER_SEC<<endl;
		
				return true;
			}
			else
			{ //try sampling more at the goal config
				int maxtrials = 1000; //_numberHandConf * _cloudSize;
				int trials = 0;

				while(trials<maxtrials && goalSamp()->getConnectedComponent() != initSamp()->getConnectedComponent())
				{
					trials++;
			
					if(getSampleInGoalRegion() == false) continue;
			
					//add to graph
					connectLastSample( goalSamp() );

					if(PRMPlanner::findPath())
					{
						cout << "PRM Free Nodes = " << _samples->getSize() << endl;
						cout<<"PATH POUND using "<<pmd<< "PMDs"<<endl;
						printConnectedComponents();
				
						clock_t finaltime = clock();
						cout<<"TIME TO COMPUTE THE PATH = "<<(double)(finaltime-entertime)/CLOCKS_PER_SEC<<endl;
				
						PRMPlanner::smoothPath();
				
						clock_t finalsmoothtime = clock();
						cout<<"TIME TO SMOOTH THE PATH = "<<(double)(finalsmoothtime - finaltime)/CLOCKS_PER_SEC<<endl;
						return true;
					}
				}
			}
	  }//endfor pmd

	  cout << "PRM Free Nodes = " << _samples->getSize() << endl;
	  cout<<"PATH NOT POUND"<<endl;
	  printConnectedComponents();
	  
	  clock_t finaltime = clock();
	  cout<<"ELAPSED TIME = "<<(double)(finaltime-entertime)/CLOCKS_PER_SEC<<endl;
	  return false;

    }



    void PRMHandPlannerIROS::createCloudInConfig(Sample *sam, int pmd)
	{
      //adding a cloud of N samples around selected sample
      // Always the problem start here, and the first step is to adquire the free cloud.
      
      Sample* tmpSam ;

	  //KthReal* tmpcoord = _goal->getCoords();
	  //KthReal* tmpcoord = _init->getCoords();
    std::vector<KthReal> tmpcoord2(_wkSpace->getNumRobControls());
	    			
	  //Set the coordinates of the robot joints 
      for(unsigned k =_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getNumRobControls(); k++)
		   tmpcoord2[k] = sam->getCoords()[k];

	    
	  //TO be yet HANDLED correctly:
	  //Only set random values for the controls dealing with the hand.
	  //Let the last controls equal

      int kend = _wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk();


      tmpSam = new Sample(_wkSpace->getNumRobControls());
	  int added=0;
      for(int i=0; i<_cloudSize*100; i++)
	  {
		    int k;
		    for(k=0;k < pmd;k++)
			{
				//tmpcoord2[k] = tmpcoord[k] + _cloudRadius*(2*(KthReal)_gen->d_rand()-1);
				tmpcoord2[k] = (KthReal)_gen->d_rand();
		    }
			for(;k < kend;k++)
			{
				//tmpcoord2[k] = tmpcoord[k] + _cloudRadius*(2*(KthReal)_gen->d_rand()-1);
				tmpcoord2[k] = 0.5;
		    }

		    tmpSam->setCoords(tmpcoord2);

			//cout<<"try: ";
            //for(int t=0;t<_wkSpace->getNumRobControls();t++) cout<<tmpcoord2[t]<<", ";
			//cout<<endl;

		    if( ! _wkSpace->collisionCheck(tmpSam))
			{
				//store with the arm joints set to zero
				/*
                for(int k =_wkSpace->getNumRobControls()-_wkSpace->getRobot(0)->getTrunk(); k < _wkSpace->getNumRobControls(); k++)
					tmpcoord2[k] = 0;
				tmpSam->setCoords(tmpcoord2);
				*/
			    _testSamples.push_back(tmpSam);
				added++;
                                tmpSam = new Sample(_wkSpace->getNumRobControls());
			}
		    if(added >= _cloudSize) break;
	    }
	    cout << "Initial cloud has now " << _testSamples.size() << " free samples" <<endl;
    }


  }
};
