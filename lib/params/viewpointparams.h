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
	
	
	float* getCameraPos() {return currentViewpoint->getCameraPos();}
	float getCameraPos(int coord) {return currentViewpoint->getCameraPos()[coord];}
	void setCameraPos(int i, float val) { currentViewpoint->setCameraPos(i, val);}
	void setCameraPos(float* val) {currentViewpoint->setCameraPos(val);}
	float* getViewDir() {return currentViewpoint->getViewDir();}
	void setViewDir(int i, float val) { currentViewpoint->setViewDir(i,val);}
	void setViewDir(float* val) {currentViewpoint->setViewDir(val);}
	float* getUpVec() {return currentViewpoint->getUpVec();}
	void setUpVec(int i, float val) { currentViewpoint->setUpVec(i,val);}
	void setUpVec(float* val) {currentViewpoint->setUpVec(val);}
	bool hasPerspective(){return currentViewpoint->hasPerspective();}
	void setPerspective(bool on) {currentViewpoint->setPerspective(on);}
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
	void centerFullRegion();
	float* getRotationCenter(){return currentViewpoint->getRotationCenter();}
	float getRotationCenter(int i){ return currentViewpoint->getRotationCenter(i);}
	void setRotationCenter(int i, float val){currentViewpoint->setRotationCenter(i,val);}
	
	

	//determine far and near distance to region based on current viewpoint
	void getFarNearDist(RegionParams* rParams, float* far, float* near);
	
	//Reset viewpoint when new session is started:
	virtual bool reinit(bool doOverride);
	virtual void restart();
	
	//Transformations to convert world coords to (unit)render cube and back
	//
	static void worldToCube(const float fromCoords[3], float toCoords[3]);

	static void worldFromCube(float fromCoords[3], float toCoords[3]);
	static void setCoordTrans();
	static float* getMinCubeCoords() {return minCubeCoord;}
	static float* getMaxCubeCoords() {return maxCubeCoord;}

	//Following determines scale factor in coord transformation:
	//
	static float getMaxCubeSide() {return maxCubeSide;}

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

protected:
	static const string _currentViewTag;
	static const string _homeViewTag;
	static const string _lightTag;
	static const string _lightDirectionAttr;
	static const string _lightNumAttr;
	static const string _diffuseLightAttr;
	static const string _ambientLightAttr;
	static const string _specularLightAttr;
	static const string _specularExponentAttr;
	
	
	Viewpoint* currentViewpoint;
	Viewpoint* homeViewpoint;
	
	int numLights;
	int parsingLightNum;
	float lightDirection[3][4];
	float diffuseCoeff[3];
	float specularCoeff[3];
	float specularExp;
	float ambientCoeff;

	//Static coeffs for affine coord conversion:
	//
	static float minCubeCoord[3];
	static float maxCubeSide;
	//Max sides
	static float maxCubeCoord[3];
	//GL state saved here since it may be shared...
	//
	double modelViewMatrix[16];
};
};
#endif //VIEWPOINTPARAMS_H 

