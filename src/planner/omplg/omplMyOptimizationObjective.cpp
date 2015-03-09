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

/* Author: Jan Rosell, Nestor Garcia Hidalgo */



#if defined(KAUTHAM_USE_OMPL)

#include <ompl/base/SpaceInformation.h>
#include <ompl/base/OptimizationObjective.h>
#include "omplMyOptimizationObjective.h"
#include <math.h>
#include <pugixml.hpp>


#define TOL 1e-8


namespace Kautham {
namespace omplplanner {

double distance(const Point3 p, const Segment s) {
    Vector3 v = s.p1 - s.p0;
    Vector3 w = p - s.p0;

    double c1 = dot(w,v);
    if (c1 <= 0.) return length(p-s.p0);

    double c2 = dot(v,v);
    if (c2 <= c1) return length(p-s.p1);

    Point3 pb = s.p0 + (c1/c2)*v;
    return length(p-pb);
}


double distance(Segment s1, Segment s2) {
    Vector3 u = s1.p1 - s1.p0;
    Vector3 v = s2.p1 - s2.p0;
    Vector3 w = s1.p0 - s2.p0;
    double a = dot(u,u);// always >= 0
    double b = dot(u,v);
    double c = dot(v,v);// always >= 0
    double d = dot(u,w);
    double e = dot(v,w);
    double D = a*c - b*b;// always >= 0
    double sc, sN, sD = D;// sc = sN / sD, default sD = D >= 0
    double tc, tN, tD = D;// tc = tN / tD, default tD = D >= 0

    // compute the line parameters of the two closest points
    if (D < TOL) {
        // the lines are almost parallel
        // force using point p0 on segment s1
        // to prevent possible division by 0.0 later
        sN = 0.;
        sD = 1.;
        tN = e;
        tD = c;
    } else {
        // get the closest points on the infinite lines
        sN = b*e - c*d;
        tN = a*e - b*d;
        if (sN < 0.) {
            // sc < 0 => the s=0 edge is visible
            sN = 0.;
            tN = e;
            tD = c;
        } else if (sN > sD) {
            // sc > 1  => the s=1 edge is visible
            sN = sD;
            tN = e + b;
            tD = c;
        }
    }

    if (tN < 0.) {
        // tc < 0 => the t=0 edge is visible
        tN = 0.;
        // recompute sc for this edge
        if (-d < 0.) {
            sN = 0.;
        } else if (-d > a) {
            sN = sD;
        } else {
            sN = -d;
            sD = a;
        }
    } else if (tN > tD) {
        // tc > 1  => the t=1 edge is visible
        tN = tD;
        // recompute sc for this edge
        if ((-d + b) < 0.)
            sN = 0.;
        else if ((-d + b) > a)
            sN = sD;
        else {
            sN = -d + b;
            sD = a;
        }
    }
    // finally do the division to get sc and tc
    sc = (fabs(sN) < TOL ? 0. : sN/sD);
    tc = (fabs(tN) < TOL ? 0. : tN/tD);

    // get the difference of the two closest points
    return length(w + (sc * u) - (tc * v));// =  S1(sc) - S2(tc)
}


/*! Constructor.
   *  \param si is the space information of the problem
   *  \param enableMotionCostInterpolation is a flag set false by default.
   */
myMWOptimizationObjective::myMWOptimizationObjective(const ob::SpaceInformationPtr &si,
                                                     omplPlanner *p,
                                                     double pathLengthWeight):
    ob::MechanicalWorkOptimizationObjective(si,pathLengthWeight),pl(p) {
}


mt::Point3 getPoint(std::string str) {
    mt::Point3 point;
    std::istringstream strs(str);
    int chars_to_read = strs.str().size();
    unsigned int num_read = 0;
    while (chars_to_read > 0 && num_read < 3) {
        getline(strs,str,' ');
        if (str.size() > 0) {
            point[num_read] = atof(str.c_str());
            num_read++;
        }
        chars_to_read -= str.size() + 1;
    }
    if (num_read != 3 || chars_to_read != -1) throw;

    return point;
}


bool myMWOptimizationObjective::setPotentialCost(std::string filename) {
    try {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filename.c_str());
        if (result) {
            point.clear();
            pointCost.clear();
            segment.clear();
            segmentCost.clear();
            for (pugi::xml_node_iterator it = doc.child("Potential").first_child();
                 it != doc.child("Potential").last_child(); ++it) {
                if (std::string(it->name()) == "Point") {
                    point.push_back(getPoint(it->attribute("p").as_string()));
                    pointCost.push_back(std::pair<double,double>
                                        (it->attribute("repulse").as_double(),
                                         it->attribute("diffusion").as_double()));
                } else if (std::string(it->name()) == "Segment") {
                    segment.push_back(Segment(getPoint(it->attribute("p0").as_string()),
                                              getPoint(it->attribute("p1").as_string())));
                    segmentCost.push_back(std::pair<double,double>
                                          (it->attribute("repulse").as_double(),
                                           it->attribute("diffusion").as_double()));
                }
            }

            return true;
        } else {
            std::cout << filename << " " << result.description() << std::endl;

            return false;
        }
    } catch(...) {
        return false;
    }
}


/*! stateCost.
   *  Computes the cost of the state s
   */
ob::Cost myMWOptimizationObjective::stateCost(const ob::State *s) const {
    Sample *smp = new Sample(3);
    //copy the conf of the init smp. Needed to capture the home positions.
    smp->setMappedConf(pl->initSamp()->getMappedConf());
    pl->omplState2smp(s,smp);
    mt::Point3 p(smp->getMappedConf()[0].getSE3().getPos().at(0),
            smp->getMappedConf()[0].getSE3().getPos().at(1),
            smp->getMappedConf()[0].getSE3().getPos().at(2));
    //std::cout << "The state is: " << p << std::endl;
    delete smp;
    
    double sqDist, cost;
    double totalCost = 0.;

    //Points cost
    for (unsigned int i = 0; i < point.size(); ++i) {
        sqDist = Vector3(p-point.at(i)).length2();
        cost = std::max(-pointCost.at(i).first,0.)+pointCost.at(i).first*exp(-pointCost.at(i).second*sqDist);
        totalCost += cost;
        //std::cout<< "Distance " << i << " is " << sqrt(sqDist) << std::endl;
        //std::cout<< "Cost " << i << " is " << cost << std::endl;
    }

    //Segments Cost
    for (unsigned int i = 0; i < segment.size(); ++i) {
        sqDist = pow(distance(p,segment.at(i)),2.);
        cost = std::max(-segmentCost.at(i).first,0.)+segmentCost.at(i).first*exp(-segmentCost.at(i).second*sqDist);
        totalCost += cost;
        //std::cout<< "Distance " << i << " is " << sqrt(sqDist) << std::endl;
        //std::cout<< "Cost " << i << " is " << cost << std::endl;
    }

    //std::cout<< "Totalcost is: " << totalcost << std::endl;
    return ob::Cost(totalCost);
}


/*! Constructor.
   *  \param si is the space information of the problem
   *  \param enableMotionCostInterpolation is a flag set false by default.
   */
myICOptimizationObjective::myICOptimizationObjective(const ob::SpaceInformationPtr &si,
                                                     omplPlanner *p,
                                                     double kP, double kI, double kD) :
    ob::MechanicalWorkOptimizationObjective(si),pl(p),kP_(kP),kI_(kI),kD_(kD) {
}


bool myICOptimizationObjective::setPotentialCost(std::string filename) {
    try {
        pugi::xml_document doc;
        pugi::xml_parse_result result = doc.load_file(filename.c_str());
        if (result) {
            point.clear();
            pointCost.clear();
            segment.clear();
            segmentCost.clear();
            for (pugi::xml_node_iterator it = doc.child("Potential").first_child();
                 it != doc.child("Potential").last_child(); ++it) {
                if (std::string(it->name()) == "Point") {
                    point.push_back(getPoint(it->attribute("p").as_string()));
                    pointCost.push_back(std::pair<double,double>
                                        (it->attribute("repulse").as_double(),
                                         it->attribute("diffusion").as_double()));
                } else if (std::string(it->name()) == "Segment") {
                    segment.push_back(Segment(getPoint(it->attribute("p0").as_string()),
                                              getPoint(it->attribute("p1").as_string())));
                    segmentCost.push_back(std::pair<double,double>
                                          (it->attribute("repulse").as_double(),
                                           it->attribute("diffusion").as_double()));
                }
            }

            return true;
        } else {
            std::cout << filename << " " << result.description() << std::endl;

            return false;
        }
    } catch(...) {
        return false;
    }
}


/*! stateCost.
   *  Computes the cost of the state s
   */
ob::Cost myICOptimizationObjective::stateCost(const ob::State *s) const {
    Sample *smp = new Sample(3);
    //copy the conf of the init smp. Needed to capture the home positions.
    smp->setMappedConf(pl->initSamp()->getMappedConf());
    pl->omplState2smp(s,smp);
    mt::Point3 p(smp->getMappedConf()[0].getSE3().getPos().at(0),
            smp->getMappedConf()[0].getSE3().getPos().at(1),
            smp->getMappedConf()[0].getSE3().getPos().at(2));
    //std::cout << "The state is: " << p << std::endl;

    double sqDist, cost;
    double totalCost = 0.;

    //Points cost
    for (unsigned int i = 0; i < point.size(); ++i) {
        sqDist = Vector3(p-point.at(i)).length2();
        cost = std::max(-pointCost.at(i).first,0.)+pointCost.at(i).first*exp(-pointCost.at(i).second*sqDist);
        totalCost += cost;
        //std::cout<< "Distance " << i << " is " << sqrt(sqDist) << std::endl;
        //std::cout<< "Cost " << i << " is " << cost << std::endl;
    }

    //Segments Cost
    for (unsigned int i = 0; i < segment.size(); ++i) {
        sqDist = pow(distance(p,segment.at(i)),2.);
        cost = std::max(-segmentCost.at(i).first,0.)+segmentCost.at(i).first*exp(-segmentCost.at(i).second*sqDist);
        totalCost += cost;
        //std::cout<< "Distance " << i << " is " << sqrt(sqDist) << std::endl;
        //std::cout<< "Cost " << i << " is " << cost << std::endl;
    }

    //std::cout<< "Total cost is: " << totalcost << std::endl;
    return ob::Cost(totalCost);
}


ob::Cost myICOptimizationObjective::motionCost(const ob::State *s1, const ob::State *s2) const {
    ob::State *test1(si_->cloneState(s1));

    ob::Cost prevStateCost(stateCost(test1));
    ob::Cost nextStateCost;
    ob::Cost totalCost(identityCost());

    unsigned int nd = si_->getStateSpace()->validSegmentCount(s1,s2);
    if (nd > 1) {
        ob::State *test2(si_->allocState());

        for (unsigned int j = 1; j < nd; ++j) {
            si_->getStateSpace()->interpolate(s1,s2,double(j)/double(nd),test2);
            nextStateCost = stateCost(test2);
            totalCost = ob::Cost(totalCost.v + costPID(prevStateCost,nextStateCost,si_->distance(test1,test2)).v);
            std::swap(test1,test2);
            prevStateCost = nextStateCost;
        }

        si_->freeState(test2);
    }

    // Lastly, add s2
    totalCost = ob::Cost(totalCost.v + costPID(prevStateCost,stateCost(s2),si_->distance(test1,s2)).v);

    si_->freeState(test1);

    return totalCost;
}
}
}

#endif // KAUTHAM_USE_OMPL
