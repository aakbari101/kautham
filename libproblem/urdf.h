#include <libpugixml/pugixml.hpp>
#include <iostream>
#include <sstream>
#include <string>
#include <stdlib.h>
#include <mt/point3.h>
#include <mt/rotation.h>
#include <mt/transform.h>

using namespace std;
using namespace pugi;

/** \addtogroup libProblem
 *  @{
 */

//! Class containing origin data
class urdf_origin {
public:
    mt::Point3 xyz;//!< Translation in mm, defaults to zero vector
    double r;//!< Roll angle around x axis in rad
    double p;//!< Pitch angle around y axis in rad
    double y;//!< Yaw angle around z axis in rad
    mt::Transform transform;//!< Transform

    urdf_origin ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given an origin node
};

//! Class containing inertia data
class urdf_inertia {
public:
    double ixx;//!< Above-diagonal element of the matrix inertia in kg·m², defaults to zero
    double ixy;//!< Above-diagonal element of the matrix inertia in kg·m², defaults to zero
    double ixz;//!< Above-diagonal element of the matrix inertia in kg·m², defaults to zero
    double iyy;//!< Above-diagonal element of the matrix inertia in kg·m², defaults to zero
    double iyz;//!< Above-diagonal element of the matrix inertia in kg·m², defaults to zero
    double izz;//!< Above-diagonal element of the matrix inertia in kg·m², defaults to zero
    mt::Matrix3x3 matrix;//!< 3x3 rotational inertia matrix, represented in the inertia frame, defaults to zero matrix

    urdf_inertia ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given an inertia node
};

//! Class containing the inertial properties of the link
class urdf_inertial {
public:
    //! Pose of the inertial reference frame, relative to the link reference frame.
    /*! The origin of the inertial reference frame needs to be at the center of gravity.
        The axes of the inertial reference frame do not need to be aligned with the principal axes of the inertia.
    */
    urdf_origin origin;
    double mass;//!< Mass in kg, defaults to zero
    urdf_inertia inertia;//!< Inertia

    urdf_inertial ();//!< Class constructor
    void fill (xml_node * node);//!< Fills variables given an inertial node
};

//! Class containing the shape of the object
class urdf_geometry {
public:
    string ivfile;//!< Path to the Inventor file that contains the solid
    double scale;//!< Model scale, defaults to one

    urdf_geometry();//!< Class constructor
};

//! Class containing joint's dynamics data
class urdf_dynamics {
public:
    //! Physical static friction, defaults to zero
    /*! For prismatic joints, in N and for revolving joints, in N·m
    */
    double friction;
    //! Physical damping value, defaults to zero
    /*! For prismatic joints, in N·s/m and for revolving joints, N·m·s/rad
    */
    double damping;

    urdf_dynamics ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given a dynamics node
};

//! Class that defines how the links behave when they are in contact with one another.
class urdf_contact_coefficients {
public:
    double mu;//!< Friction coefficient, defaults to zero
    double kp;//!< Stiffness coefficient, defaults to zero
    double kd;//!< Dampening coefficient, defaults to zero

    urdf_contact_coefficients ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given a contact_coefficients node
};

//! Class containing the limits of the joint
class urdf_limit {
public:
    double lower;//!< Lower joint limit (rad for revolute joints, m for prismatic joints), defaults to zero
    double upper;//!< Upper joint limit (rad for revolute joints, m for prismatic joints), defaults to zero
    double effort;//!< Enforces the maximum joint effort (|applied effort| < |effort|), defaults to zero
    double velocity;//!< Enforces the maximum joint velocity, defaults to zero

    urdf_limit ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given a limit node
};

//! Class that describes a link
class urdf_link {
public:
    //! Name of the link itself
    string name;
    //! Name of the joint whose child link is this link.
    string joint;
    //! Type of joint
    /*! Type can be one of the following:

        -revolute: a hinge joint that rotates along the axis and has a limited range specified by the upper and lower limits.

        -continuous: a continuous hinge joint that rotates around the axis and has no upper and lower limits.

        -prismatic: a sliding joint that slides along the axis, and has a limited range specified by the upper and lower limits.

        -fixed: this is not really a joint because it cannot move. All degrees of freedom are locked. This type of joint does not require the axis, calibration, dynamics, limits or safety_controller.

        -floating: this joint allows motion for all 6 degrees of freedom.

        -planar: this joint allows motion in a plane perpendicular to the axis.
    */
    string type;
    //! Value for weighted distance computations, defaults to 1.0
    double weight;
    //! Name of the link that is the parent of this link in the robot tree structure.
    string parent;
    //! Visual properties of the link.
    /*! This element specifies the shape of the object for visualization purposes.
    */
    urdf_geometry visual;
    //! Collision properties of a link.
    /*! Note that this can be different from the visual properties of a link, for example, simpler collision models are often used to reduce computation time.
    */
    urdf_geometry collision;
    //! Inertial propierties from the link
    urdf_inertial inertial;
    //! Transform from the parent link frane to the joint frame
    /*! The joint is located at the origin of the child link
    */
    urdf_origin origin;
    //! Defaults to (1,0,0)
    /*! The joint axis specified in the joint frame. This is the axis of rotation for revolute joints,
        the axis of translation for prismatic joints, and the surface normal for planar joints.
        The axis is specified in the joint frame of reference. Fixed and floating joints do not use the axis field.
    */
    mt::Unit3 axis;
    urdf_dynamics dynamics;//!< Dynamics of the joint
    urdf_contact_coefficients contact_coefficients;//!< Contact coefficients of the link
    urdf_limit limit;//!< Limits of the joint
    bool is_base;//!< It says if this links is the base from the robot tree structure, defaults to false

    urdf_link ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given a robot node
    mt::Transform transform (double theta);//!< Returns the transform from the parent link to the child link given the value of the joint's degree of freedom, only for revolute joints
};

//! Class that describes a robot
class urdf_robot {
public:
    string name;//!< Name of the robot
    string type;//!< Type of the robot, where type can be one of the following:
    //!< "Freeflying", "Chain" or "Tree"
    int num_links;//!< Number of links forming the robot structure
    urdf_link *link;//!< Robot's links

    urdf_robot ();//!< Class constructor
    void fill (xml_node *node);//!< Fills variables given a robot node
    void print();//!< Prints robot information
};

//! Class that describes an obstacle. Obstacles are defined as a robot with a unique link and no joints
class urdf_obstacle {
public:
    //! Visual properties of the link.
    /*! This element specifies the shape of the object for visualization purposes.
    */
    urdf_geometry visual;
    //! Collision properties of a link.
    /*! Note that this can be different from the visual properties of a link, for example, simpler collision models are often used to reduce computation time.
    */
    urdf_geometry collision;
    urdf_inertial inertial;//!< Inertial propierties from the obstacle
    urdf_contact_coefficients contact_coefficients;//!< Contact coefficients of the obstacle

    void fill(xml_node *node);//!< Fills variables given a link node
    void print();//!< Prints obstacle information
};

/** @}   end of Doxygen module "libProblem" */