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


#if !defined(_WORKSPACEGRIDPLANNER_H)
#define _WORKSPACEGRIDPLANNER_H

#include <libproblem/workspace.h>
#include <libsampling/sampling.h>
#include <boost/property_map/property_map.hpp>
#include <boost/graph/adjacency_list.hpp>
#include <boost/graph/filtered_graph.hpp>
#include <boost/graph/breadth_first_search.hpp>
#include <libsplanner/libioc/localplanner.h>
#include <libsplanner/libioc/iocplanner.h>

using namespace std;
using namespace Kautham;
using namespace IOC;

namespace GUIBRO {
	//Typedefs
	//!The location associated to a vertex of the graph will be the distance value to the obstacles
	   struct location{
		   int x; //coordinates x,y,z start at value 1
		   int y;
		   int z;
		   int d;
	   };
	//!The cost of an edge of the graph will be a KthReal
    typedef KthReal cost;
	//!Edge of a graph represented as a pair of ints (the vertices)
    typedef std::pair<unsigned long int, unsigned long int> gridEdge;
	 
	//!Definition of the property of vertices to store the value of a potential function
	struct potential_value_t {
        typedef boost::vertex_property_tag kind;
	};
	//!Graph representing the grid, defined as an adjacency_list with 
	//!a potential value associated to the vertices and a cost associated
	//!to the edges (both defined as KthReals)
    typedef boost::adjacency_list<boost::listS, boost::vecS, 
		boost::undirectedS, boost::property<potential_value_t, KthReal>, 
		boost::property<boost::edge_weight_t, cost> > gridGraph;

    typedef gridGraph::vertex_descriptor gridVertex;
    typedef boost::property_map<gridGraph, boost::edge_weight_t>::type WeightMap;
    typedef boost::property_map<gridGraph, potential_value_t>::type PotentialMap;
    typedef gridGraph::edge_descriptor edge_descriptor;
    typedef gridGraph::vertex_iterator vertex_iterator;

	
	//!Function that filters out edges with weigh below a given threshold
	template <typename EdgeWeightMap>
		struct threshold_edge_weight {
			threshold_edge_weight() { }
			threshold_edge_weight(EdgeWeightMap weight, cost threshold) : m_weight(weight), edgeweightthreshold(threshold) { }
			template <typename Edge>
				bool operator()(const Edge& e) const {
					return get(m_weight, e) > edgeweightthreshold;
				}
			cost edgeweightthreshold;
			EdgeWeightMap m_weight;
	};


	//CLASS bfs_distance_visitor
    // visitor that terminates when we find the goal
    template <class DistanceMap>
    class bfs_distance_visitor : public boost::default_bfs_visitor 
	{
		public:
			bfs_distance_visitor(DistanceMap dist, vector<location> locat, KthReal m) : d(dist),loc(locat),maxdwall(m) {};

			template <typename Edge, typename Graph> 
			void tree_edge(Edge e, Graph& g)
			{
                typename boost::graph_traits<Graph>::vertex_descriptor s=source(e,g);
                typename boost::graph_traits<Graph>::vertex_descriptor t=target(e,g);
				//expand the wave
				d[t] = d[s] + 1;
				//control the expansion value by the distance to the obstacles
				KthReal v = (KthReal)loc[t].d/maxdwall;
				if(v>0.9) v=0.9;
				d[t] -= v;
				//potmap[t] = d[t];
			}
			template <typename Edge, typename Graph> 
			void gray_target(Edge e, Graph& g)
			{
				//for the vertices already visited, update their value if necessary
                typename boost::graph_traits<Graph>::vertex_descriptor s=source(e,g);
                typename boost::graph_traits<Graph>::vertex_descriptor t=target(e,g);
				KthReal v = (KthReal)loc[t].d/maxdwall;
				if(v>0.9) v=0.9;
				KthReal newvalue = d[s] + 1 - v; 
				if(d[t] >  newvalue) 
					d[t] = newvalue;
			}

		private:
			DistanceMap d;
			vector<location> loc;
			KthReal maxdwall;
    };


	//!Graph representing the subgraph of the grid without the edges that have the source or target
	//!vertex associated to a collision cell
    typedef boost::filtered_graph<gridGraph, threshold_edge_weight<WeightMap> > filteredGridGraph;


  //! This class is an class that represents a discretization of the workspace.
	//!It contains a graph representing the whole regular grid with both collision and free cells
	//!associated to the vertices, and a subgraph that contains only those free.
	
    class workspacegridPlanner:public iocPlanner {
	    public:
		//!Constructor
        workspacegridPlanner(SPACETYPE stype, Sample *init, Sample *goal, SampleSet *samples, Sampler *sampler, WorkSpace *ws, KthReal thDist);
        ~workspacegridPlanner();
        
		//!implemented as NF1 navigation function
		inline bool trySolve(){return true;}; 
		inline bool setParameters(){return true;};

		//!Function to compute the navigation function NF1 along the filtered graf
		bool computeNF1(gridVertex vgoal);

		//!Function to compute an harmonic function  along the filtered graf
		//bool computeHF(gridVertex vgoal);

		//!retunrs the origin
		inline KthReal* getOrigin(){return &originGrid[0];};

		//!retunrs the voxelSize
		inline KthReal* getVoxelSize(){return &voxelSize[0];};
		
		//!returns the size of the grid in each axis
		inline int* getDiscretization(){return _stepsDiscretization;};

		//!returns the distance of the cell with label "label"
		bool getDistance(unsigned int label, int *dist);

		//!returns the coordinates x,y,z inthe grid of the cell with label "label"
		bool getCoordinates(unsigned int label, int *x, int *y, int *z);

		
		//!returns pointer to Locations
		inline vector<location>* getLocations(){return &locations;};

		//!retunrs the pointer to the filtered graph
		inline filteredGridGraph* getFilteredGraph(){return fg;};

		//!function to compute the index of the graph vertex from the label on the regular grid
		bool getGraphVertex(unsigned int label, gridVertex *v);

		//!Function to obtain the potential value at a given vertex
		inline KthReal getPotential(int i){return potmap[i];};

		//!Function to obtain the potential value at a given grid cell
		bool getNF1value(unsigned int label, KthReal *NF1value);

		//!Function that retruns the vector of potential values
		inline PotentialMap getpotmat(){return potmap;};

		//!Function to compute the distance gradient for cell with label "label"
		bool  computeDistanceGradient(unsigned int label,  mt::Point3 *f);

		//!Function to set the size of the CT image, in voxels 
		inline void setImageSize(int nx,int ny,int nz){_stepsDiscretization[0]=nx;_stepsDiscretization[1]=ny;_stepsDiscretization[2]=nz;};

		//!Function to to set the size a voxel in mm 
		inline void setVoxelSize(KthReal vx,KthReal vy,KthReal vz){voxelSize[0]=vx;voxelSize[1]=vy;voxelSize[2]=vz;};
		
		//!Function to construct the grid and load it as a boost graph
		void discretizeCspace();

	protected:
		//!position of the grid in the world
		KthReal originGrid[3];

		//!sizes of the voxels
		KthReal voxelSize[3];

		//!map relating the label of a regular grid to the index in the list of graph nodes
		std::map<unsigned int, unsigned int> _cellsMap;

		//!Boost graph representing the whole grid
		gridGraph *g;

		//!Boost graph representing the filtered grid
		filteredGridGraph *fg;	
		
		//!Vector of workspacespace cells associated to the vertices
		vector<location> locations;

		//!Potential values at the cells of the grid
		PotentialMap potmap;

		//!Deiscretization step of the grid, per axis
		int _stepsDiscretization[3];

		//!Potential value of the obstacles
		KthReal _obstaclePotential;

		//!Potential value of the goal cell
		KthReal _goalPotential;

		//!Vector of edges used in the construction of the grid
		vector<gridEdge*> edges;

        //!Vector of edge weights used in the construction of the grid
        vector<cost> weights;

		//!Bool to determine if the graph has been loaded
        bool _isGraphSet;

		//depth of the grid in negative distances, i.e. the graph will consdier all cells with 
		//distance above this threshold
		int _thresholdDist;

		//!maximum distance on the grid (used to tune the NF1 value)
		int maxdist;
		

		//!Function to delete the graphs
	    void clearGraph();

		//!Function that reads the distanceMap file
		void readDistances(string file);

		//!Function to create the edges connecting the vertices of the grid
		void connectgrid(vector<int> &index, int coord_i);
		
		//!Function to filter those edges of the grid  with costs below th threshold 
		void  prunegrid(cost threshold);

		//!Function to set the potential value at a given vertex
		inline void setPotential(int i, KthReal value){potmap[i]=value;};


		
		//void  connectGridFromFile(string file);
	  };
   }


#endif  //_WORKSPACEGRIDPLANNER_H

