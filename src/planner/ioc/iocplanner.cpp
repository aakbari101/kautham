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

 

#include <string>
#include <sstream>
#include <boost/algorithm/string.hpp>
#include "iocplanner.h"
#include "linearlocplan.h"
#include <pugixml.hpp>
#include <iostream>
#include <fstream>

using namespace pugi;

namespace Kautham{

 namespace IOC{

  const KthReal RAD2GRAD=180.0/M_PI;

  iocPlanner::iocPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, WorkSpace *ws):
      Planner(stype, init, goal, samples, ws)
  {
      _family = "ioc";
    _guiName = _idName = "";
      _sampler = sampler;

    _triedSamples = _generatedEdges = _collChecks = 0;
    _maxNumSamples = 1000;
    _totalTime = _smoothTime = 0. ;


    KthReal ssize = 0.05;//dummy
    _locPlanner = new LinearLocalPlanner(CONTROLSPACE, NULL,NULL, _wkSpace, ssize);
	}

  iocPlanner::~iocPlanner(){

  }




  bool iocPlanner::addQuery(int init, int goal ){
    KthQuery tmp(init, goal);
    _queries.push_back(tmp);
      return true;
  }

  int iocPlanner::findQuery(int init, int goal, int from){
    if( from < _queries.size() ){
      for(int i = from; i < _queries.size(); i++)
        if( _queries[i].sameInitGoal(init, goal) ) return i;
    }
    return -1;
  }


  bool iocPlanner::solveAndInherit(){
    Element::resetCollCheckCounter();
    WorkSpace::resetCollCheckCounter();

      clock_t entertime = clock();

    _solved = false;
    if(trySolve()){

        if(_totalTime==0.0)
        {
            clock_t finaltime = clock();
            _totalTime = (KthReal)(finaltime - entertime)/CLOCKS_PER_SEC ;
        }

      addZeroCrossingToPath();
      moveAlongPath(0);
      _wkSpace->inheritSolution(_simulationPath);
      _worldcollChecks = WorkSpace::getCollCheckCounter();
      _collChecks = Element::getCollCheckCounter();

      // Add the results to the Query vector.
      KthQuery* currQue = NULL;
      addQuery( _samples->indexOf( _init ), _samples->indexOf( _goal ));
      currQue = &(_queries.at( _queries.size() - 1 ));
      currQue->solved(_solved);
      currQue->setSampleStats(_triedSamples, _samples->getSize(), _generatedEdges, _collChecks, _worldcollChecks);
      currQue->setTotalTime( _totalTime );
      currQue->setSmoothTime( _smoothTime );
      vector<int> solu;
      for(int i = 0; i < _path.size(); i++)
        solu.push_back(_samples->indexOf( _path[i] ));
      currQue->setPath( solu );
      return true;
    }
    return false;
  }


  bool iocPlanner::saveData(string path){

    xml_document doc;
    xml_node planNode = doc.append_child();
    planNode.set_name("Planner");
    planNode.append_attribute("ProblemName") = "";
    planNode.append_attribute("Date") = "";
    xml_node paramNode = planNode.append_child();
    paramNode.set_name("Parameters");
    xml_node planname = paramNode.append_child();
    planname.set_name("Name");
    planname.append_child(node_pcdata).set_value(_idName.c_str());
    

    // Adding the parameters
    string param = getParametersAsString();
    vector<string> tokens;
    boost::split(tokens, param, boost::is_any_of("|"));

    for(int i=0; i<tokens.size(); i=i+2){
      xml_node paramItem = paramNode.append_child();
      paramItem.set_name("Parameter");
      paramItem.append_attribute("name") = tokens[i].c_str();
      paramItem.append_child(node_pcdata).set_value(tokens[i+1].c_str());
    }

    // Adding the Query information
    xml_node queryNode = planNode.append_child();
    queryNode.set_name("Queries");
    vector<KthQuery>::iterator it = _queries.begin();
    for(it = _queries.begin(); it != _queries.end(); ++it){
      xml_node queryItem = queryNode.append_child();
      queryItem.set_name("Query");
      queryItem.append_attribute("Init") = (*it).printInit().c_str();
      queryItem.append_attribute("Goal") = (*it).printGoal().c_str();
      queryItem.append_attribute("Solved") = (*it).printSolved().c_str();

      xml_node queryItemResultP = queryItem.append_child();
      queryItemResultP.set_name("Result");
      queryItemResultP.append_attribute("name") = "Path";
      queryItemResultP.append_child(node_pcdata).set_value( (*it).printPath().c_str() );

      xml_node queryItemResultT = queryItem.append_child();
      queryItemResultT.set_name("Result");
      queryItemResultT.append_attribute("name") = "TotalTime";
      queryItemResultT.append_child(node_pcdata).set_value( (*it).printTotalTime().c_str() );

      xml_node queryItemResultST = queryItem.append_child();
      queryItemResultST.set_name("Result");
      queryItemResultST.append_attribute("name") = "SmoothTime";
      queryItemResultST.append_child(node_pcdata).set_value( (*it).printSmoothTime().c_str() );

      xml_node queryItemResultG = queryItem.append_child();
      queryItemResultG.set_name("Result");
      queryItemResultG.append_attribute("name") = "Generated Samples";
      queryItemResultG.append_child(node_pcdata).set_value( (*it).printGeneratedSamples().c_str() );

      xml_node queryItemResultE = queryItem.append_child();
      queryItemResultE.set_name("Result");
      queryItemResultE.append_attribute("name") = "Generated Edges";
      queryItemResultE.append_child(node_pcdata).set_value( (*it).printGeneratedEdges().c_str() );

	  
      xml_node queryItemResultC = queryItem.append_child();
      queryItemResultC.set_name("Result");
      queryItemResultC.append_attribute("name") = "Collision-check calls";
      queryItemResultC.append_child(node_pcdata).set_value( (*it).printCollCheckCalls().c_str() );

	  
      xml_node queryItemResultWC = queryItem.append_child();
      queryItemResultWC.set_name("Result");
      queryItemResultWC.append_attribute("name") = "WorldCollision-check calls";
      queryItemResultWC.append_child(node_pcdata).set_value( (*it).printWorldCollCheckCalls().c_str() );
    }

    //  Now it is adding the samples set.
    xml_node sampNode = planNode.append_child();
    sampNode.set_name("SampleSet");
    sampNode.append_attribute("dim") = _wkSpace->getNumRobControls();
    sampNode.append_attribute("size") = _samples->getSize();
    for(int i = 0; i < _samples->getSize(); i++){
      xml_node sampItem = sampNode.append_child();
      sampItem.set_name("Sample");
      sampItem.append_attribute("conComp") = _samples->getSampleAt(i)->getConnectedComponent();
      sampItem.append_child(node_pcdata).set_value(_samples->getSampleAt(i)->print(true).c_str());
    }
    
    saveData();
    exportSimulationPath();
    return doc.save_file(path.c_str());
  }

  bool iocPlanner::loadData(string path){
    // First, I will clean SampleSet
    _samples->clear();
    _queries.clear();
    _path.clear();

    xml_document doc;
    xml_parse_result result = doc.load_file(path.c_str());

    xml_node tempNode = doc.child("Planner").child("Parameters");
    std::stringstream temp;
    if( !(_idName.compare(tempNode.child("Name").value()) )) // it means the solution has been produced with the same planner
      return false;
    std::string par="";
    for (pugi::xml_node_iterator it = tempNode.begin(); it != tempNode.end(); ++it){
      par = it->name();
      if( par == "Parameter" ){
        temp << it->attribute("name").value() << "|";
        temp << it->child_value() << "|";
      }
    }
    par = temp.str();
    try{
      setParametersFromString(par.substr(0,par.length()-1)); // Trying to set the read parameter
    }catch(...){
      std::cout << "Current planner doesn't have at least one of the parameters" 
        << " you found in the file (" << par << ")." << std::endl; 
      return false; //if it is wrong maybe the file has been generated with another planner.
    }

    tempNode = doc.child("Planner").child("SampleSet");

    char dim = _wkSpace->getNumRobControls();
    if( dim != tempNode.attribute("dim").as_int()){
      std::cout << "Dimension of samples doesn't correspond with the problem's dimension."
          << std::endl;
      return false;
    }

    vector<KthReal> coordsVec(dim);
    Sample* tmpSampPointer=NULL;

    for (pugi::xml_node_iterator it = tempNode.begin(); it != tempNode.end(); ++it){
      string sentence = it->child_value();
      vector<string> tokens;
      boost::split(tokens, sentence, boost::is_any_of("| "));
      if(tokens.size() != dim){
        std::cout << "Dimension of a samples doesn't correspond with the problem's dimension."
            << std::endl;
        return false;
      }

      tmpSampPointer = new Sample(dim);
      for(char i=0; i<dim; i++)
        coordsVec[i] = (KthReal)atof(tokens[i].c_str());

      tmpSampPointer->setCoords(coordsVec);
      tmpSampPointer->setConnectedComponent(it->attribute("conComp").as_int());
      //samples are free samples, then set the flag
      tmpSampPointer->setFree(true);

      _samples->add(tmpSampPointer);
    }
    if(_samples->getSize() != tempNode.attribute("size").as_int()){
      std::cout << "Something wrong with the samples. Not all samples has been loaded."
          << std::endl;
      return false;
    }
    // Now, the queries are loaded if exists.
    tempNode = doc.child("Planner").child("Queries");
    for (pugi::xml_node_iterator it = tempNode.begin(); it != tempNode.end(); ++it){
      // Add the results to the Query vector.
      KthQuery* currQue = NULL;
      addQuery( it->attribute("Init").as_int(),  it->attribute("Goal").as_int());
      currQue = &(_queries.at( _queries.size() - 1 ));
      currQue->solved(it->attribute("Solved").as_bool()); 
      //pugi::xml_node tmpStat=it->child("Query");
      std::string par="";
      for (pugi::xml_node_iterator it2 = it->begin(); it2 != it->end(); ++it2){
        par = it2->attribute("name").value();
        if( par=="Path" )                   currQue->setPath( it2->child_value() );
        if( par=="TotalTime" )              currQue->setTotalTime( atof( it2->child_value() )  );
        if( par=="SmoothTime" )             currQue->setSmoothTime( atof( it2->child_value() )   );
        if( par=="Generated Samples" )      currQue->setGeneratedSamples( atoi( it2->child_value() )  );
        if( par=="Generated Edges" )        currQue->setGeneratedEdges( atoi( it2->child_value() )  );
        if( par=="Collision-check calls" )  currQue->setCollCheckCalls( atoi( it2->child_value() )  );
        if( par=="WorldCollision-check calls" )  currQue->setWorldCollCheckCalls( atoi( it2->child_value() )  );
      }
    }
    // If load data of the planner includes the Queries information then the first solved
    // one is loaded if exists.
    for(size_t i=0; i< getQueries().size(); ++i){
      if(getQueries()[i].solved()){ //Load it
        _solved = true;
        this->setInitSamp( _samples->getSampleAt(getQueries()[i].getInit() ));
        this->setGoalSamp( _samples->getSampleAt(getQueries()[i].getGoal() ));

        for(size_t j=0; j< getQueries()[i].getPath().size(); ++j)
          _path.push_back( _samples->getSampleAt(getQueries()[i].getPath()[j]) );

        addZeroCrossingToPath();
        moveAlongPath(0);
        _wkSpace->inheritSolution(_simulationPath);

        std::cout << "The path loaded solves the query: " << this->getQueries()[i].getInit(); 
        std::cout << " to " << this->getQueries()[i].getGoal()  <<  std::endl;
        break;
      }
    }
    return true;
  }

  void iocPlanner::saveData(){};

  void iocPlanner::addZeroCrossingToPath(){
    // This method only takes into account the Rn part of 
    //  the MappenConfiguration associated to each sample 
    // into the _path vector. In this method, I copy in a new
    // vector all the content of the path and I add the zero
    // crossing samples as needed

    vector<Sample*>  newPath;// = new vector<Sample*>();
    size_t confDim = 0;
    for(size_t i = 0; i < _path[0]->getMappedConf().size(); ++i)
      confDim += _path[0]->getMappedConf().at(i).getRn().getDim();

    bool*    signs = new bool[ confDim ]; // This vector is used to follow the change of signs.
    KthReal* advance = new KthReal[ confDim ];
    bool     tmpsigns = false;               // Is the coordinate sign positive?
    char     needInter = 0;
    
    Sample *tmpSamp;
    size_t cummul = 0;
    for(size_t i = 0; i < _path[0]->getMappedConf().size(); ++i ){
      for(size_t k = 0; k < _path[0]->getMappedConf().at(i).getRn().getDim(); k++ ){
        signs[cummul] = _path[0]->getMappedConf().at(i).getRn().getCoordinate( k ) >= 0 ? true : false;
        cummul++;
      }
    }

    // Initialized the copy
    newPath.push_back( _path.at(0) );

    for(size_t k = 0; k < confDim; k++)
      advance[k] = 0.;
      
    needInter = 0;
    for(size_t i = 1; i < _path.size(); i++){
      cummul = 0;
      for(size_t j = 0; j < _path[i]->getMappedConf().size(); ++j){
        for(size_t k = 0; k < _path[i]->getMappedConf().at(j).getRn().getDim(); k++ ){
          tmpsigns = _path[i]->getMappedConf().at(j).getRn().getCoordinate( k ) >= 0 ? true : false;

          if( tmpsigns != signs[cummul] ){ // then needs additional interpolation to include the zero crossing point.
            advance[cummul] = - _path[i-1]->getMappedConf().at(j).getRn().getCoordinate( k );
            advance[cummul] /= _path[i]->getMappedConf().at(j).getRn().getCoordinate( k ) - 
                               _path[i-1]->getMappedConf().at(j).getRn().getCoordinate( k );

            if( advance[cummul] > 0. && advance[cummul] < 1.0 ) needInter ++;
            if( advance[cummul] == 1. ) advance[cummul] = 0. ;
            signs[cummul] = tmpsigns;
          }else
            advance[cummul] = 0.;

          ++cummul;
        } 
      }

      while( needInter > 0 ){
        size_t min = 0;
        KthReal tmpVal = 100000.;

        for(size_t k = 0; k < confDim; k++){
          if( advance[k] != 0. ){
            min = advance[k] < tmpVal ? k: min;
            tmpVal = advance[min];
          }
        }
        advance[min] = 0.;
        tmpSamp = _path.at(i-1)->interpolate( _path.at(i), tmpVal );
        _samples->add( tmpSamp );
        newPath.push_back( tmpSamp );
        --needInter;
      }

      newPath.push_back( _path.at(i) );
    }

    _path.clear();

    for( size_t i = 0; i < newPath.size(); ++i)
      _path.push_back( newPath.at(i) );
  
    delete[] advance;
    delete[] signs;
  }
 }
}
