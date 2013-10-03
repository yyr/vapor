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
		setUpVec(upvec);
		setViewDir(vdir);
		setCameraPosLocal(campos);
		setRotationCenterLocal(rotctr);
	}
	
	virtual ~Viewpoint(){}
	static ParamsBase* CreateDefaultInstance() {return new Viewpoint();}
	const vector<double>& getCameraPosLocal() {
		
		return ( GetRootNode()->GetElementDouble(_camPosTag));
	}
	double getCameraPosLocal(int i) {return getCameraPosLocal()[i];}
	int setCameraPosLocal(int i, double val) {
		
		vector<double> campos= vector<double>(getCameraPosLocal());
		campos[i] = val;
		int rc = GetRootNode()->SetElementDouble(_camPosTag, campos);
		return rc;
	}
	int setCameraPosLocal(const vector<double>&val) {
		return GetRootNode()->SetElementDouble(_camPosTag, val);
	}
	const vector<double>& getViewDir() {
		
		return ( GetRootNode()->GetElementDouble(_viewDirTag));
	}
	double getViewDir(int i) {return getViewDir()[i];}
	int setViewDir(int i, double val) {
		vector<double> vdir = vector<double>(getViewDir());
		(vdir)[i] = val;
		int rc= GetRootNode()->SetElementDouble(_viewDirTag, vdir);
		return rc;
	}
	int setViewDir(const vector<double>&val) {
		return GetRootNode()->SetElementDouble(_viewDirTag, val);
	}
	
	const vector<double>& getUpVec() {
		return ( GetRootNode()->GetElementDouble(_upVecTag));
	}
	double getUpVec(int i) {return getUpVec()[i];}
	int setUpVec(int i, double val) {
		vector<double> vdir = vector<double>(getUpVec());
		(vdir)[i] = val;
		int rc= GetRootNode()->SetElementDouble(_upVecTag, vdir);
		return rc;
	}
	int setUpVec(const vector<double>&val) {
		return GetRootNode()->SetElementDouble(_upVecTag, val);
	}

	const vector<double>& getRotationCenterLocal() {
		
		return ( GetRootNode()->GetElementDouble(_rotCenterTag));
	}
	double getRotationCenterLocal(int i) {return getRotationCenterLocal()[i];}
	int setRotationCenterLocal(int i, double val) {
		vector<double> vdir(getRotationCenterLocal());
		(vdir)[i] = val;
		int rc= GetRootNode()->SetElementDouble(_rotCenterTag, vdir);
		return rc;
	}
	int setRotationCenterLocal(const vector<double>&val) {
		return GetRootNode()->SetElementDouble(_rotCenterTag, val);
	}
	
	void alignCenter();
	//Routines that deal with stretched coordinates:
	void getStretchedRotCtrLocal(double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		const vector<double>& rvec = getRotationCenterLocal();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*rvec[i];
	}
	void setStretchedRotCtrLocal(const double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		vector<double> rotCtr(getRotationCenterLocal());
		for (int i = 0; i<3; i++) rotCtr[i] = rotCtr[i]/stretch[i];
		setRotationCenterLocal(rotCtr);
	}

	void getStretchedCamPosLocal(double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		const vector<double>& cpos = getCameraPosLocal();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*cpos[i];
	}
	void setStretchedCamPosLocal(const double* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		vector<double> cpos(getCameraPosLocal());
		for (int i = 0; i<3; i++) cpos[i] = cpos[i]/stretch[i];
		setCameraPosLocal(cpos);
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

