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

class PARAMS_API Viewpoint : public ParsedXml {
	
public: 
	Viewpoint() {previousClass = 0;}
	
	virtual ~Viewpoint(){}
	
	float* getCameraPosLocal() {return cameraPosition;}
	float getCameraPosLocal(int i) {return cameraPosition[i];}
	void setCameraPosLocal(int i, float val) { cameraPosition[i] = val;}
	void setCameraPosLocal(float* val) {cameraPosition[0] = val[0]; cameraPosition[1]=val[1]; cameraPosition[2] = val[2];}
	float* getCamPosLatLon() {return camLatLon;}
	void setCamPosLatLon(float x, float y){camLatLon[0] = x; camLatLon[1] = y;}
	float* getRotCenterLatLon() {return rotCenterLatLon;}
	void setRotCenterLatLon(float x, float y){rotCenterLatLon[0] = x; rotCenterLatLon[1] = y;}
	float* getViewDir() {return viewDirection;}
	float getViewDir(int i) {return viewDirection[i];}
	void setViewDir(int i, float val) { viewDirection[i] = val;}
	void setViewDir(float* val) {viewDirection[0] = val[0]; viewDirection[1]=val[1]; viewDirection[2] = val[2];}
	float* getUpVec() {return upVector;}
	float getUpVec(int i) {return upVector[i];}
	void setUpVec(int i, float val) { upVector[i] = val;}
	void setUpVec(float* val) {upVector[0] = val[0]; upVector[1]=val[1]; upVector[2] = val[2];}
	float* getRotationCenterLocal() {return rotationCenter;}
	float getRotationCenterLocal(int i) {return rotationCenter[i];}
	void setRotationCenterLocal(int i, float val) { rotationCenter[i] = val;}
	void setRotationCenterLocal(float* val) {rotationCenter[0] = val[0]; rotationCenter[1]=val[1]; rotationCenter[2] = val[2];}
	//Routines that deal with stretched coordinates:
	void getStretchedUpVec(float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*upVector[i];
	}
	void setStretchedUpVec(const float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) upVector[i] = vec[i]/stretch[i];
	}
	void getStretchedRotCtrLocal(float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*rotationCenter[i];
	}
	void setStretchedRotCtrLocal(const float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) rotationCenter[i] = vec[i]/stretch[i];
	}	
	void setStretchedViewDir(const float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) viewDirection[i] = vec[i]/stretch[i];
	}
	void getStretchedViewDir(float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*viewDirection[i];
	}
	void setStretchedCamPosLocal(const float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) cameraPosition[i] = vec[i]/stretch[i];
	}
	void getStretchedCamPosLocal(float* vec){
		const float* stretch = DataStatus::getInstance()->getStretchFactors();
		for (int i = 0; i<3; i++) vec[i] = stretch[i]*cameraPosition[i];
	}

	void setPerspective(bool on) {perspective = on;}
	bool hasPerspective() {return perspective;}
	ParamNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	
protected:
	static const string _camPosTag;
	static const string _viewDirTag;
	static const string _upVecTag;
	static const string _rotCenterLatLonTag;
	static const string _camLatLonTag;
	static const string _rotCenterTag;
	static const string _perspectiveAttr;
	static const string _viewpointTag;
	float cameraPosition[3];
	float viewDirection[3];
	float upVector[3];
	float rotationCenter[3];
	float rotCenterLatLon[2];
	float camLatLon[2];
	
	bool perspective;
	

};
};
#endif //VIEWPOINT_H 

