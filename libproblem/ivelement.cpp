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

/* Author: Alexander Perez, Jan Rosell */

 
 

#include "ivelement.h"

#if  defined(KAUTHAM_USE_ARMADILLO)
#include <armadillo>

using namespace arma;
#endif


namespace Kautham {
    bool valid_num_triangles(int T, int V) {
        //return (fabs((double)T/(double)V-1.5)<=0.5);

        int i = 3;
        int fact = 1;
        while (i <= V && T > fact) {
            i++;
            fact *= i;
        }
        return(T<=fact && i<=V);
    }

    // get triangles from inventor models
    void IVElement::triangle_CB(void *data, SoCallbackAction *action,
                                const SoPrimitiveVertex *v1,
                                const SoPrimitiveVertex *v2,
                                const SoPrimitiveVertex *v3) {
        vector<double>* point_vector = (vector<double>*)data;

        SbVec3f point[] = {v1->getPoint(),v2->getPoint(),v3->getPoint()};

        const SbMatrix  mm = action->getModelMatrix();
        for (int i = 0; i < 3; i++) {
            mm.multVecMatrix(point[i], point[i]);
            for (int j = 0; j < 3; j++) {
                point_vector->push_back((double)point[i][j]);
            }
        }
    }

    IVElement::IVElement(string ivfile, string collision_ivfile, KthReal sc) {
		for(int i=0;i<3;i++){
			position[i]= 0.0f;
			orientation[i]=0.0f;
		}
		orientation[2]=1.0f;
		orientation[3]=0.0f;
		scale=sc;

		trans= new SoTranslation;
		rot = new SoRotation;
		sca = new SoScale();
		color = new SoMaterial;

		posVec = new SoSFVec3f;
		trans->translation.connectFrom(posVec);
		posVec->setValue((float)position[0],(float)position[1],(float)position[2]);

		rotVec = new SoSFRotation;
		rotVec->setValue(SbVec3f((float)orientation[0],(float)orientation[1],
				(float)orientation[2]),(float)orientation[3]);
		rot->rotation.connectFrom(rotVec);

		scaVec= new SoSFVec3f;
		scaVec->setValue((float)scale,(float)scale,(float)scale);
        sca->scaleFactor.connectFrom(scaVec);

		SoInput input;
		ivmodel = new SoSeparator;
		ivmodel->ref();
		ivmodel->addChild(sca);

		if(input.openFile(ivfile.c_str()))
			ivmodel->addChild(SoDB::readAll(&input));
		else
		{
			//ivmodel->addChild(new SoSphere());
			static const char *str[] = { 
				"#VRML V1.0 ascii\n",
				"DEF pointgoal Separator {\n",
				"  Separator {\n",
				"    MaterialBinding { value PER_PART }\n",
				"    Coordinate3 {\n",
				"      point [\n",
				"        0.0 0.0 0.0\n",
				"      ]\n",
				"    }\n",
				"    DrawStyle { pointSize 5 }\n",
				"    PointSet { }\n",
				"   }\n",
				"}\n",
				NULL
			};
			input.setStringArray(str);
			SoSeparator *sep = SoDB::readAll(&input);
			sep->ref();
			while (sep->getNumChildren() > 0)
			{
				ivmodel->addChild(sep->getChild(0));
				sep->removeChild(0);
			}
			sep->unref();
			
		}
		ivmodel->ref();

        //check if collision_ivfile is a different file from ivfile
        //and if collision_ivmodel could be the minimum-volume bounding box of ivmodel
        QSettings settings("IOC", "Kautham");
        bool useBBOX = settings.value("use_BBOX","false").toBool();

        if (useBBOX && collision_ivfile == ivfile && ivModel() != NULL &&
                string::npos == collision_ivfile.find( "_bbox", 0 )) {
            //collision_ivmodel will be the minimum-volume bounding box of ivmodel
            cout << "The minimum-volume bounding box of model in " << collision_ivfile
                 << " will be computed and used for collision testing" << endl;

            string::size_type pos = collision_ivfile.find_last_of(".");
            collision_ivfile.insert(pos,"_bbox");

            fstream fin;
            //check if bbox file already exists
            fin.open(collision_ivfile.c_str(),ios::in);
            if (fin.is_open()) {
                //the file exists
                fin.close();

                if(input.openFile(collision_ivfile.c_str())) {
                    collision_ivmodel = new SoSeparator;
                    collision_ivmodel->ref();
                    collision_ivmodel->addChild(sca);
                    collision_ivmodel->addChild(SoDB::readAll(&input));
                    cout << "This box was found in " << collision_ivfile << endl;
                    cout << "Please modify problem file to include the collision ivfile" << endl;
                } else {
                    collision_ivmodel = BBOX(ivModel(),sc,collision_ivfile);
                    if (collision_ivmodel == NULL) {
                        collision_ivmodel = ivModel();
                    } else {
                        cout << "This box will be saved in " << collision_ivfile << endl;
                        cout << "Please modify problem file to include this new collision ivfile" << endl;
                    }
                }
            } else {
                //the file doesn't exist
                fin.close();

                collision_ivmodel = BBOX(ivModel(),sc,collision_ivfile);
                if (collision_ivmodel == NULL) {
                    collision_ivmodel = ivModel();
                } else {
                    cout << "This box will be saved in " << collision_ivfile << endl;
                    cout << "Please modify problem file to include this new collision ivfile" << endl;
                }
            }
        } else {
            //collision_ivmodel will be the model in collision_ivfile
            if(input.openFile(collision_ivfile.c_str())) {
                collision_ivmodel = new SoSeparator;
                collision_ivmodel->ref();
                collision_ivmodel->addChild(sca);
                collision_ivmodel->addChild(SoDB::readAll(&input));
            } else {
                //collision_ivmodel->addChild(new SoSphere());
                static const char *str[] = {
                    "#VRML V1.0 ascii\n",
                    "DEF pointgoal Separator {\n",
                    "  Separator {\n",
                    "    MaterialBinding { value PER_PART }\n",
                    "    Coordinate3 {\n",
                    "      point [\n",
                    "        0.0 0.0 0.0\n",
                    "      ]\n",
                    "    }\n",
                    "    DrawStyle { pointSize 5 }\n",
                    "    PointSet { }\n",
                    "   }\n",
                    "}\n",
                    NULL
                };
                collision_ivmodel = new SoSeparator;
                collision_ivmodel->ref();
                input.setStringArray(str);
                SoSeparator *sep = SoDB::readAll(&input);
                sep->ref();
                while (sep->getNumChildren() > 0)
                {
                    collision_ivmodel->addChild(sep->getChild(0));
                    sep->removeChild(0);
                }
                sep->unref();
            }
        }
    }


    IVElement::IVElement(SoSeparator *visual_model, SoSeparator *collision_model, float sc) {
        for(int i=0;i<3;i++){
            position[i]= 0.0f;
            orientation[i]=0.0f;
        }
        orientation[2]=1.0f;
        orientation[3]=0.0f;
        scale=sc;

        trans= new SoTranslation;
        rot = new SoRotation;
        sca = new SoScale();
        color = new SoMaterial;

        posVec = new SoSFVec3f;
        trans->translation.connectFrom(posVec);
        posVec->setValue((float)position[0],(float)position[1],(float)position[2]);

        rotVec = new SoSFRotation;
        rotVec->setValue(SbVec3f((float)orientation[0],(float)orientation[1],
                (float)orientation[2]),(float)orientation[3]);
        rot->rotation.connectFrom(rotVec);

        scaVec= new SoSFVec3f;
        scaVec->setValue((float)scale,(float)scale,(float)scale);
        sca->scaleFactor.connectFrom(scaVec);

        ivmodel = new SoSeparator;
        ivmodel->ref();
        ivmodel->addChild(sca);
        ivmodel->addChild(visual_model);

        if (collision_model != NULL) {
            collision_ivmodel = new SoSeparator;
            collision_ivmodel->ref();
            collision_ivmodel->addChild(sca);
            collision_ivmodel->addChild(collision_model);

        } else {
            QSettings settings("IOC", "Kautham");
            bool useBBOX = settings.value("use_BBOX","false").toBool();

            if (useBBOX) {
                collision_ivmodel = BBOX(ivModel(),sc);
                if (collision_ivmodel == NULL) {
                    collision_ivmodel = ivModel();
                }
            } else {
                collision_ivmodel = ivModel();
            }
        }
    }


    SoSeparator *IVElement::BBOX(SoSeparator *model, float sc, string filename) {
#if defined(KAUTHAM_USE_ARMADILLO)
        //get all vertices from all triangles in ivmodel
        SoCallbackAction pointAction;

        //coordinates from vertices will be saved one after the other
        vector<double> point_vector;
        pointAction.addTriangleCallback(SoShape::getClassTypeId(),
                                        triangle_CB,(void*)&point_vector);
        pointAction.apply(model);

        int num_vertices = point_vector.size()/3;
        int num_triangles = num_vertices/3;
        printf("has %i triangles\n",num_triangles);
        if (num_triangles < 4) {
            return NULL;
        } else {
            //erase repeated vertices
            for (int i = 0; i < num_vertices; i++) {
                for (int j = i+1; j < num_vertices; j++) {
                    if (fabs(point_vector.at(3*i+0)-point_vector.at(3*j+0))<0.00001 &&
                            fabs(point_vector.at(3*i+1)-point_vector.at(3*j+1))<0.00001 &&
                            fabs(point_vector.at(3*i+2)-point_vector.at(3*j+2))<0.00001) {
                        point_vector.erase(point_vector.begin()+3*j);
                        point_vector.erase(point_vector.begin()+3*j);
                        point_vector.erase(point_vector.begin()+3*j);
                        num_vertices--;
                        j--;
                    }
                }
            }
            printf("has %i vertices\n",num_vertices);

            mat vector_mat(num_vertices-1,3);
            gdiam_real *vertex;
            vertex = new gdiam_real[3*num_vertices];
            for (int i = 0; i < num_vertices; i++) {
                for (int j = 0; j < 3; j++) {
                    vertex[3*i+j] = point_vector.at(3*i+j);
                    if (i > 0)
                        vector_mat.at(i-1,j) = vertex[3*i+j]-vertex[0+j];
                }
            }

            //check vertices' coplanarity
            float vertex_rank = (float)rank(vector_mat);

            printf("with rank %i\n",(int)vertex_rank);
            if (vertex_rank < 3 || !valid_num_triangles(num_triangles,num_vertices)) {
                printf("bad ivfile\n");
                return NULL;
            } else {
                //data must be converted first
                gdiam_point  *pnt_arr;
                pnt_arr = gdiam_convert((gdiam_real*)vertex,num_vertices);

                //find the minimum-volume bounding box of ivmodel's vertices
                gdiam_bbox bbox = gdiam_approx_mvbb_grid_sample(pnt_arr,num_vertices,5,num_vertices);

                if (bbox.volume() < 0.0001) {
                    gdiam_point_t dir1;
                    gdiam_point_t dir2;
                    gdiam_point_t dir3;


                    dir1[0] = bbox.get_dir(0)[0];
                    dir1[1] = bbox.get_dir(0)[1];
                    dir1[2] = bbox.get_dir(0)[2];
                    pnt_normalize(dir1);

                    dir2[0] = bbox.get_dir(1)[0];
                    dir2[1] = bbox.get_dir(1)[1];
                    dir2[2] = bbox.get_dir(1)[2];
                    pnt_normalize(dir2);

                    dir3[0] = dir1[1]*dir2[2]-dir1[2]*dir2[1];
                    dir3[1] = dir1[2]*dir2[0]-dir1[0]*dir2[2];
                    dir3[2] = dir1[0]*dir2[1]-dir1[1]*dir2[0];
                    pnt_normalize(dir3);
                    bbox.init(dir1,dir2,dir3);

                    //expand it so any vertex is out of the box
                    gdiam_point_t point;
                    for (int i = 0; i < num_vertices; i++) {
                        for (int j = 0; j < 3; j++) {
                            point[j] = point_vector.at(3*i+j);
                        }

                        bbox.bound(point);
                    }
                }

                printf("bbox has a volume of %f\n",(float)bbox.volume());

                //get the 8 vertices (and correct the scale - since the collision_ivmodel has already a child sca (collision_ivmodel->addChild(sca));
                int i;
                static float vertexPositions[8][3];
                double tmp[3];
                for (int sel1 = 0; sel1 <= 1; sel1++) {
                    for (int sel2 = 0; sel2 <= 1; sel2++) {
                        for (int sel3 = 0; sel3 <= 1; sel3++) {
                            i = sel1*4 + sel2*2 + sel3;
                            bbox.get_vertex(sel1,sel2,sel3,&tmp[0],&tmp[1],&tmp[2]);
                            vertexPositions[i][0]=(float)tmp[0] / sc;
                            vertexPositions[i][1]=(float)tmp[1] / sc;
                            vertexPositions[i][2]=(float)tmp[2] / sc;
                        }
                    }
                }

                //get faces indices: 12 faces with 3 vertices each,
                //(plus the end-of-face indicator for each face)
                static int indices[48] =
                {
                    1, 3, 2, -1,
                    2, 0, 1, -1,
                    5, 7, 3, -1,
                    3, 1, 5, -1,
                    4, 6, 7, -1,
                    7, 5, 4, -1,
                    0, 2, 6, -1,
                    6, 4, 0, -1,
                    4, 5, 1, -1,
                    1, 0, 4, -1,
                    3, 7, 6, -1,
                    6, 2, 3, -1,
                };

                SoSeparator *BBOXmodel;
                BBOXmodel = new SoSeparator;
                BBOXmodel->ref();

                //define coordinates for vertices
                SoCoordinate3 *myCoords = new SoCoordinate3;
                myCoords->point.setValues(0, 8, vertexPositions);
                BBOXmodel->addChild(myCoords);

                //define the IndexedFaceSet, with indices into the vertices
                SoIndexedFaceSet *myFaceSet = new SoIndexedFaceSet;
                myFaceSet->coordIndex.setValues(0, 48, indices);
                BBOXmodel->addChild(myFaceSet);

                if (filename != "") {
                    //save collision_ivmodel in a new file
                    ofstream bbox_file;
                    bbox_file.open(filename.c_str(),ios_base::out | ios_base::trunc);
                    bbox_file << "#VRML V2.0 utf8" << endl
                              << "Shape {" << endl
                              << "    appearance Appearance {" << endl
                              << "        material Material {" << endl
                              << "            diffuseColor 1.0 0.8 0.8" << endl
                              << "            transparency 0.5"  << endl
                              << "        }" << endl
                              << "    }" << endl
                              << "    geometry IndexedFaceSet {" << endl
                              << "        coord Coordinate {" << endl
                              << "            point [" << endl;
                    for (int i = 0; i < 8; i ++) {
                        bbox_file << "                 " << vertexPositions[i][0] << "  "
                                  << vertexPositions[i][1] << "  " << vertexPositions[i][2];
                        if (i < 7){
                            bbox_file << ",";
                        }
                        bbox_file << endl;
                    }
                    bbox_file << "            ]" << endl
                              << "        }" << endl
                              << "        coordIndex [" << endl
                              << "             1, 3, 2, -1," << endl
                              << "             2, 0, 1, -1," << endl
                              << "             5, 7, 3, -1," << endl
                              << "             3, 1, 5, -1," << endl
                              << "             4, 6, 7, -1," << endl
                              << "             7, 5, 4, -1," << endl
                              << "             0, 2, 6, -1," << endl
                              << "             6, 4, 0, -1," << endl
                              << "             4, 5, 1, -1," << endl
                              << "             1, 0, 4, -1," << endl
                              << "             3, 7, 6, -1" << endl
                              << "             6, 2, 3" << endl
                              << "        ]" << endl
                              << "    }" << endl
                              << "}" << endl;
                    bbox_file.close();
                }

                return BBOXmodel;
            }
        }
#else
        return NULL;
#endif
    }


	void IVElement::setColor(KthReal c[3]){
	  color->diffuseColor.setValue((float)c[0],(float)c[1],(float)c[2]);
	}

	void IVElement::setPosition(KthReal pos[3]){
		for(int i=0;i<3;i++)
			position[i]=pos[i];
      posVec->setValue((float)position[0],(float)position[1],(float)position[2]);
	  
    
    //for(int i=0; i<3; i++)
    //  cout << posVec->getValue()[i] << "\t" ;
    //cout << endl;
	}

	void IVElement::setOrientation(KthReal ori[4]){
		for(int i=0;i<4;i++)
			orientation[i]=ori[i];
      rotVec->setValue(orientation);
//		rotVec->setValue(SbVec3f((float)orientation[0],(float)orientation[1],
//				(float)orientation[2]),(float)orientation[3]);

    //for(int i=0; i<4; i++)
    //  cout << (rotVec->getValue()).getValue()[i] << "\t" ;
    //cout << endl;
	}

  SbMatrix IVElement::orientationMatrix() {
	  SbMatrix mat;
	  SbRotation rr = rotVec->getValue();
	  rr.getValue(mat);
	  return mat.transpose();
  }

  SoSeparator* IVElement::ivModel(bool tran) {
	  if(tran){
		  SoSeparator* temp = new SoSeparator;
		  temp->ref();
		  temp->addChild(color);
		  temp->addChild(trans);
		  temp->addChild(rot);
		  temp->addChild(ivmodel);
		  return temp;
	  }else
			return ivmodel;
  }

  SoSeparator* IVElement::collision_ivModel(bool tran) {
      if(tran){
          SoSeparator* temp = new SoSeparator;
          temp->ref();
          temp->addChild(color);
          temp->addChild(trans);
          temp->addChild(rot);
          temp->addChild(collision_ivmodel);
          return temp;
      }else
            return collision_ivmodel;
  }



  bool IVElement::collideTo(Element* other) {
	  // this method only return a value;
	  // This method has been implemented to provide a common functionalities
	  // if the Kautham will be called without a collision checker system.
	  return true;
  }

  KthReal IVElement::getDistanceTo(Element* other) {
	  // this method only return a value;
	  // This method has been implemented to provide a common functionalities
	  // if the Kautham will be called without a collision checker system.
	  return 0.0;
  }


}

