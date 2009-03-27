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
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"

#include <math.h>
#include "vapor/Metadata.h"
#include "vapor/DataMgr.h"
#include "vapor/VDFIOBase.h"
#include "vapor/LayeredIO.h"
#include "vapor/errorcodes.h"
#include "vapor/WRF.h"
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

TwoDParams::TwoDParams(int winnum) : RenderParams(winnum){
	
	

}
TwoDParams::~TwoDParams(){
	
}





//Find the smallest box containing the twoD slice, in block coords, at current refinement level.
//We also need the actual box coords (min and max) to check for valid coords in the block.
//Note that the box region may be strictly smaller than the block region
//
void TwoDParams::getBoundingBox(int timestep, size_t boxMin[3], size_t boxMax[3], int numRefs){
	//Determine the box that contains the twoD slice.
	DataStatus* ds = DataStatus::getInstance();
	const VDFIOBase* myReader = ds->getRegionReader();
	double dmin[3],dmax[3];
	for (int i = 0; i<3; i++) {
		dmin[i] = twoDMin[i];
		dmax[i] = twoDMax[i];
	}
	myReader->MapUserToVox((size_t)-1,dmin, boxMin, numRefs);
	myReader->MapUserToVox((size_t)-1,dmax, boxMax, numRefs);

}

//Find the smallest stretched extents containing the twoD, 
//Similar to above, but using stretched extents
void TwoDParams::calcContainingStretchedBoxExtentsInCube(float* bigBoxExtents){
	if(!DataStatus::getInstance()) return;
	//Determine the smallest axis-aligned cube that contains the twoD.  This is
	//obtained by mapping all 8 corners into the space.
	//It will not necessarily fit inside the unit cube.
	float corners[8][3];
	calcBoxCorners(corners, 0.f);
	
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
	const float* fullExtents = DataStatus::getInstance()->getStretchedExtents();
	
	float maxSize = Max(Max(fullExtents[3]-fullExtents[0],fullExtents[4]-fullExtents[1]),fullExtents[5]-fullExtents[2]);
	for (crd = 0; crd<3; crd++){
		bigBoxExtents[crd] = (boxMin[crd]*stretch[crd] - fullExtents[crd])/maxSize;
		bigBoxExtents[crd+3] = (boxMax[crd]*stretch[crd] - fullExtents[crd])/maxSize;
	}
	return;
}



//Determine the voxel extents of plane mapped into data.





	//Construct transform of form (x,y)-> a[0]x+b[0],a[1]y+b[1],
	//Mapping [-1,1]X[-1,1] into 3D volume.
    //Also determine the first and second coords that are used in 
    //the transform and the constant value
    //mappedDims[0] and mappedDims[1] are the dimensions that are
    //varying in the 3D volume.  mappedDims[2] is constant.
    //constVal are the constant values that are used, for top and bottom of
    //box (only different if terrain mapped)
void TwoDParams::build2DTransform(float a[2],float b[2],float constVal[2], int mappedDims[3]){
	//Find out orientation:
	int dataOrientation = orientation;
	mappedDims[2] = dataOrientation;
	mappedDims[0] = (dataOrientation == 0) ? 1 : 0;  // x or y
	mappedDims[1] = (dataOrientation < 2) ? 2 : 1; // z or y
	constVal[0] = twoDMin[dataOrientation];
	constVal[1] = twoDMax[dataOrientation];
	//constant terms go to middle
	b[0] = 0.5*(twoDMin[mappedDims[0]]+twoDMax[mappedDims[0]]);
	b[1] = 0.5*(twoDMin[mappedDims[1]]+twoDMax[mappedDims[1]]);
	//linear terms send -1,1 to box min,max
	a[0] = b[0] - twoDMin[mappedDims[0]];
	a[1] = b[1] - twoDMin[mappedDims[1]];

}
//Following overrides version in Param for 2D
//Does not support rotation or thickness
void TwoDParams::
calcBoxCorners(float corners[8][3], float, float, int ){
	
	float a[2],b[2],constValue[2];
	int mapDims[3];
	build2DTransform(a,b,constValue,mapDims);
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


void TwoDParams::setOrientation(int val){
	//Determine current proportions in width and height, with previous orientation
	float boxwidth, boxheight;
	int widthIndex = 0;
	int heightIndex = 1;
	if (orientation != 2) {heightIndex = 2; }
	if (orientation == 0) {widthIndex = 1;}
	const float* extents = DataStatus::getInstance()->getExtents();
	
	boxwidth = (twoDMax[widthIndex] - twoDMin[widthIndex])/(extents[widthIndex+3]-extents[widthIndex]);
	boxheight = (twoDMax[heightIndex] - twoDMin[heightIndex])/(extents[heightIndex+3]-extents[heightIndex]);
	
	//Use the new values to reset the box extents.
	//Make them use the same height and width, relative to the extents.
	orientation = val;
	float boxmid = 0.5f*(twoDMax[orientation]+twoDMin[orientation]);
	twoDMax[orientation]=twoDMin[orientation] = boxmid;
	heightIndex = 1; 
	widthIndex = 0;
	if (orientation != 2) {heightIndex = 2; }
	if (orientation == 0) {widthIndex = 1;}
	boxmid = 0.5f*(twoDMax[widthIndex]+twoDMin[widthIndex]);
	twoDMax[widthIndex] = boxmid + 0.5f*boxwidth*(extents[widthIndex+3]-extents[widthIndex]);
	twoDMin[widthIndex] = boxmid - 0.5f*boxwidth*(extents[widthIndex+3]-extents[widthIndex]);
	boxmid = 0.5f*(twoDMax[heightIndex]+twoDMin[heightIndex]);
	twoDMax[heightIndex] = boxmid + 0.5f*boxheight*(extents[heightIndex+3]-extents[heightIndex]);
	twoDMin[heightIndex] = boxmid - 0.5f*boxheight*(extents[heightIndex+3]-extents[heightIndex]);
	
}
void TwoDParams::
getTwoDVoxelExtents(float voxdims[2]){
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) {
		voxdims[0] = voxdims[1] = 1.f;
		return;
	}
	int dataOrientation = orientation;
	int xcrd = 0, ycrd = 1;
	if (dataOrientation < 2) ycrd = 2;
	if (dataOrientation < 1) xcrd = 1;
	voxdims[0] = twoDMax[xcrd] - twoDMin[xcrd];
	voxdims[1] = twoDMax[ycrd] - twoDMin[ycrd];
	return;
}
