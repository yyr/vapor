//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		viewpointparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2004
//
//	Description:	Defines the ViewpointParams class
//		This class contains the parameters associated with viewpoint and lights
//
#ifndef VIEWPOINTPARAMS_H
#define VIEWPOINTPARAMS_H

#include <qwidget.h>
#include "params.h"

#include "viewpoint.h"

class VizTab;

namespace VAPoR {
class ExpatParseMgr;
class MainForm;
class RegionParams;
class PanelCommand;
class XmlNode;

class PARAMS_API ViewpointParams : public Params {
	
public: 
	ViewpointParams(int winnum);
	
	virtual ~ViewpointParams();
	
	virtual Params* deepCopy();
	
	//Note that all calls to get camera pos and get rot center return values
	//in local coordinates, not in lat/lon.  When the viewpoint params is in
	//latlon mode, it is necessary to perform convertFromLatLon and convertToLatLon
	//to keep the local coords current with latlons.  This conversion must occur whenever
	//the coordinates change (from the gui or the manip), 
	//and when the time step changes, whenever
	//there is a change between latlon and local mode, and whenever a new
	//data set is loaded
	//When setCameraPos or setRotCenter is called in latlon mode, the new local values
	//must be converted to latlon values.
	float* getCameraPos() {return currentViewpoint->getCameraPosLocal();}
	float getCameraPos(int coord) {return currentViewpoint->getCameraPosLocal()[coord];}
	//void setCameraPos(int i, float val) { currentViewpoint->setCameraPos(i, val);}
	void setCameraPos(float* val,int timestep ) {
		currentViewpoint->setCameraPosLocal(val);
		if (useLatLon) convertToLatLon(timestep);
	}
	float* getViewDir() {return currentViewpoint->getViewDir();}
	void setViewDir(int i, float val) { currentViewpoint->setViewDir(i,val);}
	void setViewDir(float* val) {currentViewpoint->setViewDir(val);}
	float* getUpVec() {return currentViewpoint->getUpVec();}
	void setUpVec(int i, float val) { currentViewpoint->setUpVec(i,val);}
	void setUpVec(float* val) {currentViewpoint->setUpVec(val);}
	bool hasPerspective(){return currentViewpoint->hasPerspective();}
	void setPerspective(bool on) {currentViewpoint->setPerspective(on);}
	int getStereoMode(){ return stereoMode;}
	void setStereoMode(int mode) {stereoMode = mode;}
	float getStereoSeparation() {return stereoSeparation;}
	void setStereoSeparation(float angle){stereoSeparation = angle;}
	int getNumLights() { return numLights;}
	void setNumLights(int nlights) {numLights = nlights;}
	const float* getLightDirection(int lightNum){return lightDirection[lightNum];}
	void setLightDirection(int lightNum,int dir, float val){
		lightDirection[lightNum][dir] = val;
	}
	float getDiffuseCoeff(int lightNum) {return diffuseCoeff[lightNum];}
	float getSpecularCoeff(int lightNum) {return specularCoeff[lightNum];}
	float getExponent() {return specularExp;}
	float getAmbientCoeff() {return ambientCoeff;}
	void setDiffuseCoeff(int lightNum, float val) {diffuseCoeff[lightNum]=val;}
	void setSpecularCoeff(int lightNum, float val) {specularCoeff[lightNum]=val;}
	void setExponent(float val) {specularExp=val;}
	void setAmbientCoeff(float val) {ambientCoeff=val;}
	Viewpoint* getCurrentViewpoint() { return currentViewpoint;}
	void setCurrentViewpoint(Viewpoint* newVP){
		if (currentViewpoint) delete currentViewpoint;
		currentViewpoint = newVP;
	}
	Viewpoint* getHomeViewpoint() { return homeViewpoint;}
	void setHomeViewpoint(Viewpoint* newVP){
		if (homeViewpoint) delete homeViewpoint;
		homeViewpoint = newVP;
	}
	
	//Set to default viewpoint for specified region
	void centerFullRegion(int timestep);
	float* getRotationCenter(){return currentViewpoint->getRotationCenterLocal();}
	float* getRotCenterLatLon(){return currentViewpoint->getRotCenterLatLon();}
	float getRotationCenter(int i){ return currentViewpoint->getRotationCenterLocal(i);}
	float* getCamPosLatLon() {return currentViewpoint->getCamPosLatLon();}
	//void setRotationCenter(int i, float val){currentViewpoint->setRotationCenter(i,val);}
	void setRotationCenter(float* vec, int timestep){
		currentViewpoint->setRotationCenterLocal(vec);
		if (useLatLon) convertToLatLon(timestep);
	}
	void setCamPosLatLon(float x, float y) {currentViewpoint->setCamPosLatLon(x,y);}
	void setRotCenterLatLon(float x, float y) {currentViewpoint->setRotCenterLatLon(x,y);}
	bool isLatLon() {return useLatLon;}
	void setLatLon(bool val){useLatLon = val;}
	
	void rescale(float scaleFac[3], int timestep);

	bool convertToLatLon(int timestep);
	bool convertFromLatLon(int timestep);
	//determine far and near distance to region based on current viewpoint
	void getFarNearDist(RegionParams* rParams, float* far, float* near, float* boxFar, float* boxNear);
	
	//Reset viewpoint when new session is started:
	virtual bool reinit(bool doOverride);
	virtual void restart();
	
	static void setDefaultPrefs();
	//Transformations to convert world coords to (unit)render cube and back
	//
	static void worldToCube(const float fromCoords[3], float toCoords[3]);
	static void worldToStretchedCube(const float fromCoords[3], float toCoords[3]);

	static void worldFromCube(float fromCoords[3], float toCoords[3]);
	static void worldFromStretchedCube(float fromCoords[3], float toCoords[3]);
	static void setCoordTrans();
	static float* getMinStretchedCubeCoords() {return minStretchedCubeCoord;}
	

	//Following determines scale factor in coord transformation:
	//
	//static float getMaxCubeSide() {return maxCubeSide;}
	static float getMaxStretchedCubeSide() {return maxStretchedCubeSide;}

	//Maintain the OpenGL Model Matrices, since they can be shared between visualizers
	
	const double* getModelViewMatrix() {return modelViewMatrix;}

	//Rotate a 3-vector based on current modelview matrix
	void transform3Vector(const float vec[3], float resvec[3]);
	
	void setModelViewMatrix(double* mtx){
		for (int i = 0; i<16; i++) modelViewMatrix[i] = mtx[i];
	}
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	XmlNode* buildNode();
	static const float* getDefaultViewDir(){return defaultViewDir;}
	static const float* getDefaultUpVec(){return defaultUpVec;}
	static const float* getDefaultLightDirection(int lightNum){return defaultLightDirection[lightNum];}
	static float getDefaultAmbientCoeff(){return defaultAmbientCoeff;}
	static float getDefaultSpecularExp(){return defaultSpecularExp;}
	static int getDefaultNumLights(){return defaultNumLights;}
	static const float* getDefaultDiffuseCoeff() {return defaultDiffuseCoeff;}
	static const float* getDefaultSpecularCoeff() {return defaultSpecularCoeff;}
	static void setDefaultLightDirection(int lightNum, float val[3]){
		for (int i = 0; i<3; i++) defaultLightDirection[lightNum][i] = val[i];
	}
	static void setDefaultUpVec(float val[3]){
		for (int i = 0; i<3; i++) defaultUpVec[i] = val[i];
	}
	static void setDefaultViewDir(float val[3]){
		for (int i = 0; i<3; i++) defaultViewDir[i] = val[i];
	}
	static void setDefaultSpecularCoeff(float val[3]){
		for (int i = 0; i<3; i++) defaultSpecularCoeff[i] = val[i];
	}
	static void setDefaultDiffuseCoeff(float val[3]){
		for (int i = 0; i<3; i++) defaultDiffuseCoeff[i] = val[i];
	}
	static void setDefaultAmbientCoeff(float val){ defaultAmbientCoeff = val;}
	static void setDefaultSpecularExp(float val){ defaultSpecularExp = val;}
	static void setDefaultNumLights(int val){ defaultNumLights = val;}

protected:
	static const string _latLonAttr;
	static const string _currentViewTag;
	static const string _homeViewTag;
	static const string _lightTag;
	static const string _lightDirectionAttr;
	static const string _lightNumAttr;
	static const string _diffuseLightAttr;
	static const string _ambientLightAttr;
	static const string _specularLightAttr;
	static const string _specularExponentAttr;
	static const string _stereoModeAttr;
	static const string _stereoSeparationAttr;
	
	
	Viewpoint* currentViewpoint;
	Viewpoint* homeViewpoint;
	float stereoSeparation;
	int stereoMode; //0 for center, 1 for left, 2 for right
	bool useLatLon;
	int numLights;
	int parsingLightNum;
	float lightDirection[3][4];
	float diffuseCoeff[3];
	float specularCoeff[3];
	float specularExp;
	float ambientCoeff;

	//Static coeffs for affine coord conversion:
	//
	static float minStretchedCubeCoord[3];
	static float maxStretchedCubeCoord[3];
	
	static float maxStretchedCubeSide;
	//defaults:
	static float defaultViewDir[3];
	static float defaultUpVec[3];
	static float defaultLightDirection[3][3];
	static float defaultDiffuseCoeff[3];
	static float defaultSpecularCoeff[3];
	static float defaultAmbientCoeff;
	static float defaultSpecularExp;
	static int defaultNumLights;
	//Max sides
	
	//GL state saved here since it may be shared...
	//
	double modelViewMatrix[16];
};
};
#endif //VIEWPOINTPARAMS_H 

