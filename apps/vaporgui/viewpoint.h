#ifndef VIEWPOINT_H
#define VIEWPOINT_H
/*
* This class describes a viewpoint
*/

namespace VAPoR {
class Viewpoint {
	
public: 
	Viewpoint() {}
	
	~Viewpoint(){}
	
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

	void setPerspective(bool on) {perspective = on;}
	bool hasPerspective() {return perspective;}
	
	
	
protected:
	
	float cameraPosition[3];
	float viewDirection[3];
	float upVector[3];
	
	bool perspective;
	

};
};
#endif //VIEWPOINT_H 

