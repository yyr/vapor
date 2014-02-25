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

class PARAMS_API Viewpoint : public ParamsBase {
	
public: 
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
	const vector<double>& getCameraPosLocal() {
		
		return ( GetRootNode()->GetElementDouble(_camPosTag));
	}
	double getCameraPosLocal(int i) {return getCameraPosLocal()[i];}
	int setCameraPosLocal(int i, double val, Params* p) {
		
		vector<double> campos= vector<double>(getCameraPosLocal());
		campos[i] = val;
		int rc = CaptureSetDouble(_camPosTag, "Set camera position",campos,p);
		return rc;
	}
	int setCameraPosLocal(const vector<double>&val, Params* p) {
		return CaptureSetDouble(_camPosTag, "Set camera position", val,p);
	}
	const vector<double>& getViewDir() {
		
		return ( GetRootNode()->GetElementDouble(_viewDirTag));
	}
	double getViewDir(int i) {return getViewDir()[i];}
	int setViewDir(int i, double val, Params* p) {
		vector<double> vdir = vector<double>(getViewDir());
		(vdir)[i] = val;
		return CaptureSetDouble(_viewDirTag,"Set view direction", vdir,p);
	}
	int setViewDir(const vector<double>&val, Params*p) {
		int rc = 0;
		vector<double> val2 = val;
		if ((p&& p->GetValidationMode() != NO_CHECK) && (val[0] == val[1] && val[1] == val[2] && val[2] == 0.)){
			if (p->GetValidationMode() == CHECK) return -1;
			else {val2[2] = -1.; rc = -1;}
		}
		int rc2 =  CaptureSetDouble(_viewDirTag, "Set view direction", val2,p);
		if (rc) return rc; else return rc2;
	}
	
	const vector<double>& getUpVec() {
		return ( GetRootNode()->GetElementDouble(_upVecTag));
	}
	double getUpVec(int i) {return getUpVec()[i];}
	int setUpVec(int i, double val, Params* p) {
		vector<double> vdir = vector<double>(getUpVec());
		(vdir)[i] = val;
		return CaptureSetDouble(_upVecTag, "Set up vector", vdir,p);
	}
	int setUpVec(const vector<double>&val, Params*p) {
		int rc = 0;
		vector<double> val2 = val;
		if (p && p->GetValidationMode() != NO_CHECK &&  (val[0] == val[1] && val[1] == val[2] && val[2] == 0.)){
			if (p->GetValidationMode() == CHECK) return -1;
			else {val2[1] = 1.; rc = -1;}
		}
		int rc2 =  CaptureSetDouble(_upVecTag, "Set up vector", val2,p);
		if (rc) return rc; else return rc2;
	}

	const vector<double>& getRotationCenterLocal() {
		
		return ( GetRootNode()->GetElementDouble(_rotCenterTag));
	}
	double getRotationCenterLocal(int i) {return getRotationCenterLocal()[i];}
	int setRotationCenterLocal(int i, double val, Params* p) {
		vector<double> vdir(getRotationCenterLocal());
		(vdir)[i] = val;
		return CaptureSetDouble(_rotCenterTag, "Set rotation center", vdir,p);
	}
	int setRotationCenterLocal(const vector<double>&val, Params* p) {
		return CaptureSetDouble(_rotCenterTag, "Set rotation center", val, p);
	}
	
	void alignCenter(Params* p);
	//Routines that deal with stretched coordinates:
	void getStretchedRotCtrLocal(double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		const vector<double>& rvec = getRotationCenterLocal();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*rvec[i];
	}
	int setStretchedRotCtrLocal(const double* vec, Params* p){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		vector<double> rotCtr(getRotationCenterLocal());
		for (int i = 0; i<3; i++) rotCtr[i] = rotCtr[i]/stretch[i];
		return setRotationCenterLocal(rotCtr, p);
	}

	void getStretchedCamPosLocal(double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		const vector<double>& cpos = getCameraPosLocal();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*cpos[i];
	}
	int setStretchedCamPosLocal(const double* vec, Params* p){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		vector<double> cpos(getCameraPosLocal());
		for (int i = 0; i<3; i++) cpos[i] = cpos[i]/stretch[i];
		return setCameraPosLocal(cpos, p);
	}

	

	static const string _viewpointTag;
protected:
	static const string _camPosTag;
	static const string _viewDirTag;
	static const string _upVecTag;
	static const string _rotCenterTag;
	

};
};
#endif //VIEWPOINT_H 

