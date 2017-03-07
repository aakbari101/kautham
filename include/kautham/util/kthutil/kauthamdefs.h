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

/* Author: Alexander Perez, Jan Rosell and Nestor Garcia Hidalgo */


  
 /*	This file contains some useful definitions to be used into Kautham planning
		system.
*/


/**
* \mainpage
*
* \section Abstract
* The [Kautham Project] (http://sir.upc.edu/kautham) is a software tool developped at the [Service and Industrial Robotics]
* (http://robotics.upc.edu) (SIR) group of the [Institute of Industrial and
* Control Engineering](http://ioc.upc.edu) (IOC) of the [Universitat Politècnica de Catalunya] (
* http://www.upc.edu (UPC), for teaching and research in robot motion planning.
* The tool allows to cope with problems with one or more robots, being a generic
* robot defined as a kinematic tree with a mobile base, i.e. the tool can plan
* and simulate from simple two degrees of freedom free-flying robots to multi-robot
* scenarios with mobile manipulators equipped with anthropomorphic hands.
* The main core of planners is provided by the Open Motion Planning Library ([OMPL](http://ompl.kavrakilab.org)).
* Different basic planners can be flexibly used and parameterized, allowing
* students to gain insight into the different planning algorithms. Among the
* advanced features the tool allows to easily define the coupling between degrees of
* freedom, the dynamic simulation and the integration with task planers. It is
* principally being used in the research of motion planning strategies for dexterous dual arm
* robotic systems.
*
* \section section1 Main Features
* -# Uses [OMPL](http://ompl.kavrakilab.org) suite of planners (geometric and control based)
* -# Uses [PQP](http://gamma.cs.unc.edu/SSV/) or [FCL](http://gamma.cs.unc.edu/FCL/fcl_docs/webpage/generated/index.html) for collision detection
* -# Uses [Coin3D] (http://www.coin3d.org/) for visualization
* -# Uses [ODE](http://www.ode.org/) physics engine
* -# Robot models are defined using [urdf](http://wiki.ros.org/urdf/XML/model) (or [DH](https://en.wikipedia.org/wiki/Denavit%E2%80%93Hartenberg_parameters)
*     parameters coded in an XML file, deprecated)
* -# Describes robots as kinematic trees with a mobile base (SE3xRn configuration space)
* -# Geometry is described in [VRML](https://en.wikipedia.org/wiki/VRML) files (if the [ASSIMP](http://www.assimp.org/) package
*    is available, many other formats like [DAE](https://www.khronos.org/collada/) or [STL](https://en.wikipedia.org/wiki/STL_%28file_format%29) are also supported).
* -# Allows multi-robot systems
* -# Allows the coupling of degrees of freedom
* -# Can be encapsuled as a [ROS](http://www.ros.org/) node

* \section section2 Credits
* Service and Industrial Robotics ([SIR](http://robotics.upc.edu))\n
* Institute of Industrial and Control Engineering ([IOC](http://ioc.upc.edu))\n
* Universitat Polirecnica de Catalunya ([UPC](http://www.upc.edu))\n
* [Barcelona](http://meet.barcelona.cat/en/), Spain\n
*
* Version 1.0 was developed in collaboration with  the [Escuela Colombiana
* de Ingenieria "Julio Garavito"](http://www.escuelaing.edu.co/es/) placed in Bogota D.C.
* Colombia
*
* <b>Contact:</b> Prof. [Jan Rosell](http://ioc.upc.edu/ca/personal/jan.rosell/) (email: <mailto:jan.rosell@upc.edu>)
*
* \section section3 External Documentation
*
* <b>Webpage:</b> <A HREF="http://sir.upc.edu/kautham"> sir.upc.edu/kautham</A>
*
* <b>Paper:</b> [The Kautham Project: A teaching and research tool for robot motion planning (pdf)](https://ioc.upc.edu/ca/personal/jan.rosell/publications/papers/the-kautham-project-a-teaching-and-research-tool-for-robot-motion-planning/@@download/file/PID3287499.pdf)
*
* \section Examples
*       \image html simple_examples.png "Simple examples: 2D free-flying robots"
*       \image html simple_examples2.png "Simple examples: 3D free-flying robot / 2-link manipulator"
*       \image html complex_examples.png "Complex examples: Multiple robots / dual-arm robots / hand-arm systems / virtual bronchoscopes"
*
* \page page1 Basic Features
*  \section sec11 Problem description
*       \image html problem_description.png "Problem description"
*  \section sec12 Modelling robots and obstacles
*       \image html urdf.png "URDF robot description"
*       \image html XYZcontrols.png "Control file for a free-flying robot with three translational d.o.f."
*       \image html KUKAcontrols.png "Control file for a Kuka robot"
*  \section sec13 Planners
*       \image html addParameters.png "Setting planner parameters from code"
*       \image html parametersGUI.png "GUI: Planner parameters"
*  \section sec14 Visualization
*       \image html visualizationWorkspace.png "Workspace visualization: visual and collision models"
*       \image html visualizationCspace.png "Visualization of 2D Cspaces"
*       \image html visualizationCspace2.png "Visualization of 2D projections of high dimensional Cspaces"
*
* \page page2 Advanced features
*  \section sec21 Coupling between degrees of freedom
*       \image html PMD1controls.png "Control file describing one of the coupled motions of the SAH hand"
*       \image html PMD1handmotion.png  "Hand motion along the corresponding coupled motion"
*  \section sec22 Constrained motion planning
*       \image html constrainedMotionPlanning.png "Motion planning for a car-like robot"
*  \section sec23 Dynamic simulation
*  \section sec24 Integration with task planning
*  \section sec25 Benchmarking
*       \image html benchmarking.png "A benchmarking file"
*
*/



#if !defined(_KAUTHAMDEFS_H)
#define _KAUTHAMDEFS_H

#include <iostream> 
#include <string>
#include <vector>
#include <map>



using namespace std;

namespace Kautham {
#define KthReal float


  enum ROBOTTYPE {
      FREEFLY,
      CHAIN,
      TREE,
      CLOSEDCHAIN
  };

  enum APPROACH {
      DHSTANDARD,
      DHMODIFIED,
      URDF
  };

  enum NEIGHSEARCHTYPE {
      BRUTEFORCE,
      ANNMETHOD
  };

  enum CONFIGTYPE {
      SE2,
      SE3,
      Rn
  };

  enum SPACETYPE {
      SAMPLEDSPACE,
      CONTROLSPACE,
      CONFIGSPACE
  };

  enum PROBLEMSTATE {
      INITIAL,
      PROBLEMLOADED,
  };

  enum CONSTRAINEDKINEMATICS {
      UNCONSTRAINED,
      BRONCHOSCOPY
  };

  //! This enumeration has the relationship of all Inverse Kinematic models
  //! available to be used as solver of the robot inverse kinematics.
  enum INVKINECLASS {
      NOINVKIN,
      UNIMPLEMENTED,
      RR2D,
      TX90,
      HAND,
      TX90HAND,
      UR5
  };

  enum PLANNERFAMILY {
      NOFAMILY,
      IOCPLANNER,
      OMPLPLANNER,
      OMPLCPLANNER,
      ODEPLANNER
  };

#ifndef INFINITY
#define INFINITY 1.0e40
#endif

  typedef std::map<std::string, KthReal> HASH_S_K;
  typedef std::map<std::string, std::string> HASH_S_S;

	
   //! Structure  that save the object position and orientation in the WSpace.
   /*! This structure save  temporarily the most important information about
   * the object position and orientation in the 3D physical space regardless
   * If the problem is or not 3D.*/
  struct DATA{
      DATA(){
          for(int i=0;i<3;i++)
              pos[i]=ori[i]= (KthReal)0.0;
          ori[3]= (KthReal)0.0;
      }
      KthReal pos[3];
      KthReal ori[4];
  };
}

#endif //_KAUTHAMDEFS_H
