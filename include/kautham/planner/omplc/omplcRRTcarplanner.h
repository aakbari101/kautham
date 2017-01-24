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

#if !defined(_omplcRRTcarPLANNER_H)
#define _omplcRRTcarPLANNER_H


#if defined(KAUTHAM_USE_OMPL)
#include <ompl/control/planners/rrt/RRT.h>
#include <ompl/control/SimpleSetup.h>
#include <ompl/control/spaces/RealVectorControlSpace.h>
#include <ompl/control/SpaceInformation.h>
#include <ompl/config.h>

#include <ompl/base/spaces/RealVectorStateSpace.h>
namespace ob = ompl::base;
namespace oc = ompl::control;


#include "omplcplanner.h"

#include <problem/workspace.h>
#include <sampling/sampling.h>


using namespace std;

namespace Kautham {
/** \addtogroup Planner
 *  @{
 */

  namespace omplcplanner{
    class omplcRRTcarPlanner:public omplcPlanner {
	    public:
        omplcRRTcarPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws);
        ~omplcRRTcarPlanner();

        bool setParameters();

         KthReal _GoalBias;
         double _propagationStepSize;
         unsigned int _duration;
         double _controlBound_Tras;
         double _controlBound_Rot;
         int _onlyForward;
         double _carLength;
	  };
  }
  /** @}   end of Doxygen module "Planner */
}

#endif // KAUTHAM_USE_OMPL
#endif  //_omplcRRTcarPLANNER_H
