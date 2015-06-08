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

#include "omplplanner.h"


#include <Inventor/nodes/SoTranslation.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoLineSet.h>
#include <Inventor/nodes/SoDrawStyle.h>
#include <Inventor/nodes/SoMaterial.h>
#include <Inventor/nodes/SoIndexedLineSet.h>

#include <ompl/base/ProblemDefinition.h>
//#include <ompl/base/OptimizationObjective.h>
//#include <ompl/base/objectives/PathLengthOptimizationObjective.h>
//#include <ompl/base/objectives/MaximizeMinClearanceObjective.h>

namespace Kautham {

//! Namespace omplplanner contains the planners based on the OMPL::geometric library
namespace omplplanner{


/* DEBUG
  double xm=100.0;
  double xM=-100.0;
  double ym=100.0;
  double yM=-100.0;
*/


//for visualization purposes
vector< vector<double> > steersamples;
//int countwithinbounds=0;//for DEBUG

//declaration of class
class weightedRealVectorStateSpace;


/////////////////////////////////////////////////////////////////////////////////////////////////
// weigthedRealVectorStateSpace functions
/////////////////////////////////////////////////////////////////////////////////////////////////
//! The constructor initializes all the weights to 1
weigthedRealVectorStateSpace::weigthedRealVectorStateSpace(unsigned int dim) : RealVectorStateSpace(dim)
{
    //by default set all the weights to 1
    for(unsigned i=0; i<dim; i++)
    {
        weights.push_back(1.0);
    }
}

//! The destructor
weigthedRealVectorStateSpace::~weigthedRealVectorStateSpace(void){}


//! This function sets the values of the weights. The values passed as a parameter are scaled in order not to change the maximim extend of the space
void weigthedRealVectorStateSpace::setWeights(vector<KthReal> w)
{
    double fitFactor;

    //compute the maximum weigthed distance
    double maxweightdist=0.0;
    for(unsigned i=0; i<dimension_; i++)
    {
        double diff = getBounds().getDifference()[i]*w[i];
        maxweightdist += diff * diff;
    }
    maxweightdist = sqrt(maxweightdist);
    //compute the scale factor
    fitFactor = getMaximumExtent()/maxweightdist;
    //set the weights
    for(unsigned i=0; i<dimension_; i++)
    {
        weights[i] = w[i]*fitFactor;
    }
}

//! This function computes the weighted distance between states
double weigthedRealVectorStateSpace::distance(const ob::State *state1, const ob::State *state2) const
{
    double dist = 0.0;
    const double *s1 = static_cast<const StateType*>(state1)->values;
    const double *s2 = static_cast<const StateType*>(state2)->values;

    //JAN DEBUG
    //for (unsigned int i = 0 ; i < 3; ++i)
    for (unsigned int i = 0 ; i < dimension_ ; ++i)
    {
        double diff = ((*s1++) - (*s2++))*weights[i];
        dist += diff * diff;
    }
    return sqrt(dist);
}



/////////////////////////////////////////////////////////////////////////////////////////////////
// KauthamStateSampler functions
/////////////////////////////////////////////////////////////////////////////////////////////////
KauthamStateSampler::KauthamStateSampler(const ob::StateSpace *sspace, Planner *p) : ob::CompoundStateSampler(sspace)
{
    kauthamPlanner_ = p;
    centersmp = NULL;
    _samplerRandom = new RandomSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
    // _samplerHalton = new HaltonSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
}

KauthamStateSampler::~KauthamStateSampler() {
    delete _samplerRandom;
    delete centersmp;
}

void KauthamStateSampler::setCenterSample(ob::State *state, double th)
{
    if(state!=NULL)
    {
        //create sample
        int d = kauthamPlanner_->wkSpace()->getNumRobControls();
        centersmp = new Sample(d);
        //copy the conf of the init smp. Needed to capture the home positions.
        centersmp->setMappedConf(kauthamPlanner_->initSamp()->getMappedConf());
        //load the RobConf of smp form the values of the ompl::state
        ((omplPlanner*)kauthamPlanner_)->omplState2smp(state,centersmp);
    }
    else
        centersmp = NULL;

    //initialize threshold
    threshold = th;
}

void KauthamStateSampler::sampleUniform(ob::State *state)
{

    //Sample around centersmp
    //this does the same as sampleUniformNear, but the Near configuration is set beforehand as "centersmp" configuration.
    //this has been added to modify the behavior of the randombounce walk of the PRM. It used the sampleUniform and
    //we wanted to use the sampleUniformNear

    ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );

    if(centersmp != NULL && threshold > 0.0)
    {
        int trials = 0;
        int maxtrials=100;
        bool found = false;
        int d = kauthamPlanner_->wkSpace()->getNumRobControls();
        Sample *smp = new Sample(d);
        Sample *smp2 = new Sample(d);
        double dist;

        bool withinbounds=false;
        vector<KthReal> deltacoords(d);
        vector<KthReal> coords(d);
        double fraction;
        do{
            /*
                    //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
                    for(int i=0;i<d;i++)
                        coords[i] = rng_.uniformReal(0,1.0);
                    //those controls that are disabled for sampling are now restored to 0.5
                    for(int j=0; j < ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
                        coords[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;

                    //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
                    smp->setCoords(coords);
                    kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
                    withinbounds = smp->getwithinbounds();
                    //if within bounds then check if its within the given distance threshold
                    if(withinbounds)
                    {
                        dist = kauthamPlanner_->wkSpace()->distanceBetweenSamples(*smp,*centersmp,CONFIGSPACE);
                        if(dist < threshold)
                            found = true;
                    }
                    trials ++;
                */

            //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
            for(int i=0;i<d;i++)
                coords[i] = rng_.uniformReal(0,1.0);
            //those controls that are disabled for sampling are now restored to 0.5
            for(unsigned j=0; j < ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
                coords[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;
            //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
            smp2->setCoords(coords);
            kauthamPlanner_->wkSpace()->moveRobotsTo(smp2);
            //interpolate from the centersample towards smp a fraction determined by the threshold

            dist = kauthamPlanner_->wkSpace()->distanceBetweenSamples(*smp2,*centersmp,CONFIGSPACE);
            if(trials==0){
                fraction = rng_.uniformReal(0,1.0)*threshold/dist;
                if(fraction>1.0) fraction=1.0;
            }
            smp = centersmp->interpolate(smp2,fraction);
            kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
            //check
            //double dist1 = kauthamPlanner_->wkSpace()->distanceBetweenSamples(*smp,*centersmp,CONFIGSPACE);

            withinbounds = smp->getwithinbounds();
            if(withinbounds==false) fraction = fraction-fraction/maxtrials;
            else found = true;
            trials ++;
        }while(found==false && trials <maxtrials);
        /*DEBUG
if(coords[0]<xm) xm=coords[0];
if(coords[0]>xM) xM=coords[0];
if(coords[1]<ym) ym=coords[1];
if(coords[1]>yM) yM=coords[1];
*/

        //if(withinbounds){
        //    countwithinbounds++;
        //}

        if(trials==maxtrials)
        {
            //not found within the limits. return the centersmp
            ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(centersmp, &sstate);
        }
        else
        {
            //convert the sample found to scoped state
            ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
        }

        //return in parameter state
        ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
    }
    //sample the whole workspace
    else
    {
        /*
              //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
              int d = kauthamPlanner_->wkSpace()->getNumRobControls();
              vector<KthReal> coords(d);
              for(int i=0;i<d;i++)
                  coords[i] = rng_.uniformReal(0,1.0);

              //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
              Sample *smp = new Sample(d);
              smp->setCoords(coords);
              kauthamPlanner_->wkSpace()->moveRobotsTo(smp);

              //convert from sample to scoped state
              ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
              ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);

              //return in parameter state
             ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
              */

        bool withinbounds=false;
        int trials=0;
        Sample* smp = NULL;
        do{
            //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
            smp = _samplerRandom->nextSample();
            //smp = _samplerHalton->nextSample();

            //those controls that are disabled for sampling are now restored to 0.5
            for(unsigned j=0; j<((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
                smp->getCoords()[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;

            //compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
            kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
            withinbounds = smp->getwithinbounds();
            trials++;
        }while(withinbounds==false && trials<100);


        /*DEBUG
 if(smp->getCoords()[0]<xm) xm=smp->getCoords()[0];
 if(smp->getCoords()[0]>xM) xM=smp->getCoords()[0];
 if(smp->getCoords()[1]<ym) ym=smp->getCoords()[1];
if(smp->getCoords()[1]>yM) yM=smp->getCoords()[1];
*/

        //If trials==100 is because we have not been able to find a sample within limits
        //In this case the config is set to the border in the moveRobotsTo function.
        //The smp is finally converted to state and returned

        //convert from sample to scoped state
        ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
        //return in parameter state
        ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());

    }
    //JAN DEBUG
    //temporary store for visualizaytion purposes
    int dim = kauthamPlanner_->wkSpace()->getNumRobControls();
    Sample* smp2 = new Sample(dim);
    //convert back to a sample to make sure that the state has been well computed
    smp2->setMappedConf(((omplPlanner*)kauthamPlanner_)->initSamp()->getMappedConf());//NEEDED
    ((omplPlanner*)kauthamPlanner_)->omplScopedState2smp(sstate, smp2);

    vector<double> point(3);

    if(kauthamPlanner_->wkSpace()->getRobot(0)->isSE3Enabled())
    {
        point[0] = smp2->getMappedConf()[0].getSE3().getPos()[0];
        point[1] = smp2->getMappedConf()[0].getSE3().getPos()[1];
        point[2] = smp2->getMappedConf()[0].getSE3().getPos()[2];
    }
    else{
        point[0] = smp2->getMappedConf()[0].getRn().getCoordinate(0);
        point[1] = smp2->getMappedConf()[0].getRn().getCoordinate(1);
        if(dim>=3) point[2] = smp2->getMappedConf()[0].getRn().getCoordinate(2);
        else point[2]=0.0;
    }

    steersamples.push_back(point);


}


void KauthamStateSampler::sampleUniformNear(ob::State *state, const ob::State *near, const double distance)
{
    int trials = 0;
    int maxtrials=100;
    bool found = false;
    Sample *smp;
    do{
        bool withinbounds=false;
        int trialsbounds=0;
        do{
            //sample the kautham control space. Controls are defined in the input xml files. Eeach control value lies in the [0,1] interval
            smp = _samplerRandom->nextSample();
            //load the obtained coords to a sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
            kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
            withinbounds = smp->getwithinbounds();
            trialsbounds++;
        }while(withinbounds==false && trialsbounds<100);
        //convert from sample to scoped state
        ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
        ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
        //return the stae in the parameter state and a bool telling if the smp is in collision or not
        ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
        if (((omplPlanner*)kauthamPlanner_)->getSpace()->distance(state,near)> distance)
            found = false;
        else
            found=true;
        trials ++;
    }while(found==false && trials <maxtrials);

    if (!found){
        ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, near);
    }


    //throw ompl::Exception("KauthamValidStateSampler::sampleNear", "not implemented");
    //return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// KauthamValidStateSampler functions
/////////////////////////////////////////////////////////////////////////////////////////////////
//! Creator. The parameter samplername is defaulted to "Random" and the value to "0.0
KauthamValidStateSampler::KauthamValidStateSampler(const ob::SpaceInformation *si, Planner *p) : ob::ValidStateSampler(si)
{
    name_ = "kautham sampler";
    kauthamPlanner_ = p;
    si_ = si;
    //be careful these values should be set somehow!
    int level = 3;
    KthReal sigma = 0.1;

    _samplerRandom = new RandomSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
    _samplerHalton = new HaltonSampler(kauthamPlanner_->wkSpace()->getNumRobControls());
    _samplerSDK = new SDKSampler(kauthamPlanner_->wkSpace()->getNumRobControls(), level);
    _samplerGaussian = new GaussianSampler(kauthamPlanner_->wkSpace()->getNumRobControls(), sigma, kauthamPlanner_->wkSpace());
    _samplerGaussianLike = new GaussianLikeSampler(kauthamPlanner_->wkSpace()->getNumRobControls(), level, kauthamPlanner_->wkSpace());

    _samplerVector.push_back(_samplerRandom);
    _samplerVector.push_back(_samplerHalton);
    _samplerVector.push_back(_samplerSDK);
    _samplerVector.push_back(_samplerGaussian);
    _samplerVector.push_back(_samplerGaussianLike);
}


//!Gets a sample. The samplername parameter is defaulted to Random.
//bool KauthamValidStateSampler::sample(ob::State *state, string samplername)
bool KauthamValidStateSampler::sample(ob::State *state)
{
    //gets a new sample using the sampler specified by the planner
    Sample* smp = NULL;
    unsigned numSampler = ((omplPlanner*)kauthamPlanner_)->getSamplerUsed();
    if(numSampler>= _samplerVector.size()) numSampler = 0;//set default Random sampler if out of bounds value
    smp = _samplerVector[numSampler]->nextSample();

    //those controls that are disabled for sampling are now restored to 0.5
    for(unsigned j=0; j<((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
        smp->getCoords()[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;

    /*DEBUG
   if(smp->getCoords()[0]<xm) xm=smp->getCoords()[0];
   if(smp->getCoords()[0]>xM) xM=smp->getCoords()[0];
   if(smp->getCoords()[1]<ym) ym=smp->getCoords()[1];
   if(smp->getCoords()[1]>yM) yM=smp->getCoords()[1];

   */
    //computes the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
    kauthamPlanner_->wkSpace()->moveRobotsTo(smp);

    //convert from sample to scoped state
    ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
    ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);

    //return the stae in the parameter state and a bool telling if the smp is in collision or not
    ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());

    if(  (si_->satisfiesBounds(state)==false) || (kauthamPlanner_->wkSpace()->collisionCheck(smp)) )
        return false;
    return true;
}

//!Gets a sample near a given state, after several trials (retruns false if not found)
bool KauthamValidStateSampler::sampleNear(ob::State *state, const ob::State *near, const double distance)
{
    int trials = 0;
    int maxtrials=100;
    bool found = false;
    do{
        //get a random sample, and compute the mapped configurations (i.e.se3+Rn values) by calling MoveRobotsto function.
        Sample* smp = NULL;
        int numSampler = 0; //Random sampler
        smp = _samplerVector[numSampler]->nextSample();

        //those controls that are disabled for sampling are now restored to 0.5
        for(unsigned j=0; j<((omplPlanner*)kauthamPlanner_)->getDisabledControls()->size(); j++)
            smp->getCoords()[ ((omplPlanner*)kauthamPlanner_)->getDisabledControls()->at(j) ] = 0.5;


        kauthamPlanner_->wkSpace()->moveRobotsTo(smp);
        //convert from sample to scoped state
        ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
        ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);
        //return the stae in the parameter state and a bool telling if the smp is in collision or not
        ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
        if( kauthamPlanner_->wkSpace()->collisionCheck(smp) | (((omplPlanner*)kauthamPlanner_)->getSpace()->distance(state,near)> distance) | !(si_->satisfiesBounds(state)))
            found = false;
        else
            found=true;
        trials ++;
    }while(found==false && trials <maxtrials);
    return found;
    //throw ompl::Exception("KauthamValidStateSampler::sampleNear", "not implemented");
    //return false;
}


/////////////////////////////////////////////////////////////////////////////////////////////////
// AUXILIAR functions
/////////////////////////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////////////////////////
//! This function is used to allocate a state sampler
ob::StateSamplerPtr allocStateSampler(const ob::StateSpace *mysspace, Planner *p)
{
    return ob::StateSamplerPtr(new KauthamStateSampler(mysspace, p));

    /*
      //Create sampler
      ob::StateSamplerPtr globalSampler(new ob::CompoundStateSampler(mysspace));
      //weights defined for when sampling near a state
      //they are now all set to 1.0. To be explored later...
      double weightImportanceRobots; //weight of robot i
      double weightSO3; //rotational weight
      double weightR3; //translational weight
      double weightSE3; //weight of the monile base
      double weightRn; //weight of the chain
      //loop for all robots
      for(int i=0; i<p->wkSpace()->getNumRobots(); i++)
      {
          weightImportanceRobots = 1.0; //all robots weight the same

          //Create sampler for robot i
          //ssRoboti is the subspace corresponding to robot i
          ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) mysspace->as<ob::CompoundStateSpace>()->getSubspace(i));
          ob::StateSamplerPtr samplerRoboti(new ob::CompoundStateSampler(ssRoboti.get()));

          int numsubspace=0;
          //If SE3 workspace exisits
          if(p->wkSpace()->getRobot(i)->isSE3Enabled())
          {
              //ssRoboti_sub is the subspace corresponding to the SE3 part of robot i
              ob::StateSpacePtr ssRoboti_sub_SE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(numsubspace));
              numsubspace++;
              //the sampler is a compound sampler
              ob::StateSamplerPtr samplerRoboti_SE3(new ob::CompoundStateSampler(ssRoboti_sub_SE3.get()));
              //ssRoboti_sub_R3 is the R3 subspace of robot i
              ob::StateSpacePtr ssRoboti_sub_R3  = ((ob::StateSpacePtr) ssRoboti_sub_SE3->as<ob::SE3StateSpace>()->getSubspace(0));
              //add the sampler of the R3 part
              weightR3=1.0;
              ob::StateSamplerPtr samplerRoboti_R3(new ob::RealVectorStateSampler(ssRoboti_sub_R3.get()));
              ((ob::CompoundStateSampler*) samplerRoboti_SE3.get())->addSampler(samplerRoboti_R3, weightR3);
              //ssRoboti_sub_SO3 is the SO3 subspace of robot i
              ob::StateSpacePtr ssRoboti_sub_SO3 = ((ob::StateSpacePtr) ssRoboti_sub_SE3->as<ob::SE3StateSpace>()->getSubspace(1));
              //add the sampler of the SO3 part
              weightSO3=1.0;
              ob::StateSamplerPtr samplerRoboti_SO3(new ob::SO3StateSampler(ssRoboti_sub_SO3.get()));
              ((ob::CompoundStateSampler*) samplerRoboti_SE3.get())->addSampler(samplerRoboti_SO3, weightSO3);
              //add the compound sampler of the SE3 part
              weightSE3 = 1.0;
              ((ob::CompoundStateSampler*) samplerRoboti.get())->addSampler(samplerRoboti_SE3, weightSE3);
          }
          //If Rn state space exisits
          if(p->wkSpace()->getRobot(i)->getNumJoints()>0)
          {
              //ssRoboti_sub is the subspace corresponding to the Rn part of robot i
              ob::StateSpacePtr ssRoboti_sub_Rn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(numsubspace));
              //add the sampler of the Rn part
              weightRn = 1.0;
              ob::StateSamplerPtr samplerRoboti_Rn(new ob::RealVectorStateSampler(ssRoboti_sub_Rn.get()));
              ((ob::CompoundStateSampler*) samplerRoboti.get())->addSampler(samplerRoboti_Rn, weightRn);
          }
          //add the sampler of robot i to global sampler
          ((ob::CompoundStateSampler*) globalSampler.get())->addSampler(samplerRoboti, weightImportanceRobots);
      }

      return globalSampler;
*/

}

/////////////////////////////////////////////////////////////////////////////////////////////////
//! This function is used to allocate a valid state sampler
ob::ValidStateSamplerPtr allocValidStateSampler(const ob::SpaceInformation *si, Planner *p)
{
    return ob::ValidStateSamplerPtr(new KauthamValidStateSampler(si, p));
}

//  /////////////////////////////////////////////////////////////////////////////////////////////////
//  //! This function converts a state to a smp and tests if it is in collision or not
//  bool isStateValid(const ob::SpaceInformation *si, const ob::State *state, Planner *p)
//  {
//      //verify bounds
//      if(si->satisfiesBounds(state)==false)
//          return false;
//      //create sample
//      int d = p->wkSpace()->getNumRobControls();
//      Sample *smp = new Sample(d);
//      //copy the conf of the init smp. Needed to capture the home positions.
//      smp->setMappedConf(p->initSamp()->getMappedConf());
//      //load the RobConf of smp form the values of the ompl::state
//      ((omplPlanner*)p)->omplState2smp(state,smp);
//      //collision-check
//      if( p->wkSpace()->collisionCheck(smp) )
//          return false;
//      return true;
//  }




/////////////////////////////////////////////////////////////////////////////////////////////////
// omplPlanner functions
/////////////////////////////////////////////////////////////////////////////////////////////////
//! Constructor
omplPlanner::omplPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, WorkSpace *ws, og::SimpleSetup *ssptr):
    Planner(stype, init, goal, samples, ws)
{
    _family = OMPLPLANNER;
    //set intial values from parent class data
    _speedFactor = 1;
    _solved = false;
    _guiName = "ompl Planner";
    _idName = "ompl Planner";

    _samplerUsed = 0;

    //set own intial values
    _planningTime = 10;
    _simplify = 2;//by default shorten and smooth
    _incremental=0;//by default makes a clear before any new call to solve in function trysolve().
    _drawnrobot=0; //by default we draw the cspace of robot 0.

    //add planner parameters
    addParameter("Incremental (0/1)", _incremental);
    addParameter("Max Planning Time", _planningTime);
    addParameter("Speed Factor", _speedFactor);
    addParameter("Simplify Solution", _simplify);
    addParameter("Cspace Drawn", _drawnrobot);


    if (ssptr == NULL) {
        //Construct the state space we are planning in. It is a compound state space composed of a compound state space for each robot
        //Each robot has a compound state space composed of a (oprional) SE3 state space and a (optional) Rn state space
        vector<ob::StateSpacePtr> spaceRn;
        vector<ob::StateSpacePtr> spaceSE3;
        vector<ob::StateSpacePtr> spaceRob;
        vector< double > weights;

        spaceRn.resize(_wkSpace->getNumRobots());
        spaceSE3.resize(_wkSpace->getNumRobots());
        spaceRob.resize(_wkSpace->getNumRobots());
        weights.resize(_wkSpace->getNumRobots());

        //loop for all robots
        for(unsigned i=0; i<_wkSpace->getNumRobots(); i++)
        {
            vector<ob::StateSpacePtr> compoundspaceRob;
            vector< double > weightsRob;
            std::stringstream sstm;

            //create state space SE3 for the mobile base, if necessary
            if(_wkSpace->getRobot(i)->isSE3Enabled())
            {
                //create the SE3 state space
                spaceSE3[i] = ((ob::StateSpacePtr) new ob::SE3StateSpace());
                sstm << "ssRobot" << i<<"_SE3";
                spaceSE3[i]->setName(sstm.str());

                //set the bounds. If the bounds are equal or its difference is below a given epsilon value (0.001) then
                //set the higher bound to the lower bound plus this eplsion
                ob::RealVectorBounds bounds(3);

                //x-direction
                double low = _wkSpace->getRobot(i)->getLimits(0)[0];
                double high = _wkSpace->getRobot(i)->getLimits(0)[1];
                filterBounds(low, high, 0.001);
                bounds.setLow(0, low);
                bounds.setHigh(0, high);

                //y-direction
                low = _wkSpace->getRobot(i)->getLimits(1)[0];
                high = _wkSpace->getRobot(i)->getLimits(1)[1];
                filterBounds(low, high, 0.001);
                bounds.setLow(1, low);
                bounds.setHigh(1, high);

                //z-direction
                low = _wkSpace->getRobot(i)->getLimits(2)[0];
                high = _wkSpace->getRobot(i)->getLimits(2)[1];
                filterBounds(low, high, 0.001);
                bounds.setLow(2, low);
                bounds.setHigh(2, high);

                spaceSE3[i]->as<ob::SE3StateSpace>()->setBounds(bounds);

                //create projections evaluator for this spaces -
                //The default projections (needed for some planners) and
                //the projections called "drawprojections"for the drawcspace function.
                //(the use of defaultProjections for drawspace did not succeed because the ss->setup() calls registerProjections() function that
                //for the case of RealVectorStateSpace sets the projections as random for dim>2 and identity otherwise, thus
                //resetting what the user could have tried to do.
                ob::ProjectionEvaluatorPtr peR3; //projection for R3
                peR3 = (ob::ProjectionEvaluatorPtr) new ob::RealVectorIdentityProjectionEvaluator(spaceSE3[i]->as<ob::SE3StateSpace>()->getSubspace(0));
                peR3->setup();//??
                spaceSE3[i]->as<ob::SE3StateSpace>()->getSubspace(0)->registerProjection("drawprojection",peR3);
                spaceSE3[i]->as<ob::SE3StateSpace>()->getSubspace(0)->registerDefaultProjection(peR3);
                ob::ProjectionEvaluatorPtr peSE3; //projection for SE3
                ob::ProjectionEvaluatorPtr projToUse = spaceSE3[i]->as<ob::CompoundStateSpace>()->getSubspace(0)->getProjection("drawprojection");
                peSE3 = (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*spaceSE3[i],0,projToUse);
                peSE3->setup(); //necessary to set projToUse as theprojection
                spaceSE3[i]->registerProjection("drawprojection",peSE3);
                spaceSE3[i]->registerDefaultProjection(peSE3);

                //sets the weights between translation and rotation
                spaceSE3[i]->as<ob::SE3StateSpace>()->setSubspaceWeight(0,_wkSpace->getRobot(i)->getWeightSE3()[0]);//translational weight
                spaceSE3[i]->as<ob::SE3StateSpace>()->setSubspaceWeight(1,_wkSpace->getRobot(i)->getWeightSE3()[1]);//rotational weight

                //load to the compound state space of robot i
                compoundspaceRob.push_back(spaceSE3[i]);
                weightsRob.push_back(1);
            }

            //create the Rn state space for the kinematic chain, if necessary
            int nj = _wkSpace->getRobot(i)->getNumJoints();
            if(nj>0)
            {
                //create the Rn state space
                spaceRn[i] = ((ob::StateSpacePtr) new weigthedRealVectorStateSpace(nj));
                sstm << "ssRobot" << i<<"_Rn";
                spaceRn[i]->setName(sstm.str());

                //create projections evaluator for this spaces
                ob::ProjectionEvaluatorPtr peRn;
                peRn = ((ob::ProjectionEvaluatorPtr) new ob::RealVectorIdentityProjectionEvaluator(spaceRn[i]));
                peRn->setup();
                spaceRn[i]->registerProjection("drawprojection",peRn);
                spaceRn[i]->registerDefaultProjection(peRn);

                // set the bounds and the weights
                vector<KthReal> jointweights;
                ob::RealVectorBounds bounds(nj);
                double low, high;
                for(int j=0; j<nj;j++)
                {
                    //the limits of joint j between link j and link (j+1) are stroed in the data structure of link (j+1)
                    low = *_wkSpace->getRobot(i)->getLink(j+1)->getLimits(true);
                    high = *_wkSpace->getRobot(i)->getLink(j+1)->getLimits(false);
                    filterBounds(low, high, 0.001);
                    bounds.setLow(j, low);
                    bounds.setHigh(j, high);
                    //the weights
                    jointweights.push_back(_wkSpace->getRobot(i)->getLink(j+1)->getWeight());
                }
                spaceRn[i]->as<weigthedRealVectorStateSpace>()->setBounds(bounds);
                spaceRn[i]->as<weigthedRealVectorStateSpace>()->setWeights(jointweights);

                //load to the compound state space of robot i
                compoundspaceRob.push_back(spaceRn[i]);
                weightsRob.push_back(1);
            }
            //the compound state space for robot i is (SE3xRn), and either SE3 or Rn may be missing
            spaceRob[i] = ((ob::StateSpacePtr) new ob::CompoundStateSpace(compoundspaceRob,weightsRob));
            weights[i] = 1;
            sstm.str("");
            sstm << "ssRobot" << i;
            spaceRob[i]->setName(sstm.str());

            ob::ProjectionEvaluatorPtr peRob;
            ob::ProjectionEvaluatorPtr projToUse = spaceRob[i]->as<ob::CompoundStateSpace>()->getSubspace(0)->getProjection("drawprojection");
            peRob = (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*spaceRob[i],0,projToUse);
            peRob->setup();
            spaceRob[i]->registerProjection("drawprojection",peRob);
            spaceRob[i]->registerDefaultProjection(peRob);
        }
        //the state space for the set of robots. All the robots have the same weight.
        space = ((ob::StateSpacePtr) new ob::CompoundStateSpace(spaceRob,weights));

        /*
        ob::ProjectionEvaluatorPtr peSpace;
        ob::ProjectionEvaluatorPtr projToUse = space->as<ob::CompoundStateSpace>()->getSubspace(0)->getProjection("drawprojection");
        peSpace = (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*space,0,projToUse);
        peSpace->setup();
        space->registerProjection("drawprojection",peSpace);
        space->registerDefaultProjection(peSpace);
        */
        vector<ob::ProjectionEvaluatorPtr> peSpace;
        for(unsigned i=0; i<_wkSpace->getNumRobots();i++)
        {
            ob::ProjectionEvaluatorPtr projToUse = space->as<ob::CompoundStateSpace>()->getSubspace(i)->getProjection("drawprojection");
            peSpace.push_back( (ob::ProjectionEvaluatorPtr) new ob::SubspaceProjectionEvaluator(&*space,i,projToUse) );
            peSpace[i]->setup();
            string projname = "drawprojection"; //
            string robotnumber = static_cast<ostringstream*>( &(ostringstream() << i) )->str();//the string correspoding to number i
            projname.append(robotnumber); //the name of the projection: "drawprojection0", "drawprojection1",...
            space->registerProjection(projname.c_str(),peSpace[i]);
        }
        space->registerDefaultProjection(peSpace[0]);//the one corresponding to the first robot is set as default

        //create simple setup
        ss = ((og::SimpleSetupPtr) new og::SimpleSetup(space));
        si=ss->getSpaceInformation();
        //set validity checker
        si->setStateValidityChecker(ob::StateValidityCheckerPtr(new ValidityChecker(si,  (Planner*)this)));
        //ss->setStateValidityChecker(boost::bind(&omplplanner::isStateValid, si.get(),_1, (Planner*)this));

        //Start state: convert from smp to scoped state
        ob::ScopedState<ob::CompoundStateSpace> startompl(space);
        smp2omplScopedState(_init, &startompl);
        cout<<"startompl:"<<endl;
        startompl.print();

        //Goal state: convert from smp to scoped state
        ob::ScopedState<ob::CompoundStateSpace> goalompl(space);
        smp2omplScopedState(_goal, &goalompl);
        cout<<"goalompl:"<<endl;
        goalompl.print();

        // set the start and goal states
        ss->setStartAndGoalStates(startompl, goalompl);
    } else {
        ss = (og::SimpleSetupPtr)ssptr;
        si = ss->getSpaceInformation();
        space = ss->getStateSpace();
    }
}

//! void destructor
omplPlanner::~omplPlanner(){

}

/*!
     * disablePMDControlsFromSampling disables from sampling those controls that have the PMD in its name.
     * \param enableall if its true the function is used to enable all, It is defaulted to false.
     */
void omplPlanner::disablePMDControlsFromSampling(bool enableall)
{
    _disabledcontrols.clear();
    //enable all
    if(enableall)
    {
        return;
    }
    //else diable those that are called PMD


    string listcontrolsname = wkSpace()->getRobControlsName();
    vector<string*> controlname;
    string *newcontrol = new string;
    for(unsigned i=0; i<listcontrolsname.length();i++)
    {
        if(listcontrolsname[i]=='|')
        {
            controlname.push_back(newcontrol);
            newcontrol = new string;
        }
        else
            newcontrol->push_back(listcontrolsname[i]);
    }
    //add last control (since listcontrolsname does not end with a |)
    controlname.push_back(newcontrol);

    for(unsigned i=0;i<controlname.size();i++)
    {
        if(controlname[i]->find("PMD") != string::npos)
        {
            //Load to the diable vector for disabling sampling. We do not want to sample coupled controls.
            //JAN DEBUG: commented next line
            _disabledcontrols.push_back(i);
        }
        //JAN DEBUG - added next line: add the non PMD controls to be disabled
        //else _disabledcontrols.push_back(i);
    }
}


//! This function setParameters sets the parameters of the planner
bool omplPlanner::setParameters(){
    try{
        HASH_S_K::iterator it = _parameters.find("Speed Factor");
        if(it != _parameters.end())
            _speedFactor = it->second;
        else
            return false;

        it = _parameters.find("Max Planning Time");
        if(it != _parameters.end())
            _planningTime = it->second;
        else
            return false;


        it = _parameters.find("Cspace Drawn");
        if(it != _parameters.end()){
            if(it->second < 0 || it->second >= _wkSpace->getNumRobots()) {
                _drawnrobot = 0;
                setParameter("Cspace Drawn",0);
            } else {
                _drawnrobot = it->second;
            }
        }
        else
            return false;

        it = _parameters.find("Simplify Solution");
        if(it != _parameters.end())
        {
            if(it->second==0) _simplify=0;
            else if(it->second==1) _simplify=1;
            else _simplify=2;
        }
        else
            return false;

        it = _parameters.find("Incremental (0/1)");
        if (it != _parameters.end()) {
            _incremental = (it->second == 1);
        }
        else
            return false;

    }catch(...){
        return false;
    }
    return true;
}

//! This function is used to verify that the low bound is below the high bound
void omplPlanner::filterBounds(double &l, double &h, double epsilon)
{
    if((h - l) < epsilon) h = l + epsilon;
}

//! This function creates the separator for the ivscene to show the configuration space.
SoSeparator *omplPlanner::getIvCspaceScene()
{
    _sceneCspace = new SoSeparator();
    _sceneCspace->ref();
    return Planner::getIvCspaceScene();
}


//! This routine allows to draw the 2D projection of a roadmap or tree.
//! The one corresponding to robot number numrob is drawn.
void omplPlanner::drawCspace(unsigned int robot, unsigned int link) {
    if (!_sceneCspace) return;


    //Delete whatever is already drawn
    while (_sceneCspace->getNumChildren() > 0) {
        _sceneCspace->removeChild(0);
    }


    //Get the subspace
    ob::StateSpacePtr stateSpace(space->as<ob::CompoundStateSpace>()->getSubspace(robot)->
                                 as<ob::CompoundStateSpace>()->getSubspace(link));


    //Set space bounds
    unsigned int k;
    KthReal xmin, xmax, ymin, ymax, zmin, zmax;
    if (_wkSpace->getRobot(robot)->isSE3Enabled()) {
        k = stateSpace->as<ob::SE3StateSpace>()->getDimension();

        xmin = stateSpace->as<ob::SE3StateSpace>()->getBounds().low[0];
        xmax = stateSpace->as<ob::SE3StateSpace>()->getBounds().high[0];
        ymin = stateSpace->as<ob::SE3StateSpace>()->getBounds().low[1];
        ymax = stateSpace->as<ob::SE3StateSpace>()->getBounds().high[1];
        zmin = stateSpace->as<ob::SE3StateSpace>()->getBounds().low[2];
        zmax = stateSpace->as<ob::SE3StateSpace>()->getBounds().high[2];
    } else {
        k = stateSpace->as<ob::RealVectorStateSpace>()->getDimension();

        xmin = stateSpace->as<ob::RealVectorStateSpace>()->getBounds().low[0];
        xmax = stateSpace->as<ob::RealVectorStateSpace>()->getBounds().high[0];
        ymin = stateSpace->as<ob::RealVectorStateSpace>()->getBounds().low[1];
        ymax = stateSpace->as<ob::RealVectorStateSpace>()->getBounds().high[1];
        if (k > 2) {
            zmin = stateSpace->as<ob::RealVectorStateSpace>()->getBounds().low[2];
            zmax = stateSpace->as<ob::RealVectorStateSpace>()->getBounds().high[2];
        } else {
            zmin = -FLT_MIN;
            xmax =  FLT_MIN;
        }
    }


    //Use the projection associated to the subspace of the robot passed as a parameter.
    string projectionName = "drawprojection" + static_cast<ostringstream*>
            (&(ostringstream() << robot))->str();
    ob::ProjectionEvaluatorPtr projection(space->getProjection(projectionName));
    ob::EuclideanProjection state(k);


    //Draw path
    if (_solved) {
        SoDrawStyle *drawStyle(new SoDrawStyle);
        drawStyle->lineWidth = 4.;

        std::vector<ob::State*> &pathStates(ss->getSolutionPath().getStates());
        float vertices[pathStates.size()][3];
        for (unsigned int i = 0; i < pathStates.size(); ++i) {
            projection->project(pathStates.at(i),state);

            vertices[i][0] = state[0];
            vertices[i][1] = state[1];
            if (k > 2) {
                vertices[i][2] = state[2];
            } else {
                vertices[i][2] = 0.;
            }
        }

        SoVertexProperty *vertexProperty(new SoVertexProperty);
        vertexProperty->vertex.setValues(0,pathStates.size(),vertices);
        vertexProperty->orderedRGBA.setValue(SbColor(1.,0.,0.5).getPackedValue());
        vertexProperty->materialBinding.setValue(SoVertexProperty::OVERALL);

        SoLineSet *lineSet(new SoLineSet);
        lineSet->vertexProperty.setValue(vertexProperty);

        SoSeparator *path(new SoSeparator);
        path->addChild(drawStyle);
        path->addChild(lineSet);

        _sceneCspace->addChild(path);
    }


    //Load the planner data to be drawn
    ob::PlannerDataPtr pdata(new ob::PlannerData(ss->getSpaceInformation()));
    ss->getPlanner()->getPlannerData(*pdata);
    if (ss->getPlanner()->getProblemDefinition()->hasOptimizationObjective()) {
        pdata->computeEdgeWeights(*ss->getPlanner()->getProblemDefinition()->getOptimizationObjective());
    } else {
        pdata->computeEdgeWeights();
    }


    //Draw tree
    SoDrawStyle *drawStyle(new SoDrawStyle);
    drawStyle->lineWidth = 1.;
    drawStyle->pointSize = 3.;

    bool bidirectional = (_idName == "omplRRTConnect") || (_idName == "omplTRRTConnect") ||
            (_idName == "omplFOSTRRTConnect");
    int32_t materialIndices[pdata->numEdges()];
    float vertices[pdata->numVertices()][3];
    int32_t coordIndices[3*pdata->numEdges()];
    std::vector<unsigned int> outgoingVertices;
    unsigned int j = 0;
    for (unsigned int i = 0; i < pdata->numVertices(); ++i) {
        projection->project(pdata->getVertex(i).getState(),state);

        vertices[i][0] = state[0];
        vertices[i][1] = state[1];
        if (k > 2) {
            vertices[i][2] = state[2];
        } else {
            vertices[i][2] = 0.;
        }

        pdata->getEdges(i,outgoingVertices);
        for (std::vector<unsigned int>::const_iterator it = outgoingVertices.begin();
             it != outgoingVertices.end(); ++it) {
            coordIndices[3*j+0] = i;
            coordIndices[3*j+1] = *it;
            coordIndices[3*j+2] = SO_END_LINE_INDEX;

            //ob::Cost edgeWeight;
            //pdata->getEdgeWeight(i,*it,&edgeWeight);

            if (bidirectional) {
                //Start tree vertices have tag = 1,goal tree vertices tag = 2 and the connection point tag = 0
                if (pdata->getVertex(i).getTag() == 0) {
                    materialIndices[j] = pdata->getVertex(*it).getTag()-1;
                } else {
                    materialIndices[j] = pdata->getVertex(i).getTag()-1;
                }
            }

            j++;
        }
    }

    SoVertexProperty *vertexProperty(new SoVertexProperty);
    vertexProperty->vertex.setValues(0,pdata->numVertices(),vertices);
    vertexProperty->orderedRGBA.setValue(SbColor(1.,1.,1.f).getPackedValue());
    vertexProperty->materialBinding.setValue(SoVertexProperty::OVERALL);

    SoPointSet *pointSet(new SoPointSet);
    pointSet->vertexProperty.setValue(vertexProperty);

    SoSeparator *points(new SoSeparator);
    points->addChild(drawStyle);
    points->addChild(pointSet);

    _sceneCspace->addChild(points);

    vertexProperty = new SoVertexProperty;
    vertexProperty->vertex.setValues(0,pdata->numVertices(),vertices);
    if (bidirectional) {
        uint32_t colors[2] = {SbColor(0.,1.,0.).getPackedValue(),SbColor(0.,0.5,1.).getPackedValue()};
        vertexProperty->orderedRGBA.setValues(0,2,colors);
        vertexProperty->materialBinding.setValue(SoVertexProperty::PER_FACE_INDEXED);
    } else {
        vertexProperty->orderedRGBA.setValue(SbColor(0.,1.,0.).getPackedValue());
        vertexProperty->materialBinding.setValue(SoVertexProperty::OVERALL);
    }

    SoIndexedLineSet *lineSet(new SoIndexedLineSet);
    lineSet->coordIndex.setValues(0,3*pdata->numEdges(),coordIndices);
    lineSet->vertexProperty.setValue(vertexProperty);
    if (bidirectional) {
        lineSet->materialIndex.setValues(0,pdata->numEdges(),materialIndices);
    }

    SoSeparator *lines(new SoSeparator);
    lines->addChild(drawStyle);
    lines->addChild(lineSet);

    _sceneCspace->addChild(lines);


    //Draw bounds
    SoMaterial *material(new SoMaterial);
    material->diffuseColor.setValue(0.2,0.2,0.2);
    material->transparency.setValue(0.3);

    SoTranslation *translation(new SoTranslation);
    translation->translation.setValue((xmax+xmin)/2.,(ymax+ymin)/2.,(zmax+zmin)/2.);

    SoCube *cube(new SoCube);
    cube->width = xmax-xmin;
    cube->height = ymax-ymin;
    cube->depth = zmax-zmin;

    SoSeparator *bounds(new SoSeparator);
    bounds->addChild(material);
    bounds->addChild(translation);
    bounds->addChild(cube);

    _sceneCspace->addChild(bounds);
}

//! This function converts a Kautham sample to an ompl scoped state.
void omplPlanner::smp2omplScopedState(Sample* smp, ob::ScopedState<ob::CompoundStateSpace> *sstate)
{
    //Extract the mapped configuration of the sample. It is a vector with as many components as robots.
    //each component has the RobConf of the robot (the SE3 and the Rn configurations)
    if(smp->getMappedConf().size()==0)
    {
        _wkSpace->moveRobotsTo(smp); // to set the mapped configuration
    }
    std::vector<RobConf>& smpRobotsConf = smp->getMappedConf();


    //loop for all the robots
    for(unsigned i=0; i<_wkSpace->getNumRobots(); i++)
    {
        int k=0; //counter of subspaces contained in subspace of robot i

        //get the subspace of robot i
        ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(i));
        string ssRobotiname = ssRoboti->getName();

        //if it has se3 part
        if(_wkSpace->getRobot(i)->isSE3Enabled())
        {
            //get the kautham SE3 configuration
            SE3Conf c = smpRobotsConf.at(i).getSE3();
            vector<KthReal>& pp = c.getPos();
            vector<KthReal>& aa = c.getAxisAngle();

            //set the ompl SE3 configuration
            ob::StateSpacePtr ssRobotiSE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));
            string ssRobotiSE3name = ssRobotiSE3->getName();

            ob::ScopedState<ob::SE3StateSpace> cstart(ssRobotiSE3);
            cstart->setX(pp[0]);
            cstart->setY(pp[1]);
            cstart->setZ(pp[2]);
            cstart->rotation().setAxisAngle(aa[0],aa[1],aa[2],aa[3]);

            //load the global scoped state with the info of the se3 data of robot i
            (*sstate)<<cstart;
            k++;
        }

        //has Rn part
        if(_wkSpace->getRobot(i)->getNumJoints()>0)
        {
            //get the kautham Rn configuration
            RnConf r = smpRobotsConf.at(i).getRn();

            //set the ompl Rn configuration
            ob::StateSpacePtr ssRobotiRn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));
            ob::ScopedState<weigthedRealVectorStateSpace> rstart(ssRobotiRn);

            for(unsigned j=0; j<_wkSpace->getRobot(i)->getNumJoints();j++)
                rstart->values[j] = r.getCoordinate(j);

            //cout<<"sstate[0]="<<rstart->values[0]<<"sstate[1]="<<rstart->values[1]<<endl;


            //load the global scoped state with the info of the Rn data of robot i
            (*sstate) << rstart;
            k++;//dummy
        }
    }
}

//! This member function converts an ompl State to a Kautham sample
void omplPlanner::omplState2smp(const ob::State *state, Sample* smp)
{
    ob::ScopedState<ob::CompoundStateSpace> sstate(space);
    sstate = *state;
    omplScopedState2smp( sstate, smp);
}

//! This member function converts an ompl ScopedState to a Kautham sample
void omplPlanner::omplScopedState2smp(ob::ScopedState<ob::CompoundStateSpace> sstate, Sample* smp)
{
    vector<RobConf> rc;

    //Loop for all the robots
    for (unsigned int i = 0; i < _wkSpace->getNumRobots(); ++i) {
        //RobConf to store the robots configurations read from the ompl state
        RobConf *rcj = new RobConf;

        //Get the subspace corresponding to robot i
        ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr)space->as<ob::CompoundStateSpace>()->getSubspace(i));

        //Get the SE3 subspace of robot i, if it exists, extract the SE3 configuration
        unsigned int k = 0; //counter of subspaces of robot i
        if (_wkSpace->getRobot(i)->isSE3Enabled()) {
            //Get the SE3 subspace of robot i
            ob::StateSpacePtr ssRobotiSE3 = ((ob::StateSpacePtr)ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));

            //Create a SE3 scoped state and load it with the data extracted from the global scoped state
            ob::ScopedState<ob::SE3StateSpace> pathscopedstatese3(ssRobotiSE3);
            sstate >> pathscopedstatese3;

            //Convert it to a vector of 7 components
            vector<KthReal> se3coords;
            se3coords.resize(7);
            se3coords[0] = pathscopedstatese3->getX();
            se3coords[1] = pathscopedstatese3->getY();
            se3coords[2] = pathscopedstatese3->getZ();
            se3coords[3] = pathscopedstatese3->rotation().x;
            se3coords[4] = pathscopedstatese3->rotation().y;
            se3coords[5] = pathscopedstatese3->rotation().z;
            se3coords[6] = pathscopedstatese3->rotation().w;

            //Create the sample
            SE3Conf se3;
            se3.setCoordinates(se3coords);
            rcj->setSE3(se3);

            k++;
        } else {
            //If the robot does not have mobile SE3 dofs then the SE3 configuration of the sample is maintained
            if (smp->getMappedConf().size() == 0) {
                throw ompl::Exception("omplPlanner::omplScopedState2smp", "parameter smp must be a sample with the MappedConf");
            } else {
                rcj->setSE3(smp->getMappedConf()[i].getSE3());
            }
        }

        //Get the Rn subspace of robot i, if it exisits, and extract the Rn configuration
        if (_wkSpace->getRobot(i)->getNumJoints() > 0) {
            //Get the Rn subspace of robot i
            ob::StateSpacePtr ssRobotiRn = ((ob::StateSpacePtr)ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));

            //Create a Rn scoped state and load it with the data extracted from the global scoped state
            ob::ScopedState<weigthedRealVectorStateSpace> pathscopedstateRn(ssRobotiRn);
            sstate >> pathscopedstateRn;

            //Convert it to a vector of n components
            vector<KthReal> coords;
            for (unsigned int j = 0; j < _wkSpace->getRobot(i)->getNumJoints(); ++j){
                coords.push_back(pathscopedstateRn->values[j]);
            }
            rcj->setRn(coords);

            k++;//dummy
        } else {
            //If the robot does not have mobile Rn dofs then the Rn configuration of the sample is maintained
            if (smp->getMappedConf().size() == 0) {
                throw ompl::Exception("omplPlanner::omplScopedState2smp", "parameter smp must be a sample with the MappedConf");
            } else {
                rcj->setRn(smp->getMappedConf()[i].getRn());
            }
        }

        //Load the RobConf with the data of robot i
        rc.push_back(*rcj);
    }
    //create the sample with the RobConf
    //the coords (controls) of the sample are kept void
    smp->setMappedConf(rc);
}


//! function to find a solution path
bool omplPlanner::trySolve()
{

    /* DEBUG
 xm=100.0;
 xM=-100.0;
 ym=100.0;
 yM=-100.0;
*/
    //JAN DEBUG
    //double stats;
    //if(steersamples.size())
    //    stats=((double)(countwithinbounds))/steersamples.size();
    //else stats=-1;
    steersamples.clear();
    //countwithinbounds=0;


    //Start state: convert from smp to scoped state
    ob::ScopedState<ob::CompoundStateSpace> startompl(space);
    smp2omplScopedState(_init, &startompl);
    cout<<"startompl:"<<endl;
    startompl.print();

    //Goal state: convert from smp to scoped state
    ob::ScopedState<ob::CompoundStateSpace> goalompl(space);
    smp2omplScopedState(_goal, &goalompl);
    cout<<"goalompl:"<<endl;
    goalompl.print();

    // set the start and goal states
    ss->setStartAndGoalStates(startompl, goalompl);


    //remove previous solutions, if any
    if (_incremental) {
        ss->getProblemDefinition()->clearSolutionPaths();
    } else {
        ss->clear();
        ss->getPlanner()->clear();
    }

    // attempt to solve the problem within _planningTime seconds of planning time
    ss->setup();

    ob::PlannerStatus solved = ss->solve(_planningTime);

    //ss->print();

    //retrieve all the states. Load the SampleSet _samples
    ob::PlannerData data(ss->getSpaceInformation());
    ss->getPlannerData(data);
    /*
         for(int i=0; i<data.numVertices();i++)
         {
                smp=new Sample(_wkSpace->getNumRobControls());
                smp->setMappedConf(_init->getMappedConf());//copy the conf of the start smp
                omplState2smp(data.getVertex(i).getState(), smp);
                _samples->add(smp);
         }
         */

    if (solved==ob::PlannerStatus::EXACT_SOLUTION)
    {
        std::cout << "\nEXACT_SOLUTION found" << std::endl;

        // print the path to screen
        if(_simplify==1) {//smooth
            //ss->getPathSimplifier()->shortcutPath(ss->getSolutionPath(),0,0,0.15);
            ss->getPathSimplifier()->smoothBSpline(ss->getSolutionPath(),5);
        }
        else if(_simplify==2) {//shorten and smoot
            ss->simplifySolution();
        }
        //std::cout<<"Path: ";
        //ss->getSolutionPath().print(std::cout);

        ss->getSolutionPath().interpolate();

        //std::cout<<"Path after interpolation: ";
        //ss->getSolutionPath().interpolate(10);
        //ss->getSolutionPath().print(std::cout);


        //refine
        //ss->getSolutionPath().interpolate();

        Sample *smp;

        _path.clear();
        clearSimulationPath();



        //load the kautham _path variable from the ompl solution
        for(unsigned j=0;j<ss->getSolutionPath().getStateCount();j++){
            //create a smp and load the RobConf of the init configuration (to have the same if the state does not changi it)
            smp=new Sample(_wkSpace->getNumRobControls());
            smp->setMappedConf(_init->getMappedConf());
            //convert form state to smp
            omplState2smp(ss->getSolutionPath().getState(j)->as<ob::CompoundStateSpace::StateType>(), smp);

            _path.push_back(smp);
            _samples->add(smp);
        }
        _solved = true;
        drawCspace(_drawnrobot);
        return _solved;
    }
    //solution not found
    else if (solved==ob::PlannerStatus::APPROXIMATE_SOLUTION){
        std::cout << "APPROXIMATE_SOLUTION - No exact solution found" << std::endl;
        std::cout<<"Difference = "<<ss->getProblemDefinition()->getSolutions().at(0).difference_<< std::endl;
        _solved = false;
        drawCspace(_drawnrobot);
        return _solved;
    }
    else if (solved==ob::PlannerStatus::TIMEOUT){
        std::cout << "TIMEOUT - No solution found" << std::endl;
        _solved = false;
        drawCspace(_drawnrobot);
        return _solved;
    }
    else{
        std::cout << "solve returned "<<solved<< "- No exact solution found" << std::endl;
        _solved = false;
        drawCspace(_drawnrobot);
        return _solved;
    }
}

}//close namespace ompl
}//close namespace kautham


#endif // KAUTHAM_USE_OMPL


