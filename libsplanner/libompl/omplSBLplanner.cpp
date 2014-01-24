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


#if defined(KAUTHAM_USE_OMPL)
#include <libproblem/workspace.h>
#include <libsampling/sampling.h>

#include <boost/bind/mem_fn.hpp>

#include <ompl/base/ProjectionEvaluator.h>
#include <ompl/base/spaces/RealVectorStateProjections.h>

#include "omplSBLplanner.h"
#include "omplValidityChecker.h"


namespace Kautham {
/** \addtogroup libPlanner
 *  @{
 */
  namespace omplplanner{

    //! Constructor
    omplSBLPlanner::omplSBLPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws):
              omplPlanner(stype, init, goal, samples, ws)
    {
        _guiName = "ompl SBL Planner";
        _idName = "omplSBL";


        //create simple setup
        ss = ((og::SimpleSetupPtr) new og::SimpleSetup(space));
        ob::SpaceInformationPtr si=ss->getSpaceInformation();
        //set validity checker
        si->setStateValidityChecker(ob::StateValidityCheckerPtr(new omplplanner::ValidityChecker(si,  (Planner*)this)));
        //ss->setStateValidityChecker(boost::bind(&omplplanner::isStateValid, si.get(),_1, (Planner*)this));

        //alloc valid state sampler
        si->setValidStateSamplerAllocator(boost::bind(&omplplanner::allocValidStateSampler, _1, (Planner*)this));
        //alloc state sampler
        space->setStateSamplerAllocator(boost::bind(&omplplanner::allocStateSampler, _1, (Planner*)this));

        //create planner
        ob::PlannerPtr planner(new og::SBL(si));
        //set planner parameters: range and goalbias
        _Range=0.05;
        //_GoalBias=(planner->as<og::EST>())->getGoalBias();
        addParameter("Range", _Range);
        //addParameter("Goal Bias", _GoalBias);
        planner->as<og::SBL>()->setRange(_Range);
       // planner->as<og::EST>()->setGoalBias(_GoalBias);


        //sets the projection evaluator
        //ob::ProjectionMatrix *pmat;
        //pmat = new ob::ProjectionMatrix;
        //pmat->ComputeRandom(2,1);
        planner->as<og::SBL>()->setProjectionEvaluator(space->getDefaultProjection());


        //set the planner
        ss->setPlanner(planner);
    }

    //! void destructor
    omplSBLPlanner::~omplSBLPlanner(){

    }

    //! setParameters sets the parameters of the planner
    bool omplSBLPlanner::setParameters(){

      omplPlanner::setParameters();
      try{
        HASH_S_K::iterator it = _parameters.find("Range");
        if(it != _parameters.end()){
          _Range = it->second;
          ss->getPlanner()->as<og::SBL>()->setRange(_Range);
         }
        else
          return false;

        //it = _parameters.find("Goal Bias");
        //if(it != _parameters.end()){
        //    _GoalBias = it->second;
          //  ss->getPlanner()->as<og::EST>()->setGoalBias(_GoalBias);
        //}
        //else
          //return false;

      }catch(...){
        return false;
      }
      return true;
    }
  }
  /** @}   end of Doxygen module "libPlanner */
}


#endif // KAUTHAM_USE_OMPL
