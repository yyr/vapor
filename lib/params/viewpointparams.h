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

#include "params.h"
#include "viewpoint.h"
#include "command.h"

class VizTab;

namespace VAPoR {
class ExpatParseMgr;
class RegionParams;
class XmlNode;
class ParamNode;
//! \class ViewpointParams
//! \brief A class for describing the viewpoint and lights
//! \author Alan Norton
//! \version 3.0
//! \date    February 2014
//! This class provides methods for determining the viewpoint
//! and the direction of lights.  If it is shared, all windows can
//! use the same viewpoint and lights.  Local viewpoints are
//! just applicable in one visualizer.
class PARAMS_API ViewpointParams : public Params {
	
public: 
	//! Constructor
	//! \param[in] int window num, indicates the visualizer number, or -1 for a shared ViewpointParams
	ViewpointParams(XmlNode* parent, int winnum);
	//! Destructor
	virtual ~ViewpointParams();
	

	//! This method tells how many lights are specified and whether
	//! lighting is on or not (i.e. if there are more than 0 lights).
	//! Note that only the first (light 0) is used in DVR and Isosurface rendering.
	//! \retval int number of lights (0,1,2, or 3)
	int getNumLights() { 
		vector<long> defNumLights(defaultNumLights,1);
		return (GetRootNode()->GetElementLong(_numLightsTag,defNumLights)[0]);
	}
	
	//! Obtain the current specular exponent.
	//! This value should be used in setting the material properties
	//! of all geometry being rendered.
	//! \retval float Specular exponent
	double getExponent() {
		vector<double> defSpecExp(defaultSpecularExp,1);
		return (GetRootNode()->GetElementDouble(_specularExpTag,defSpecExp)[0]);
	}

	//! This method gives the current camera position in world coordinates.
	//! \retval float[3] camera position
	const vector<double>& getCameraPosLocal() {return getCurrentViewpoint()->getCameraPosLocal();}

	//! This method gives the direction vector of the viewer, pointing from the camera into the scene.
	//! \retval float[3] view direction
	const vector<double>& getViewDir() {return getCurrentViewpoint()->getViewDir();}

	//! Method that specifies the upward-pointing vector of the current viewpoint.
	//! \retval float[3] up vector
	const vector<double>& getUpVec() {return getCurrentViewpoint()->getUpVec();}

	//! Method returns the position used as the center for rotation.
	//! Usually this is in the center of the view, but it can be changed
	//! by user translation.
	//! \retval float[3] Rotation center coordinates
	const vector<double>& getRotationCenterLocal(){return getCurrentViewpoint()->getRotationCenterLocal();}
	//Note that all calls to get camera pos and get rot center return values
	//in local coordinates, not in lat/lon.  When the viewpoint params is in
	//latlon mode, it is necessary to perform convertLocalFromLonLat and convertLocalToLonLat
	//to keep the local coords current with latlons.  This conversion must occur whenever
	//the coordinates change (from the gui or the manip), 
	//and when the time step changes, whenever
	//there is a change between latlon and local mode, and whenever a new
	//data set is loaded
	//When setCameraPos or setRotCenter is called in latlon mode, the new local values
	//must be converted to latlon values.
	
	double getCameraPosLocal(int coord) {return getCurrentViewpoint()->getCameraPosLocal()[coord];}
	//! Set the camera position in local coordinates at a specified time
	//! \param[in] vector<double>& coordinates to be set
	//! \param[in] int timestep to be used
	//! \retval int 0 if successful
	int setCameraPosLocal(const vector<double>& val,int timestep ) {
		return getCurrentViewpoint()->setCameraPosLocal(val, this);
	}
	//! Set a component of viewer direction in the current viewpoint
	//! \param[in] int coordinate (0,1,2)
	//! \param[in] double value to be set
	//! \retval int 0 on success
	int setViewDir(int coord, double val) {return getCurrentViewpoint()->setViewDir(coord,val, this);}
	//! Set the viewer direction in the current viewpoint
	//! \param[in] vector<double> direction vector to be set
	//! \retval int 0 on success
	int setViewDir(const vector<double>& val) {return getCurrentViewpoint()->setViewDir(val, this);}
	//! Set a component of upward direction vector in the current viewpoint
	//! \param[in] int coordinate (0,1,2)
	//! \param[in] double value to be set
	//! \retval int 0 on success
	int setUpVec(int i, double val) { return getCurrentViewpoint()->setUpVec(i,val,this);}
	//! Set upward direction vector in the current viewpoint
	//! \param[in] vector<double> value to be set
	//! \retval int 0 on success
	int setUpVec(const vector<double>& val) {return getCurrentViewpoint()->setUpVec(val,this);}
	//! Obtain a coordinate of the current rotation center
	//! \param[in] coordinate
	//! \retval double component of rotation center
	double getRotationCenterLocal(int coord){ return getCurrentViewpoint()->getRotationCenterLocal(coord);}
	//! Specify the location of the rotation center in local coordinates.
	//! \param[in] vector<double> position
	//! \retval 0 on success
	int setRotationCenterLocal(const vector<double>& vec){
		return getCurrentViewpoint()->setRotationCenterLocal(vec,this);
	}
	//! Set the number of directional light sources
	//! \param[in] int number of lights (0,1,2,3)
	//! \retval 0 on success
	int setNumLights(int nlights) {
		return CaptureSetLong(_numLightsTag,"Set number of lights", nlights);
	}
	//! get one component of a light direction vector
	//! \param[in] int lightNum identifies which light source
	//! \param[in] int dir coordinate of direction vector
	//! \retval double requested component of light direction vector
	double getLightDirection(int lightNum, int dir){
		return GetRootNode()->GetElementDouble(_lightDirectionsTag)[dir+3*lightNum];
	}
	//! Set one component of a light direction vector
	//! \param[in] int lightNum identifies which light source
	//! \param[in] int dir coordinate of direction vector
	//! \param[in] double value to be set
	//! \retval int 0 on success
	int setLightDirection(int lightNum, int dir, double val){
		
		vector<double> ldirs = vector<double>(GetRootNode()->GetElementDouble(_lightDirectionsTag));
		ldirs[dir+3*lightNum] = val;
		return  CaptureSetDouble(_lightDirectionsTag,"Set light direction",ldirs);
	}
	//! Optain the diffuse lighting coefficient of a light source
	//! \param[in] int light number (0..2)
	//! \retval double diffuse coefficient
	double getDiffuseCoeff(int lightNum) {
		vector<double> defaultDiffCoeff;
		for (int i = 0; i<3; i++) defaultDiffCoeff.push_back(defaultDiffuseCoeff[i]);
		return GetRootNode()->GetElementDouble(_diffuseCoeffTag,defaultDiffCoeff)[lightNum];
	}
	//! Optain the specular lighting coefficient of a light source
	//! \param[in] int light number (0..2)
	//! \retval double specular coefficient
	double getSpecularCoeff(int lightNum) {
		vector<double> defaultSpecCoeff;
		for (int i = 0; i<3; i++) defaultSpecCoeff.push_back(defaultSpecularCoeff[i]);
		return GetRootNode()->GetElementDouble(_specularCoeffTag,defaultSpecCoeff)[lightNum];
	}
	//! Optain the ambient lighting coefficient of the lights
	//! \retval double ambient coefficient
	double getAmbientCoeff() {
		vector<double> defaultAmbient(defaultAmbientCoeff,1);
		return GetRootNode()->GetElementDouble(_ambientCoeffTag,defaultAmbient)[0];
	}
	//! Set the diffuse lighting coefficient of a light source
	//! \param[in] int light number (0..2)
	//! \param[in] double diffuse coefficent
	//! \retval int 0 if successful
	int setDiffuseCoeff(int lightNum, double val) {
		
		vector<double>diffCoeff(GetRootNode()->GetElementDouble(_diffuseCoeffTag));
		diffCoeff[lightNum]=val;
		return CaptureSetDouble(_diffuseCoeffTag,"Set diffuse coefficient",diffCoeff);
	}
	//! Set the specular lighting coefficient of a light source
	//! \param[in] int light number (0..2)
	//! \param[in] double specular coefficent
	//! \retval int 0 if successful
	int setSpecularCoeff(int lightNum, double val) {
		
		vector<double>specCoeff(GetRootNode()->GetElementDouble(_specularCoeffTag));
		specCoeff[lightNum]=val;
		return  CaptureSetDouble(_specularCoeffTag,"Set specular coefficient",specCoeff);
	}
	//! Set the specular lighting exponent of light sources
	//! \param[in] double specular exponent
	//! \retval int 0 if successful
	int setExponent(double val) {
		return CaptureSetDouble(_specularExpTag, "Set specular lighting",val);
	}
	//! Set the ambient lighting coefficient
	//! \param[in] double ambient coefficient
	//! \retval int 0 if successful
	int setAmbientCoeff(double val) {
		return   CaptureSetDouble(_ambientCoeffTag,"Set ambient lighting",val);
	}
	//! Set the current viewpoint
	//! \param[in] Viewpoint* viewpoint to be set
	//! \retval int 0 if successful
	//! \sa Viewpoint
	int setCurrentViewpoint(Viewpoint* newVP){
		Command* cmd = Command::CaptureStart(this,"set current viewpoint");
		ParamNode* pNode = GetRootNode()->GetNode(_currentViewTag);
		if (pNode) GetRootNode()->DeleteNode(_currentViewTag);
		int rc = GetRootNode()->AddRegisteredNode(_currentViewTag, newVP->GetRootNode(),newVP);
		Command::CaptureEnd(cmd,this);
		return rc;
	}
	//! Set the home viewpoint
	//! \param[in] Viewpoint* home viewpoint to be set
	//! \retval int 0 if successful
	//! \sa Viewpoint
	int setHomeViewpoint(Viewpoint* newVP){
		Command* cmd = Command::CaptureStart(this, "set home viewpoint");
		ParamNode* pNode = GetRootNode()->GetNode(_homeViewTag);
		if (pNode) GetRootNode()->DeleteNode(_homeViewTag);
		int rc = GetRootNode()->AddRegisteredNode(_homeViewTag, newVP->GetRootNode(),newVP);
		Command::CaptureEnd(cmd,this);
		return rc;
	}
	//! Center the viewpoint so as to view the full region at a timestep
	//! Modifies the camera position and the rotation center, maintaining the current
	//! camera direction and distance from center.
	//! param[in] int timestep
	void centerFullRegion(int timestep);
#ifndef DOXYGEN_SKIP_THIS
	static ParamsBase* CreateDefaultInstance() {return new ViewpointParams(0,-1);}
	const std::string& getShortName() {return _shortName;}
	
	
	void rescale(double scaleFac[3], int timestep);

	//determine far and near distance to region based on current viewpoint
	void getFarNearDist(float* boxFar, float* boxNear);
	
	
	virtual void Validate(bool useDefault);
	virtual void restart();
	
	static void setDefaultPrefs();
	
	static void setCoordTrans();
	
	//Maintain the OpenGL Model Matrices
	
	const double* getModelViewMatrix() {return modelViewMatrix;}
	const double* getProjectionMatrix() {return projectionMatrix;}

	//Rotate a 3-vector based on current modelview matrix
	void transform3Vector(const float vec[3], float resvec[3]);
	
	void setModelViewMatrix(const double* mtx){
		for (int i = 0; i<16; i++) modelViewMatrix[i] = mtx[i];
	}
	void setProjectionMatrix(double* mtx){
		for (int i = 0; i<16; i++) projectionMatrix[i] = mtx[i];
	}
	
	static const double* getDefaultViewDir(){return defaultViewDir;}
	static const double* getDefaultUpVec(){return defaultUpVec;}
	static const double* getDefaultLightDirection(int lightNum){return defaultLightDirection[lightNum];}
	static double getDefaultAmbientCoeff(){return defaultAmbientCoeff;}
	static double getDefaultSpecularExp(){return defaultSpecularExp;}
	static int getDefaultNumLights(){return defaultNumLights;}
	static const double* getDefaultDiffuseCoeff() {return defaultDiffuseCoeff;}
	static const double* getDefaultSpecularCoeff() {return defaultSpecularCoeff;}
	static void setDefaultLightDirection(int lightNum, double val[3]){
		for (int i = 0; i<3; i++) defaultLightDirection[lightNum][i] = val[i];
	}
	static void setDefaultUpVec(double val[3]){
		for (int i = 0; i<3; i++) defaultUpVec[i] = val[i];
	}
	static void setDefaultViewDir(double val[3]){
		for (int i = 0; i<3; i++) defaultViewDir[i] = val[i];
	}
	static void setDefaultSpecularCoeff(double val[3]){
		for (int i = 0; i<3; i++) defaultSpecularCoeff[i] = val[i];
	}
	static void setDefaultDiffuseCoeff(double val[3]){
		for (int i = 0; i<3; i++) defaultDiffuseCoeff[i] = val[i];
	}
	static void setDefaultAmbientCoeff(double val){ defaultAmbientCoeff = val;}
	static void setDefaultSpecularExp(double val){ defaultSpecularExp = val;}
	static void setDefaultNumLights(int val){ defaultNumLights = val;}
	virtual Viewpoint* getCurrentViewpoint() {
		ParamNode* pNode = GetRootNode()->GetNode(_currentViewTag);
		if (pNode) return (Viewpoint*)pNode->GetParamsBase();
		Viewpoint* vp = new Viewpoint();
		GetRootNode()->AddNode(_currentViewTag, vp->GetRootNode());
		return vp;
	}
	virtual Viewpoint* getHomeViewpoint() {
		ParamNode* pNode = GetRootNode()->GetNode(_homeViewTag);
		if (pNode) return (Viewpoint*)pNode->GetParamsBase();
		Viewpoint* vp = new Viewpoint();
		GetRootNode()->AddNode(_homeViewTag, vp->GetRootNode());
		return vp;
	}


protected:

	static const string _shortName;
	static const string _currentViewTag;
	static const string _homeViewTag;
	static const string _lightDirectionsTag;
	static const string _diffuseCoeffTag;
	static const string _specularCoeffTag;
	static const string _specularExpTag;
	static const string _ambientCoeffTag;
	static const string _numLightsTag;
	
	
	//defaults:
	static double defaultViewDir[3];
	static double defaultUpVec[3];
	static double defaultLightDirection[3][3];
	static double defaultDiffuseCoeff[3];
	static double defaultSpecularCoeff[3];
	static double defaultAmbientCoeff;
	static double defaultSpecularExp;
	static int defaultNumLights;
	double modelViewMatrix[16];
	double projectionMatrix[16];
#endif //DOXYGEN_SKIP_THIS
};
};
#endif //VIEWPOINTPARAMS_H 

