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

#include "glutil.h"	// Must be included first!!!
#include "params.h"
#include "isolinerenderer.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"
#include <vapor/errorcodes.h>
#include <vapor/GetAppPath.h>

#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include "renderer.h"
#include <sstream>
#include <string>
#include "textRenderer.h"
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
	
	myGLWindow->clearTextObjects(this);
	
}


// Perform the rendering
//
  

void IsolineRenderer::paintGL()
{
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;

	int timestep = myGLWindow->getActiveAnimationParams()->getCurrentTimestep();

	if (!cacheIsValid(timestep) || myGLWindow->textRenderersAreDirty()){
		if (!buildLineCache(timestep)) return;
	}
		

	
	//
	//Perform OpenGL rendering of line segments
	//
	performRendering(timestep);
	
	
}
void IsolineRenderer::performRendering(int timestep){
	
	IsolineParams* iParams = (IsolineParams*)getRenderParams();
	//Set up lighting 
	
	
	glDisable(GL_LIGHTING);
	glEnable(GL_LINE_SMOOTH);
	glHint(GL_LINE_SMOOTH_HINT,GL_NICEST);
	
	glLineWidth(iParams->GetLineThickness());
	//Need to convert the iso-box coordinates to user coordinates, then to unit box coords.
	float transformMatrix[12];
	iParams->buildLocalCoordTransform(transformMatrix, 0.f, -1);
	float pointa[3],pointb[3]; //points in cache
	float point1[3],point2[3]; //points in local box
	pointa[2]=pointb[2] = 0.;
	
	glBegin(GL_LINES);
	
	for(int iso = 0; iso< iParams->getNumIsovalues(); iso++){
		float lineColor[3];
		iParams->getLineColor(iso,lineColor);
		glColor3fv(lineColor);
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
	float boxexts[6];

	bool is3D = iParams->VariablesAre3D();
	if (is3D) iParams->getLocalContainingRegion(boxexts, boxexts+3);
	else iParams->GetBox()->GetLocalExtents(boxexts);

	for (int i = 0; i<6; i++){
		extents[i] = boxexts[i]+userExts[i%3];
	}
	
	int rc = iParams->getGrids( (size_t)timestep, varname, extents, &actualRefLevel, &lod, &isolineGrid);
	if(!rc){
		return false;
	}
	isolineGrid->SetInterpolationOrder(1);
	//Determine resolution of grid to use.  
	size_t min_dim[3],max_dim[3];
	dataMgr->GetEnclosingRegion((size_t)timestep, extents, extents+3, min_dim, max_dim, actualRefLevel,lod);
	// Choose the grid sizes to both equal the largest number
	//of integer extents.
	gridSize = Max(max_dim[2]-min_dim[2],Max(max_dim[1]-min_dim[1],max_dim[0]-min_dim[0]));
	float* dataVals = new float[gridSize*gridSize];
	//Set up to transform from isoline plane into volume:
	float a[2],b[2],constValue[2];
	int mapDims[3];
	double transformMatrix[12];

	if(is3D)iParams->buildLocalCoordTransform(transformMatrix, 0.f, -1);
	else iParams->buildLocal2DTransform(2, a,b,constValue,mapDims);
	
	double planeCoords[3], dataCoords[3];
	planeCoords[2] = 0.;

	

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
	for (int i = 0; i<gridSize; i++){
		planeCoords[0] = -1. + 2.*(double)i/(gridSize-1.);
		for (int j = 0; j<gridSize; j++){
			dataVals[i+j*gridSize] = mv;  
			planeCoords[1] = -1. + 2.*(double)j/(gridSize-1.);
			if (is3D) vtransform(planeCoords, transformMatrix, dataCoords);
			else {
				//2D transform is a*x + b
				dataCoords[0] = a[0]*planeCoords[0] + b[0];
				dataCoords[1] = a[1]*planeCoords[1] + b[1];
				dataCoords[2] = 0.;
			}
			//find the coords that the texture maps to
			bool dataOK = true;
			for (int k = 0; k< 3; k++){
				if (dataCoords[k] < extExtents[k] || dataCoords[k] > extExtents[k+3]) dataOK = false;
				dataCoords[k] += userExts[k]; //Convert to user coordinates.
			}
			if(dataOK) { //find the coordinate in the data array
				dataVals[i+j*gridSize] = isolineGrid->GetValue(dataCoords[0],dataCoords[1],dataCoords[2]);
			}
		}
	}
	//Loop over each isovalue and cell, and classify the cell as to which edges are crossed by the isoline.
	//when there is an isoline crossing, a line segment is saved in the cache, defined by the two endpoints.
	const vector<double>& isovals = iParams->GetIsovalues();
	
	//Clear the textObjects (if they exist)
	myGLWindow->clearTextObjects(this);
	//Create a new textObject for each isoline:
	float bgc[4];
	const QColor clr = DataStatus::getInstance()->getBackgroundColor();
	bgc[0] = (float)clr.red()/255.;
	bgc[1] = (float)clr.green()/255.;
	bgc[2] = (float)clr.blue()/255.;
	bgc[3] = 1.;
	objectNums.clear();
	for (int iso = 0; iso < isovals.size(); iso++){
		if(iParams->GetTextDensity() > 0. && iParams->textEnabled()) { //put a textObject in the GLWindow to hold annotation of this isovalue
			
			float bgc[4] = {0,0,0,1.};
			const QColor c = DataStatus::getInstance()->getBackgroundColor();
			bgc[0] = c.redF();
			bgc[1] = c.greenF();
			bgc[2] = c.blueF();
			float lineColor[4];
			iParams->getLineColor(iso,lineColor);
			lineColor[3]=1.;
			vector<string> vec;
			vec.push_back("fonts");
			vec.push_back("Vera.ttf");
			string isoText;
			doubleToString((iParams->GetIsovalues()[iso]), isoText, iParams->GetNumDigits());
			int objNum = myGLWindow->addTextObject(this, GetAppPath("VAPOR","share",vec).c_str(),(int)iParams->GetTextSize(),lineColor, bgc,1, isoText);
			objectNums[iso] = objNum;
		}
		//Clear out temporary caches for this isovalue:
		
		edgeSeg.clear(); //map an edge to one segment
		edgeEdge1.clear(); //map an edge to one adjacent edge;
		edgeEdge2.clear(); //map an edge to other adjacent edge;

		float isoval = (float)isovals[iso];
		int cellCase;
		float x1,y1,x2,y2;  //coordinates of intersection points
		int segIndex;
		//loop over cells (identified by lower-left vertices
		for (int i = 0; i<gridSize-1; i++){
			for (int j = 0; j<gridSize-1; j++){
				//Determine which case is associated with cell cornered at i,j
				if ((dataVals[i+j*gridSize] == mv) || (dataVals[i+1+j*gridSize] == mv) ||
					(dataVals[i+(j+1)*gridSize] == mv) || (dataVals[i+1+(j+1)*gridSize] == mv)) cellCase = 0;
				else
					cellCase = edgeCode(i,j, isoval, dataVals);
			
				//Note the vertices are numbered counterclockwise starting with 0 at (i,j)
				switch (cellCase) {
					case(0): //no lines
						break;
					case(1): //lines intersect 0-3 [point 1] and 0-1 [point 2]
						{
						y2 = -1. + 2.*(double)(j)/(gridSize-1.);
						x1 = -1. + 2.*(double)(i)/(gridSize-1.);
						
						x2 = interp_i(i,j,isoval, dataVals);
						y1 = interp_j(i,j,isoval, dataVals);
						//line segment connects horizontal edge i,j with vertical edge (i,j)
						pair<int,int> edge1 = make_pair(i,j);
						pair<int,int> edge2 = make_pair(-i-1,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						}
						break;
					case(2): //lines intersect between vertices 0-1 [1] and vertices 1-2 [2]
						{
						y1 = -1. + 2.*(double)j/(gridSize-1.);
						x2 = -1. + 2.*(double)(i+1)/(gridSize-1.);
						x1 = interp_i(i,j,isoval,dataVals);
						y2 = interp_j(i+1,j,isoval,dataVals);
						//line segment connects horizontal edge i,j with vertical edge (i+1,j)
						pair<int,int> edge1 = make_pair(i,j);
						pair<int,int> edge2 = make_pair(-i-2,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						}
						break;
					case(3): //lines intersect 1-2 [1] and 2-3 [2]
						{
						y2 = -1. + 2.*(double)(j+1)/(gridSize-1.);
						x1 = -1. + 2.*(double)(i+1)/(gridSize-1.);
						x2 = interp_i(i,j+1,isoval,dataVals);
						y1 = interp_j(i+1,j,isoval,dataVals);
						//line segment connects horizontal edge i,j+1 with vertical edge (i+1,j)
						pair<int,int> edge1 = make_pair(i,j+1);
						pair<int,int> edge2 = make_pair(-i-2,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						}
						break;
					case(4): //lines intersect 2-3 [1] and 0-3 [2]
						{
						y1 = -1. + 2.*(double)(j+1)/(gridSize-1.);
						x2 = -1. + 2.*(double)(i)/(gridSize-1.);
						x1 = interp_i(i,j+1,isoval,dataVals);
						y2 = interp_j(i,j,isoval,dataVals);
						//line segment connects horizontal edge i,j+1 with vertical edge (i,j)
						pair<int,int> edge1 = make_pair(i,j+1);
						pair<int,int> edge2 = make_pair(-i-1,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						}
						break;
					
					case(5): //lines intersect 0-1 [1] and 2-3 [2]
						{
						y2 = -1. + 2.*(double)(j+1)/(gridSize-1.);
						y1 = -1. + 2.*(double)(j)/(gridSize-1.);
						x1 = interp_i(i,j,isoval,dataVals);
						x2 = interp_i(i,j+1,isoval,dataVals);
						//line segment connects horizontal edge i,j with horizontal edge (i,j+1)
						pair<int,int> edge1 = make_pair(i,j);
						pair<int,int> edge2 = make_pair(i,j+1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						}
						break;
					case(6): //line intersect 1-2 [1] and 0-3 [2]
						{
						x1 = -1. + 2.*(double)(i+1)/(gridSize-1.);
						x2 = -1. + 2.*(double)(i)/(gridSize-1.);
						y1 = interp_j(i+1,j,isoval,dataVals);
						y2 = interp_j(i,j,isoval,dataVals);
						//line segment connects vertical edge i+1,j with vertical edge (i,j)
						pair<int,int> edge1 = make_pair(-i-2,-j-1);
						pair<int,int> edge2 = make_pair(-i-1,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						}
						break;
					case(7): //both cases 2 and 4
						//lines intersect between vertices 0-1 [1] and vertices 1-2 [2]
						{
						y1 = -1. + 2.*(double)j/(gridSize-1.);
						x2 = -1. + 2.*(double)(i+1)/(gridSize-1.);
						x1 = interp_i(i,j,isoval,dataVals);
						y2 = interp_j(i+1,j,isoval,dataVals);
						//line segment connects horizontal edge i,j with vertical edge (i+1,j)
						pair<int,int> edge1 = make_pair(i,j);
						pair<int,int> edge2 = make_pair(-i-2,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						//lines intersect 2-3 [1] and 0-3 [2]
						y1 = -1. + 2.*(double)(j+1)/(gridSize-1.);
						x2 = -1. + 2.*(double)(i)/(gridSize-1.);
						x1 = interp_i(i,j+1,isoval,dataVals);
						y2 = interp_j(i,j,isoval,dataVals);
						//line segment connects horizontal edge i,j+1 with vertical edge (i,j)
						pair<int,int> edge3 = make_pair(i,j+1);
						pair<int,int> edge4 = make_pair(-i-1,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge3,edge4);
						}
						break;
					case(8):  //both cases 1 and 3
						//lines intersect 0-3 [point 1] and 0-1 [point 2]
						{
						y2 = -1. + 2.*(double)(j)/(gridSize-1.);
						x1 = -1. + 2.*(double)(i)/(gridSize-1.);
						x2 = interp_i(i,j,isoval, dataVals);
						y1 = interp_j(i,j,isoval, dataVals);
						//line segment connects horizontal edge i,j with vertical edge (i,j)
						pair<int,int> edge1 = make_pair(i,j);
						pair<int,int> edge2 = make_pair(-i-1,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge1,edge2);
						//lines intersect 1-2 [1] and 2-3 [2]
						y2 = -1. + 2.*(double)(j+1)/(gridSize-1.);
						x1 = -1. + 2.*(double)(i+1)/(gridSize-1.);
						x2 = interp_i(i,j+1,isoval,dataVals);
						y1 = interp_j(i+1,j,isoval,dataVals);
						//line segment connects horizontal edge i,j+1 with vertical edge (i+1,j)
						pair<int,int> edge3 = make_pair(i,j+1);
						pair<int,int> edge4 = make_pair(-i-2,-j-1);
						segIndex = addLineSegment(timestep,iso,x1,y1,x2,y2);
						addEdges(segIndex,edge3,edge4);
						}
						break;
					default:
						assert(0);
				}
			} //for j
		} //for i

		//Now traverse the edges to determine the isolines in order
		if(iParams->GetTextDensity() > 0. && iParams->textEnabled()) {
			traverseCurves(iso, timestep);
		}

	} //for iso...		
	numIsovalsCached = isovals.size();
	cacheValidFlags[timestep] = true;
	delete dataVals;
	return true;
}
void IsolineRenderer::invalidateLineCache(int timestep){
	int numisovals = numIsovalsInCache();
	for (int iso = 0; iso<numisovals; iso++){
		pair<int,int> indexpair = make_pair(timestep,iso);
		for (int i = 0; i< lineCache[indexpair].size(); i++){
			delete lineCache[indexpair][i];
		}
		lineCache[indexpair].clear();
	}
	cacheValidFlags[timestep]=false;
}
void IsolineRenderer::invalidateLineCache(){
	DataStatus* ds = DataStatus::getInstance();
	
	for (int ts = ds->getMinTimestep(); ts <= ds->getMaxTimestep(); ts++)
		invalidateLineCache(ts);
	numIsovalsCached = 0;
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
//1,2,3,4 : cross at 1 corner (vertices 0,1,2,3 respectively)
//5,6: cross opposite edges (between vertices 0-1 & 2-3 for 5, and between  1-2 & 0-3 for 6.
//7,8: cross all 4 corners, center agrees or disagrees with vertex 0
//All but 7 & 8 come from a lookup of 8 inputs (4 vertices, each above or below isovalue)
//Note that none of the vertices should have missing value
int IsolineRenderer::edgeCode(int i, int j, float isoval, float* dataVals){
	// intersection code is 1 if it intersects the first edge, 
	// 2 for the second edge, 4 for the third, 8 for the fourth.
	// resulting combinations include 1+2, 1+4, 1+8, 2+4, 2+8, 4+8, 1+2+4+8 = 3,5,6,9,10,12,15.
	// These remap as follows: 3->2; 5->5; 6->3; 9->1; 10->6; 12->4; 15-> 7 or 8
	int intersectionCode = 0;
	//check for crossing between (i,j) and (i+1,j)
	if((dataVals[i+gridSize*j] < isoval &&  dataVals[i+1+gridSize*j] >= isoval) ||
			(dataVals[i+gridSize*j] >= isoval &&  dataVals[i+1+gridSize*j] < isoval)) intersectionCode+=1;
	//check for crossing between (i+1,j+1) and (i+1,j)
	if((dataVals[i+1+gridSize*j] < isoval &&  dataVals[i+1+gridSize*(j+1)] >= isoval) ||
			(dataVals[i+1+gridSize*j] >= isoval &&  dataVals[i+1+gridSize*(j+1)] < isoval)) intersectionCode+=2;
	//check for crossing between (i,j+1) and (i+1,j+1)
	if((dataVals[i+gridSize*(j+1)] < isoval &&  dataVals[i+1+gridSize*(j+1)] >= isoval) ||
			(dataVals[i+gridSize*(j+1)] >= isoval &&  dataVals[i+1+gridSize*(j+1)] < isoval)) intersectionCode+=4;
	//check for crossing between (i,j+1) and (i,j)
	if((dataVals[i+gridSize*j] < isoval &&  dataVals[i+gridSize*(j+1)] >= isoval) ||
			(dataVals[i+gridSize*j] >= isoval &&  dataVals[i+gridSize*(j+1)] < isoval)) intersectionCode+=8;
	
	int ecode;
	float avgvalue;
	// Remap intersectionCode to ecode: 0->0, 3->2; 5->5; 6->3; 9->1; 10->6; 12->4; 15-> 7 or 8
	switch(intersectionCode){
		case(0):
			ecode = 0; break;
		case(3):
			ecode = 2; break;
		case(5):
			ecode = 5; break;
		case(6): 
			ecode = 3; break;
		case(9):
			ecode = 1; break;
		case(10):
			ecode = 6; break;
		case(12):
			ecode = 4; break;
		case(15):  //disambiguate 7 and 8, based on whether or not average is on same side of isovalue as (i,j) vertex.
				//average is used as best approximation of value at center of cell.
			avgvalue = 0.25*(dataVals[i+gridSize*j]+dataVals[i+gridSize*(j+1)]+dataVals[i+1+gridSize*(j+1)]+ dataVals[i+1+gridSize*j]);
			if( ((dataVals[i+gridSize*j] < isoval) && (avgvalue < isoval)) ||
				((dataVals[i+gridSize*j] > isoval) && (avgvalue > isoval)) ) {
					//average agrees with (i,j), so use segments that connect 0-1 and 1-2 edge [case 2] as well as 2-3 and 3-0 [case 4]
				ecode = 7;
			} else //use segments that connect edges 1-2 and 2-3 as well as 3-0 and 0-1 [case 1 and case 3]
				ecode = 8;
			break;
		default:
			ecode = -1;
			assert(0);
	}
	return ecode;
}
int IsolineRenderer::addLineSegment(int timestep, int isoIndex, float x1, float y1, float x2, float y2){
	float* floatvec = new float[4];
	floatvec[0] = x1; floatvec[1]=y1; floatvec[2]=x2; floatvec[3] = y2;
	pair<int,int> indexpair = make_pair(timestep,isoIndex);
	lineCache[indexpair].push_back(floatvec);
	return (lineCache[indexpair].size()-1);
	
}
	//recall temporary mappings
	//std::map< pair<int,int>, int> edgeSeg; //map an edge to one segment
	//std::map<pair<int,int>,pair<int,int> > edgeEdge1; //map an edge to one adjacent edge;
	//std::map<pair<int,int>,pair<int,int> > edgeEdge2; //map an edge to other adjacent edge;
	//std::map<pair<int,int>, bool> markerBit;  //indicate whether or not an edge has been visited during traversal

//Whenever a segment is added, construct associated edge->edge mappings (both directions) 
//and a mapping of the first edge to the segment.  The bidirectional edge mappings are used
//to enable traversal of the isoline in segment order.  The edge->segment mapping is used to
//determine the coordinates to place the annotation while traversing the isoline.
void IsolineRenderer::addEdges(int segIndex, pair<int,int> edge1, pair<int,int> edge2) {
	
	std::map<pair<int,int>, pair<int,int> >::iterator edgeEdgeIter;
	
	//Set mapping of both edges to segment:
	edgeSeg[edge1] = segIndex;
	edgeSeg[edge2] = segIndex;
	//Set marker bits for both edges:
	markerBit[edge1] = false;
	markerBit[edge2] = false;
	//Create the edge1->edge2 mapping:
	//See if edge1->?? mapping is in first map:
	edgeEdgeIter = edgeEdge1.find(edge1);
	if (edgeEdgeIter == edgeEdge1.end()){
		//Not found; insert it:
		edgeEdge1[edge1] = edge2;
	} else {
		//check:  It should not be in the second map yet
		edgeEdgeIter = edgeEdge2.find(edge1);
		assert(edgeEdgeIter == edgeEdge2.end());
		edgeEdge2[edge1] = edge2;
	}
	//Now create the edge2->edge1 mapping
	edgeEdgeIter = edgeEdge1.find(edge2);
	if (edgeEdgeIter == edgeEdge1.end()){
		//Not found; insert it:
		edgeEdge1[edge2] = edge1;
	} else {
		//check:  It should not be in the second map yet
		edgeEdgeIter = edgeEdge2.find(edge2);
		assert(edgeEdgeIter == edgeEdge2.end());
		edgeEdge2[edge2] = edge1;
	}
	
}
//Use the edge-edge mappings to traverse each isoline
//First pass is simply to determine an endpoint and length.  
//Second pass will write annotation
void IsolineRenderer::traverseCurves(int iso, int timestep){
	//Initialize counters:
	int numComponents = 0;
	int maxLength = -1;
	int minLength = 100000000;
	int totLength = 0;
	int currentLength;  //length of current component
	componentLength.clear();
	endEdge.clear();
	IsolineParams* iParams = (IsolineParams*)getRenderParams();

	//string isoText = std::to_string((long double)(iParams->GetIsovalues()[iso]));
	string isoText;
	doubleToString((iParams->GetIsovalues()[iso]), isoText, iParams->GetNumDigits());
	//Prepare to convert the iso-box coordinates to user coordinates, then to unit box coords.
	float transformMatrix[12];
	iParams->buildLocalCoordTransform(transformMatrix, 0.f, -1);
	float pointa[3]; //point in cache
	float point1[3]; //point in local box
	pointa[2] = 0.;
	
	
	//Repeat the following until no more edges are found:
	//Find an unmarked edge.  Mark it.
	std::map<pair<int,int>, pair<int,int> >::iterator edgeEdgeIter;
	std::map<pair<int,int>, pair<int,int> >::iterator edgeEdgeTestIter;
	std::map<pair<int,int>, pair<int,int> >::iterator edgeEdgeTestIter2;
	
	//Iterate looking for unmarked edge in first mapping.  Note that all edges must appear in both mappings
	for (edgeEdgeIter = edgeEdge1.begin(); edgeEdgeIter != edgeEdge1.end(); edgeEdgeIter++){
		pair<int,int> startingEdge = edgeEdgeIter->first;
		if (markerBit[startingEdge]) continue;  //already marked; keep looking
		//OK, found an unmarked edge
		markerBit[startingEdge] = true; //Mark it
		currentLength = 0;
		bool firstDirection = true;
		//Make sure there is a second direction possible
		edgeEdgeTestIter = edgeEdge2.find(startingEdge);
		if (edgeEdgeTestIter == edgeEdge2.end()) firstDirection = false;

		pair<int,int> currentEdge = startingEdge;
		//Obtain next edges in both directions:
		//Repeat until cannot go further:
		while(1){
			//Check an adjacent edge.  If it's not marked, make it the current edge, repeat
			//make sure current edge is in at least one mapping
			
			edgeEdgeTestIter = edgeEdge1.find(currentEdge);
			if(edgeEdgeTestIter == edgeEdge1.end()){
				edgeEdgeTestIter = edgeEdge2.find(currentEdge);
				if (edgeEdgeTestIter == edgeEdge2.end()){
					//this means currentEdge goes nowhere (in either mapping)
					//See if we can go the other direction from the start (note this code is replicated below...)
					if (firstDirection){
						firstDirection = false;
						//Try other direction at start 		
						currentEdge = edgeEdge2[startingEdge];
						if (!markerBit[currentEdge]) {
							currentLength++;
							markerBit[currentEdge] = true;
							continue; //continue our traversal with the new currentEdge
						}
					}
					//Otherwise we are all done with this isoline; collect some stats
					if (currentLength == 0) break;
					numComponents++;
					totLength += currentLength;
					if (currentLength > maxLength) maxLength = currentLength;
					if (currentLength < minLength) minLength = currentLength;
					componentLength.push_back(currentLength);
					endEdge.push_back(currentEdge);
					assert(componentLength.size() == numComponents);
					break;  //exit while(1) loop
				}
			} 
			//set nextEdge to the connected edge we found
			pair<int,int> nextEdge = edgeEdgeTestIter->second;
			if (!markerBit[nextEdge]){
				currentEdge = nextEdge;
				currentLength++;
				markerBit[currentEdge] = true; //mark it...
				continue;
			} else {
				//it's marked false.  Need to consider other direction:
				edgeEdgeTestIter = edgeEdge2.find(currentEdge);
				
				if ((edgeEdgeTestIter == edgeEdge2.end()) || (markerBit[edgeEdgeTestIter->second])){
					//Both ends are marked (or there is no other end) so
					//we are at (one) end of isoline
					//See if we can go the other direction from the start (this is replication of above code!)
					if (firstDirection){
						firstDirection = false;
						//Try other direction at start 		
						currentEdge = edgeEdge2[startingEdge];
						if (!markerBit[currentEdge]) {
							markerBit[currentEdge] = true;
							currentLength++;
							continue; //continue our traversal with the new currentEdge
						}
					}
					//Otherwise we are all done with this isoline; collect some stats
					if (currentLength == 0) break;
					numComponents++;
					totLength += currentLength;
					if (currentLength > maxLength) maxLength = currentLength;
					if (currentLength < minLength) minLength = currentLength;
					componentLength.push_back(currentLength);
					endEdge.push_back(currentEdge);
					assert(componentLength.size() == numComponents);
					break;  //exit while(1) loop
				} else {
					//Not marked; continue with nextEdge:
					currentEdge = edgeEdgeTestIter->second;
					currentLength++;
					markerBit[currentEdge] = true;
					continue;
				}
			}
		}
	}
	//Done with traversals.  check if every edge has been marked
/* Comment out these tests for the release version:
	for (edgeEdgeIter = edgeEdge1.begin(); edgeEdgeIter != edgeEdge1.end(); edgeEdgeIter++){
		pair<int,int> thisEdge = edgeEdgeIter->first;
		assert (markerBit[thisEdge]) ;
	}
	for (edgeEdgeIter = edgeEdge2.begin(); edgeEdgeIter != edgeEdge2.end(); edgeEdgeIter++){
		pair<int,int> thisEdge = edgeEdgeIter->first;
		assert (markerBit[thisEdge]) ;
	}
*/
	//Now traverse each component, writing annotation at specified interval
	//Note that all marker bits are true,so we can use markerBit[i]==false as a new marker
	//When textDensity is 1, there is annotation at every point.  When textDensity is 0.5 (typical)
	//there should be about A annotations in crossing the domain; i.e. annotation interval should be about 1/A times the grid size
	//when textDensity is .5, and A is a normalization constant.  So define 
	// annotSpace = (2-g/A) + (g/A-1)/density where g is grid length or 2*A, whichever is larger
	//If the interval is shorter than the component and larger than 0.1 times the component, then just one annotation is generated
	float A = 3.;
	
	float g = (float)gridSize;
	if (g < 2*A) g = 2*A;
	int annotSpace = (2- g/A) + (g/A -1.f)/iParams->GetTextDensity();
	int numAnnotations = 0;
	markerBit.clear();
	for (int comp = 0; comp<numComponents; comp++){
		int annotInterval = annotSpace;
		pair<int,int> currentEdge = endEdge[comp];
		markerBit[currentEdge] = true;  //Mark it
		int length = componentLength[comp];
		if (length < annotInterval/10) continue; //No annotation for this component
		if (length < annotInterval) annotInterval = length;
		//Modify annotInterval so that it evenly divides length
		if (annotInterval < length){
			int frac = length/annotInterval;
			annotInterval = length/frac;
		}
		//Determine the first point as halfway between 0 and annotInterval-1;
		assert(annotInterval>0);
		int startDist = 1+annotInterval/2;
		//Traverse along component, marking as we go.
		//first, count until we get startDist along; after that, increment by annotInterval
		//Obtain next edges in both directions for traversal, but no need to go back to start.
		int advancedDist = 0;
		int currentAdvancedDist = 0;
		int gapInterval = startDist;
		const vector<double>&tvExts = DataStatus::getInstance()->getDataMgr()->GetExtents(timestep);
		std::pair<int,int> mapPair= make_pair(timestep, iso);
		vector<float*> lines = lineCache[mapPair];
		while(1){
			//Check an adjacent edge.  If it's not marked, make it the current edge, repeat
			//make sure current edge is in at least one mapping
			
			edgeEdgeTestIter = edgeEdge1.find(currentEdge);
			if(edgeEdgeTestIter == edgeEdge1.end()){
				edgeEdgeTestIter = edgeEdge2.find(currentEdge);
				if (edgeEdgeTestIter == edgeEdge2.end()){
					//this means currentEdge goes nowhere (in either mapping); must be at end already
					assert(advancedDist == length);
					break;  //exit while(1) loop
				}
			} 
			//set nextEdge to the connected edge we found
			pair<int,int> nextEdge = edgeEdgeTestIter->second;
			if (!markerBit[nextEdge]){ //not marked, so ok to continue
				currentEdge = nextEdge;
				advancedDist++;
				currentAdvancedDist++;
				markerBit[currentEdge] = true; //mark it...
				if (currentAdvancedDist == gapInterval){

					//display annotation here!
					int linenum = edgeSeg[currentEdge];
					pointa[0] = lines[linenum][0];
					pointa[1] = lines[linenum][1];
					vtransform(pointa,transformMatrix,point1);
					//Convert local to user:
					for (int i = 0; i<3; i++) point1[i] += tvExts[i];
					
					myGLWindow->addText(this, objectNums[iso],point1);
					numAnnotations++;
					//Reset the next interval:
					gapInterval = annotInterval;
					currentAdvancedDist = 0;
				}
				continue;
			} else {
				//it's marked true.  Need to consider other direction:
				edgeEdgeTestIter = edgeEdge2.find(currentEdge);
				
				if ((edgeEdgeTestIter == edgeEdge2.end()) || (markerBit[edgeEdgeTestIter->second])){
					//Both ends are marked (or there is no other end) so
					//we are at end of isoline
					assert(advancedDist == length);
					break;  //exit while(1) loop
					
				} else {
					//Not marked; continue with nextEdge:
					currentEdge = edgeEdgeTestIter->second;
					advancedDist++;
					currentAdvancedDist++;
					markerBit[currentEdge] = true; //mark it...
					if (currentAdvancedDist == gapInterval){
						
						//display annotation here!
						int linenum = edgeSeg[currentEdge];
						pointa[0] = lines[linenum][0];
						pointa[1] = lines[linenum][1];
						vtransform(pointa,transformMatrix,point1);
						//Convert local to user:
						for (int i = 0; i<3; i++) point1[i] += tvExts[i];
					
						myGLWindow->addText(this, objectNums[iso],point1);
						numAnnotations++;
						//Reset the next interval:
						gapInterval = annotInterval;
						currentAdvancedDist = 0;
					}
					continue;
				}
			}
		}
	//Done with component
	}
}
