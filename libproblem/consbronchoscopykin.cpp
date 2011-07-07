#include "ConsBronchoscopyKin.h"
#include "robot.h"
#include <mt/mt.h> 

namespace libProblem {
	
	ConsBronchoscopyKin::ConsBronchoscopyKin(Robot* const rob):ConstrainedKinematic(rob){
		
		_robot = rob;
		_robConf.setRn(_robot->getNumJoints());	
		_currentvalues[0] = 0.; 
		_currentvalues[1] = 0.; 
		_currentvalues[2] = 0.;
		_maxbending = (KthReal) (2*M_PI/3);
		_maxalpha = (KthReal) (M_PI/4);//(KthReal) (M_PI/2);

	}
	
	ConsBronchoscopyKin::~ConsBronchoscopyKin()
	{

	}

	bool ConsBronchoscopyKin::solve(){
		_robConf = solve(_target);
		return true;
	}


	RobConf ConsBronchoscopyKin::solve(vector<KthReal> &values)
	{
		//Links characteristic
		Link* _linkn = _robot->getLink(_robot->getNumJoints()-1);
		KthReal l = abs(_linkn->getA()); //one link length
		int n=_robot->getNumJoints()-1; //number of bending joints (do not consider base(fixed) and link0(alpha rotation))

		// Current values
		KthReal curr_alpha = getvalues(0);
		KthReal curr_beta = getvalues(1);
		//KthReal curr_z = getvalues(2); // it will not be used cause the slider already gives a Delta
		
		//cout<<"alpha = "<<curr_alpha<<" psi = "<<curr_beta<<endl;

		// Read values
		KthReal Dalpha = _maxalpha *(values[0]-curr_alpha); // slider goes from -1000 to 1000
		KthReal psi = _maxbending*(values[1]/n); // values[1]=total bending angle read from the slider
		KthReal Dzeta = (KthReal)-values[2];//directly the value read from the slider //-1000*(values[2]-currentvalues2);
   

		// sin and cos of rotation angles
		KthReal calpha=cos(Dalpha); 
		KthReal salpha=sin(Dalpha);
    
		// alpha rotation around z-axis
		 mt::Matrix3x3 RzDalpha=Matrix3x3( calpha,   -salpha,  0,
											salpha,    calpha,  0,
											 0,         0,  1);
		mt::Transform T_RzDalpha;
		T_RzDalpha.setRotation(mt::Rotation(RzDalpha));
		T_RzDalpha.setTranslation(mt::Vector3(0,0,0));
    
		//Current Robot config
		RobConf* _CurrentPos = _robot->getCurrentPos();
		SE3Conf& _SE3Conf = _CurrentPos->getSE3();

		vector<KthReal>& PosSE3 = _SE3Conf.getPos();
		vector<KthReal>& OriSE3 = _SE3Conf.getOrient();

		mt::Transform currTran;
		currTran.setRotation(mt::Rotation(OriSE3[0], OriSE3[1], OriSE3[2], OriSE3[3]));
		currTran.setTranslation(mt::Point3(PosSE3[0], PosSE3[1], PosSE3[2]));
		//mt::Transform Tbase;

		//advance rotating
		if (abs(psi)>0.0001){
			KthReal d=l/2*cos(psi/2)/sin(psi/2);	 //dradio = -length/(2*tan(theta/2));	
			KthReal Dbeta = Dzeta/d; //(2*zeta*tan(theta/2))/length;
  		
			KthReal cpsi=cos(psi); 
			KthReal spsi=sin(psi);
			KthReal cbeta=cos(Dbeta);  
			KthReal sbeta=sin(Dbeta);

			mt::Transform T_Y_d_Z_l2;
			T_Y_d_Z_l2.setRotation(mt::Rotation(0.0,0.0,0.0,1));
			T_Y_d_Z_l2.setTranslation(mt::Vector3(0,d,-l/2));

			// Rotation of beta along the x axis -> the bronchoscope go frwd(rotate) in local z-y plane
			mt::Matrix3x3 RxDbeta=Matrix3x3( 1,      0,      0,
											0.0,  cbeta, -sbeta,
											0.0,  sbeta,  cbeta);
                                      
			mt::Transform T_RxDbeta;
			T_RxDbeta.setRotation(mt::Rotation(RxDbeta));
			T_RxDbeta.setTranslation(mt::Vector3(0,0,0));

			// updating the base transform
			//mt::Transform 
			Tbase=currTran*T_RzDalpha*T_Y_d_Z_l2*T_RxDbeta*T_Y_d_Z_l2.inverse(); // beta around x axis
		}
		//else rectilinear advance
		else {
			mt::Transform T_X0_Y0_Zvel;
			T_X0_Y0_Zvel.setRotation(mt::Rotation(0.0,0.0,0.0,1));
			//JAN : changed Dzeta to -Dzeta in next line 2011-03-26
			T_X0_Y0_Zvel.setTranslation(mt::Vector3(0,0,-Dzeta));
      
			//mt::Transform 
			Tbase=currTran*T_RzDalpha*T_X0_Y0_Zvel; // beta around x axis
		}

		//update joint values
		RnConf& _RnConf = _CurrentPos->getRn();
		vector<KthReal>& rncoor= _RnConf.getCoordinates();
		rncoor[0] = values[0]; //alpha
		for(int i =1; i< rncoor.size(); i++)
			rncoor[i] = values[1]/n;//i.e. psi/_maxbending;


    // updating the _currentvalues variable with the new read values
		setvalues(values[0],0);
		setvalues(values[1],1);
		setvalues(values[2],2);		

		const mt::Point3& basePos = Tbase.getTranslationRef(); //resTra=result.getTranslationRef();

		vector<KthReal> coords(7);
		for(int i =0; i < 3; i++)
			coords[i] = basePos[i];

		const mt::Rotation& baseRot = Tbase.getRotationRef(); //resRot=result.getRotationRef();

		for(int i =0; i < 4; i++)
			coords[i+3] = baseRot[i];

		_robConf.setSE3(coords);
		_robConf.setRn(_RnConf);
		_robot->Kinematics(_robConf);
		
		return _robConf;
	}


	//computes the values alpha and beta corresponding to the current robot configuration
	void ConsBronchoscopyKin::registerValues()
	{
		RobConf* _CurrentPos = _robot->getCurrentPos();
		RnConf& _RnConf = _CurrentPos->getRn();
		vector<KthReal>& rncoor= _RnConf.getCoordinates();
		setvalues(rncoor[0],0);//alpha
		setvalues(rncoor[1]*(rncoor.size()-1),1);//beta
	}

	bool ConsBronchoscopyKin::setParameters(){
		return true;
	}

}
