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
 
 
 

#if !defined(_INVERSEKINEMATIC_H)
#define _INVERSEKINEMATIC_H

#include <libutil/kauthamdefs.h>
#include <libsampling/robconf.h>
#include <libsampling/se3conf.h>
#include <libsampling/rnconf.h>
#include <libutil/kauthamobject.h>

using namespace Kautham;
using namespace libSampling;

namespace libProblem {
  class Robot;

  class InvKinEx: public exception {
    public:
     InvKinEx(int cau) : exception(), causes(cau) {}

     inline const char* what() const throw() {
        switch(causes) {
        case 0:
          return "Inverse Kinematics has not a solution. Take care with the robot workspace and singularities."  ;
        case 1:
          return "Join out of range";
        }
        return "Unspected error in the Inverse Kinematic resolution";
     }
    private:
     int causes;
  };



  class InverseKinematic:public KauthamObject{
  public:
    InverseKinematic(Robot* const rob);

	  //!	This method must be implemented in order to obtain a solution
	  //! of the inverse kinematic model for a specific target.
    virtual bool      solve()=0;

	  //!	This method allows extract the information from the Hash_Map
	  //! and setup the internal variables.
    virtual bool      setParameters()=0;

	  //!	This method allow to setup the target.
    void              setTarget(vector<KthReal> &target);
    virtual void	  setTarget(vector<KthReal> &target, vector<KthReal> masterconf, bool maintainSameWrist);
    inline void       setTarget(mt::Transform& tcp){_targetTrans = tcp;}
    inline SE3Conf&   getSE3(){return _robConf.getSE3();}
    inline RnConf&    getRn(){return _robConf.getRn();}
    inline RobConf&   getRobConf(){return _robConf;}
    inline Robot&     getRobot(){return *_robot;}

  private:
    InverseKinematic();
  protected:

	  //!	This is a robot pointer. It will be free, chain or tree robot.
    Robot*          _robot;
    RobConf         _robConf;

	  //! This vector contains the target to be solved.
    vector<KthReal> _target;      //!< This is a generic way to set up the target.
    mt::Transform   _targetTrans; //!< This is the Scene target
  };

}
#endif  //_INVERSEKINEMATIC_H
