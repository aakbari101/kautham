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

 

#if !defined(_OBSTACLE_H)
#define _OBSTACLE_H

#include "ivpqpelement.h"
#include <kthutil/kauthamdefs.h>


namespace Kautham {

/** \addtogroup libProblem
 *  @{
 */

	class Obstacle {
	public:
		Obstacle(string modFile, KthReal pos[3], KthReal ori[4], KthReal scale, LIBUSED lib, bool flagCol);
		void setLinVelocity(KthReal vel[3]);
		void setAngVelocity(KthReal vel[3]);
		void* getModel(bool tran = true);
        void* getCollisionModel(bool tran = true);
		void* getModelFromColl(bool tran = true);
		inline KthReal* getLinVelocity(){return linVel;}
		inline KthReal* getAngVelocity(){return angVel;}
		inline Element* getElement(){return element;}
		inline void setEnableCollisions(bool c){enablecollisions = c;};
		inline bool getEnableCollisions() const {return enablecollisions;};
    inline string   getName(){return _name;}
    inline void     setName(string name){_name = name;}
		//~Obstacle();
	private:
		KthReal linVel[3];
		KthReal angVel[3];
		Element* element;
		bool enablecollisions;
		LIBUSED libs;
    string    _name;
	};

    /** @}   end of Doxygen module "libProblem" */
}

#endif  //_OBSTACLE_H
