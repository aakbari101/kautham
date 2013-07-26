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
*     Copyright (C) 2007 - 2009 by Alexander Pérez and Jan Rosell          *
*            alexander.perez@upc.edu and jan.rosell@upc.edu                *
*                                                                          *
*             This is a motion planning tool to be used into               *
*             academic environment and it's provided without               *
*                     any warranty by the authors.                         *
*                                                                          *
*          Alexander Pérez is also with the Escuela Colombiana             *
*          de Ingeniería "Julio Garavito" placed in Bogotá D.C.            *
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
#include "ML_locplan.h"

using namespace libProblem;

namespace libPlanner {
  ManhattanLikeLocalPlanner::ManhattanLikeLocalPlanner(SPACETYPE stype, Sample *init, Sample *goal, WorkSpace *ws, KthReal st )
    :LocalPlanner(stype,init,goal,ws,st) {
    vanderMethod = true;
	  _init2=NULL;
	  _goal2=NULL;
    _idName = "Manhattan Like";
	}



	void ManhattanLikeLocalPlanner::setIntermediateConfs()
	{
		int wkDim = wkSpace()->getDimension();
		int indexFrom = _samp->indexOf(_init);
		int indexTo   = _samp->indexOf(_goal);

    std::vector<KthReal> coordsFrom(wkDim);
    std::vector<KthReal> coordsTo(wkDim);

		//FOR GOAL2 configuration
    for(int i=1; i< _init->getDim(); i++)
		{
			//coordsFrom[i] = _init->getCoords()[i];
			coordsTo[i]   = _goal->getCoords()[i];
		}
		
		//FOR INIT2 configuration
		//PMDs (except thumb)
		for(int i=1; i< _wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); i++)
		{
			coordsFrom[i] = _init->getCoords()[i];
		}
		//arm joints
		for(int i =_wkSpace->getDimension()-_wkSpace->getRobot(0)->getTrunk(); i < _wkSpace->getDimension(); i++)
		{
			coordsFrom[i] = _init->getCoords()[i] + (_goal->getCoords()[i]-_init->getCoords()[i])/2.0;
		}


		KthReal minFrom, maxFrom, minTo, maxTo, min, max;
		minFrom = (*_thumb)[indexFrom]->first;
		maxFrom = (*_thumb)[indexFrom]->second;
		minTo   = (*_thumb)[indexTo]->first;
		maxTo   = (*_thumb)[indexTo]->second;

		if(maxFrom < maxTo) max = maxFrom;
		else max = maxTo;
		if(minFrom > minTo) min = minFrom;
		else min = minTo;

		coordsFrom[0] = min + (max-min)/2.0;
		coordsTo[0] = coordsFrom[0];

    if(_init2==NULL) _init2 = new Sample(coordsFrom.size());
    if(_goal2==NULL) _goal2 = new Sample(coordsTo.size());
		_init2->setCoords(coordsFrom);
		_goal2->setCoords(coordsTo);
	}

	bool ManhattanLikeLocalPlanner::canConect(Sample *init, Sample *goal)
	{
		if(initSamp() == NULL || goalSamp() == NULL) return false; //problem not set.

    if(initSamp()->getDim() != wkSpace()->getDimension()) return false;  //sample is not for the workspace.
    
		int wkDim = wkSpace()->getDimension();

			
		//start trying to connect
		KthReal dist = 0;
		libSampling::Sample tmpSample(wkDim);
    std::vector<KthReal> steps(wkDim);
    std::vector<KthReal> coord(wkDim);

		for(int i = 0; i < wkDim; i++)
		{
			steps[i] = goal->getCoords()[i] - init->getCoords()[i];
			dist += (steps[i]*steps[i]);
		}
		dist = sqrt(dist);

		int maxsteps = (dist/stepSize())+2; //the 2 is necessary to always reduce the distance...

		if( !vanderMethod )		
		{
			for(int k=0;k<wkDim;k++) 
				steps[k] = steps[k]/maxsteps; //this is the delta step for each dimension
			
			for(int i=0; i<maxsteps; i++)	
			{	
				for(int k = 0; k < wkDim; k++)
					coord[k] = init->getCoords()[k] + i*steps[k];

				tmpSample.setCoords(coord);
				if(wkSpace()->collisionCheck(&tmpSample)) return false; 
			}
		}
		else
		{ //method == VANDERCORPUT
			//find how many bits are needed to code the maxsteps
			int b= ceil(log10( (double) maxsteps) / log10( 2.0 ));
			int finalmaxsteps = (0x01<<b);
			//cout<<"maxsteps= "<<maxsteps<<" b= "<<b<<" finalmaxsteps = "<<finalmaxsteps<<endl;

			//index is the index of the Van der Corput sequence, using b bites the sequence has 2^b elements
			//dj is the bit j of the binary representation of the index
			//rj are the elements of the sequence
			//deltarj codes the jumps between successive elements of the sequence
			double rj=0;
			for(int index = 0; index < finalmaxsteps ; index++)
			{
				int dj;
				double deltaj;
				double newrj=0;
				for(int j = 0; j < b ; j++){
					dj = (index >> j) & 0x01;
					newrj += ((double)dj /  (double)(0x01<<(j+1)) );
				}
				deltaj = newrj - rj;
				
				rj = newrj;

				for(int k=0;k<wkDim;k++)
					coord[k] = init->getCoords()[k] + rj*steps[k];;
				 
				tmpSample.setCoords(coord);
				if(wkSpace()->collisionCheck(&tmpSample)) return false; 
			}
		}
		return true;
	}


	bool ManhattanLikeLocalPlanner::canConect()
	{
		//set the intermediate init and goal configs
		setIntermediateConfs();
		
		if(canConect( initSamp(), initSamp2()))
		{
			return canConect(initSamp2(), goalSamp2());
		}
		return false;
	}


	KthReal ManhattanLikeLocalPlanner::distance(Sample* from, Sample* to)
	{
    return _wkSpace->distanceBetweenSamples(*from, *to, CONFIGSPACE);
		/*
		if(from == _init && to==_goal && _init2!= NULL && _goal2!=NULL)
		{
			KthReal dist = 0.0;
			KthReal dd=0.0;
			if( weights != NULL )
			{
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
						dd = _init->getCoords()[k] - _init2->getCoords()[k];
						dist += (dd*dd*weights[k]);
				}
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
						dd = _init2->getCoords()[k] - _goal2->getCoords()[k];
						dist += (dd*dd*weights[k]);
				}
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
						dd = _goal2->getCoords()[k] - _goal->getCoords()[k];
						dist += (dd*dd*weights[k]);
				}
			}
			else
			{
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
						dd = _init->getCoords()[k] - _init2->getCoords()[k];
						dist += (dd*dd);
				}
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
					dd = _init2->getCoords()[k] - _goal2->getCoords()[k];
					dist += (dd*dd);
				}
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
						dd = _goal2->getCoords()[k] - _goal->getCoords()[k];
						dist += (dd*dd);
				}
			}
			return sqrt(dist);
		}
		else
		{
			KthReal dist = 0.0;
			KthReal dd=0.0;
			if( weights != NULL )
			{
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
						dd = from->getCoords()[k] - to->getCoords()[k];
						dist += (dd*dd*weights[k]);
				}
			}
			else
			{
				for(int k=0; k < _wkSpace->getDimension() ; k++)
				{
					dd = from->getCoords()[k] - to->getCoords()[k];
					dist += (dd*dd);
				}
			}
			return sqrt(dist);
		}
		*/
  }
}



