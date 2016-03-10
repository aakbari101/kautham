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

/* Author: Joan Fontanals Martinez, Muhayyuddin */


#if defined(KAUTHAM_USE_OMPL)
#if defined(KAUTHAM_USE_ODE)
#if !defined(_KauthamOpenDEplanner_H)
#define _KauthamOpenDEplanner_H
#define dDOUBLE
#include <ompl/control/planners/rrt/RRT.h>
#include <ompl/control/SimpleSetup.h>
#include <ompl/control/spaces/RealVectorControlSpace.h>
#include <ompl/control/SpaceInformation.h>
#include <ompl/config.h>
#include <ompl/control/PathControl.h>
#include<ompl/control/Control.h>

#include <ompl/base/spaces/RealVectorStateSpace.h>
//#include "omplcplanner.h"
#include <problem/workspace.h>
#include <sampling/sampling.h>

#include <ompl/extensions/opende/OpenDESimpleSetup.h>
#include <ompl/extensions/opende/OpenDEControlSpace.h>
#include <ompl/extensions/opende/OpenDEStateSpace.h>
#include <ompl/extensions/opende/OpenDESimpleSetup.h>
#include <ompl/extensions/opende/OpenDEStatePropagator.h>
#include <ompl/extensions/opende/OpenDEStateValidityChecker.h>
#include <ompl/base/goals/GoalRegion.h>
#include <ompl/config.h>
#include <iostream>

#include <ode/ode.h>
#include "../goal/KauthamDEGoalRegion.h"
#include"../goal/KauthamDEGoalSamplableRegion.h"
#include "planner/planner.h"
#include "sampling/state.h"
#include "../environment/KauthamOpenDEEnvironment.h"
//#include "KauthamOpenDERRTTX90Planner.h"

//#include <ompl/control/planners/ltl/LTLProblemDefinition.h>
//#include<ompl/control/planners/ltl/LTLPlanner.h>


#define _USE_MATH_DEFINES

namespace ob = ompl::base;
namespace og = ompl::geometric;
namespace oc = ompl::control;

using namespace std;

namespace Kautham {
/** \addtogroup Planner
 *  @{
 */
namespace omplcplanner{


/////////////////////////////////////////////////////////////////////////////////////////////////
// Class KauthamOpenDEPlanner
/////////////////////////////////////////////////////////////////////////////////////////////////

/*! KauthanDEplanner is the base class for all the planner that use the dynamic enviroment for planning.
 * All the planners will be drived from this class and reimplement the trysolve function.
 */
class KauthamDEPlanner: public Planner
{

public:

    KthReal _propagationStepSize; //!< Define the step size of the world.
    KthReal _maxspeed; //!< describe the max. speed of motors.
    bool _onlyend; //!< describe that only TCP will move of complete robot.
    KthReal _planningTime; //!< describe the max. planning time.
    KthReal _maxContacts;
    KthReal _minControlSteps;
    KthReal _maxControlSteps;
    KthReal _controlDimensions;
    KthReal _erp;
    KthReal _cfm;
    ob::StateSpacePtr stateSpacePtr; //!< state space pointer to KauthamDEStateSpace.
    oc::OpenDEEnvironmentPtr envPtr; //!< pointer to KauthamDE enviroment.
    oc::OpenDEStateSpace *stateSpace; //!< pointer to kauthamDEStatespace.
    //oc::ControlSamplerPtr controlSamplerptr;

    oc::ControlSpacePtr csp;
    oc::OpenDESimpleSetup *ss;
    oc::SimpleSetupPtr ss1;

    vector<State>  worldState;
   // int m;
    std::vector<Sample*> Rob;
    std::vector<Sample*> Obs;
    int _drawnrobot; //!< Index of the robot whose Cspace is drawn. Defaults to 0.
    double Action;
   std::vector<double> JerkIndex;
double PowerConsumed;
double Smoothness;
    //! The constructor will define all the necessary parameters for planning.
    KauthamDEPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws);
    ~KauthamDEPlanner();
    virtual bool trySolve();//!< Compute the path and returns the boolean value.
    bool setParameters();//!< set the planning parameters.
    void KauthamOpenDEState2Robsmp(const ob::State *state, Sample* smp,const oc::Control *control, const double duration, std::vector<double> *Angle,std::vector<float> Ang);
    void KauthamOpenDEState2Obssmp(const ob::State *state, Sample* smp,const oc::Control *control,const double duration);
    //void omplScopedState2smp( const ob::ScopedState<oc::OpenDEStateSpace> sstate, Sample* smp);
    void moveAlongPath(unsigned int step);
    SoSeparator *getIvCspaceScene();//reimplemented
    void drawCspace(int numrob=0);
    void ComputeAction(const std::vector<ob::State*> &states, const std::vector<oc::Control*> &control, const std::vector<double> duration);
    void ComputeJerkIndex(const std::vector<ob::State*> &states, const std::vector<double> duration);
    void ComputePowerConsumed(const std::vector<ob::State*> &states,const std::vector<oc::Control*> &control, const std::vector<double> duration);
    bool computePath(oc::OpenDESimpleSetup *ssetup, ob::RealVectorBounds vb,ob::RealVectorBounds bounds, double x, double y,double planningTime);
    ompl::control::PathControl *RectMotion();

    typedef struct
     {
         KthReal objectposition[3];
         KthReal objectorientation[4];
     }KauthamDEobject;
    vector<KauthamDEobject> smp2KauthamOpenDEState(WorkSpace *wkSpace,Sample *smp);
    typedef struct
    {
    std::vector<ob::State*> substates;
    std::vector<oc::Control*> control;
    std::vector<double> duration;
    }solutionStates;

    std::vector<solutionStates> sStates;
    std::vector< vector<float> >  JointAngle;
//    oc::LTLProblemDefinitionPtr pDefp;
//    oc::LTLSpaceInformationPtr ltlsi;
//    oc::LTLPlanner* ltlplanner;
    std::string PROBTYPE;

    typedef struct
       {
       std::vector<double> pose;
       }goalPoses;


};

}
 /** @}   end of Doxygen module "Planner */
}

#endif  //_KauthamOpenDEplanner_H
#endif  //KAUTHAM_USE_ODE
#endif // KAUTHAM_USE_OMPL

