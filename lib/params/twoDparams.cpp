//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implementation of the twoDparams class
//		This is parent for twoDImageParams and TwoDDataParams.
//		Implements common elements of both
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif



#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "glutil.h"
#include "twoDparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"

#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>
//tiff stuff:

#include "geo_normalize.h"
#include "geotiff.h"
#include "xtiffio.h"
#include "proj_api.h"


using namespace VAPoR;

const string TwoDParams::_geometryTag = "TwoDGeometry";
const string TwoDParams::_twoDMinAttr = "TwoDMin";
const string TwoDParams::_twoDMaxAttr = "TwoDMax";
const string TwoDParams::_cursorCoordsAttr = "CursorCoords";
const string TwoDParams::_terrainMapAttr = "MapToTerrain";
const string TwoDParams::_verticalDisplacementAttr = "VerticalDisplacement";
const string TwoDParams::_numTransformsAttr = "NumTransforms";
const string TwoDParams::_orientationAttr = "Orientation";
const string TwoDParams::_heightVariableAttr = "HeightVariable";


TwoDParams::TwoDParams(int winnum, const string& name) : RenderParams(winnum, name){
	
	lastTwoDTexture = 0;
	myBox = 0;

}
TwoDParams::~TwoDParams(){
	if (myBox) delete myBox;
	
}







//Find the smallest stretched extents containing the twoD, 
//Similar to above, but using stretched extents
void TwoDParams::calcContainingStretchedBoxExtentsInCube(float* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the twoD.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	float corners[8][3];
	calcBoxCorners(corners, 0.f, -1);
	
	float boxMin[3],boxMax[3];
	int crd, cor;
	
	//initialize extents, and variables that will be min,max
	for (crd = 0; crd< 3; crd++){
		boxMin[crd] = 1.e30f;
		boxMax[crd] = -1.e30f;
	}
	
	
	for (cor = 0; cor< 8; cor++){
		
		
		//make sure the container includes it:
		for(crd = 0; crd< 3; crd++){
			if (corners[cor][crd]<boxMin[crd]) boxMin[crd] = corners[cor][crd];
			if (corners[cor][crd]>boxMax[crd]) boxMax[crd] = corners[cor][crd];
		}
	}
	//Now convert the min,max back into extents in unit cube:
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	const float* sizes = DataStatus::getInstance()->getFullStretchedSizes();
	
	float maxSize = Max(Max(sizes[0],sizes[1]),sizes[2]);
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd]*stretch[crd])/maxSize;
		bigBoxExtents[crd+3] = (boxMax[crd]*stretch[crd])/maxSize;
	}
	return;
}

	//Construct transform of form (x,y)-> a[0]x+b[0],a[1]y+b[1],
	//Mapping [-1,1]X[-1,1] into 3D volume, in local coordinates.
    //Also determine the first and second coords that are used in 
    //the transform and the constant value
    //mappedDims[0] and mappedDims[1] are the dimensions that are
    //varying in the 3D volume.  mappedDims[2] is constant.
    //constVal are the constant values that are used, for top and bottom of
    //box (only different if terrain mapped)
void TwoDParams::buildLocal2DTransform(float a[2],float b[2],float constVal[2], int mappedDims[3]){
	//Find out orientation:
	int dataOrientation = orientation;
	mappedDims[2] = dataOrientation;
	mappedDims[0] = (dataOrientation == 0) ? 1 : 0;  // x or y
	mappedDims[1] = (dataOrientation < 2) ? 2 : 1; // z or y
	const vector<double>& exts = GetBox()->GetLocalExtents();
	constVal[0] = exts[dataOrientation];
	constVal[1] = exts[dataOrientation+3];
	//constant terms go to middle
	b[0] = 0.5*(exts[mappedDims[0]]+exts[3+mappedDims[0]]);
	b[1] = 0.5*(exts[mappedDims[1]]+exts[3+mappedDims[1]]);
	//linear terms send -1,1 to box min,max
	a[0] = b[0] - exts[mappedDims[0]];
	a[1] = b[1] - exts[mappedDims[1]];

}
//Following overrides version in Param for 2D
//Does not support rotation or thickness
void TwoDParams::
calcBoxCorners(float corners[8][3], float, int, float, int ){
	
	float a[2],b[2],constValue[2];
	int mapDims[3];
	buildLocal2DTransform(a,b,constValue,mapDims);
	float boxCoord[3];
	//Return the corners of the box (in world space)
	//Go counter-clockwise around the back, then around the front
	//X increases fastest, then y then z; 

	//Fatten box slightly, in case it is degenerate.  This will
	//prevent us from getting invalid face normals.

	boxCoord[0] = -1.f;
	boxCoord[1] = -1.f;
	boxCoord[2] = -1.f;
	// calculate the mapping of each corner,
	corners[0][mapDims[2]] = constValue[0];
	corners[0][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[0][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = 1.f;
	corners[1][mapDims[2]] = constValue[0];
	corners[1][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[1][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[1] = 1.f;
	corners[3][mapDims[2]] = constValue[0];
	corners[3][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[3][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = -1.f;
	corners[2][mapDims[2]] = constValue[0];
	corners[2][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[2][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[1] = -1.f;
	boxCoord[2] = 1.f;
	corners[4][mapDims[2]] = constValue[1];
	corners[4][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[4][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = 1.f;
	corners[5][mapDims[2]] = constValue[1];
	corners[5][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[5][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[1] = 1.f;
	corners[7][mapDims[2]] = constValue[1];
	corners[7][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[7][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
	boxCoord[0] = -1.f;
	corners[6][mapDims[2]] = constValue[1];
	corners[6][mapDims[0]] = a[0]*boxCoord[mapDims[0]]+b[0];
	corners[6][mapDims[1]] = a[1]*boxCoord[mapDims[1]]+b[1];
	
}

//Reorient the box.  Maintain the same relative size in the length and width
//directions (relative to full domain size).
void TwoDParams::setOrientation(int val){
	if (orientation == val) return;
	float boxmin[3],boxmax[3],sizeRatio[3],newBoxSize[3], boxmid[3];
	getLocalBox(boxmin,boxmax,0);
	const float * localExtents = DataStatus::getInstance()->getLocalExtents();
	for (int i = 0; i<3; i++){
		sizeRatio[i] = (boxmax[i]-boxmin[i])/(localExtents[i+3]);
		newBoxSize[i] = (boxmax[i]-boxmin[i]);
		boxmid[i] = 0.5f*(boxmax[i]+boxmin[i]);
	}
	//change size in the dimension to maintain size ratio:
	//Always set the boxsize to zero in the new orientation-direction:
	newBoxSize[val] = 0.f;
	//The relative size in the old direction becomes the relative size from the new direction
	newBoxSize[orientation] = sizeRatio[val]*(localExtents[orientation+3]);
	
	for (int i = 0; i< 3; i++){
		boxmin[i] = boxmid[i] -0.5f*newBoxSize[i];
		boxmax[i] = boxmid[i] +0.5f*newBoxSize[i];
	}
	setLocalBox(boxmin,boxmax,0);
	orientation = val;
	return;
	
}
void TwoDParams::
getTwoDVoxelExtents(float voxdims[2]){
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) {
		voxdims[0] = voxdims[1] = 1.f;
		return;
	}
	const vector<double>& box = GetBox()->GetLocalExtents();
	int dataOrientation = orientation;
	int xcrd = 0, ycrd = 1;
	if (dataOrientation < 2) ycrd = 2;
	if (dataOrientation < 1) xcrd = 1;
	voxdims[0] = box[3+xcrd] - box[xcrd];
	voxdims[1] = box[3+ycrd] - box[ycrd];
	return;
}
//Find distance from camera to planar surface, in stretched local coordinates
float TwoDParams::getCameraDistance(ViewpointParams* vpParams, RegionParams*, int){
	//Intersect surface with camera ray.  If no intersection, just shoot ray from
	//camera to surface center.
	const float* camPos = vpParams->getCameraPosLocal();
	const float* camDir = vpParams->getViewDir();
	//Solve for intersection with surface:
	//If behind or parallel, make infinitely far (low priority in sorting)
	if (camDir[orientation] == 0.f) return 1.e30f;
	//if mapped to terrain, will need to add height to planar coordinate
	float ht = 0.f;
	if (orientation == 2 && isMappedToTerrain()){
		DataStatus* ds = DataStatus::getInstance();
		int hgtvar = ds->getSessionVariableNum2D("HGT");
		if (hgtvar >= 0)
			ht =  0.1f*ds->getDefaultDataMax2D(hgtvar);
	}
	//Stretch coordinates of everything:
	float cPos[3];
	float tdmin[3];
	float tdmax[3];
	const float* stretch = DataStatus::getInstance()->getStretchFactors();
	const vector<double>& box = GetBox()->GetLocalExtents();
	for (int i = 0; i<3; i++){
		cPos[i] = stretch[i]*camPos[i];
		tdmin[i] = box[i]*stretch[i];
		tdmax[i] = box[i+3]*stretch[i];
	}
	
	ht *= stretch[orientation];


	//Find parameter T (position along ray) so that camPos+T*camDir intersects plane
	float T = (tdmin[orientation]+ ht - cPos[orientation])/camDir[orientation];
	if (T < 0.f) return 1.e30f;  //intersection is behind camera

	//Test if resulting point is inside twoD extents:
	float hitPoint[3];
	for (int i = 0; i<3; i++) hitPoint[i] = cPos[i]+T*camDir[i];

	int i1 = (orientation+1)%3;
	int i2 = (orientation+2)%3;

	if (hitPoint[i1] >= tdmin[i1] && hitPoint[i2] >= tdmin[i2] &&
		hitPoint[i1] <= tdmax[i1] && hitPoint[i2] <= tdmax[i2])
	{
		//OK, we are in:
		return vdist(cPos, hitPoint);
	}
	//Otherwise, find distance to center:
	for (int i = 0; i<3; i++){
		hitPoint[i] = 0.5f*(tdmin[i]+tdmax[i]);
	}
	hitPoint[orientation] += ht*stretch[orientation];
	return (vdist(cPos, hitPoint));

	/*  Following code gets closest point.  Doesn't always help order geometry for transparency
	const float* camPos = vpParams->getCameraPos();
	float ht = 0.f;
	if (orientation == 2 && isMappedToTerrain()){
		DataStatus* ds = DataStatus::getInstance();
		int hgtvar = ds->getSessionVariableNum2D("HGT");
		if (hgtvar >= 0)
			ht =  0.5f*ds->getDefaultDataMax2D(hgtvar);
	}
	//test if camera is in box extents
	if (orientation == 2) {  //horizontal
		if (camPos[0] >= twoDMin[0] && camPos[0] <= twoDMax[0] &&
			camPos[1] >= twoDMin[1] && camPos[1] <= twoDMax[1]) 
				return (abs(camPos[2]-twoDMin[2]-ht));
	} else if (orientation == 1) { //XZ orientation
		if (camPos[0] >= twoDMin[0] && camPos[0] <= twoDMax[0] &&
			camPos[2] >= twoDMin[2] && camPos[2] <= twoDMax[2]) 
				return (abs(camPos[1]-twoDMin[1]));
	} else { //orientation = 0:
		if (camPos[1] >= twoDMin[1] && camPos[1] <= twoDMax[1] &&
			camPos[2] >= twoDMin[2] && camPos[2] <= twoDMax[2]) 
				return (abs(camPos[0]-twoDMin[0]));
	}
	//Otherwise must find closest corner:
	float minDist = 1.e30f;
	float boxmin[3], boxmax[3];
	getLocalBox(boxmin, boxmax);
	boxmin[2]+=ht;
	boxmax[2]+=ht;
	//Start by testing boxmin and boxmax.. they are always corners
	float dist = vdist(camPos, boxmin);
	if (minDist > dist) minDist = dist;
	dist = vdist(camPos,boxmax);
	if (minDist > dist) minDist = dist;
	float corPoint[3];
	//two combo's of boxmin and boxmax:
	int firstcoord = 0;
	int secondcoord = 1;
	if (orientation == 0) {firstcoord++;secondcoord++;}
	if (orientation == 1) {secondcoord++;}

	corPoint[firstcoord] = boxmin[firstcoord];
	corPoint[secondcoord] = boxmax[secondcoord];
	corPoint[orientation] = boxmin[orientation];
	dist = vdist(camPos,corPoint);
	if (minDist > dist) minDist = dist;
	corPoint[firstcoord] = boxmax[firstcoord];
	corPoint[secondcoord] = boxmin[secondcoord];
	corPoint[orientation] = boxmin[orientation];
	dist = vdist(camPos,corPoint);
	if (minDist > dist) minDist = dist;

	return minDist;
		*/
}
