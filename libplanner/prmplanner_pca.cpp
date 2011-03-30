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
 
 
#if defined(KAUTHAM_USE_ARMADILLO)

#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include <boost/graph/astar_search.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/algorithm/string.hpp>
#include "localplanner.h"
#include "prmplanner_pca.h"
#include <stdio.h>
#include <libutil/pugixml/pugixml.hpp>

#include <libsampling/lcprng.h>

///////////////Armadillo////////
#include <iostream>


using namespace arma;
using namespace std;

//////////////////////



using namespace libSampling;
using namespace SDK;
using namespace pugi;

namespace libPlanner {
  namespace PRM{
    PRMPlannerPCA::PRMPlannerPCA(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize,int numPCA,int numRandom):
              Planner(stype, init, goal, samples, sampler, ws, lcPlan, ssize){

      _guiName = _idName = "PRM PCA";
      _neighThress = 1.5;//0.5//50000.0;
      _kNeighs = 10;
      _isGraphSet = false;
	    _maxNumSamples = 300;//1000;
	    _speedFactor = 1;
	    _solved = false;
	    setStepSize(ssize);//also changes stpssize of localplanner
      _drawnLink = -1; //the path of last link is defaulted
	  _numMaxSamplingPCA=numPCA;
	  _numMaxSamplingRandom=numRandom;

  	  
      addParameter("Step Size", ssize);
     // addParameter("Neigh Thresshold", _neighThress);
      addParameter("Max. Neighs", _kNeighs);
      addParameter("Max. Samples", _maxNumSamples);
      addParameter("Speed Factor", _speedFactor);
      addParameter("Drawn Path Link",_drawnLink);
	  /////////////////////////////////////////////////
		addParameter("Num Max Sampling PCA", _numMaxSamplingPCA);
      addParameter("Num Max Sampling Random",_numMaxSamplingRandom);

	   removeParameter("Neigh Thresshold");

	  /////////////////////////////////////////////////

	    _labelCC=0;

	    _samples->setTypeSearch(ANNMETHOD);//(BRUTEFORCE);//
	    _samples->setWorkspacePtr(_wkSpace);
	    _samples->setANNdatastructures(_kNeighs, _maxNumSamples);

      for(int i=0; i<_wkSpace->robotsCount();i++)
        _wkSpace->getRobot(i)->setLinkPathDrawn(_drawnLink);
    }


	//!Void destructor
	PRMPlannerPCA::~PRMPlannerPCA(){
			
	}

	//!Function to set the parameters of the PRM planner 
    bool PRMPlannerPCA::setParameters(){
      try{
        HASH_S_K::iterator it = _parameters.find("Step Size");
        if(it != _parameters.end())
			    setStepSize(it->second);//also changes stpssize of localplanner
        else
          return false;

        it = _parameters.find("Speed Factor");
        if(it != _parameters.end())
          _speedFactor = it->second;
        else
          return false;

        it = _parameters.find("Drawn Path Link");
		if(it != _parameters.end()){
          _drawnLink = it->second;
          for(int i=0; i<_wkSpace->robotsCount();i++)
            _wkSpace->getRobot(i)->setLinkPathDrawn(_drawnLink);
		}else
          return false;

        it = _parameters.find("Max. Samples");
        if(it != _parameters.end()){
          _maxNumSamples = it->second;
		      _samples->setANNdatastructures(_kNeighs, _maxNumSamples);
		    }else
          return false;

		it = _parameters.find("Num Max Sampling PCA");
        if(it != _parameters.end()){
        _numMaxSamplingPCA = it->second;
		      _samples->setANNdatastructures(_numMaxSamplingPCA);
		    }else
          return false;

		it = _parameters.find("Num Max Sampling Random");
        if(it != _parameters.end()){
       _numMaxSamplingRandom = it->second;
		      _samples->setANNdatastructures(_numMaxSamplingRandom);
		    }else
          return false;

        /*it = _parameters.find("Neigh Thresshold");
        if(it != _parameters.end())
          _neighThress = it->second;
        else
          return false;*/

        it = _parameters.find("Max. Neighs");
        if(it != _parameters.end()){
          _kNeighs = (int)it->second;
		      _samples->setANNdatastructures(_kNeighs, _maxNumSamples);
		    }else
          return false;
      }catch(...){
        return false;
      }
      return true;
    }

    bool PRMPlannerPCA::saveData(string path){
      //  First the Planner method is invoked to store the planner's 
      //  parameters and the samples in sample set. 
      if(Planner::saveData(path)){
      
        //  Then, the saved file is loaded again in order to appending 
        //  the planner own information
        xml_document doc;
        xml_parse_result result = doc.load_file(path.c_str());

        if(result){
          xml_node conNode = doc.child("Planner").append_child();
          conNode.set_name("Conectivity");
          vector<prmEdge*>::iterator itC;
          vector<cost>::iterator itW = weights.begin();
          stringstream ss;
          for(itC = edges.begin(); itC != edges.end(); ++itC){
            ss.str("");
            ss << (*itC)->first << " " << (*itC)->second;
            xml_node pairNode = conNode.append_child();
            pairNode.set_name("Pair");
            pairNode.append_child(node_pcdata).set_value(ss.str().c_str());
            ss.str("");
            ss << *itW++;
            pairNode.append_attribute("Weight") = ss.str().c_str();
          }

          //Adding the connected components
          xml_node compNode = doc.child("Planner").append_child();
          compNode.set_name("ConnComponents");
          compNode.append_attribute("size") = (int) _ccMap.size();
          map<int,SampleSet*>::iterator it;
          for ( it=_ccMap.begin(); it != _ccMap.end(); it++ ){
            xml_node ccNode = compNode.append_child();
            ccNode.set_name("Component");
            ccNode.append_attribute("name") = (*it).first;
            ccNode.append_attribute("size") = (*it).second->getSize();
            ss.str("");
            vector<Sample*>::iterator itera = (*it).second->getBeginIterator();
            for(itera = (*it).second->getBeginIterator(); itera != (*it).second->getEndIterator();
                ++itera)
              ss <<_samples->indexOf( (*itera) ) << " ";

            ccNode.append_child(node_pcdata).set_value(ss.str().c_str());
          }

          return doc.save_file(path.c_str());
        }
      }
      return false;
    }

    bool PRMPlannerPCA::loadData(string path){
      // First, the current graph is reset.
      clearGraph();
      if(Planner::loadData(path)){
        //  If it is correct, the planner has loaded the 
        //  SampleSet with the that samples from the file.
        xml_document doc;
        xml_parse_result result = doc.load_file(path.c_str());
        if(result){
          xml_node tempNode = doc.child("Planner").child("Conectivity");
          prmEdge *e;
          for (pugi::xml_node_iterator it = tempNode.begin(); it != tempNode.end(); ++it){
            string sentence = it->child_value();
            vector<string> tokens;
            boost::split(tokens, sentence, boost::is_any_of("| "));
            if(tokens.size() != 2){
              std::cout << "Connectivity information is wrong. Graph is not set."
                  << std::endl;
              return false;
            }
            e = new prmEdge(atoi(tokens[0].c_str()), atoi(tokens[1].c_str()));
            edges.push_back(e);
			  	  weights.push_back((KthReal)(it->attribute("Weight").as_double()));
          } 
          loadGraph(); // Calling this method, the graph is regenerated.
          std::cout << "The PRM connectivity has been loaded correctly from the file." << std::endl;
          return true;
        }
      }
      return false;
    }

	  void PRMPlannerPCA::saveData()
	  {  // Put here the code you want in order to save your data in an informal way.

	  }

	//!function that constructs the PRM and finds a solution path
    bool PRMPlannerPCA::trySolve(){
      _solved = false;
      int count = 0;
	  ///////////////////////////
	  int n=0;
	  _indexpca.clear();
	  ///////////////////////////
      if( _isGraphSet ){  //If graph already available
        //If new configurations have been sampled the graph is rebuild
        if( _samples->changed())
        {
          clearGraph();
          _samples->findNeighs(_neighThress, _kNeighs);
          connectSamples();
          loadGraph();
        }
      }
      //No grah is already avaliable, then build it
      else
      {
        _samples->findNeighs(_neighThress, _kNeighs);
        connectSamples();
        loadGraph();
      }
      //try to find a path with the samples already available in the sampleSet
      //If found, smooth it.
      if( findPath() )
      {
        printConnectedComponents();
        smoothPath();
        _solved = true;
        count = _samples->getSize();
      }
      //if not found, keep trying until a max number of samples
      //iteratively growing the PRM

	  ////////////////////////////////////
		
	  /////////////////////////////////////
      else
      {

        LCPRNG* rgen = new LCPRNG(15485341);//15485341 is a big prime number
        count = _samples->getSize();
       // Sample* smp = NULL;
        count = _samples->getSize();
		Sample *smp=new Sample(2);

		countpca=0;
            
          
		///////////////////////////////////////
	
        //////////poblacion///////////////////
			 
		
		for(int i=0;i<50;i++)
		{
		   
           // smp = _sampler->nextSample();
		  
			
			/////////////////////////////////////////////////
			vector<KthReal> _coords(2);
			for(int j = 0; j < 2 ; j++)
			{
				_coords[j] = (KthReal)rgen->d_rand();
			}
			smp->setCoords(_coords);
			////////////////////////////////////////////////
			count++;

			if(!_wkSpace->collisionCheck(smp))
			{
				//i++;
				_samples->add(smp) ;
				//connectLastSample();
				smp=new Sample(2);
				//count++;
			
			}
			
			_coords.clear();
			

		
		}
		
		//int countpca=0;
		callpca=0;
		////////////////////////////////////////////////////////////////////////////
		do{
			try{	
							
					if(getSampleRandPCA2D())
					{
						if(matPCA2D.n_rows>0)//matrand
						{
							////////////////Creación de variables////////////////////
							vector<KthReal> coord(2);
							Sample *tmpSample;
							tmpSample = new Sample(2);
							//Sample *smp=new Sample(2);
							//float r=0;
							///////////////////////////////////////////////////////////
							//Procesamiento para comprobar las nuevas muestas 20x11, son libres de colisión
							for(int z=0; z<matPCA2D.n_rows ; z++)
							{
								rowvec pointpca=matPCA2D.row(z);//Fila de 11 elementos(6 primeros del brazo y los 5 ultimos de la mano)

								///////////////////////////////////////////////////

								
								float x1=0.0,y1=0.0,lower=0.0,upper=1.0;
								x1=pointpca(0,0);
								y1=pointpca(0,1);

								if ((lower <= x1 && x1 <=upper)&&(lower <= y1 && y1 <=upper))
								{
									
									///////////////////////////////////////////////////

									for(int k=0; k < 2; k++)
									{
										coord[k]=pointpca(0,k);//coord:variable para almacenar los 11 elementos
									}
									
									
									//tmpSample->setCoords(coord);
									//smp=coord;
									smp->setCoords (coord);
									count++;
								
									//samplefree2d=true;

									//samplefree2d=_wkSpace->collisionCheck(tmpSample);

									if( !(_wkSpace->collisionCheck(smp))) //comprobación si la muestra esta libre de colisiones
									{ 	
									
										n++;
										countpca++;
										int _index=0;						
										_index=_samples->getSize();
										_indexpca.push_back(_index);//Se agrega el indice de la muestra generada por Sampling PCA
										_samples->add(smp);
										
										smp=new Sample(2);
										
										  double r=rgen->d_rand();
										  if(r < 0.1) connectLastSample(_init);
										  else if(r < 0.2) connectLastSample(_goal);
										  else connectLastSample();
										  if( findPath() )
										  {
											printConnectedComponents();
											smoothPath();
											//cout << "Calls to collision-check = " << count <<endl;
											//cout << "Calls PCA " << callpca <<endl;
											_solved = true;
											//break;
											return _solved;
										  }
									
									}
								}

								///////////////////////////////////
								if(countpca>=_numMaxSamplingPCA)
								{
								  countpca=0;
								  break;
								}
								///////////////////////////////////
							}
						}
					}
			}
						//////////////////////////////////////////////////////////
				catch (...){ cout<<"Data not recognize"<<endl;}


				for(int i=0;i<_numMaxSamplingRandom;i)
				{
				   
				   // smp = _sampler->nextSample();
				  
					
					/////////////////////////////////////////////////
					vector<KthReal> _coords(2);
					for(int j = 0; j < 2 ; j++)
					{
						_coords[j] = (KthReal)rgen->d_rand();
					}
					smp->setCoords(_coords);
					////////////////////////////////////////////////
					count++;
					if(!_wkSpace->collisionCheck(smp))
					{
						i++;
						_samples->add(smp) ;
						connectLastSample();
						smp=new Sample(2);
						n++;
					
					}
				}
		}while(n<_maxNumSamples);
	
	//	}//end while
	  }

      printConnectedComponents();
	  // cout << "Calls to collision-check = " << count <<endl;
      _triedSamples = count;
      return _solved;
	}




	bool PRMPlannerPCA::getSampleRandPCA2D()
{
	 LCPRNG* gen = new LCPRNG(15485341);
	///////////////////////////////////////////////////////////////////////
	int sizevd=0; //inicialización de la variable

		sizevd=this->_samples->getSize();///numero de elementos del vector de distancias
		
		//if(sizevd>=11)
		//{	
			///////////////////////////////////
			mat PCA2PMDs(sizevd,2);
			//mat PCA2PMDs(2,2);
			
			//PCA2PMDs(sizevd,11);
			PCA2PMDs.fill(0.0);//inicilización de la matriz
			int rowpca=0;//contador de filas para la matriz
			std::vector<KthReal> coordpca(wkSpace()->getDimension());//variable para recuperar las muestras libres hasta el momento

			for(int i=0;i<sizevd;i++)

			{
				
					Sample *tmpSample=_samples->getSampleAt(i);
					
					for(int k=0; k <2; k++) 
					{coordpca[k]=tmpSample->getCoords()[k];}//Almacenamos el vector de 2 elementos en coordpca
					
					///////////Fill Matrix/////////////////////
					for(int jk = 0; jk <2; jk++) 
					{
						PCA2PMDs(rowpca,jk)=coordpca[jk];
					}
				
					rowpca++;
					
				//}
			}
			
			
		
			int freesamples=PCA2PMDs.n_rows;//numero de muestras libres de 11 elementos

			try{
			if(freesamples>=2)//Condición para el calculo de PCA(minimo 11 elementos)
			{
				///////////////PCA Armadillo//////////////////////////////
				mat coeff;//coeff: principal component coefficients
				vec latent;//latent: principal component variances.
				vec explained;//explained: percentage of the total variance explained by each principal component. 
				princomp_cov(coeff, latent, explained,PCA2PMDs);//Calcula el PCA
				/////////Print///////////////////////////////////////////////////////////
				//cout << "Matrix Rotation:" << endl << coeff << endl;
				//////////Contador Call PCA/////////////////////////////////////////
				callpca++;

				////////Baricentro-Media///////////////////////////
				rowvec bar  = mean(PCA2PMDs);
				////////Print Media/////////////////////////////
				cout << "Media of Data:" << endl << bar << endl;
				//////////Lambdas/////////////////////////////////////
				 rowvec lambdapca = trans(2*sqrt(latent));
				/////////Print Lambdas////////////////////////////
				cout << "Lambdas:" << endl << lambdapca << endl;
				cout << "Matriz:" << endl << PCA2PMDs << endl;

				///////////Inicilización de la Matriz para almacenar 20x11 nuevas muestras en el Espacio PCA/////////////////
				//mat::fixed<50,11> matrand;
				//matrand.fill(0.0);
				int numsamplespca=100;
				mat matrand2D(numsamplespca,2);
				//matrand(numsamplespca,11);
				matrand2D.fill(0.0);

				///////////////////////////////////////Varibles para los 11 lambdas////////////////////////////////////////
				float lambdaC1,lambdaC2;//,lambdaC3,lambdaC4,lambdaC5,lambdaC6,lambdaC7,lambdaC8,lambdaC9,lambdaC10,lambdaC11;
				////////Asignación de los 11 lambdas///////////////////////
				lambdaC1=lambdapca(0,0);			  
				lambdaC2=lambdapca(0,1);
			
		
			//////////////Numero de Muestras por cada vez que se llama a Sampling PCA=20//////////////////
			for(int i=0; i < numsamplespca; i++)
			{
				
			  for(int j=0; j<2; j++)
		       { 
				switch(j)
				{
					///////////Sampling Random desde [-lambda,+lambda]: C1...C11.
					//double rx=rgen->d_rand();
				 case 0: matrand2D(i,j)=-lambdaC1+(2*lambdaC1*(gen->d_rand())); break;
				  //Para la componente J2
				 case 1: matrand2D(i,j)=-lambdaC2+(2*lambdaC2*(gen->d_rand()));break;
				  //Para la componente J3		
		
				  default:
					  break;
				}	   
			  }
			}
			//Procesamiento de las 50x11 muestras, para regresar al Espacio del Mundo Real
			for(int i=0; i<matrand2D.n_rows ; i++)
				 { 
					rowvec pointpcas=matrand2D.row(i);//Fila de 11 elementos(6 primeros del brazo y los 5 ultimos de la mano)
						
					rowvec zeta=trans((coeff*trans(pointpcas))+trans(bar));//Transformacion del Space PCA al Space Real

					matrand2D(i,0)=zeta(0,0);//J1
					matrand2D(i,1)=zeta(0,1);//J2
			
				}
				matPCA2D= matrand2D;
				matrand2D.~Mat();
				PCA2PMDs.~Mat();

				return true;
			}
			}
			/////////////////////////////////
			
			catch (...){return false; }
		//}
		return false;
	///////////////////////////////////////////////////////////////////////
	
}
    //!Finds a solution path in the graph using A*
    bool PRMPlannerPCA::findPath()
	{
		_solved = false;

		if(_init->getConnectedComponent() != _goal->getConnectedComponent()) return false;


		clearSimulationPath();
	    shortest_path.clear(); //path as a vector of prmvertex
	    _path.clear();//path as a vector of samples

	    prmVertex start = _samples->indexOf(_init);
	    prmVertex  goal = _samples->indexOf(_goal);

		//vector to store the parent information of each vertex
		vector<prmGraph::vertex_descriptor> p(num_vertices(*g));
		//vector with cost of reaching each vertex
		vector<cost> d(num_vertices(*g));

		try {
			// call astar named parameter interface
			astar_search(*g, start, 
                      distance_heuristicpca<prmGraph, cost, vector<location> >(locations, goal, _locPlanner),
                      predecessor_map(&p[0]).distance_map(&d[0]).
                      visitor(astar_goal_visitorpca<prmVertex>(goal)));

		}catch(PRM::found_goalpca fg){ // found a path to the goal
			_solved = true;
			//Load the vector shortest_path that represents the solution as a sequence of prmvertex
			for( prmVertex v = goal; ; v = p[v] )
			{
				shortest_path.push_front(v);
				if( p[v] == v ) break;
			}
			//Print solution path
			cout << "Shortest path from " << start << " to " << goal << ": ";
			cout << start;
			list<prmVertex>::iterator spi = shortest_path.begin();
			for(++spi; spi != shortest_path.end(); ++spi)
				cout << " -> " << *spi;
			cout << endl;


			//Load the vector _path that represents the solution as a sequence of samples
			list<prmVertex>::iterator spi2 = shortest_path.begin();
			_path.push_back(_samples->getSampleAt(start));  
			for(++spi2; spi2 != shortest_path.end(); ++spi2)
			{
				_path.push_back(_samples->getSampleAt(*spi2));
			}
			return true;
    }
      
		cout << "Didn't find a path from " << start << " to "
			 << goal << "!" << endl;
		return false;
    }




    //!Load boost graph data
    void PRMPlannerPCA::loadGraph()
	{
	    int maxNodes = this->_samples->getSize();
		unsigned int num_edges = edges.size(); 
      
		// create graph
		g = new prmGraph(maxNodes);
		WeightMap weightmap = get(edge_weight, *g);

		for(std::size_t j = 0; j < num_edges; ++j) 
		{
			edge_descriptor e; 
			bool inserted;//when the efge already exisits or is a self-loop
						  //then this flag is set to false and the edge is not inserted 
			tie(e, inserted) = add_edge(edges[j]->first,edges[j]->second, *g);
			if(inserted) weightmap[e] = weights[j];
		}

		//locations are the pointer to the samples of cspace that 
		//are at each node of the graph and is used to compute the
		//distance when using the heuristic (function distance_heuristicpca)
		for(unsigned int i=0;i<num_vertices(*g); i++)
		    locations.push_back( _samples->getSampleAt(i) );

		_isGraphSet = true;
	}


   //!Update boost graph data
    void PRMPlannerPCA::updateGraph()
	{
        unsigned int newVertices = _samples->getSize();
	    unsigned int oldVertices = num_vertices(*g);
        unsigned int newEdges = edges.size();
	    unsigned int oldEdges = num_edges(*g);

	    // add new vertices
		for(std::size_t i=oldVertices; i<newVertices;i++)
		{	
			prmVertex vd=add_vertex(*g);
		}
      
		WeightMap weightmap = get(edge_weight, *g);

		//add new edges
		for(std::size_t j = oldEdges; j < newEdges; j++) 
		{
			edge_descriptor e; 
			bool inserted;//when the efge already exisits or is a self-loop
						  //then this flag is set to false and the edge is not inserted 
			tie(e, inserted) = add_edge(edges[j]->first,
                                    edges[j]->second, *g);
			if(inserted) weightmap[e] = weights[j];
		}

		//locations are the pointer to the samples of cspace that 
		//are at each node of the graph and is used to compute the
		//distance when using the heuristic (function distance_heuristicpca)
		for(unsigned int i=oldVertices; i<newVertices; i++)
		    locations.push_back( _samples->getSampleAt(i) );
    }


	//! Computes neighbors for the last sample, tries to connect to them, and updates graph 
	//! If parameter connectToSmp is not NULL then the sample pointed is set as a neighbor
	//! of the sample to be connected.
	//! Finally adds the sample to the graph
    void PRMPlannerPCA::connectLastSample(Sample* connectToSmp)
	{
	    int n;
	    Sample *smpFrom;
	    Sample *smpTo;
	    prmEdge *e; //prmEdge is a type defined in PRM.h as std::pair<int, int>
      
 		typedef std::pair<int, SampleSet*> ccPair;
	    
		int ccFrom; //label co connected component of intial sample
		int ccTo; //label co connected component of goal sample

		
		int indexFrom = _samples->getSize() - 1;
	    smpFrom = _samples->getSampleAt(indexFrom);
		
	    //set initial sample of local planner
		_locPlanner->setInitSamp(smpFrom);

		//if already labeled, return
		if(smpFrom->getConnectedComponent() != -1) return;

		smpFrom->setConnectedComponent(_labelCC);	//label sample with connected component
		SampleSet *tmpSS = new SampleSet();			//create new sample set
		tmpSS->add( smpFrom );						//add sample to sample set	
		_ccMap.insert(ccPair(_labelCC,tmpSS));		//create connected component as a labeled sample set
		_labelCC++;

		//compute neighs
		_samples->findNeighs(smpFrom, _neighThress, _kNeighs);
		if(connectToSmp != NULL)
		{
			//smpFrom->addNeigh( _samples->indexOf( connectToSmp ) );
			//add sample connectToSmp as the first neighbor of sample smpFrom (set distance = 0)
			smpFrom->addNeighOrdered(_samples->indexOf( connectToSmp ), 0.0, _kNeighs);
		}

		//srtart connecting with neighs
	    for(unsigned int j=0; j<smpFrom->getNeighs()->size(); j++)
		{
			n = smpFrom->getNeighs()->at(j);
			smpTo = _samples->getSampleAt(n);

			//if same connected component do no try to connect them
			if(smpFrom->getConnectedComponent() == smpTo->getConnectedComponent()) continue;

		    //set goal sample of local planner
			_locPlanner->setGoalSamp(smpTo);

		    //local planner collision checks the edge (if required)
			int canconnect=0;
			//if(smpFrom==goalSamp() || smpTo==goalSamp()) canconnect=1; //do not check
			//else canconnect = _locPlanner->canConect();
			canconnect = _locPlanner->canConect();
		    if( canconnect )
			{
				e = new prmEdge(indexFrom,n);
			  	edges.push_back(e);
			  	weights.push_back(_locPlanner->distance(smpFrom,smpTo));

				//set neigh sample with same label as current sample
				ccTo = smpTo->getConnectedComponent();
				ccFrom = smpFrom->getConnectedComponent();
				if(ccTo == -1)
				{
					smpTo->setConnectedComponent( ccFrom );
					_ccMap[ccFrom]->add( smpTo );
				}
				//set all samples of the connected component of the neigh sample to 
				//the same label as the current sample
				else
				{
					//unify lists under the lowest label
					if( ccFrom < ccTo)
					{
						vector<Sample*>::iterator itera = _ccMap[ccTo]->getBeginIterator();
						while((itera != _ccMap[ccTo]->getEndIterator()))
						{
							(*itera)->setConnectedComponent( ccFrom );
							_ccMap[ccFrom]->add( (*itera) );
							itera++;
						}
            _ccMap[ccTo]->clean();
						_ccMap.erase(ccTo);
					}
					else
					{
						vector<Sample*>::iterator itera = _ccMap[ccFrom]->getBeginIterator();
						while((itera != _ccMap[ccFrom]->getEndIterator()))
						{
							(*itera)->setConnectedComponent( ccTo );
							_ccMap[ccTo]->add( (*itera) );
							itera++;
						}
            _ccMap[ccFrom]->clean();
						_ccMap.erase(ccFrom);
					}
				}
			}
			else
			{
			  //cout<<": FAILED"<<endl;
			  //cout << "edge from " << i << " to " << n << " is NOT free" << endl;
			}
	    }

		//Adds the sample as node of the graph
		updateGraph();
    }


	//!connect samples - put weights 
    bool PRMPlannerPCA::connectSamples(bool assumeAllwaysFree)
	{
	    int n;
	    Sample *smpFrom;
	    Sample *smpTo;
	    prmEdge *e; //prmEdge is a type defined in PRM.h as std::pair<int, int>
      
      cout << "CONNECTING  " << _samples->getSize() << " FREE SAMPLES" << endl;

      typedef std::pair<int, SampleSet*> ccPair;

      int ccFrom; //label co connected component of intial sample
      int ccTo;   //label co connected component of goal sample

	  	unsigned int max;
      if(_samples->getSize() < _maxNumSamples)
          max = _samples->getSize();
      else {
        max = _maxNumSamples;
        cout<<"connectSamples::Using a maximum of "<<max<<" samples"<<endl;
      }

	    for(unsigned int i=0; i<max; i++){
		    //cout<<"Connect sample "<<i<<" with:"<<endl;
        smpFrom = _samples->getSampleAt(i);

        //if not yet labeled, labelwith a new connected component
        if(smpFrom->getConnectedComponent() == -1){
          smpFrom->setConnectedComponent(_labelCC);	//label sample with connected component
          SampleSet *tmpSS = new SampleSet();       //create new sample set
          tmpSS->add( smpFrom );                    //add sample to sample set
          _ccMap.insert(ccPair(_labelCC,tmpSS));		//create connected component as a labeled sample set
          _labelCC++;
        }else
          continue;//already in a connected component

        //set initial sample of local planner
        _locPlanner->setInitSamp(smpFrom);

        //srtart connecting with neighs
		    for(unsigned int j=0; j<smpFrom->getNeighs()->size(); j++){
          n = smpFrom->getNeighs()->at(j);
          smpTo = _samples->getSampleAt(n);

          //if same connected component do no try to connect them
          if(smpFrom->getConnectedComponent() == smpTo->getConnectedComponent()) continue;

			    //set goal sample of local planner
          _locPlanner->setGoalSamp(smpTo);

			    //local planner collision checks the edge (if required)
			    if(assumeAllwaysFree || _locPlanner->canConect())
          {
			  		e = new prmEdge(i,n);
			  		edges.push_back(e);
			  		weights.push_back(_locPlanner->distance(smpFrom,smpTo));

            //set neigh sample with same label as current sample
            ccTo = smpTo->getConnectedComponent();
            ccFrom = smpFrom->getConnectedComponent();
            if(ccTo == -1)
            {
              smpTo->setConnectedComponent( ccFrom );
              _ccMap[ccFrom]->add( smpTo );
            }
            else                        //set all samples of the connected component of the neigh sample to
            {                           //the same label as the current sample
              if( ccFrom < ccTo)        //unify lists under the lowest label
              {
                vector<Sample*>::iterator itera = _ccMap[ccTo]->getBeginIterator();
                while((itera != _ccMap[ccTo]->getEndIterator()))
                {
                  (*itera)->setConnectedComponent( ccFrom );
                  _ccMap[ccFrom]->add( (*itera) );
                  //itera = NULL;
                  itera++;
                }
                 _ccMap[ccTo]->clean();
                 _ccMap.erase(ccTo);
              }
              else
              {
                vector<Sample*>::iterator itera = _ccMap[ccFrom]->getBeginIterator();
                while((itera != _ccMap[ccFrom]->getEndIterator()))
                {
                  (*itera)->setConnectedComponent( ccTo );
                  _ccMap[ccTo]->add( (*itera) );
                  //itera = NULL;
                  itera++;
                }
                _ccMap[ccFrom]->clean();
                _ccMap.erase(ccFrom);
              }
            }
          }
          else
          {
              //cout<<": FAILED"<<endl;
              //cout << "edge from " << i << " to " << n << " is NOT free" << endl;
          }
        }
	    }
	    cout << "END CONNECTING  " << max << " FREE SAMPLES" << endl;

	    return true;
    }


    //!Delete the graph g
    void PRMPlannerPCA::clearGraph(){
  	  weights.clear();
	    edges.clear();
      _samples->clearNeighs();
	    if(_isGraphSet){
		    locations.clear();
		    delete g;
	    }
	    _isGraphSet = false;
      _solved = false;
      _labelCC = 0;
      _ccMap.clear();
      for(int i=0;i<_samples->getSize();i++)
        _samples->getSampleAt(i)->setConnectedComponent(-1);
    }



	//!Print connected components 
    void PRMPlannerPCA::printConnectedComponents()
	{
		std::map<int, SampleSet*>::iterator it;
		cout<<"NUM CONNECTED COMPONENTS = "<<_ccMap.size()<<endl;
		for ( it=_ccMap.begin(); it != _ccMap.end(); it++ )
		{
			cout << "CC " << (*it).first << " => " << (*it).second->getSize()<<" samples: " ;
			vector<Sample*>::iterator itera = (*it).second->getBeginIterator();
			while((itera != (*it).second->getEndIterator()))
			{
				cout <<_samples->indexOf( (*itera) )<<", ";
				itera++;
			}
			cout << endl;
		}	
		cout<<"TOTAL NUMBER OF NODES = "<< _samples->getSize() <<endl;
		//cout << "Weights.size = "<< weights.size() <<" Edges size = " << edges.size()<<endl;
		cout << "Calls PCA " << callpca <<endl;
		// cout << "Calls to collision-check = " << count <<endl;
		
		//cout<<"Num Sampling Goal Region PCA  = "<<_indexpca.size()<<endl;
		cout << "PCA Sampling=> " ;
		for ( int i=0;i<_indexpca.size(); i++ )
		{
			cout<< _indexpca[i]<< "," ;
				
		}	
		cout << endl;
	}



	//!Smooths the path. 
	//!If maintainfirst is set then the first edge is maintained in the smoothed path 
	//!If maintainlast is set then the last edge is maintained in the smoothed path 
	//! They are both initialized to false
	void PRMPlannerPCA::smoothPath(bool maintainfirst, bool maintainlast)
	{
		if(!_solved){
			cout<<"Cannot smooth path - path is not yet solved"<<endl;
			return;
		}
		//START CREATING AN AUXILIAR GRAPH
		//Create a graph with all the samples but with edges connecting those samples
		//of the original path (if collision-free)  
	    int maxNodesPath = _path.size();

		if(maxNodesPath==2)
		{
			cout<<"smoothPath not performed: Path has only two nodes!!"<<endl;
			return;
		}


        vector<prmEdge*> edgesPath;
		vector<cost> weightsPath;

		int last;
		if(maintainlast==true) last=maxNodesPath-1;//do not try to connect other nodes to the final
		else last=maxNodesPath; 

		for(int i=0;i<maxNodesPath-1;i++)
		{
			//connect node i with the next sample (i+1) in path (known to be connectable)
			prmEdge *e = new prmEdge(_samples->indexOf(_path[i]),_samples->indexOf(_path[i+1]));
			edgesPath.push_back(e);
			weightsPath.push_back(_locPlanner->distance(_path[i],_path[i+1]));
			//try to connect with the other samples in the path
			if(i==0 && maintainfirst==true) continue; //do not try to connect initial node with others
			for(int n=i+2;n<last;n++)
			{
				_locPlanner->setInitSamp(_path[i]);
				_locPlanner->setGoalSamp(_path[n]);
				if(_locPlanner->canConect())
				{
					prmEdge *e = new prmEdge(_samples->indexOf(_path[i]),_samples->indexOf(_path[n]));
					edgesPath.push_back(e);
					weightsPath.push_back(_locPlanner->distance(_path[i],_path[n]));
				}
			}
		}
            
		// create graph
		prmGraph *gPath = new prmGraph(_samples->getSize());
		WeightMap weightmapPath = get(edge_weight, *gPath);

		for(std::size_t j = 0; j < edgesPath.size(); ++j) {
			edge_descriptor e; 
			bool inserted;
			tie(e, inserted) = add_edge(edgesPath[j]->first, edgesPath[j]->second, *gPath);
			weightmapPath[e] = weightsPath[j];
		}
	  //END CREATING AN AUXILIAR GRAPH


	  //START FINDING A SMOOTHER PATH
	  //Find path from cini to cgoal along the nodes of gPath
	    prmVertex start = shortest_path.front();
	    prmVertex goal = shortest_path.back();

		//Now clear the unsmoothed solution
        shortest_path.clear(); //path as a vector of prmvertex
	    _path.clear();//path as a vector of samples

		//vector to store the parent information of each vertex
		vector<prmGraph::vertex_descriptor> p(num_vertices(*gPath));
		//vector with cost of reaching each vertex
		vector<cost> d(num_vertices(*gPath));

		try {
        // call astar named parameter interface
        astar_search(*gPath, start, 
                      distance_heuristicpca<prmGraph, cost, vector<location> >(locations, goal, _locPlanner),
                      predecessor_map(&p[0]).distance_map(&d[0]).
                      visitor(astar_goal_visitorpca<prmVertex>(goal)));

    }catch(PRM::found_goalpca fg) { // found a path to the goal
			//list<prmVertex> shortest_path; now is a class parameter
			for( prmVertex v = goal; ; v = p[v] ){
				shortest_path.push_front(v);
				if( p[v] == v ) break;
			}
			cout << "Smoothed path from " << start << " to " << goal << ": ";
			cout << start;
			list<prmVertex>::iterator spi = shortest_path.begin();
			for(++spi; spi != shortest_path.end(); ++spi)
				cout << " -> " << *spi;
			cout << endl;

			//set solution vector
			list<prmVertex>::iterator spi2 = shortest_path.begin();
			_path.push_back(_samples->getSampleAt(start));
	      
			for(++spi2; spi2 != shortest_path.end(); ++spi2){
		      _path.push_back(_samples->getSampleAt(*spi2));
			}
    }
  }

  
} //namespace PRMPlanner
} //namespace libPlanner

#endif

