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
 
 

#if !defined(_PRMPLANNER_H)
#define _PRMPLANNER_H

#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/astar_search.hpp>
#include "localplanner.h"
#include "planner.h"

using namespace std;
using namespace libSampling;
using namespace boost;

namespace libPlanner {
  namespace PRM{
	//Typedefs
    typedef Sample* location;
    typedef KthReal cost;
    typedef std::pair<int, int> prmEdge;
    typedef boost::adjacency_list<boost::listS, boost::vecS, boost::undirectedS, boost::no_property, boost::property<boost::edge_weight_t, cost> > prmGraph;
    typedef prmGraph::vertex_descriptor prmVertex;
    typedef property_map<prmGraph, boost::edge_weight_t>::type WeightMap;
    typedef prmGraph::edge_descriptor edge_descriptor;
    typedef prmGraph::vertex_iterator vertex_iterator;

	//CLASS distance heuristic
    template <class Graph, class CostType, class LocMap>
    class distance_heuristic : public astar_heuristic<Graph, CostType>  
	{
		public:
		typedef typename graph_traits<Graph>::vertex_descriptor prmVertex;

		distance_heuristic(LocMap l, prmVertex goal, LocalPlanner* lPlan) : m_location(l), m_goal(goal), locPlan(lPlan) {};
		CostType operator()(prmVertex u) 
		{
			//returns euclidean distance form vertex u to vertex goal
			return locPlan->distance(m_location[u],m_location[m_goal]);
		}

		private:
			LocMap m_location;
			prmVertex m_goal;
			LocalPlanner* locPlan;
    };


    struct found_goal{}; // exception for termination

	//CLASS astar_goal_visitor
    // visitor that terminates when we find the goal
    template <class prmVertex>
    class astar_goal_visitor : public boost::default_astar_visitor 
	{
		public:
			astar_goal_visitor(prmVertex goal) : m_goal(goal) {};

			template <class Graph> 
			void examine_vertex(prmVertex u, Graph& g) 
			{
				if(u == m_goal)throw found_goal();
			}

		private:
      prmVertex m_goal;
    };


	//CLASS PRMPlanner
    class PRMPlanner:public Planner 
	{
	    public:
        PRMPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, 
          WorkSpace *ws, LocalPlanner *lcPlan, KthReal ssize);
        ~PRMPlanner();
			bool        setParameters();

      //! This method overwrites the original method in Planner class in order to improve the capabilities
      //! to store the data used in this kind of planner. This method invokes  the parent method to save 
      //! the SampleSet and the planner's parameters and then It stores the connectivity of the samples 
      //!to compound the PRM.
			bool        saveData(string path); // Overwriting the Planner::saveData(string) method.

      void        saveData(); // This is a convenient way to store solution path. Jan.

      bool        loadData(string path); // Overwriting the Planner::loadData(string) method.
			//void        setIniGoal();
			bool        trySolve();

  		//!find path
	    bool findPath();
	    //!load boost graph data
	    void loadGraph();
	    //!connect samples 
	    bool connectSamples(bool assumeAllwaysFree = false);
		  //! connects last sampled configuration & adds to graph
			void connectLastSample(Sample* connectToSmp = NULL);
	    //!delete g
	    void clearGraph();
			void updateGraph();
			void smoothPath(bool maintainfirst=false, bool maintainlast=false);
			bool isGraphSet(){return _isGraphSet;}
			void printConnectedComponents();

	 SoSeparator *getIvCspaceScene();//reimplemented
	 void drawCspace();

		protected:
			vector<prmEdge*> edges;
      //!edge weights
      vector<cost> weights;
      //!bool to determine if the graph has been loaded
      bool _isGraphSet;
			KthReal _neighThress;
			int     _kNeighs;
			std::map<int, SampleSet*> _ccMap;
      int _labelCC;
      int _drawnLink; //!>flag to show which link path is to be drawn
	  KthReal _probabilityConnectionIniGoal; //probability to connect last samp`le to init and goal samp`les

		private:
      PRMPlanner();
      //!boost graph
      prmGraph *g;
      //!solution to query
      list<prmVertex> shortest_path;
      //!pointer to the samples of cspace to be used by the distance_heuristic function used in A*
      vector<location> locations;
	  };
  }
}

#endif  //_PRMPLANNER_H

