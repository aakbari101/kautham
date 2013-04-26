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

#include "localplanner.h"
#include "omplplanner.h"


#include <Inventor/nodes/SoTransform.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoCube.h>
#include <Inventor/nodes/SoPointSet.h>
#include <Inventor/nodes/SoLineSet.h>


using namespace libSampling;

namespace libPlanner {
  namespace omplplanner{

class weightedRealVectorStateSpace;

  bool isStateValid(const ob::State *state, Planner *p)//, Sample *smp)
  {

      int d = p->wkSpace()->getDimension();
      Sample *smp = new Sample(d);
      //copy the conf of the init smp. Needed to capture the home positions.
      smp->setMappedConf(p->initSamp()->getMappedConf());
      //load the RobConf of smp form the values of the ompl::state
      ((omplPlanner*)p)->omplState2smp(state,smp);
      if( p->wkSpace()->collisionCheck(smp) )
          return false;
      return true;
  }


  ////////////////////////////////////////////
  class weigthedRealVectorStateSpace:public ob::RealVectorStateSpace {
    public:
      vector<KthReal> weights;

      ~weigthedRealVectorStateSpace(void){};

      weigthedRealVectorStateSpace(unsigned int dim=0) : RealVectorStateSpace(dim)
      {
          for(int i=0; i<dim; i++)
          {
              weights.push_back(1.0);
          }
      };


      void setWeights(vector<KthReal> w)
      {
          double fitFactor; //factor to force weighted distance to be within the getExtend (determined by the bounds of the RealVectorStateSpace)

          double maxweightdist=0.0;
          for(int i=0; i<dimension_; i++)
          {
              double diff = getBounds().getDifference()[i]*w[i];
              maxweightdist += diff * diff;
          }
          maxweightdist = sqrt(maxweightdist);
          fitFactor = getMaximumExtent()/maxweightdist;

          //set the weights
          for(int i=0; i<dimension_; i++)
          {
              weights[i] = w[i]*fitFactor;
          }
      };

      double distance(const ob::State *state1, const ob::State *state2) const
      {
         double dist = 0.0;
         const double *s1 = static_cast<const StateType*>(state1)->values;
         const double *s2 = static_cast<const StateType*>(state2)->values;

        for (unsigned int i = 0 ; i < dimension_ ; ++i)
        {
            double diff = ((*s1++) - (*s2++))*weights[i];
            dist += diff * diff;
        }
        return sqrt(dist);
      };
};

//////////////////////////////////////////////////////



  class KauthamStateSampler : public ob::CompoundStateSampler //public ob::RealVectorStateSampler //
  {
  public:
      KauthamStateSampler(const ob::StateSpace *sspace, Planner *p) : ob::CompoundStateSampler(sspace) //ob::RealVectorStateSampler(sspace) //
      //  KauthamStateSampler(const ob::StateSpace *sspace) : ob::RealVectorStateSampler(sspace)
      {
          kauthamPlanner_ = p;
          //s_ =  ((ob::StateSamplerPtr) new ob::CompoundStateSampler(sspace));
          //s_->addSampler(const StateSamplerPtr &sampler, double weightImportance)
      }

      virtual void sampleUniform(ob::State *state)
      {

          //s_->sampleUniform(state);


          int d = kauthamPlanner_->wkSpace()->getDimension();
          vector<KthReal> coords(d);
          for(int i=0;i<d;i++)
            coords[i] = rng_.uniformReal(0,1.0);

          Sample *smp = new Sample(d);
          smp->setCoords(coords);
          kauthamPlanner_->wkSpace()->moveRobotsTo(smp);

          ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
          ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);

          ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());
      }

  protected:
      ompl::RNG rng_;
      Planner *kauthamPlanner_;
  };

  class KauthamValidStateSampler : public ob::ValidStateSampler
  {
  public:
      KauthamValidStateSampler(const ob::SpaceInformation *si, Planner *p) : ob::ValidStateSampler(si)
      {
          name_ = "kautham sampler";
          kauthamPlanner_ = p;
      }

      virtual bool sample(ob::State *state)
      {

          int d = kauthamPlanner_->wkSpace()->getDimension();
          vector<KthReal> coords(d);
          for(int i=0;i<d;i++)
            coords[i] = rng_.uniformReal(0,1.0);

          Sample *smp = new Sample(d);
          smp->setCoords(coords);
          kauthamPlanner_->wkSpace()->moveRobotsTo(smp);

          ob::ScopedState<ob::CompoundStateSpace> sstate(  ((omplPlanner*)kauthamPlanner_)->getSpace() );
          ((omplPlanner*)kauthamPlanner_)->smp2omplScopedState(smp, &sstate);


          ((omplPlanner*)kauthamPlanner_)->getSpace()->copyState(state, sstate.get());

          if( kauthamPlanner_->wkSpace()->collisionCheck(smp) )
              return false;
          return true;

      }
      // We don't need this in the example below.
      virtual bool sampleNear(ob::State *state, const ob::State *near, const double distance)
      {
          throw ompl::Exception("KauthamValidStateSampler::sampleNear", "not implemented");
          return false;
      }

  protected:
      ompl::RNG rng_;
      Planner *kauthamPlanner_;
     // ob::ValidStateSamplerPtr us_;
  };


  // return an instance of my sampler
  ob::StateSamplerPtr allocStateSampler(const ob::StateSpace *mysspace, Planner *p)
  {
      return ob::StateSamplerPtr(new KauthamStateSampler(mysspace, p));
  }

  // return an instance of my sampler
  ob::ValidStateSamplerPtr allocValidStateSampler(const ob::SpaceInformation *si, Planner *p)
  {
      return ob::ValidStateSamplerPtr(new KauthamValidStateSampler(si, p));
  }


  void omplPlanner::filterBounds(double &l, double &h, double epsilon)
  {
      if((h - l) < epsilon) h = l + epsilon;
  }

	//! Constructor
    omplPlanner::omplPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize):
              Planner(stype, init, goal, samples, sampler, ws, lcPlan, ssize)
	{
		//set intial values
        _planningTime = 10;

		//set intial values from parent class data
		_speedFactor = 1;
        _solved = false;
        _stepSize = ssize;
	  
        _guiName = "ompl Planner";
        _idName = "ompl Planner";
        addParameter("Step Size", ssize);
        addParameter("Speed Factor", _speedFactor);
        addParameter("Max Planning Time", _planningTime);

        // construct the state space we are planning in
        vector<ob::StateSpacePtr> spaceRn;
        vector<ob::StateSpacePtr> spaceSE3;
        vector<ob::StateSpacePtr> spaceRob;
        vector< double > weights;

        spaceRn.resize(_wkSpace->robotsCount());
        spaceSE3.resize(_wkSpace->robotsCount());
        spaceRob.resize(_wkSpace->robotsCount());
        weights.resize(_wkSpace->robotsCount());


        for(int i=0; i<_wkSpace->robotsCount(); i++)
        {

            vector<ob::StateSpacePtr> compoundspaceRob;
            vector< double > weightsRob;
            std::stringstream sstm;

            //create state space SE3 for the mobile base, if necessary
            if(_wkSpace->getRobot(i)->isSE3Enabled())
            {
                spaceSE3[i] = ((ob::StateSpacePtr) new ob::SE3StateSpace());

                sstm << "ssRobot" << i<<"_SE3";
                spaceSE3[i]->setName(sstm.str());
                // set the bounds

                ob::RealVectorBounds bounds(3);

                double low = _wkSpace->getRobot(i)->getLimits(0)[0];
                double high = _wkSpace->getRobot(i)->getLimits(0)[1];
                filterBounds(low, high, 0.001);
                bounds.setLow(0, low);
                bounds.setHigh(0, high);

                low = _wkSpace->getRobot(i)->getLimits(1)[0];
                high = _wkSpace->getRobot(i)->getLimits(1)[1];
                filterBounds(low, high, 0.001);
                bounds.setLow(1, low);
                bounds.setHigh(1, high);

                low = _wkSpace->getRobot(i)->getLimits(2)[0];
                high = _wkSpace->getRobot(i)->getLimits(2)[1];
                filterBounds(low, high, 0.001);
                bounds.setLow(2, low);
                bounds.setHigh(2, high);

                spaceSE3[i]->as<ob::SE3StateSpace>()->setBounds(bounds);

                //sets the weights between translation and rotation
                spaceSE3[i]->as<ob::SE3StateSpace>()->setSubspaceWeight(0,_wkSpace->getRobot(i)->getWeightSE3()[0]);//translational weight
                spaceSE3[i]->as<ob::SE3StateSpace>()->setSubspaceWeight(1,_wkSpace->getRobot(i)->getWeightSE3()[1]);//rotational weight

                compoundspaceRob.push_back(spaceSE3[i]);
                weightsRob.push_back(1);
            }

            //create the Rn state space for the kinematic chain, if necessary
            int nj = _wkSpace->getRobot(i)->getNumJoints();
            if(nj>0)
            {
                spaceRn[i] = ((ob::StateSpacePtr) new weigthedRealVectorStateSpace(nj));
                sstm << "ssRobot" << i<<"_Rn";
                spaceRn[i]->setName(sstm.str());
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



                compoundspaceRob.push_back(spaceRn[i]);
                weightsRob.push_back(1);
            }
            //the compound state space (SE3xRn), either SE3 or Rn may be missing
            spaceRob[i] = ((ob::StateSpacePtr) new ob::CompoundStateSpace(compoundspaceRob,weightsRob));
            weights[i] = 1;
            sstm.str("");
            sstm << "ssRobot" << i;
            spaceRob[i]->setName(sstm.str());
        }

        //the state space for the set of robots
        space = ((ob::StateSpacePtr) new ob::CompoundStateSpace(spaceRob,weights));

        //The derived classes will create a planner,
        //the simplesetup and call the setStateValididyChecker function
    }

	//! void destructor
    omplPlanner::~omplPlanner(){
			
	}
	
	//! setParameters sets the parameters of the planner
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

        it = _parameters.find("Step Size");
        if(it != _parameters.end())
             _stepSize = it->second;
        else
          return false;

      }catch(...){
        return false;
      }
      return true;
    }

  	
    SoSeparator *omplPlanner::getIvCspaceScene()
    {
        if(_wkSpace->getDimension()==2)
        {
            //_sceneCspace = ((IVWorkSpace*)_wkSpace)->getIvScene();
            _sceneCspace = new SoSeparator();
        }
        else _sceneCspace=NULL;
        return Planner::getIvCspaceScene();
    }


    //This routine allows to draw the roadmap or tree for a sigle robot with 2 dof
    void omplPlanner::drawCspace()
    {
        if(_sceneCspace==NULL) return;

        if(_wkSpace->getDimension()==2)
        {
            //drawCspaceRn();
            drawCspaceSE3();
        }
    }


    //This routine allows to draw the roadmap or tree for a sigle robot with 2 dof
    void omplPlanner::drawCspaceSE3()
    {
            //first delete whatever is already drawn
            while (_sceneCspace->getNumChildren() > 0)
            {
                _sceneCspace->removeChild(0);
            }


            //draw points
            SoSeparator *psep = new SoSeparator();
            SoCoordinate3 *points  = new SoCoordinate3();
            SoPointSet *pset  = new SoPointSet();

            const ob::RealVectorStateSpace::StateType *pos;
            const ob::SE3StateSpace::StateType *se3state;
            ob::ScopedState<ob::CompoundStateSpace> pathscopedstate(space);

            //KthReal xmin=100000000.0;
            //KthReal xmax=-100000000.0;
            //KthReal ymin=100000000.0;
            //KthReal ymax=-100000000.0;


            ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(0));
            ob::StateSpacePtr ssRobotiSE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(0));


            KthReal xmin=ssRobotiSE3->as<ob::SE3StateSpace>()->getBounds().low[0];
            KthReal xmax=ssRobotiSE3->as<ob::SE3StateSpace>()->getBounds().high[0];
            KthReal ymin=ssRobotiSE3->as<ob::SE3StateSpace>()->getBounds().low[1];
            KthReal ymax=ssRobotiSE3->as<ob::SE3StateSpace>()->getBounds().high[1];

            KthReal x,y;

            ob::PlannerDataPtr pdata;
            pdata = ((ob::PlannerDataPtr) new ob::PlannerData(ss->getSpaceInformation()));
            ss->getPlanner()->getPlannerData(*pdata);

            for(int i=0;i<pdata->numVertices();i++)
            {

                pathscopedstate = pdata->getVertex(i).getState()->as<ob::CompoundStateSpace::StateType>();
                ob::ScopedState<ob::SE3StateSpace> pathscopedstatese3(ssRobotiSE3);
                pathscopedstate >> pathscopedstatese3;
                x = pathscopedstatese3->getX();
                y = pathscopedstatese3->getY();
                //ob::SE3StateSpace::StateType *pse3 = pathscopedstatese3.get();

                /*
                se3state = pdata->getVertex(i).getState()->as<ob::SE3StateSpace::StateType>();
                pos = se3state->as<ob::RealVectorStateSpace::StateType>(0);
                x=pos->values[0];
                y=pos->values[1];
                double xx=pos->values[0];
                double  yy=pos->values[1];
                */

                points->point.set1Value(i,x,y,0);

                //if(x<xmin) xmin=x;
                //if(x>xmax) xmax=x;
                //if(y<ymin) ymin=y;
                //if(y>ymax) ymax=y;
            }

            SoDrawStyle *pstyle = new SoDrawStyle;
            pstyle->pointSize = 2;
            SoMaterial *color = new SoMaterial;
            color->diffuseColor.setValue(0.2,0.8,0.2);

            //draw samples
            psep->addChild(color);
            psep->addChild(points);
            psep->addChild(pstyle);
            psep->addChild(pset);

            _sceneCspace->addChild(psep);


            //draw edges:
            SoSeparator *lsep = new SoSeparator();

            int numOutgoingEdges;

            //std::map< unsigned int, const ob::PlannerDataEdge * > edgeMap;
            std::vector< unsigned int > outgoingVertices;

            for(int i=0;i<pdata->numVertices();i++)
            {
                //numOutgoingEdges = plannerdata->getEdges (i, edgeMap);
                //std::map< unsigned int, const ob::PlannerDataEdge * >::iterator it;
                //for ( it=edgeMap.begin(); it != edgeMap.end(); it++ ){

                numOutgoingEdges = pdata->getEdges (i, outgoingVertices);
                for ( int j=0; j<numOutgoingEdges; j++ ){

                  SoCoordinate3 *edgepoints  = new SoCoordinate3();

                  //edgepoints->point.set1Value((0,points->point[i]);
                  //edgepoints->point.set1Value(1,points->point[(*it).first]);
                  //edgepoints->point.set1Value(1,points->point[outgoingVertices.at(j)]);


                  float x1,y1,x2,y2,z;
                  pathscopedstate = pdata->getVertex(i).getState()->as<ob::CompoundStateSpace::StateType>();
                  ob::ScopedState<ob::SE3StateSpace> pathscopedstatese3(ssRobotiSE3);
                  pathscopedstate >> pathscopedstatese3;
                  x1 = pathscopedstatese3->getX();
                  y1 = pathscopedstatese3->getY();
                  z=0.0;
                  edgepoints->point.set1Value(0,x1,y1,z);

                  pathscopedstate = pdata->getVertex(outgoingVertices.at(j)).getState()->as<ob::CompoundStateSpace::StateType>();
                  pathscopedstate >> pathscopedstatese3;
                  x2 = pathscopedstatese3->getX();
                  y2 = pathscopedstatese3->getY();
                  edgepoints->point.set1Value(1,x2,y2,z);


                  //cout<<"i:"<<i<<" j:"<<outgoingVertices.at(j)<<" weight:"<<plannerdata->getEdgeWeight(i,outgoingVertices.at(j))<<endl;

                  lsep->addChild(edgepoints);

                  SoLineSet *ls = new SoLineSet;
                  ls->numVertices.set1Value(0,2);//two values
                  //cout<<"EDGE "<<(*itC)->first<<" "<<(*itC)->second<<endl;
                  lsep->addChild(ls);
                }
            }
            _sceneCspace->addChild(lsep);

            //draw path:
            if(_solved)
            {
                SoSeparator *pathsep = new SoSeparator();
                std::vector< ob::State * > & pathstates = ss->getSolutionPath().getStates();

                for(int i=0; i<ss->getSolutionPath().getStateCount()-1; i++)
                {
                    SoCoordinate3 *edgepoints  = new SoCoordinate3();
                    pathscopedstate = pathstates[i]->as<ob::CompoundStateSpace::StateType>();
                    ob::ScopedState<ob::SE3StateSpace> pathscopedstatese3(ssRobotiSE3);
                    pathscopedstate >> pathscopedstatese3;
                    x = pathscopedstatese3->getX();
                    y = pathscopedstatese3->getY();
                    edgepoints->point.set1Value(0,x,y,0);

                    pathscopedstate = pathstates[i+1]->as<ob::CompoundStateSpace::StateType>();
                    pathscopedstate >> pathscopedstatese3;
                    x = pathscopedstatese3->getX();
                    y = pathscopedstatese3->getY();
                    edgepoints->point.set1Value(1,x,y,0);

                    pathsep->addChild(edgepoints);

                    SoLineSet *ls = new SoLineSet;
                    ls->numVertices.set1Value(0,2);//two values
                    SoDrawStyle *lstyle = new SoDrawStyle;
                    lstyle->lineWidth=2;
                    SoMaterial *path_color = new SoMaterial;
                    path_color->diffuseColor.setValue(0.8,0.2,0.2);
                    pathsep->addChild(path_color);
                    pathsep->addChild(lstyle);
                    pathsep->addChild(ls);
                }
                _sceneCspace->addChild(pathsep);
            }


            //draw floor
            SoSeparator *floorsep = new SoSeparator();
            SoCube *cs = new SoCube();
            cs->width = xmax-xmin;
            cs->depth = (xmax-xmin)/50.0;
            cs->height = ymax-ymin;

            SoTransform *cub_transf = new SoTransform;
            SbVec3f centre;
            centre.setValue(xmin+(xmax-xmin)/2,ymin+(ymax-ymin)/2,-cs->depth.getValue());
            cub_transf->translation.setValue(centre);
            cub_transf->recenter(centre);

            SoMaterial *cub_color = new SoMaterial;
            cub_color->diffuseColor.setValue(0.2,0.2,0.2);

            floorsep->addChild(cub_color);
            floorsep->addChild(cub_transf);
            floorsep->addChild(cs);
            _sceneCspace->addChild(floorsep);

            //cout<<"xmin:"<<xmin<<" xmax: "<<xmax<<endl;
            //cout<<"ymin:"<<ymin<<" ymax: "<<ymax<<endl;

            //plannerdata->printGraphviz(outGraphviz);
            //plannerdata->printGraphviz(cout);

    }


    //This routine allows to draw the roadmap or tree for a sigle robot with 2 dof
    void omplPlanner::drawCspaceRn()
    {


        //           pos = pdata->getVertex(i).getState()->as<ob::RealVectorStateSpace::StateType>()


    }


    void omplPlanner::smp2omplScopedState(Sample* smp, ob::ScopedState<ob::CompoundStateSpace> *sstate)
    {

        std::vector<RobConf>& smpRobotsConf = smp->getMappedConf();
        for(int i=0; i<_wkSpace->robotsCount(); i++)
        {
            int k=0; //counter of subspaces contained in subspace of robot i

            //ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(i)->as<ob::CompoundStateSpace>());
            ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(i));
            string ssRobotiname = ssRoboti->getName();

            //has se3 part
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

                (*sstate)<<cstart;
                k++;
            }

            //has Rn part
            if(_wkSpace->getRobot(i)->getNumJoints()>0)
            {
                //get the kautham Rn configuration
                RnConf r = smpRobotsConf.at(i).getRn();

                //set the omple Rn configuration
                ob::StateSpacePtr ssRobotiRn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));
                ob::ScopedState<weigthedRealVectorStateSpace> rstart(ssRobotiRn);

                for(int j=0; j<_wkSpace->getRobot(i)->getNumJoints();j++)
                {
                    rstart->values[j] = r.getCoordinate(j);
                    double rr = r.getCoordinate(j);
                    double dd=rstart->values[j];
                    int hh;
                    hh=0;
                }
                (*sstate) << rstart;
                k++;//dummy
            }
        }
    }



    void omplPlanner::omplState2smp(const ob::State *state, Sample* smp)
    {
        ob::ScopedState<ob::CompoundStateSpace> sstate(space);
        sstate = *state;
        omplScopedState2smp( sstate, smp);
    }

    void omplPlanner::omplScopedState2smp(ob::ScopedState<ob::CompoundStateSpace> sstate, Sample* smp)
    {
        int k=0;
        vector<RobConf> rc;
        for(int i=0; i<_wkSpace->robotsCount(); i++)
        {
            RobConf *rcj = new RobConf;


            ob::StateSpacePtr ssRoboti = ((ob::StateSpacePtr) space->as<ob::CompoundStateSpace>()->getSubspace(i));

             int k=0;
             if(_wkSpace->getRobot(i)->isSE3Enabled())
             {
                 ob::StateSpacePtr ssRobotiSE3 =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));

                 ob::ScopedState<ob::SE3StateSpace> pathscopedstatese3(ssRobotiSE3);
                 sstate >> pathscopedstatese3;

                 vector<KthReal> se3coords;
                 se3coords.resize(7);
                 se3coords[0] = pathscopedstatese3->getX();
                 se3coords[1] = pathscopedstatese3->getY();
                 se3coords[2] = pathscopedstatese3->getZ();
                 se3coords[3] = pathscopedstatese3->rotation().x;
                 se3coords[4] = pathscopedstatese3->rotation().y;
                 se3coords[5] = pathscopedstatese3->rotation().z;
                 se3coords[6] = pathscopedstatese3->rotation().w;
                 SE3Conf se3;
                 se3.setCoordinates(se3coords);
                 rcj->setSE3(se3);
                 k++;
             }
             else
             {
                 rcj->setSE3(smp->getMappedConf()[i].getSE3());
             }
             if(_wkSpace->getRobot(i)->getNumJoints()>0)
             {
                 ob::StateSpacePtr ssRobotiRn =  ((ob::StateSpacePtr) ssRoboti->as<ob::CompoundStateSpace>()->getSubspace(k));

                 ob::ScopedState<weigthedRealVectorStateSpace> pathscopedstateRn(ssRobotiRn);
                 sstate >> pathscopedstateRn;

                 vector<KthReal> coords;
                 for(int j=0;j<_wkSpace->getRobot(i)->getNumJoints();j++) coords.push_back(pathscopedstateRn->values[j]);
                 rcj->setRn(coords);
                 k++;//dummy
             }
             else
             {
                 rcj->setRn(smp->getMappedConf()[i].getRn());
             }
             rc.push_back(*rcj);
        }
        smp->setMappedConf(rc);
    }


	//! function to find a solution path
        bool omplPlanner::trySolve()
		{

           //Start
            ob::ScopedState<ob::CompoundStateSpace> startompl(space);
            smp2omplScopedState(_init, &startompl);
            cout<<"startompl:"<<endl;
            startompl.print();

            //Goal
            ob::ScopedState<ob::CompoundStateSpace> goalompl(space);
            smp2omplScopedState(_goal, &goalompl);
            cout<<"goalompl:"<<endl;
            goalompl.print();

            // set the start and goal states
            ss->setStartAndGoalStates(startompl, goalompl);

            // attempt to solve the problem within _planningTime seconds of planning time
            ss->clear();//to remove previous solutions, if any
            ss->getPlanner()->clear();
            ob::PlannerStatus solved = ss->solve(_planningTime);
            ss->print();
            //retrieve all the states
            Sample *smp;
            ob::PlannerData data(ss->getSpaceInformation());
            ss->getPlannerData(data);
            for(int i=0; i<data.numVertices();i++)
            {
                smp=new Sample(_wkSpace->getDimension());
                smp->setMappedConf(_init->getMappedConf());//copy the conf of the start smp
                omplState2smp(data.getVertex(i).getState(), smp);
                _samples->add(smp);
            }

            if (solved)
            {
                    std::cout << "Found solution:" << std::endl;
                    // print the path to screen
                    ss->simplifySolution();
                    ss->getSolutionPath().print(std::cout);
                    std::vector< ob::State * > & pathstates = ss->getSolutionPath().getStates();
                    ob::ScopedState<ob::CompoundStateSpace> pathscopedstate(space);

                    Sample *smp;

                    _path.clear();
                    clearSimulationPath();

                    //load the kautham _path variable from the ompl solution
                    for(int j=0;j<ss->getSolutionPath().getStateCount();j++){

                        pathscopedstate = (*pathstates[j]->as<ob::CompoundStateSpace::StateType>());

                        smp=new Sample(_wkSpace->getDimension());
                        smp->setMappedConf(_init->getMappedConf());//copy the conf of the start smp

                        omplScopedState2smp(pathscopedstate,smp);
                        _path.push_back(smp);
                   }

                    _solved = true;
                    drawCspace();
                    return _solved;
                }
                else{
                    std::cout << "No solution found" << std::endl;
                    _solved = false;
                    drawCspace();
                    return _solved;
            }
		}
    }
}

#endif // KAUTHAM_USE_OMPL


