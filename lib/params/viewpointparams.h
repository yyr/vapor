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
class ParamNode;
//! \class ViewpointParams
//! \brief A class for describing the viewpoint and lights
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//! This class provides methods for determining the viewpoint
//! and the direction of lights.  If it is shared, all windows can
//! use the same viewpoint and lights.  Local viewpoints are
//! just applicable to one window.
class PARAMS_API ViewpointParams : public Params {
	
public: 
	//! Constructor
	//! \param[in] int window num, indicates the visualizer number, or -1 for a shared ViewpointParams
	ViewpointParams(int winnum);
	//! Destructor
	virtual ~ViewpointParams();
	

	//! Static method specifies the scale factor that will be used in coordinate transformation, as is
	//! used to map the full stretched data domain into the unit box.
	//! The reciprocal of this value is the scale factor that is applied.
	//! \retval float Maximum side of stretched cube
	static float getMaxStretchedCubeSide() {return maxStretchedCubeSide;}

	//! This static method returns the minimum coordinates in the stretched
	//! world.  This is used as the translation needed to put the user coordinates
	//! at the origin.
	//! \retval float* Minimum world coordinates stretched domain
	static float* getMinStretchedCubeCoords() {return minStretchedCubeCoord;}

	//! This method tells how many lights are specified and whether
	//! lighting is on or not (i.e. if there are more than 0 lights).
	//! Note that only the first (light 0) is used in DVR and Isosurface rendering.
	//! \retval int number of lights (0,1,2, or 3)
	int getNumLights() { return numLights;}
	
	//! Obtain the current specular exponent.
	//! This value should be used in setting the material properties
	//! of all geometry being rendered.
	//! \retval float Specular exponent
	float getExponent() {return specularExp;}

	//! This method gives the current camera position in world coordinates.
	//! \retval float[3] camera position
	float* getCameraPos() {return currentViewpoint->getCameraPosLocal();}

	//! This method gives the direction vector of the viewer, pointing from the camera into the scene.
	//! \retval float[3] view direction
	float* getViewDir() {return currentViewpoint->getViewDir();}

	//! Method that specifies the upward-pointing vector of the current viewpoint.
	//! \retval float[3] up vector
	float* getUpVec() {return currentViewpoint->getUpVec();}

	//! Method returns the position used as the center for rotation.
	//! Usually this is in the center of the view, but it can be changed
	//! by user translation.
	//! \retval float[3] Rotation center coordinates
	float* getRotationCenter(){return currentViewpoint->getRotationCenterLocal();}

#ifndef DOXYGEN_SKIP_THIS
	static ParamsBase* CreateDefaultInstance() {return new ViewpointParams(-1);}
	const std::string& getShortName() {return _shortName;}
	virtual Params* deepCopy(ParamNode* n = 0);
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
	
	float getCameraPos(int coord) {return currentViewpoint->getCameraPosLocal()[coord];}
	
	void setCameraPos(float* val,int timestep ) {
		currentViewpoint->setCameraPosLocal(val);
		if (useLatLon) convertToLatLon(timestep);
	}
	
	void setViewDir(int i, float val) { currentViewpoint->setViewDir(i,val);}
	void setViewDir(float* val) {currentViewpoint->setViewDir(val);}
	
	void setUpVec(int i, float val) { currentViewpoint->setUpVec(i,val);}
	void setUpVec(float* val) {currentViewpoint->setUpVec(val);}
	bool hasPerspective(){return currentViewpoint->hasPerspective();}
	void setPerspective(bool on) {currentViewpoint->setPerspective(on);}
	int getStereoMode(){ return stereoMode;}
	void setStereoMode(int mode) {stereoMode = mode;}
	float getStereoSeparation() {return stereoSeparation;}
	void setStereoSeparation(float angle){stereoSeparation = angle;}
	
	void setNumLights(int nlights) {numLights = nlights;}
	const float* getLightDirection(int lightNum){return lightDirection[lightNum];}
	void setLightDirection(int lightNum,int dir, float val){
		lightDirection[lightNum][dir] = val;
	}
	float getDiffuseCoeff(int lightNum) {return diffuseCoeff[lightNum];}
	float getSpecularCoeff(int lightNum) {return specularCoeff[lightNum];}
	
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
	void getFarNearDist(float* boxFar, float* boxNear);
	
	//Reset viewpoint when new session is started:
	virtual bool reinit(bool doOverride);
	virtual void restart();
	
	static void setDefaultPrefs();
	//Transformations to convert world coords to (unit)render cube and back
	//
	static void worldToCube(const float fromCoords[3], float toCoords[3]);
	static void worldToStretchedCube(const float fromCoords[3], float toCoords[3]);
	static void worldToStretchedCube(const double fromCoords[3], double toCoords[3]);

	static void worldFromCube(float fromCoords[3], float toCoords[3]);
	static void worldFromStretchedCube(float fromCoords[3], float toCoords[3]);
	static void setCoordTrans();
	
	//Maintain the OpenGL Model Matrices, since they can be shared between visualizers
	
	const double* getModelViewMatrix() {return modelViewMatrix;}

	//Rotate a 3-vector based on current modelview matrix
	void transform3Vector(const float vec[3], float resvec[3]);
	
	void setModelViewMatrix(double* mtx){
		for (int i = 0; i<16; i++) modelViewMatrix[i] = mtx[i];
	}
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	ParamNode* buildNode();
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
	static const string _shortName;
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
#endif //DOXYGEN_SKIP_THIS
};
};
#endif //VIEWPOINTPARAMS_H 

