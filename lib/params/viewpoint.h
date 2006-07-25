#ifndef VIEWPOINT_H
#define VIEWPOINT_H
/*
* This class describes a viewpoint
*/
#include "vapor/ExpatParseMgr.h"
namespace VAPoR {
class XmlNode;

class PARAMS_API Viewpoint : public ParsedXml {
	
public: 
	Viewpoint() {previousClass = 0;}
	
	virtual ~Viewpoint(){}
	
	float* getCameraPos() {return cameraPosition;}
	float getCameraPos(int i) {return cameraPosition[i];}
	void setCameraPos(int i, float val) { cameraPosition[i] = val;}
	void setCameraPos(float* val) {cameraPosition[0] = val[0]; cameraPosition[1]=val[1]; cameraPosition[2] = val[2];}
	float* getViewDir() {return viewDirection;}
	float getViewDir(int i) {return viewDirection[i];}
	void setViewDir(int i, float val) { viewDirection[i] = val;}
	void setViewDir(float* val) {viewDirection[0] = val[0]; viewDirection[1]=val[1]; viewDirection[2] = val[2];}
	float* getUpVec() {return upVector;}
	float getUpVec(int i) {return upVector[i];}
	void setUpVec(int i, float val) { upVector[i] = val;}
	void setUpVec(float* val) {upVector[0] = val[0]; upVector[1]=val[1]; upVector[2] = val[2];}
	float* getRotationCenter() {return rotationCenter;}
	float getRotationCenter(int i) {return rotationCenter[i];}
	void setRotationCenter(int i, float val) { rotationCenter[i] = val;}
	void setPerspective(bool on) {perspective = on;}
	bool hasPerspective() {return perspective;}
	XmlNode* buildNode();
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	
protected:
	static const string _camPosTag;
	static const string _viewDirTag;
	static const string _upVecTag;
	static const string _rotCenterTag;
	static const string _perspectiveAttr;
	static const string _viewpointTag;
	float cameraPosition[3];
	float viewDirection[3];
	float upVector[3];
	float rotationCenter[3];
	
	bool perspective;
	

};
};
#endif //VIEWPOINT_H 

