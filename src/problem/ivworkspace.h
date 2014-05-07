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

 
#if !defined(_IVWORKSPACE_H)
#define _IVWORKSPACE_H

#include "workspace.h"
#include <kthutil/kauthamdefs.h>
#include <Inventor/engines/SoComputeBoundingBox.h>
#include <Inventor/engines/SoCompose.h>
#include <Inventor/engines/SoConcatenate.h>
#include <Inventor/nodes/SoCoordinate3.h>
#include <Inventor/nodes/SoIndexedLineSet.h>
#include <Inventor/nodes/SoTransform.h>


namespace Kautham{

/** \addtogroup libProblem
 *  @{
 */

  class IVWorkSpace: public WorkSpace {
	  public:
		  IVWorkSpace();
		  //~IVWorkSpace();
          SoSeparator* getIvScene(bool bounding = false);
          SoSeparator* getCollisionIvScene(bool bounding = false);
		  SoSeparator* getIvFromPQPScene(bool bounding = false);
		  static SoSeparator* calculateBoundingBox(SoSeparator * roots);
	  protected:
		  void updateScene();
		  SoSeparator *scene;
          SoSeparator *collisionscene;
  };

  /** @}   end of Doxygen module "libProblem" */
}

#endif  //_IVWORKSPACE_H

