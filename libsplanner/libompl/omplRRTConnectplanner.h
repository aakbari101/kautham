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

#if !defined(_omplRRTconnectPLANNER_H)
#define _omplRRTconnectPLANNER_H

#if defined(KAUTHAM_USE_OMPL)

#include <ompl/base/SpaceInformation.h>
#include <ompl/geometric/planners/rrt/RRTConnect.h>
#include <ompl/geometric/SimpleSetup.h>
#include <ompl/config.h>

#include <ompl/base/spaces/RealVectorStateSpace.h>
namespace ob = ompl::base;
namespace og = ompl::geometric;


#include <libompl/omplplanner.h>

#include <libproblem/workspace.h>
#include <libsampling/sampling.h>


using namespace std;

namespace Kautham {
/** \addtogroup libPlanner
 *  @{
 */
  namespace omplplanner{
    class omplRRTConnectPlanner:public omplPlanner {
	    public:
        omplRRTConnectPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws, og::SimpleSetup *ssptr);
        ~omplRRTConnectPlanner();

        bool setParameters();

         KthReal _Range;
	  };
  }
  /** @}   end of Doxygen module "libPlanner */
}

#endif // KAUTHAM_USE_OMPL
#endif  //_omplRRTconnectPLANNER_H

