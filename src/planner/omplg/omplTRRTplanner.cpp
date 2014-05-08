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

/* Author: Alexander Perez, Jan Rosell, Nestor Garcia Hidalgo */

 

#if defined(KAUTHAM_USE_OMPL)
#include <problem/workspace.h>
#include <sampling/sampling.h>

#include <boost/bind/mem_fn.hpp>

#include "omplTRRTplanner.h"
#include "omplValidityChecker.h"



namespace Kautham {
  namespace omplplanner{

	//! Constructor
    omplTRRTPlanner::omplTRRTPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws, og::SimpleSetup *ssptr):
              omplPlanner(stype, init, goal, samples, ws, ssptr)
	{
        _guiName = "ompl TRRT Planner";
        _idName = "omplTRRT";


        //alloc valid state sampler
        si->setValidStateSamplerAllocator(boost::bind(&omplplanner::allocValidStateSampler, _1, (Planner*)this));
        //alloc state sampler
        space->setStateSamplerAllocator(boost::bind(&omplplanner::allocStateSampler, _1, (Planner*)this));

        //create planner
        ob::PlannerPtr planner(new og::TRRT(si));
        //set planner parameters: range and goalbias
        _Range=0.05;
        _GoalBias=(planner->as<og::TRRT>())->getGoalBias();
        _maxStatesFailed = (planner->as<og::TRRT>())->getMaxStatesFailed();
        _tempChangeFactor = (planner->as<og::TRRT>())->getTempChangeFactor();
        _frontierThreshold = (planner->as<og::TRRT>())->getFrontierThreshold();
        _frontierNodesRatio = (planner->as<og::TRRT>())->getFrontierNodeRatio();

        addParameter("Range", _Range);
        addParameter("Goal Bias", _GoalBias);
        addParameter("Max States Failed", _maxStatesFailed);
        addParameter("T Change Factor", _tempChangeFactor);
        addParameter("Frontier Threshold", _frontierThreshold);
        addParameter("Frontier Nodes Ratio", _frontierNodesRatio);

        planner->as<og::TRRT>()->setRange(_Range);
        planner->as<og::TRRT>()->setGoalBias(_GoalBias);

        //set the planner
        ss->setPlanner(planner);
    }

	//! void destructor
    omplTRRTPlanner::~omplTRRTPlanner(){
			
	}
	
	//! setParameters sets the parameters of the planner
    bool omplTRRTPlanner::setParameters(){

      omplPlanner::setParameters();
      try{
        HASH_S_K::iterator it = _parameters.find("Range");
        if(it != _parameters.end()){
          _Range = it->second;
          ss->getPlanner()->as<og::TRRT>()->setRange(_Range);
         }
        else
          return false;

        it = _parameters.find("Goal Bias");
        if(it != _parameters.end()){
            _GoalBias = it->second;
            ss->getPlanner()->as<og::TRRT>()->setGoalBias(_GoalBias);
        }
        else
          return false;

        it = _parameters.find("Max States Failed");
        if(it != _parameters.end()){
            _maxStatesFailed = it->second;
            ss->getPlanner()->as<og::TRRT>()->setMaxStatesFailed(_maxStatesFailed);
        }
        else
          return false;

        it = _parameters.find("T Change Factor");
        if(it != _parameters.end()){
            _tempChangeFactor = it->second;
            ss->getPlanner()->as<og::TRRT>()->setTempChangeFactor(_tempChangeFactor);
        }
        else
          return false;

        it = _parameters.find("Frontier Threshold");
        if(it != _parameters.end()){
            _frontierThreshold = it->second;
            ss->getPlanner()->as<og::TRRT>()->setFrontierThreshold(_frontierThreshold);
        }
        else
          return false;

        it = _parameters.find("Frontier Nodes Ratio");
        if(it != _parameters.end()){
            _frontierNodesRatio = it->second;
            ss->getPlanner()->as<og::TRRT>()->setFrontierNodeRatio(_frontierNodesRatio);
        }
        else
          return false;

      }catch(...){
        return false;
      }
      return true;
    }
  }
}


#endif // KAUTHAM_USE_OMPL