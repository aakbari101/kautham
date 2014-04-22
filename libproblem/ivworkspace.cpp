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
 
 
#include "ivworkspace.h"



namespace Kautham {

 
  IVWorkSpace::IVWorkSpace():WorkSpace(){
        scene = NULL;
        collisionscene=NULL;
	}

	SoSeparator* IVWorkSpace::getIvScene(bool bounding){
        if(scene == NULL){
            if(robots.size() != 0){
                scene = new SoSeparator();

                for(unsigned int i=0; i<robots.size(); i++)
                    scene->addChild((SoSeparator*)robots[i]->getModel());

                for(unsigned int i=0; i<obstacles.size(); i++)
                    scene->addChild((SoSeparator*)obstacles[i]->getModel());

                scene->ref();
            }
        }

		if(bounding)
			scene->addChild(calculateBoundingBox(scene));
		return scene;
	}

    SoSeparator* IVWorkSpace::getCollisionIvScene(bool bounding){
        if(collisionscene == NULL){
            if(robots.size() != 0){
                collisionscene = new SoSeparator();
                //scene->addChild(SoUnits::MILLIMETERS);
                for(unsigned int i=0; i<robots.size(); i++)
                    collisionscene->addChild((SoSeparator*)robots[i]->getCollisionModel());

                for(unsigned int i=0; i<obstacles.size(); i++)
                    collisionscene->addChild((SoSeparator*)obstacles[i]->getCollisionModel());

                collisionscene->ref();
            }
        }

        if(bounding)
            collisionscene->addChild(calculateBoundingBox(scene));
        return collisionscene;
    }

	
	void IVWorkSpace::updateScene(){
	
	}

	SoSeparator* IVWorkSpace::getIvFromPQPScene(bool bounding) {
		SoSeparator* pqp= new SoSeparator;
		for(unsigned int i=0;i<robots.size(); i++){
			pqp->addChild((SoSeparator*)robots[i]->getModelFromColl());
		}
		for(unsigned int i=0;i<obstacles.size(); i++){
			pqp->addChild((SoSeparator*)obstacles[i]->getModelFromColl());
		}
    if(bounding)
      pqp->addChild(calculateBoundingBox(pqp));
		return pqp;
	}
	

	
	SoSeparator* IVWorkSpace::calculateBoundingBox(SoSeparator *element){
		// set up an engine to calculate the bounding box of the element parameter
		SoComputeBoundingBox * bboxEngine = new SoComputeBoundingBox;
		bboxEngine->node = element;

		// decompose the vectors into separate floating point values
		SoDecomposeVec3f * dep1 = new SoDecomposeVec3f;
		SoDecomposeVec3f * dep2 = new SoDecomposeVec3f;
		dep1->vector.connectFrom(&bboxEngine->min);
		dep2->vector.connectFrom(&bboxEngine->max);

		// create engines to compose vectors again. We need eight vectors,
		// one for each vertex in the bounding box.
		SoComposeVec3f * comp1 = new SoComposeVec3f;
		SoComposeVec3f * comp2 = new SoComposeVec3f;
		SoComposeVec3f * comp3 = new SoComposeVec3f;
		SoComposeVec3f * comp4 = new SoComposeVec3f;
		SoComposeVec3f * comp5 = new SoComposeVec3f;
		SoComposeVec3f * comp6 = new SoComposeVec3f;
		SoComposeVec3f * comp7 = new SoComposeVec3f;
		SoComposeVec3f * comp8 = new SoComposeVec3f;

		// connect the engines so that all corner points are covered
		comp1->x.connectFrom(&dep1->x);
		comp1->y.connectFrom(&dep1->y);
		comp1->z.connectFrom(&dep1->z);

		comp2->x.connectFrom(&dep2->x);
		comp2->y.connectFrom(&dep1->y);
		comp2->z.connectFrom(&dep1->z);

		comp3->x.connectFrom(&dep2->x);
		comp3->y.connectFrom(&dep2->y);
		comp3->z.connectFrom(&dep1->z);

		comp4->x.connectFrom(&dep1->x);
		comp4->y.connectFrom(&dep2->y);
		comp4->z.connectFrom(&dep1->z);

		comp5->x.connectFrom(&dep1->x);
		comp5->y.connectFrom(&dep1->y);
		comp5->z.connectFrom(&dep2->z);

		comp6->x.connectFrom(&dep2->x);
		comp6->y.connectFrom(&dep1->y);
		comp6->z.connectFrom(&dep2->z);

		comp7->x.connectFrom(&dep2->x);
		comp7->y.connectFrom(&dep2->y);
		comp7->z.connectFrom(&dep2->z);

		comp8->x.connectFrom(&dep1->x);
		comp8->y.connectFrom(&dep2->y);
		comp8->z.connectFrom(&dep2->z);

		// concatenate the eight vectors into a single SoMFVec3f field
		SoConcatenate * con = new SoConcatenate(SoMFVec3f::getClassTypeId());
		con->input[0]->connectFrom(&comp1->vector);
		con->input[1]->connectFrom(&comp2->vector);
		con->input[2]->connectFrom(&comp3->vector);
		con->input[3]->connectFrom(&comp4->vector);
		con->input[4]->connectFrom(&comp5->vector);
		con->input[5]->connectFrom(&comp6->vector);
		con->input[6]->connectFrom(&comp7->vector);
		con->input[7]->connectFrom(&comp8->vector);

		// construct a new scene graph, that includes an indexed line to
		// render the bounding box.

		SoSeparator* sep = new SoSeparator;

		SoLightModel* lm = new SoLightModel;
		lm->model = SoLightModel::BASE_COLOR;
		SoCoordinate3* coords = new SoCoordinate3;

		// use the points calculated by our engines
		coords->point.connectFrom(con->output);

		SoDrawStyle* ds = new SoDrawStyle;
		ds->lineWidth = 2;
		ds->linePattern = 0xf0f0;

		SoIndexedLineSet* ls = new SoIndexedLineSet;
		const int32_t idx[] = {0,1,2,3,0,-1,4,5,6,7,4,-1,
													 1,5,-1,2,6,-1,3,7,-1,0,4,-1};
		ls->coordIndex.setNum(24);
		ls->coordIndex.setValues(0, 24, idx);

		sep->addChild(lm);
		sep->addChild(coords);
		sep->addChild(ds);
		sep->addChild(ls);
		sep->ref();
		return sep;
	}

}

