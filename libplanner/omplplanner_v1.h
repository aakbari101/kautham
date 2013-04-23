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
#if !defined(_omplPLANNERV1_H)
#define _omplPLANNERV1_H



#if defined(KAUTHAM_USE_OMPL)
#include <ompl/base/SpaceInformation.h>
#include <ompl/geometric/SimpleSetup.h>
#include <ompl/control/SimpleSetup.h>
#include <ompl/config.h>

#include <ompl/base/spaces/RealVectorStateSpace.h>
namespace ob = ompl::base;
namespace og = ompl::geometric;
namespace oc = ompl::control;


#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
//#include "localplanner.h"
#include "planner.h"


using namespace std;
using namespace libSampling;

namespace libPlanner {
  namespace omplplannerV1{

    bool isStateValid(const ob::State *state, Planner *p);

    class omplPlannerV1:public Planner {
	    public:
        omplPlannerV1(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler,
          WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize);
        ~omplPlannerV1();
        
		bool trySolve();
		bool setParameters();
        //Add public data and functionsvoid
        SoSeparator *getIvCspaceScene();//reimplemented
        void drawCspace();

		protected:
		//Add protected data and functions
        KthReal _planningTime;


        og::SimpleSetupPtr ss;
        ob::StateSpacePtr space;


	    private:
		//Add private data and functions
	  };
  }
}

#endif // KAUTHAM_USE_OMPL
#endif  //_omplPLANNERV1_H

