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
 
 
#include "libsplanner/libompl/omplplanner.h"
#include "kauthamshell.h"
#include <iostream>


namespace ob = ompl::base;

namespace Kautham{


bool kauthamshell::openProblem(ifstream* inputfile, string modelsfolder)
{
    _problem = new Problem();
    if(_problem->setupFromFile( inputfile, modelsfolder))
    {
      _problem->getPlanner()->setInitSamp( _problem->getSampleSet()->getSampleAt(0) );
      _problem->getPlanner()->setGoalSamp( _problem->getSampleSet()->getSampleAt(1) );
      return true;
    }
    return false;
}


bool kauthamshell::openProblem(string problemfilename)
{
    std::cout << "Kautham is opening a problem file: " << problemfilename << endl;

    _problem = new Problem();
    if(_problem->setupFromFile( problemfilename.c_str() ))
    {
      _problem->getPlanner()->setInitSamp( _problem->getSampleSet()->getSampleAt(0) );
      _problem->getPlanner()->setGoalSamp( _problem->getSampleSet()->getSampleAt(1) );
      return true;
    }
    return false;
}

  bool kauthamshell::checkCollision(vector<KthReal> smpcoords)
  {
      Sample* smp = new Sample(_problem->wSpace()->getDimension());
      smp->setCoords(smpcoords);
      return _problem->wSpace()->collisionCheck(smp);
  }


  void kauthamshell::setRobotsConfig(vector<KthReal> smpcoords)
  {
      Sample* smp = new Sample(_problem->wSpace()->getDimension());
      smp->setCoords(smpcoords);
      _problem->wSpace()->moveRobotsTo(smp);
  }


  bool kauthamshell::setQuery(vector<KthReal> init, vector<KthReal> goal)
  {
      int d = _problem->wSpace()->getDimension();
      SampleSet* samples = _problem->getSampleSet();
      samples->clear();

      //init
      Sample* smp = new Sample(d);
      smp->setCoords( init );
      if(_problem->wSpace()->collisionCheck( smp ))
      {
          return false;
      }
      samples->add( smp );

      //goal
      smp = new Sample(d);
      smp->setCoords( goal );
      _problem->wSpace()->collisionCheck( smp);
      if(_problem->wSpace()->collisionCheck( smp ))
      {
          return false;
      }
      samples->add( smp );
      _problem->getPlanner()->setInitSamp( samples->getSampleAt(0) );
      _problem->getPlanner()->setGoalSamp( samples->getSampleAt(1) );
  }

  bool kauthamshell::solve()
  {
       return _problem->getPlanner()->solveAndInherit();
  }

  bool kauthamshell::solve(ostream &graphVizPlannerDataFile)
  {
       bool ret = _problem->getPlanner()->solveAndInherit();

       if(_problem->getPlanner()->getFamily()=="ompl")
       {
           omplplanner::omplPlanner* p = (omplplanner::omplPlanner*)_problem->getPlanner();

           ob::PlannerDataPtr pdata;
           pdata = ((ob::PlannerDataPtr) new ob::PlannerData(p->ss->getSpaceInformation()));

           p->ss->getPlanner()->getPlannerData(*pdata);
           pdata->printGraphviz(graphVizPlannerDataFile);
       }

       return ret;
  }

}
