//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isolinerenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2006
//
//	Description:	Implementation of the isolinerenderer class
//
//Temporary values for testing:
#define GRIDX 10
#define GRIDY 10
#include "glutil.h"	// Must be included first!!!
#include "params.h"
#include "isolinerenderer.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"
#include <vapor/errorcodes.h>

#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include "renderer.h"
#include <sstream>
using namespace VAPoR;

IsolineRenderer::IsolineRenderer(GLWindow* glw, IsolineParams* pParams )
:Renderer(glw, pParams, "IsolineRenderer")
{
	numIsovalsCached = 0;
	setupCache();
}


/*
  Release allocated resources
*/

IsolineRenderer::~IsolineRenderer()
{
	//De-allocate cache
	
	DataStatus* ds = DataStatus::getInstance();
	
	for (size_t ts = ds->getMinTimestep(); ts <= ds->getMaxTimestep(); ts++){
		invalidateLineCache((int)ts);
	}
	lineCache.clear();
	
}


// Perform the rendering
//
  

void IsolineRenderer::paintGL()
{
	IsolineParams* iParams = (IsolineParams*)currentRenderParams;
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;

	int timestep = myGLWindow->getActiveAnimationParams()->getCurrentTimestep();

	if (!cacheIsValid(timestep)){
		if (!buildLineCache(timestep)) return;
	}
		

	
	//
	//Perform OpenGL rendering of line segments
	//
	performRendering(timestep);
	
	
}
void IsolineRenderer::performRendering(int timestep){
//Perform setup of OpenGL transform matrix.  This transforms the full stretched domain into the unit box
	//by scaling and translating.
	myGLWindow->TransformToUnitBox();
	IsolineParams* iParams = (IsolineParams*)getRenderParams();
	//Set up lighting and color
	const vector<double>& dcolors = iParams->GetConstantColor();
	float fcolors[3];
	for (int i = 0; i<3; i++) fcolors[i] = (float)dcolors[i];
	ViewpointParams* vpParams =  myGLWindow->getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	if (nLights == 0) {
		glDisable(GL_LIGHTING);
	}
	else {
		glShadeModel(GL_SMOOTH);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, fcolors);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());
		//All the geometry will get a white specular color:
		float specColor[4];
		specColor[0]=specColor[1]=specColor[2]=0.8f;
		specColor[3] = 1.f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
	}
	glColor3fv(fcolors);
	glLineWidth(iParams->GetLineThickness());
	//Need to convert the iso-box coordinates to user coordinates, then to unit box coords.
	float transformMatrix[12];
	iParams->buildLocalCoordTransform(transformMatrix, 0.f, -1);
	float pointa[3],pointb[3]; //points in cache
	float point1[3],point2[3]; //points in local box
	pointa[2]=pointb[2] = 0.;

	glBegin(GL_LINES);
	
	for(int iso = 0; iso< iParams->getNumIsovalues(); iso++){
		pair<int,int> mapPair = make_pair(timestep, iso);
		vector<float*> lines = lineCache[mapPair];
		for (int linenum = 0; linenum < lines.size(); linenum++){
			pointa[0] = lines[linenum][0];
			pointa[1] = lines[linenum][1];
			pointb[0] = lines[linenum][2];
			pointb[1] = lines[linenum][3];
			vtransform(pointa,transformMatrix,point1);
			vtransform(pointb,transformMatrix,point2);
			ViewpointParams::localToStretchedCube(point1,point1);
			ViewpointParams::localToStretchedCube(point2,point2);
			glVertex3fv(point1);
			glVertex3fv(point2);
		}
	}
	glEnd();

}
/*
  Set up the OpenGL rendering state, 
*/

void IsolineRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	
	initialized = true;
}

bool IsolineRenderer::buildLineCache(int timestep){
	
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return false;
	IsolineParams* iParams = (IsolineParams*)getRenderParams();
	if (doBypass(timestep)) return false;

	int actualRefLevel = iParams->GetRefinementLevel();
	int lod = iParams->GetCompressionLevel();
		
	const vector<double>&userExts = dataMgr->GetExtents((size_t)timestep);
	RegularGrid* isolineGrid;
	vector<string>varname;
	varname.push_back(iParams->GetVariableName());
	double extents[6];
	float boxmin[3],boxmax[3];
	iParams->getLocalContainingRegion(boxmin, boxmax);
	for (int i = 0; i<3; i++){
		extents[i] = boxmin[i]+userExts[i];
		extents[i+3] = boxmax[i]+userExts[i];
	}
	
	int rc = iParams->getGrids( (size_t)timestep, varname, extents, &actualRefLevel, &lod, &isolineGrid);
	if(!rc){
		return false;
	}
	isolineGrid->SetInterpolationOrder(1);
	float dataVals[GRIDX*GRIDY];
	//Set up to transform from probe into volume:
	double transformMatrix[12];
	double planeCoords[3], dataCoords[3];
	planeCoords[2] = 0.;

	iParams->buildLocalCoordTransform(transformMatrix, 0.f, -1);

	//Get the data dimensions (at this resolution):
	int dataSize[3];
	//Start by initializing extents
	for (int i = 0; i< 3; i++){
		dataSize[i] = (int)ds->getFullSizeAtLevel(actualRefLevel,i);
	}
	const float* sizes = ds->getFullSizes();
	double extExtents[6]; //Extend extents 1/2 voxel on each side so no bdry issues.
	for (int i = 0; i<3; i++){
		double mid = (sizes[i])*0.5;
		double halfExtendedSize = sizes[i]*0.5*(1.f+dataSize[i])/(double)(dataSize[i]);
		extExtents[i] = mid - halfExtendedSize;
		extExtents[i+3] = mid + halfExtendedSize;
	}
	//Loop over grid, finding data values at each grid corner.
	//Missing value is also valid.
	float mv = isolineGrid->GetMissingValue();
	for (int i = 0; i<GRIDX; i++){
		planeCoords[0] = -1. + 2.*(double)i/(GRIDX-1.);
		for (int j = 0; j<GRIDY; j++){
			dataVals[i+j*GRIDX] = mv;  
			planeCoords[1] = -1. + 2.*(double)j/(GRIDY-1.);
			vtransform(planeCoords, transformMatrix, dataCoords);
			//find the coords that the texture maps to
			bool dataOK = true;
			for (int k = 0; k< 3; k++){
				if (dataCoords[k] < extExtents[k] || dataCoords[k] > extExtents[k+3]) dataOK = false;
				dataCoords[k] += userExts[k]; //Convert to user coordinates.
			}
			if(dataOK) { //find the coordinate in the data array
				dataVals[i+j*GRIDX] = isolineGrid->GetValue(dataCoords[0],dataCoords[1],dataCoords[2]);
			}
		}
	}
	//Loop over each isovalue and cell, and classify the cell as to which edges are crossed by the isoline.
	//when there is an isoline crossing, a line segment is saved in the cache, defined by the two endpoints.
	const vector<double>& isovals = iParams->GetIsovalues();
	for (int iso = 0; iso < isovals.size(); iso++){
		float isoval = (float)isovals[iso];
		int cellCase;
		float x1,y1,x2,y2;  //coordinates of intersection points
		//loop over cells (identified by lower-left vertices
		for (int i = 0; i<GRIDX-1; i++){
			for (int j = 0; j<GRIDY-1; j++){
				//Determine which case is associated with cell cornered at i,j
				if ((dataVals[i+j*GRIDX] == mv) || (dataVals[i+1+j*GRIDX] == mv) ||
					(dataVals[i+(j+1)*GRIDX] == mv) || (dataVals[i+1+(j+1)*GRIDX] == mv)) cellCase = 0;
				else
					cellCase = edgeCode(i,j, GRIDX, isoval, dataVals);
			
				//Note the vertices are numbered counterclockwise starting with 0 at (i,j)
				switch (cellCase) {
					case(0): //no lines
						break;
					case(1): //lines intersect between vertices 0-1 and vertices 1-2
						y1 = -1. + 2.*(double)j/(GRIDY-1.);
						x2 = -1. + 2.*(double)(i+1)/(GRIDX-1.);
						x1 = (dataVals[i+j*GRIDX]-isoval)/(dataVals[i+j*GRIDX]-dataVals[i+1+j*GRIDX]);
						y2 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+1+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(2): //lines intersect 1-2 and 2-3
						y2 = -1. + 2.*(double)(j+1)/(GRIDY-1.);
						x1 = -1. + 2.*(double)(i+1)/(GRIDX-1.);
						x2 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+(j+1)*GRIDX]);
						y1 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+1+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(3): //lines intersect 2-3 and 0-3
						y1 = -1. + 2.*(double)(j+1)/(GRIDY-1.);
						x2 = -1. + 2.*(double)(i)/(GRIDX-1.);
						x1 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+(j+1)*GRIDX]);
						y2 = (dataVals[i+(j+1)*GRIDX]-isoval)/(dataVals[i+(j+1)*GRIDX]-dataVals[i+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(4): //lines intersect 0-3 and 0-1
						y2 = -1. + 2.*(double)(j)/(GRIDY-1.);
						x1 = -1. + 2.*(double)(i)/(GRIDX-1.);
						x2 = (dataVals[i+1+(j)*GRIDX]-isoval)/(dataVals[i+1+(j)*GRIDX]-dataVals[i+(j)*GRIDX]);
						y1 = (dataVals[i+(j+1)*GRIDX]-isoval)/(dataVals[i+(j+1)*GRIDX]-dataVals[i+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(5): //lines intersect 0-1 and 2-3
						y2 = -1. + 2.*(double)(j+1)/(GRIDY-1.);
						y1 = -1. + 2.*(double)(j)/(GRIDY-1.);
						x1 = (dataVals[i+j*GRIDX]-isoval)/(dataVals[i+j*GRIDX]-dataVals[i+1+j*GRIDX]);
						x2 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+(j+1)*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(6): //line intersect 1-2 and 0-3
						x1 = -1. + 2.*(double)(i+1)/(GRIDX-1.);
						x2 = -1. + 2.*(double)(i)/(GRIDX-1.);
						y1 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+1+j*GRIDX]);
						y2 = (dataVals[i+(j+1)*GRIDX]-isoval)/(dataVals[i+(j+1)*GRIDX]-dataVals[i+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(7): //both cases 1 and 3
						y1 = -1. + 2.*(double)j/(GRIDY-1.);
						x2 = -1. + 2.*(double)(i+1)/(GRIDX-1.);
						x1 = (dataVals[i+j*GRIDX]-isoval)/(dataVals[i+j*GRIDX]-dataVals[i+1+j*GRIDX]);
						y2 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+1+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						y1 = -1. + 2.*(double)(j+1)/(GRIDY-1.);
						x2 = -1. + 2.*(double)(i)/(GRIDX-1.);
						x1 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+(j+1)*GRIDX]);
						y2 = (dataVals[i+(j+1)*GRIDX]-isoval)/(dataVals[i+(j+1)*GRIDX]-dataVals[i+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					case(8):  //both cases 2 and 4:
						y2 = -1. + 2.*(double)(j+1)/(GRIDY-1.);
						x1 = -1. + 2.*(double)(i+1)/(GRIDX-1.);
						x2 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+(j+1)*GRIDX]);
						y1 = (dataVals[i+1+(j+1)*GRIDX]-isoval)/(dataVals[i+1+(j+1)*GRIDX]-dataVals[i+1+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						
						y2 = -1. + 2.*(double)(j)/(GRIDY-1.);
						x1 = -1. + 2.*(double)(i)/(GRIDX-1.);
						x2 = (dataVals[i+1+(j)*GRIDX]-isoval)/(dataVals[i+1+(j)*GRIDX]-dataVals[i+(j)*GRIDX]);
						y1 = (dataVals[i+(j+1)*GRIDX]-isoval)/(dataVals[i+(j+1)*GRIDX]-dataVals[i+j*GRIDX]);
						addLineSegment(timestep,iso,x1,y1,x2,y2);
						break;
					default:
						assert(0);
				}
			} //for j
		} //for i
	} //for iso...		
	numIsovalsCached = isovals.size();
	cacheValidFlags[timestep] = true;
	return true;
}
void IsolineRenderer::invalidateLineCache(int timestep){
	int numisovals = numIsovalsInCache();
	for (int iso = 0; iso<numisovals; iso++){
		pair<int,int> indexpair = make_pair(timestep,iso);
		vector<float*> isoPoints = lineCache[indexpair];
		for (int i = 0; i<isoPoints.size(); i++){
			delete isoPoints[i];
		}
		isoPoints.clear();
	}
	cacheValidFlags[timestep]=false;
}

//setupCache must be called whenever a new renderer is created, and whenever the number of isovalues changes.
void IsolineRenderer::setupCache(){
	DataStatus* ds = DataStatus::getInstance();
	IsolineParams* iParams = (IsolineParams*)getRenderParams();
	for (size_t ts = ds->getMinTimestep(); ts <= ds->getMaxTimestep(); ts++){
		invalidateLineCache((int)ts);
		for (int iso = 0; iso < iParams->getNumIsovalues(); iso++){
			pair<int,int> indexpair = make_pair((int)ts,iso);
			lineCache[indexpair] = *(new vector<float*>);
		}
	}
	numIsovalsCached = iParams->getNumIsovalues();
}
//Classify a cell to one of 9 possibilities:
//0: no crossing
//1,2,3,4 : cross at 1 corner
//5,6: cross opposite edges (0-1, 2-3, and 1-2, 0-3)
//7,8: cross all 4 corners, center agrees or disagrees with starting vertex.
//All but 7 & 8 come from a lookup of 8 inputs (4 vertices, each above or below isovalue)
//Note that none of the vertices should have missing value
int IsolineRenderer::edgeCode(int i, int j, int gridx, float isoval, float* dataVals){
	// intersection code is 1 if it intersects the first edge, 
	// 2 for the second edge, 4 for the third, 8 for the fourth.
	// resulting combinations include 1+2, 1+4, 1+8, 2+4, 2+8, 4+8, 1+2+4+8 = 3,5,6,9,10,12,15.
	// These remap as follows: 3->1; 5->5; 6->2; 9->3; 10->6; 12->4; 15-> 7 or 8
	int intersectionCode = 0;
	//check for crossing between (i,j) and (i+1,j)
	if((dataVals[i+gridx*j] < isoval &&  dataVals[i+1+gridx*j] > isoval) ||
			(dataVals[i+gridx*j] > isoval &&  dataVals[i+1+gridx*j] < isoval)) intersectionCode+=1;
	//check for crossing between (i+1,j+1) and (i+1,j)
	if((dataVals[i+1+gridx*j] < isoval &&  dataVals[i+1+gridx*(j+1)] > isoval) ||
			(dataVals[i+1+gridx*j] > isoval &&  dataVals[i+1+gridx*(j+1)] < isoval)) intersectionCode+=2;
	//check for crossing between (i,j+1) and (i+1,j+1)
	if((dataVals[i+gridx*(j+1)] < isoval &&  dataVals[i+1+gridx*(j+1)] > isoval) ||
			(dataVals[i+gridx*(j+1)] > isoval &&  dataVals[i+1+gridx*(j+1)] < isoval)) intersectionCode+=4;
	//check for crossing between (i,j+1) and (i,j)
	if((dataVals[i+gridx*j] < isoval &&  dataVals[i+gridx*(j+1)] > isoval) ||
			(dataVals[i+gridx*j] > isoval &&  dataVals[i+gridx*(j+1)] < isoval)) intersectionCode+=8;
	
	int ecode;
	float avgvalue;
	// Remap intersectionCode to ecode: 0->0, 3->1; 5->5; 6->2; 9->3; 10->6; 12->4; 15-> 7 or 8
	switch(intersectionCode){
		case(0):
			ecode = 0; break;
		case(3):
			ecode = 1; break;
		case(5):
			ecode = 5; break;
		case(6): 
			ecode = 2; break;
		case(9):
			ecode = 3; break;
		case(10):
			ecode = 6; break;
		case(12):
			ecode = 4; break;
		case(15):  //disambiguate 7 and 8, based on whether or not average is on same side of isovalue as (i,j) vertex.
				//average is used as best approximation of value at center of cell.
			avgvalue = 0.25*(dataVals[i+gridx*j]+dataVals[i+gridx*(j+1)]+dataVals[i+1+gridx*(j+1)]+ dataVals[i+1+gridx*j]);
			if( ((dataVals[i+gridx*j] < isoval) && (avgvalue < isoval)) ||
				((dataVals[i+gridx*j] > isoval) && (avgvalue > isoval)) ) {
					//average agrees with (i,j), so use segments that connect 0-1 and 1-2 edge as well as 2-3 and 3-0
				ecode = 7;
			} else //use segments that connect edges 1-2 and 2-3 as well as 3-0 and 0-1
				ecode = 8;
			break;
		default:
			ecode = -1;
			assert(0);
	}
	return ecode;
}
void IsolineRenderer::addLineSegment(int timestep, int isoIndex, float x1, float y1, float x2, float y2){
	float* floatvec = new float[4];
	floatvec[0] = x1; floatvec[1]=y1; floatvec[2]=x2; floatvec[3] = y2;
	pair<int,int> indexpair = make_pair(timestep,isoIndex);
	lineCache[indexpair].push_back(floatvec);
	
}