#ifndef VIEWPOINT_H
#define VIEWPOINT_H
/*
* This class describes a viewpoint
*/
#include <vapor/ExpatParseMgr.h>
#include "datastatus.h"
namespace VAPoR {
class XmlNode;
class ParamNode;
//! \class Viewpoint
//! \brief class that indicates location and direction of view
//! \author Alan Norton
//! \version 3.0
//! \date February 2014

class PARAMS_API Viewpoint : public ParamsBase {
	
public: 
	//! Constructor, set everything to default view
	Viewpoint() : ParamsBase(0, Viewpoint::_viewpointTag) {//set to default
		previousClass = 0; 
		vector<double> campos(3,0.);
		vector<double> vdir(3,0.);
		vector<double> upvec(3,0.);
		vector<double> rotctr(3,0.);
		campos[2] = 0.5;
		vdir[2]= -1;
		upvec[1] = 1;
		setUpVec(upvec,0);
		setViewDir(vdir,0);
		setCameraPosLocal(campos,0);
		setRotationCenterLocal(rotctr,0);
	}
	
	virtual ~Viewpoint(){}
	static ParamsBase* CreateDefaultInstance() {return new Viewpoint();}
	//! Identify current camera position in local coordinates
	//! \retval const vector<double>& camera position
	const vector<double>& getCameraPosLocal() {
		
		return ( GetValueDoubleVec(_camPosTag));
	}
	//! Determine one local coordinate of current camera position
	//! \param[in] integer coordinate
	//! \retval double corresponding component of local position
	double getCameraPosLocal(int i) {return getCameraPosLocal()[i];}
	//! Specify a coordinate of the camera position (in local coordinates)
	//! \param[in] int coord The 0,1,2 coordinate to be set
	//! \param[in] double val value to be set
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setCameraPosLocal(int coord, double val, Params* p) {
		vector<double> campos= vector<double>(getCameraPosLocal());
		campos[coord] = val;
		return SetValueDouble(_camPosTag, "Set camera position",campos,p);
	}
	//! Specify the camera position (in local coordinates)
	//! \param[in] vector<double> new position
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setCameraPosLocal(const vector<double>&val, Params* p) {
		return SetValueDouble(_camPosTag, "Set camera position", val,p);
	}
	//! Obtain the view direction vector
	//! \retval const vector<double> direction vector
	const vector<double>& getViewDir() {	
		return ( GetValueDoubleVec(_viewDirTag));
	}
	//! Obtain a component of the view direction vector
	//! \param[in] int coordinate
	//! \retval double direction vector component
	double getViewDir(int i) {return getViewDir()[i];}
	//! Specify a component of the camera direction vector
	//! \param[in] int coordinate index (0,1,2)
	//! \param[in] double value of new coordinate
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setViewDir(int coord, double val, Params* p) {	
		vector<double> vdir = vector<double>(getViewDir());
		(vdir)[coord] = val;
		return SetValueDouble(_viewDirTag,"Set view direction", vdir,p);
	}
	//! Specify a component of the camera direction vector
	//! \param[in] const vector<double>& direction vector
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setViewDir(const vector<double>&val, Params*p) {
		return SetValueDouble(_viewDirTag, "Set view direction", val,p);
	}
	//! Obtain view up-vector
	//! \retval const vector<double>& direction vector
	const vector<double>& getUpVec() {
		return ( GetValueDoubleVec(_upVecTag));
	}
	//! Obtain a component of the view up vector
	//! \param[in] int coordinate
	//! \retval const vector<double> direction vector
	double getUpVec(int i) {return getUpVec()[i];}
	//! Specify a component of the view upward direction vector
	//! \param[in] int coordinate index
	//! \param[in] double direction vector component
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setUpVec(int coord, double val, Params* p) {
		
		vector<double> vdir = vector<double>(getUpVec());
		vdir[coord] = val;
		return  SetValueDouble(_upVecTag, "Set up vector", vdir,p);
	}
	//! Specify a component of the view upward direction vector
	//! \param[in] int coordinate index
	//! \param[in] double direction vector component
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setUpVec(const vector<double>&val, Params*p) {
		return  SetValueDouble(_upVecTag, "Set up vector", val,p);
	}
	//! Obtain rotation center
	//! \retval vector<double>& rotation center coordinates
	const vector<double>& getRotationCenterLocal() {
		return ( GetValueDoubleVec(_rotCenterTag));
	}
	//! Obtain one local coordinate of rotation center
	//! \param[in] integer coordinate
	//! \retval double corresponding component of local position
	double getRotationCenterLocal(int i) {return getRotationCenterLocal()[i];}
	//! Specify a coordinate of rotation center in local coordinates
	//! \param[in] int coord;
	//! \param[in] double rotation center local coordinate
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setRotationCenterLocal(int coord, double val, Params* p) {	
		vector<double> vdir(getRotationCenterLocal());
		vdir[coord] = val;
		return  SetValueDouble(_rotCenterTag, "Set rotation center", vdir,p);
	}
	//! Specify rotation center in local coordinates
	//! \param[in] vector<double>& rotation center in local coordinates
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setRotationCenterLocal(const vector<double>&val, Params* p) {
		return SetValueDouble(_rotCenterTag, "Set rotation center", val, p);
	}
	//! Force the rotation center to be aligned with view direction
	//! \param[in] Params* p The Params instance that is requesting this 
	void alignCenter(Params* p);
	//Routines that deal with stretched coordinates:
	//! Obtain rotation center in stretched coordinate
	//! \retval double[3] Position of rotation center in stretched coordinates.
	void getStretchedRotCtrLocal(double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		const vector<double>& rvec = getRotationCenterLocal();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*rvec[i];
	}
	//! Specify rotation center in stretched local coordinates
	//! \param[in] double[3] rotation center in stretched local coordinates
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setStretchedRotCtrLocal(const double* vec, Params* p){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		vector<double> rotCtr(getRotationCenterLocal());
		for (int i = 0; i<3; i++) rotCtr[i] = rotCtr[i]/stretch[i];
		return setRotationCenterLocal(rotCtr, p);
	}
	//! obtain camera position in stretched local coordinates
	//! \param[out] double[3] camera position in stretched local coordinates
	void getStretchedCamPosLocal(double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		const vector<double>& cpos = getCameraPosLocal();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*cpos[i];
	}
	//! Specify camera position in stretched local coordinates
	//! \param[in] double[3] camera position in stretched local coordinates
	//! \param[in] Param* the Params instance that is requesting this setvalue
	//! \retval int 0 if successful
	int setStretchedCamPosLocal(const double* vec, Params* p){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		vector<double> cpos(getCameraPosLocal());
		for (int i = 0; i<3; i++) cpos[i] = cpos[i]/stretch[i];
		return setCameraPosLocal(cpos, p);
	}
#ifndef DOXYGEN_SKIP_THIS
	
	static const string _viewpointTag;
protected:
	static const string _camPosTag;
	static const string _viewDirTag;
	static const string _upVecTag;
	static const string _rotCenterTag;
	
#endif //DOXYGEN_SKIP_THIS
};
};

#endif //VIEWPOINT_H 

