//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowrenderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:	Implementation of the flowrenderer class
//

#include "params.h"
#include "flowrenderer.h"
#include "vapor/VaporFlow.h"
#include "vapor/errorcodes.h"
#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"


#include <math.h>
#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>

#include "renderer.h"
#include "mapperfunction.h"

//Constants used for arrow design:
//VERTEX_ANGLE = 45 degrees (angle between direction vector and head edge
#define ARROW_LENGTH_FACTOR  0.90f //fraction of full length used by cylinder
/*!
  Create a FlowRenderer 
*/
using namespace VAPoR;
FlowRenderer::FlowRenderer(GLWindow* glw, FlowParams* fParams )
:Renderer(glw, fParams, "FlowRenderer")
{
    myFlowLib = 0;
	
	
	for (int i = 0; i<4; i++) constFlowColor[i] = 1.f;

	//Set up flow data cache arrays, potentially for all
	//timesteps in data
	numFrames = DataStatus::getInstance()->getNumTimesteps();

	steadyFlowCache = new FlowLineData*[numFrames];
	unsteadyFlowCache = 0;
	
	numListSeedPointsUsed = new int[numFrames];
	steadyFlowRGBAs = new float*[numFrames];
	
	flowDataDirty = new bool[numFrames];
	needRefreshFlag = new bool[numFrames];
	flowMapDirty = new bool[numFrames];
	numRakeSeedPointsUsed = 0;
	for (int i = 0; i<numFrames; i++){
		steadyFlowCache[i] = 0;

		numListSeedPointsUsed[i] = 0;
		steadyFlowRGBAs[i] = 0;
		
		flowDataDirty[i] = false;
		needRefreshFlag[i] = false;
		flowMapDirty[i] = 0;
	}
	unsteadyFlowRGBAs = 0;
	allFlowMapDirtyFlag = false;
	unsteadyNeedsRefreshFlag = false;
	allDataDirtyFlag = false;

	setRegionValid(true);
	lastTimeStep = -1;
	interruptFlag = true;
	
	curVaSize = 0;
	lastShapeType = -1;
	vertexArray =  NULL;
	indexArray = NULL;
	useDisplayLists = false;
	flowDisplayList = glGenLists(1);
}


/*!
  Release allocated resources
*/

FlowRenderer::~FlowRenderer()
{
	for (int i = 0; i<numFrames; i++){
		if (steadyFlowCache[i]) delete steadyFlowCache[i];

		if (steadyFlowRGBAs[i]) delete steadyFlowRGBAs[i];
		
	}
	delete steadyFlowCache;
	delete steadyFlowRGBAs;
	if (unsteadyFlowRGBAs) delete unsteadyFlowRGBAs;
	
	delete numListSeedPointsUsed;
	delete needRefreshFlag;
	delete flowDataDirty;
	delete flowMapDirty;
	if (myFlowLib) delete myFlowLib;

	/*Clean up memory used for verticies and displaylists*/
	delete[] vertexArray;
	delete[] indexArray;
	glDeleteLists(flowDisplayList, 1);
}


// Perform the rendering
//
  

void FlowRenderer::paintGL()
{
	//If the regionValid flag is off, need a change before we try to render again..
	if (!regionValid()) return;
	
	
	AnimationParams* myAnimationParams = myGLWindow->getActiveAnimationParams();
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	//If the region is dirty, always need to rebuild:
	if(myGLWindow->regionIsDirty()) setDataDirty();
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	int flowType = myFlowParams->getFlowType();
	int timeStep = currentFrameNum;
	bool didRebuild = false;
	bool didRemap = false;
	bool constColors = ((myFlowParams->getColorMapEntityIndex() + myFlowParams->getOpacMapEntityIndex()) == 0);
	
	constFlowColor[3] = myFlowParams->getConstantOpacity();
	QRgb constRgb = myFlowParams->getConstantColor();
	constFlowColor[0] = qRed(constRgb)/255.f;
	constFlowColor[1] = qGreen(constRgb)/255.f;
	constFlowColor[2] = qBlue(constRgb)/255.f;

	//Check if the cache needs rebuilding, and/or rgba's need rebuilding:
	if ((flowDataIsDirty(timeStep) && needsRefresh(myFlowParams,timeStep))){
		if(!rebuildFlowData(timeStep)) {
			if(myFlowParams->refreshIsAuto())setBypass(timeStep);
			return;
		}
		didRebuild = true;
		didRemap = true;
	} else { //just rebuild the rgba's if necessary:
		if (!constColors && flowMapIsDirty(timeStep)){
			if (flowType != 1){
				if (steadyFlowCache[timeStep]) {
					myFlowParams->mapColors(steadyFlowCache[timeStep],timeStep, minFrame, myGLWindow->getActiveRegionParams());
					didRemap = true;
				}
			}
			else {//flowtype = 1
				if(unsteadyFlowCache) {
					myFlowParams->mapColors(unsteadyFlowCache,timeStep, minFrame, myGLWindow->getActiveRegionParams());
					//First and last age can change with flow graphics.
					firstDisplayAge = myFlowParams->getFirstDisplayFrame();
					lastDisplayAge = myFlowParams->getLastDisplayFrame();
					didRemap = true;
				}
			}
		}
	}
	if (!wasConstColors && constColors) didRemap = true;
	//OK, now render the cache.  The rgba's were rebuilt too.

	/* Decide if the display list needs to be re-compiled. */
	newType = lastShapeType != myFlowParams->getShapeType();
	dirtyDL = (dirtyDL || !useDisplayLists || didRemap || didRebuild || timeStep != lastTimeStep || newType ); 
	useDisplayLists = myFlowParams->usingDisplayLists();

	if (useDisplayLists && dirtyDL) {
		glNewList(flowDisplayList, GL_COMPILE);
	}
	
	if (dirtyDL || !useDisplayLists) {
		renderFlowData(constColors, currentFrameNum);
	}
	
	if (useDisplayLists) {
		if (dirtyDL) {
			glEndList();
			dirtyDL = false;
		}
		glCallList(flowDisplayList);
	}

	//Capture the flow, if capture is on,  
	//Ignore whether the current flow has previously been captured.
	if (myGLWindow->isCapturingFlow()){
		//Capture only steady or fla flow.
		// FLA only captures seeds.
		if (flowType != 1){
			captureFlow(currentFrameNum);
		}
	}
		
	if (didRemap) {
		setFlowMapClean(timeStep);
	}
	if (didRebuild){
		setFlowDataClean(timeStep);
		setFlowMapClean(timeStep);
		myGLWindow->setRenderNew();
	}
	
	lastShapeType = myFlowParams->getShapeType();
	wasConstColors = constColors;
}
//New version of rendering, uses FlowLineData, used on both steady and unsteady flow.
void FlowRenderer::
renderFlowData(bool constColors, int currentFrameNum){
	RegionParams* myRegionParams = myGLWindow->getActiveRegionParams();
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;

	int myFlowType = myFlowParams->getFlowType();
	FlowLineData* flowLineData = (myFlowType != 1)?
		(steadyFlowCache? steadyFlowCache[currentFrameNum]: 0):
		unsteadyFlowCache;

	//Don't render anything if no data
	if (flowLineData == 0) return;

	calcPeriodicExtents();
	int mxPoints = flowLineData->getMaxPoints();
	
	GLdouble topPlane[] = {0., -1., 0., 1.};
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};
	GLdouble leftPlane[] = {1., 0., 0., 0.};
	GLdouble botPlane[] = {0., 1., 0., 0.};
	GLdouble frontPlane[] = {0., 0., -1., 1.};//z largest
	GLdouble backPlane[] = {0., 0., 1., 0.};

	//Make the depth buffer writable
	glDepthMask(GL_TRUE);
	//and readable
	glEnable(GL_DEPTH_TEST);
	//Prepare for alpha values:
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	//Set up lighting, if we are rendering tubes or lines:
	ViewpointParams* vpParams =  myGLWindow->getActiveViewpointParams();
	int nLights = vpParams->getNumLights();
	if (myFlowParams->getShapeType() == 1 || nLights == 0) {
		nLights = 0; //Points are unlit
		glDisable(GL_LIGHTING);
	}
	else {
		glShadeModel(GL_SMOOTH);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, constFlowColor);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());
		//All the geometry will get a white specular color:
		float specColor[4];
		specColor[0]=specColor[1]=specColor[2]=0.8f;
		specColor[3] = 1.f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
		glEnable(GL_LIGHTING);
		glEnable(GL_COLOR_MATERIAL);
	}
	
	//Apply a coord transform that moves the full region to the unit cube.
	
	glPushMatrix();
	
	//scale:
	float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
	glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);

	//translate to put origin at corner:
	float* transVec = ViewpointParams::getMinStretchedCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);
	
	//Set up clipping planes
	const float* scales = DataStatus::getInstance()->getStretchFactors();
	const float* regExts = myRegionParams->getRegionExtents(currentFrameNum);
	topPlane[3] = regExts[4]*scales[1];
	botPlane[3] = -regExts[1]*scales[1];
	leftPlane[3] = -regExts[0]*scales[0];
	rightPlane[3] = regExts[3]*scales[0];
	frontPlane[3] = regExts[5]*scales[2];
	backPlane[3] = -regExts[2]*scales[2];
	
	glClipPlane(GL_CLIP_PLANE0, topPlane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, rightPlane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, botPlane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, leftPlane);
	glEnable(GL_CLIP_PLANE3);
	glClipPlane(GL_CLIP_PLANE4, frontPlane);
	glEnable(GL_CLIP_PLANE4);
	glClipPlane(GL_CLIP_PLANE5, backPlane);
	glEnable(GL_CLIP_PLANE5);

	
	float diam = myFlowParams->getShapeDiameter();
	//Don't allow zero diameter, it causes OpenGL error code 1281
	if (diam < 1.e-10) diam = 1.e-10f;

	//Set up size constants:
	//voxelSize is actually the max of the sides of the voxel in user coords,
	//At full resolution
	const float* fullExtent = DataStatus::getInstance()->getStretchedExtents();
	const size_t* fullDims = DataStatus::getInstance()->getFullDataSize();
	voxelSize = Max((fullExtent[5]-fullExtent[2])/fullDims[2],
		Max((fullExtent[4]-fullExtent[1])/fullDims[1], (fullExtent[3]-fullExtent[0])/fullDims[0]));
		
	
	stationaryRadius = 0.5f*voxelSize*myFlowParams->getDiamondDiameter();
	float userRadius = 0.5f*diam*voxelSize;
	arrowHeadRadius = (myFlowParams->getArrowDiameter())*userRadius;

	
	//If we are doing unsteady flow, handle setup differently:
	if (myFlowType != 1){
		if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
				
			if (diam < 0.05f){//Render as lines, not cylinders
				renderCurves(flowLineData, diam, (nLights>0), 0, mxPoints-1, constColors);
			
			} else { //render as cylinders
				//Determine cylinder radius in actual coords.
				//One voxel is (full region size)/(region array size)
				
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				//Render all tubes
				//Note that lastDisplayFrame is maxPoints -1
				//and firstDisplayFrame is 0
				//
				renderTubes(flowLineData,userRadius, (nLights > 0), 0, mxPoints-1, constColors);
				
			}
		} else if (myFlowParams->getShapeType() == 1 ){ //rendering points 
			//just convert the flow data to a set of points..
			renderPoints(flowLineData,diam, 0, mxPoints-1, constColors);
		} else { //render arrows
			
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			//Render arrows
			//Note that lastDisplayFrame is maxPoints -1
			//and firstDisplayFrame is 0
			//
			renderArrows(flowLineData,userRadius, (nLights > 0), 0, mxPoints-1, constColors);
		}

	} else { //unsteady flow:
		//Determine first and last seeding that is visible.
		//The portion of a flow that is visible is that portion whose age
		//goes from currentFrame - firstDisplayAge to currentFrame + lastDisplayAge. 

		
		//In this case we have a pathLineData
		PathLineData* pData = (PathLineData*)flowLineData;

		int firstGeom = pData->getIndexFromTimestep(currentFrameNum+firstDisplayAge);
		int lastGeom = pData->getIndexFromTimestep(currentFrameNum+lastDisplayAge);
		
		
		//Now do the rendering of this interval:
		if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
				
			if (diam < .05f){//Render as lines, not cylinders
				renderCurves(flowLineData, diam, (nLights>0), firstGeom, lastGeom,
					constColors);
			
			} else { //render as cylinders
				
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				//Render all tubes
				renderTubes(flowLineData, userRadius, (nLights > 0), firstGeom, lastGeom,
						constColors);
				
			}
		} else if(myFlowParams->getShapeType() == 1) { //rendering points 
			//just convert the flow data to a set of points..
			renderPoints(flowLineData, diam, firstGeom, lastGeom,
					constColors);
		} else { //rendering arrows:
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			
			renderArrows(flowLineData,userRadius, (nLights > 0), firstGeom, lastGeom,
					constColors);
		}
	} //End unsteady flow 
	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
	glDisable(GL_COLOR_MATERIAL);
	glPopMatrix();
	printOpenGLError();
	if (currentFrameNum != lastTimeStep){
		myGLWindow->setRenderNew();
		lastTimeStep = currentFrameNum;
	}

}


/*!
  Set up the OpenGL rendering state, and define display list
*/

void FlowRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	//myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	setRegionValid(true);
	initialized = true;
}
/****************************************************************************
 * 
 *  Following rendering loops are to render points, lines, tubes, or arrows
 *  Based on a sequence of points, possibly with flags.
 *  These loops must deal with the following cases
 *  1.  Sequence starts with END_FLOW_FLAG.  Render nothing
 *  2.  Sequence starts with STATIONARY_FLAG and has no valid points.
 *		The next element in the sequence is STATIONARY_FLAG (if any).
 *		Render nothing.
 *  3.  Otherwise, sequence contains a sequence of 1 or more valid points.
 *      These points can be preceded by:
 *		A.  Nothing
 *		B.  Zero or more IGNORE FLAGS, followed by zero or one STATIONARY_FLAG
 *		C.  Zero or one STATIONARY_FLAG
 *		These points can be followed by:
 *		A.  Nothing
 *		B.  Zero or one STATIONARY_FLAG
 *		C.  Zero or one END_FLOW_FLAG
 *
 *  The rendering loops must deal with these correctly; i.e.
 *  There may be a stationary symbol rendered at the last coord on either end.
 *  Otherwise the sequence of points results in the appropriate geometric entities.
 *
 ********************************************************************************/
//  Issue OpenGL calls for point list

//Issue OpenGL calls to draw a cylinder with orthogonal ends from one point to another.
//color indexed by startIndex and startIndex+1
//Orthogonal frame at first point is (dirVec, UVec, bVec)
//U and B are orthog to direction of cylinder, determine plane for 6 points
//
void FlowRenderer::drawArrow(bool isLit, float* firstColor, float* startPoint, float *endPoint, float* dirVec, float* bVec, float* uVec, float radius, bool constMap) {

	//Parameters that control arrow head:
	float nextPoint[3];
	float vertexPoint[3];
	float headCenter[3];
	
	//Calculate nextPoint and vertexPoint, for arrowhead
	for (int i = 0; i< 3; i++){
		nextPoint[i] = (1. - ARROW_LENGTH_FACTOR)*startPoint[i]+ ARROW_LENGTH_FACTOR*endPoint[i];
		vertexPoint[i] = nextPoint[i] + dirVec[i]*radius;
		headCenter[i] = nextPoint[i] - dirVec[i]*(arrowHeadRadius-radius); 
	}
	// add two rings to vertex array, one for each end of the new tube
	makeRing(vertexArray, &curVaIndex, firstColor+4, startPoint, bVec, uVec, radius, constMap);
	makeRing(vertexArray, &curVaIndex, firstColor+4, nextPoint, bVec, uVec, radius, constMap);

	// copy arrow's point to vertex array
	vcopy(dirVec, vertexArray[curVaIndex2].normal);
	vcopy(vertexPoint, vertexArray[curVaIndex2].vertex);
	for (int j=0; j<4 && (!constMap); j++ ) vertexArray[curVaIndex2].color[j]= firstColor[4+j];
	curVaIndex2++;

	// add outer ring of arrowhead to vertex array
	makeRing(vertexArray, &curVaIndex2, firstColor+4, nextPoint, bVec, uVec, arrowHeadRadius, constMap);
}

//Issue OpenGL calls to draw a cylinder from one point to another.
//center points indexed by startIndex and startIndex+1
//Orthogonal frame at first point is (dirVec, UVec, bVec)
//U and B are orthog to direction of cylinder, determine plane for 6 points
//Note that this has the side-effect of evaluating currentNormal and currentVertex,
//Uses existing values of prevNormal, PrevVertex
//

void FlowRenderer::drawTube(bool isLit, float* secondColor, float startPoint[3], float endPoint[3], float* currentB, float* currentU, float radius, bool constMap,
							float* prevNormal, float* prevVertex, float* currentNormal, float* currentVertex) {

	// just add a ring around this point to the vertex array, index array will handle the rest
	makeRing(vertexArray, &curVaIndex, secondColor, endPoint, currentB, currentU, radius, constMap);
}

void FlowRenderer::makeRing(flowTubeVertexData* storage, unsigned int* offset, float* color_in, float* point_in, float* B_in, float* U_in, float radius_in, bool constMap)
{

	//Constants are needed for cosines and sines, at 60 degree intervals
	const float sines[6] = {0.f, sqrt(3.)/2., sqrt(3.)/2., 0.f, -sqrt(3.)/2., -sqrt(3.)/2.};
	const float coses[6] = {1.f, 0.5, -0.5, -1., -.5, 0.5};
	float tmpVec[3],tmpVec2[3];

	//calculate 6 points in plane orthog to currentA, in plane of endPoint:
	for (int i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		vmult(U_in, coses[i], tmpVec);
		vmult(B_in, sines[i], tmpVec2);

		vadd(tmpVec, tmpVec2, storage[*offset].normal);
		vmult(storage[*offset].normal,  radius_in, storage[*offset].vertex);
		vadd(storage[*offset].vertex, point_in, storage[*offset].vertex);

		if (!constMap) {
			storage[*offset].color[0]= color_in[0];	//r
			storage[*offset].color[1]= color_in[1];	//g
			storage[*offset].color[2]= color_in[2];	//b
			storage[*offset].color[3]= color_in[3];	//a
		}
				
		/* increment vertexarray position */
		(*offset)++;
	}
}

//Render a symbol for stationary flowline (octahedron?)
void FlowRenderer::renderStationary(float* point){
	if (stationaryRadius <= 0.f) return;
	const float stationaryColor[4] = {.5f,.5f,.5f,1.f};
	//Normals for each face
	float normalVecs[24] = {
		0.5f,.5f,.707f,
		-0.5f,.5f,.707f,
		-0.5f,-.5f,.707f,
		0.5f,-.5f,.707f,
		0.5f,.5f,-.707f,
		-0.5f,.5f,-.707f,
		-0.5f,-.5f,-.707f,
		0.5f,-.5f,-.707f };
		


	glColor4fv(stationaryColor);
	glBegin(GL_TRIANGLES);
	glNormal3fv(normalVecs);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glNormal3fv(normalVecs+3);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	
	glNormal3fv(normalVecs+6);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glNormal3fv(normalVecs+9);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	
	glNormal3fv(normalVecs+12);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	glNormal3fv(normalVecs+15);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glNormal3fv(normalVecs+18);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	glNormal3fv(normalVecs+21);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glEnd();
	
	/* set back to constant color, in case we've interrupted a set of const-color primitives */
	glColor4fv(constFlowColor);
}
//find out if it's necessary to refresh:
bool FlowRenderer::needsRefresh(FlowParams* fParams, int timeStep) {
	if (fParams->getFlowType() == 1) {
		return unsteadyNeedsRefreshFlag;
	} else {
		return (fParams->refreshIsAuto() || !needRefreshFlag || needRefreshFlag[timeStep]);
	}
}
	
//Set the flow mapping clean.  
void FlowRenderer::setFlowDataClean(int timeStep){
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	allDataDirtyFlag = false;
	if (myFlowParams->getFlowType() == 1){
		unsteadyNeedsRefreshFlag = false;
		interruptFlag = true;
	}
	else {
		flowDataDirty[timeStep] = false;
		needRefreshFlag[timeStep] = false;
	}
}
//Set the flow mapping clean.  
void FlowRenderer::setFlowMapClean(int timeStep){
	allFlowMapDirtyFlag = false;
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	if (myFlowParams->getFlowType() != 1)
		flowMapDirty[timeStep] = false;
}

void FlowRenderer::setDataDirty(bool doInterrupt)
{
	interruptFlag = doInterrupt;
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	setRegionValid(true);  // reset this bit so we will try to render again...
	//set all the dirty flags for all the frames
	//set the needRefresh flags too if autoRefresh is on.
	allDataDirtyFlag = true;
	allFlowMapDirtyFlag = true;
	bool doRefresh = myFlowParams->refreshIsAuto();
	if (doRefresh) unsteadyNeedsRefreshFlag = true;
	
	for (int i = 0; i<numFrames; i++){
		flowDataDirty[i] = true;
		flowMapDirty[i] = true;
		if (doRefresh) needRefreshFlag[i] = true; 
	}
}
void FlowRenderer::setDisplayListDirty() 
{
	dirtyDL = true;
}
void FlowRenderer::setGraphicsDirty()
{
	allFlowMapDirtyFlag = true;
	for (int i = 0; i< numFrames; i++){
		flowMapDirty[i] = true;
	}
	dirtyDL = true;
}
bool FlowRenderer::
flowDataIsDirty(int timeStep){
	if (allDataDirtyFlag) return true;
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	if (myFlowParams->getFlowType() != 1)
		return ((flowDataDirty != 0) && flowDataDirty[timeStep]);
	else return false; //The allDataDirty flag indicates the unsteady data is dirty
}
bool FlowRenderer::
flowMapIsDirty(int timeStep){
	if (allFlowMapDirtyFlag) return true;
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	if (myFlowParams->getFlowType() != 1)
		return (flowMapDirty && flowMapDirty[timeStep]);
	else return false;
}

//Rebuild the data cache (full rebuild or partial)
//RGBA mapping data 
//Return false on failure.
//If the 
bool FlowRenderer::rebuildFlowData(int timeStep){
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	RegionParams* rParams = myGLWindow->getActiveRegionParams();
	
	//If we are using a rake, force it to lie (slightly) within the data extents
	//Check it out:
	if (!myFlowParams->validateSettings(timeStep)) {
		//If it's not OK, we turn off autoRefresh.  
		//Also turn off all the needsRefreshflags, so we won't try to render.
		setAllNeedRefresh(false);
		return false;
	}
	myFlowParams->setStopFlag(false);
	int flowType = myFlowParams->getFlowType();  //steady, unsteady, and field line advect

	//Check that we have a large-enough cache.  
	
	
	int numRefs = myFlowParams->getNumRefinements();
	int numMBs = RegionParams::getMBStorageNeeded(rParams->getRegionMin(timeStep), rParams->getRegionMax(timeStep), numRefs);
	
	//3 variables are needed for
	//steady integration, 6 are needed for unsteady integration.
	if (flowType == 0) numMBs *= 3; else numMBs *= 6;
	int cacheSize = (int)DataStatus::getInstance()->getCacheMB();
	if (numMBs > (int)(0.75*cacheSize)){
		MyBase::SetErrMsg(VAPOR_ERROR_DATA_TOO_BIG, "Current cache size is too \nsmall for flow integration at current region and resolution.\n%s\n%s",
			"Lower the refinement level, \nreduce the region size, \nor increase the cache size.",
			"autoRefresh has been disabled");
		myFlowParams->setAutoRefresh(false);
		setAllNeedRefresh(false);
		return false;
	}
	//Establish parameters that will be saved with the cache:
	
	bool doRake = myFlowParams->rakeEnabled();
	doList = !doRake;
	
	//For unsteady flow, the min/max frame interval can be narrowed by the
	//timesampling interval.  It doesn't make any sense to extrapolate outside
	//that interval.  Likewise, the seeding cannot happen before the start of the
	//timesampling interval. 

	if (flowType != 0){
		//In unsteady flow or in field line advection, need to allow timesteps that are not in animation parameters.
		minFrame = (int)DataStatus::getInstance()->getMinTimestep();
		maxFrame = (int)DataStatus::getInstance()->getMaxTimestep();
		if (minFrame < myFlowParams->getFirstSampleTimestep())
			minFrame = myFlowParams->getFirstSampleTimestep();
		if (maxFrame > myFlowParams->getLastSampleTimestep())
			maxFrame = myFlowParams->getLastSampleTimestep();
	} else { //steady flow, only use animation times
		minFrame = myGLWindow->getActiveAnimationParams()->getStartFrameNumber();
		maxFrame = myGLWindow->getActiveAnimationParams()->getEndFrameNumber();
	}

	maxPoints = myFlowParams->calcMaxPoints();
	
	//Check if we are just doing graphics (not reintegrating flow)
	//that occurs if the map bit is dirty, but there's no need to rebuild data.
	bool graphicsOnly = (flowMapIsDirty(timeStep) && 
		!(flowDataIsDirty(timeStep) && needsRefresh(myFlowParams,timeStep)) );
	bool constColors = ((myFlowParams->getColorMapEntityIndex() + myFlowParams->getOpacMapEntityIndex())== 0);
	
	//Clean the dirty parts of cache, unless this is just a rebuild of the graphics:
	bool OK = true;
	if (!graphicsOnly){
		if (allFlowDataIsDirty()){
			if (steadyFlowCache[timeStep]){
				delete steadyFlowCache[timeStep];
				steadyFlowCache[timeStep] = 0;
			}
			if (unsteadyFlowCache) {
				delete unsteadyFlowCache;
				unsteadyFlowCache = 0;
			}
		} else { //Just delete the stuff for this one timestep:
			if (steadyFlowCache[timeStep]){
				delete steadyFlowCache[timeStep];
				steadyFlowCache[timeStep] = 0;
			}
		}
	}
	
	//Set up parameters as needed based on flow type:
	if (flowType == 1){
		//Set values for special parameters used only for unsteady flow:
		seedIncrement = myFlowParams->getSeedTimeIncrement();
		if (seedIncrement < 1) seedIncrement = 1;
		startSeed = myFlowParams->getSeedTimeStart();
		endSeed = myFlowParams->getSeedTimeEnd();
		firstDisplayAge = myFlowParams->getFirstDisplayFrame();
		lastDisplayAge = myFlowParams->getLastDisplayFrame();
		objectsPerTimestep = myFlowParams->getObjectsPerTimestep();
	} 
	if (!myFlowLib){
		//create a new flow lib:
		DataMgr* dataMgr = (DataMgr*)DataStatus::getInstance()->getDataMgr();
		assert(dataMgr);
		myFlowLib = new VaporFlow(dataMgr);
	}
	//Construct the caches if needed:
	if (!graphicsOnly) {
		OK = false;
		int prevStep, nextStep;
		int unsteadyFlowDir;
		int numTimestepsToRender;
		switch (flowType) {
			case (0): 
				steadyFlowCache[timeStep] = myFlowParams->regenerateSteadyFieldLines(myFlowLib, 0, 0, timeStep, minFrame, rParams, false);
				OK = (steadyFlowCache[timeStep] != 0);
				break;
			case (1):
				
				//Create a PathLineData with all the seeds in it
				unsteadyFlowCache = (PathLineData*)myFlowParams->setupUnsteadyStartData(myFlowLib, minFrame, maxFrame, rParams);
				if (!unsteadyFlowCache) {
					if (myFlowParams->rakeEnabled() || myFlowParams->getNumListSeedPoints() > 0){
						if(myFlowParams->refreshIsAuto())setAllBypass(true);
						MyBase::SetErrMsg(VAPOR_ERROR_SEEDS, 
							"No seeds for unsteady flow from time steps %d to %d .\n%s",
							minFrame, maxFrame,
							"Ensure sample times are consistent with seed times\n");
					}
					
					return false;
				}
				QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
				unsteadyFlowDir = myFlowParams->getUnsteadyDirection();
				//Then incrementally advect from start to end, via sampled timesteps.  
				prevStep = myFlowParams->getUnsteadyTimestepSample(0,minFrame, maxFrame, unsteadyFlowDir);
				numTimestepsToRender = 0;
				for (int i = 1;; i++) {
					nextStep = myFlowParams->getUnsteadyTimestepSample(i,minFrame, maxFrame,unsteadyFlowDir);
					if (nextStep < 0 || prevStep < 0) break;
					assert(nextStep != prevStep);
					//Check if there's nothing to advect at this timestep
					//If so just bypass it:
					if(unsteadyFlowCache->getNumLinesAtTime(prevStep) == 0) {
						prevStep = nextStep;
						continue;
					}
					numTimestepsToRender++;
					//allow users to interrupt at this point:
					DataStatus* ds = DataStatus::getInstance();
					if(interruptFlag) ds->getApp()->processEvents();
					if (myFlowParams->getStopFlag()){
						delete unsteadyFlowCache;
						unsteadyFlowCache = 0;
						setAllNeedRefresh(false);
						QApplication::restoreOverrideCursor();
						MyBase::SetErrMsg(VAPOR_WARNING_FLOW_STOP,"Flow Integration Terminated by user");
						return false;
					}

					//The following is time consuming...
					OK = myFlowLib->ExtendPathLines(unsteadyFlowCache, prevStep, nextStep, false);
					if (!OK) {
						delete unsteadyFlowCache;
						unsteadyFlowCache = 0;
						QApplication::restoreOverrideCursor();
						return false;
					}
					prevStep = nextStep;
				}
				QApplication::restoreOverrideCursor();
				if (numTimestepsToRender < 1){
					if(myFlowParams->refreshIsAuto())setAllBypass(true);
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW, "Insufficient valid timesteps for unsteady integration");
				} else {
					MyBase::SetDiagMsg("Integrated %d timesteps", numTimestepsToRender);
				}
				//Note: the flow lines need to be rescaled here, before colors are mapped,
				//because color mapping can use the values in the flow lines if a variable is
				//being mapped to colors.
				unsteadyFlowCache->scaleLines(DataStatus::getInstance()->getStretchFactors());
				if(!constColors) myFlowParams->mapColors(unsteadyFlowCache, timeStep, minFrame, rParams);
				break;
			case (2):
				{
					int seedTimeStart = myFlowParams->getSeedTimeStart();
					unsteadyFlowDir = (seedTimeStart > timeStep) ? -1 : 1;
					QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
					//Check if we need to do a partial or full rebuild.
					if (allDataDirtyFlag){
						if (unsteadyFlowCache) { delete unsteadyFlowCache; unsteadyFlowCache = 0;}
						for (int i = 0; i< numFrames; i++){
							if (steadyFlowCache[i]) {
								delete steadyFlowCache[i];
								steadyFlowCache[i] = 0;
							}
						}
						if (!myFlowParams->getFLAAdvectBeforePrioritize()){
							
							//Create a PathLineData with only the initial timestep of seeds in it:
							unsteadyFlowCache = (PathLineData*)myFlowParams->setupUnsteadyStartData(myFlowLib, minFrame, maxFrame, rParams);
							//And build the first time step:
							allDataDirtyFlag = false;
							
							//Perform steady integration at first time step, reprioritizing
							//the seeds in the unsteadycache.  This should only be needed the first time.
							steadyFlowCache[seedTimeStart] = myFlowParams->regenerateSteadyFieldLines(myFlowLib, 0, unsteadyFlowCache, seedTimeStart, minFrame, rParams, true);
							if(!steadyFlowCache[seedTimeStart]){
								if(myFlowParams->refreshIsAuto())setAllBypass(true);
								MyBase::SetErrMsg(VAPOR_ERROR_INTEGRATION,"Unable to perform steady integration at timestep %d", seedTimeStart);
								QApplication::restoreOverrideCursor();
								return false;
							}
							flowDataDirty[seedTimeStart] = false;
							//It's possible that we changed the flow direction, so tell the flow params:
							
							myFlowParams->setUnsteadyDirection(unsteadyFlowDir);
							unsteadyFlowCache->setPathDirection(unsteadyFlowDir);
						} else { //set up first time step for multi-advect.
							steadyFlowCache[seedTimeStart] = myFlowParams->setupUnsteadyStartData(myFlowLib, minFrame, maxFrame, rParams);
							allDataDirtyFlag = false;
						}
					}
					
					// Stop here if the start time step is current time step
					if (timeStep == seedTimeStart) {
						QApplication::restoreOverrideCursor();
						return true;
					}
					//Otherwise set up for iterating from seedTimeStart to the
					//current time.
					//Find the seed time step in the sample time steps
					//Start at the seed point and go forwards or backwards to current time:
					//Iterate to find the first sample step that coincides with the first (and only)
					//seed time.
					int startSampleNum;
					
					for (int i = 0;; i++){
						prevStep = myFlowParams->getUnsteadyTimestepSample(i, minFrame, maxFrame, unsteadyFlowDir);
						if (prevStep < 0) {
							if(myFlowParams->refreshIsAuto())setAllBypass(true);
							MyBase::SetErrMsg(VAPOR_ERROR_FLOW,"Seed time is not a sample time \nfor field line advection");
							return false;
						}
						if (prevStep == seedTimeStart) {
							startSampleNum = i;
							break;
						}
					}
					//At the completion of the following, the steady flow will be calculated for
					//every sample time between the seedTimeStart up to and including the current time.
					//This can be done in either forward or reverse direction from the seedTimeStart.
					//Repeatedly, starting with the first seed time, which must also be a sample time,
					//iterate over sample times:
					//0.  Check if this sample time has a valid steady flow.
					//If so, continue (to the next sample time)
					//1.  Perform steady integration at this sample time, prioritizing seeds.
					//  (stop if we are at or beyond the current time step)
					//2.  Perform unsteady integration to the next sample time. 
					//

					//Note that we assume the current time step is a sample time.  If it's not
					//Then we won't build the steady flow there.
					
					for (int i = startSampleNum;; i++){
						prevStep = myFlowParams->getUnsteadyTimestepSample(i, minFrame, maxFrame,unsteadyFlowDir);
						nextStep = myFlowParams->getUnsteadyTimestepSample(i+1, minFrame, maxFrame,unsteadyFlowDir);
						if (timeStep != prevStep) {
							if (prevStep < 0 || nextStep < 0) { 
								// past the end...
								MyBase::SetErrMsg(VAPOR_WARNING_FLOW,"Field Line Advection\nCannot advect to unsampled timestep %d", timeStep);
								if(myFlowParams->refreshIsAuto())setAllBypass(true);
								QApplication::restoreOverrideCursor();
								return false;
							}
							//Check if prevStep and nextStep are valid, if so, skip:
							if (!flowDataDirty[prevStep] && !flowDataDirty[nextStep]) continue;
						}
						//allow users to interrupt at this point:
						DataStatus* ds = DataStatus::getInstance();
						ds->getApp()->processEvents();
						if (myFlowParams->getStopFlag()){
							
							setAllNeedRefresh(false);
							QApplication::restoreOverrideCursor();
							MyBase::SetErrMsg(VAPOR_WARNING_FLOW_STOP,"Flow Integration Interrupted");
							return false;
						}
						//At this point we know we need to advect from prevStep to nextStep
						//This is either performed by advecting before or after prioritization

						if (myFlowParams->getFLAAdvectBeforePrioritize()){
							OK = myFlowParams->multiAdvectFieldLines(myFlowLib, steadyFlowCache, prevStep, nextStep,minFrame,rParams);
						} else {
							OK = myFlowParams->singleAdvectFieldLines(myFlowLib, steadyFlowCache,unsteadyFlowCache,prevStep, nextStep, minFrame, rParams);
						}
						if (OK) { //Clear all the dirty flags
							int increm = (nextStep > prevStep) ? 1 : -1;
							for (int i = prevStep;; i += increm){
								flowDataDirty[i] = false;
								if (i == nextStep) break;
							}
						} 
						
						//Stop here if the current timestep is done:
						if (unsteadyFlowDir > 0 && timeStep <= nextStep) { OK = true; break;}
						if (unsteadyFlowDir < 0 && timeStep >= nextStep) { OK = true; break;}
					}
					QApplication::restoreOverrideCursor();
				}
				break;  //End of case(2)
			default: assert(0); break;
		} //End of switch()
	}//End of cache reconstruction
	//Now we only rebuild the rgba's if we didn't build the flow lines
	else if (!constColors) {
		OK = true;
		if (flowType != 1)
			myFlowParams->mapColors(steadyFlowCache[timeStep],timeStep, minFrame, rParams);
		else 
			myFlowParams->mapColors(unsteadyFlowCache,timeStep, minFrame, rParams);
	}
	return OK;
}

void FlowRenderer::
setAllNeedRefresh(bool value){
	int mxframe = DataStatus::getInstance()->getNumTimesteps();
	for (int i = 0; i< mxframe; i++){
		needRefreshFlag[i] = value;
	}
	unsteadyNeedsRefreshFlag = value;
}
//Map periodic coords into data extents.
//This is called each time a new point is rendered along a stream line.  oldcycle is initially (0,0,0)
//establishing that the first point is rendered inside the base cycle (even if it is outside, as with a pre-point).
//If the first point is out, then the second point must be in??? so oldcycle = 0,0,0 is also passed in with the
//second point.
//If the first point is inside, the second point will pass in oldcycle from the first call (which must be (0,0,0)).
//In any case, this function should not be called for the first point of the streamline, but it must be called for the
//second and subsequent points.  
//Whenever a streamline exits the region, this returns true, indicating that the current streamline must be stopped
//and then the next streamline is started by calling this function again.
//
bool FlowRenderer::mapPeriodicCycle(float origCoord[3], float mappedCoord[3], int oldcycle[3], int newcycle[3]){
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	bool changed = false;
	for (int i = 0; i<3; i++){
		mappedCoord[i] = origCoord[i];
		newcycle[i] = oldcycle[i];
		if (myFlowParams->getPeriodicDim(i)){
			//correct according to oldcycle:
			mappedCoord[i] -= (((float)oldcycle[i])*(periodicExtents[i+3]-periodicExtents[i]));
			//Then make newcycle for additional correction:
			float mapCoord = mappedCoord[i];
			while (mapCoord < periodicExtents[i]) {
				mapCoord += (periodicExtents[i+3]-periodicExtents[i]); 
				newcycle[i]--; 
				changed=true;
			}
			while (mapCoord > periodicExtents[i+3]) {
				mapCoord -= (periodicExtents[i+3]-periodicExtents[i]); 
				newcycle[i]++; 
				changed=true;
			}
		}
	}
	return changed;
}
void FlowRenderer::calcPeriodicExtents() {
	//The periodic extents go slightly further than the extents, going to the end of the last voxel.  
	// This is because the user specification of extents is the difference between the first and last voxel
	// position in the data.  The period is actually one voxel beyond the end of the data, since that point
	// is not repeated.
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	const float* extents = DataStatus::getInstance()->getStretchedExtents();
	size_t dims[3];
	DataStatus::getInstance()->getDataMgr()->GetDim(dims, -1);
	for (int i = 0; i<3; i++){
		periodicExtents[i] = extents[i];
		if (myFlowParams->getPeriodicDim(i)){
			float dim = (float)dims[i];
			periodicExtents[i+3] = extents[i] + (extents[i+3]-extents[i])*(dim/(dim-1.f));
		}
		//With nonperiodic data, just use the extents (close enough, prevents surprises!)
		else periodicExtents[i+3] = extents[i+3];
	}

}

//  Issue OpenGL calls for point list
// startIndex is injection num.
void FlowRenderer::
renderPoints(FlowLineData* flowLineData, float radius, int firstAge, int lastAge, bool constMap){
	FlowParams* myFlowParams = (FlowParams*)currentRenderParams;
	float mappedPoint[3];
	//just convert the flow data to a set of points..
	glPointSize(radius);
	glDisable(GL_LIGHTING);
	
	/* (re)allocate memory for Vertex Array */
	if (lastAge > curVaSize || newType) {
		delete[] vertexArray;
		vertexArray = new flowTubeVertexData[(lastAge+1)];
		curVaSize = lastAge;
	}
	
	/* enable client states for vertex arrays*/
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+7);
	if (!constMap){ 
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 10*sizeof(float), vertexArray);
	}
	
	for (int i = 0; i< flowLineData->getNumLines(); i++){
		curVaIndex = 0;
		bool firstPoint = true;
		bool stationaryStart = false;
		int firstIndex = Max(firstAge, flowLineData->getStartIndex(i));
		int lastIndex = Min(lastAge, flowLineData->getEndIndex(i));
		for (int j = firstIndex; j<= lastIndex; j++){
			float* point = flowLineData->getFlowPoint(i,j);
				
			if (*point == END_FLOW_FLAG) break;
			if ((*point) == IGNORE_FLAG) continue;
			
			//Check for an initial STATIONARY_FLAG
			if (firstPoint && (*point == STATIONARY_STREAM_FLAG)){
				stationaryStart = true;
				continue;
			}
			
			if (stationaryStart){
				if (*point == STATIONARY_STREAM_FLAG) break;
				renderStationary(point);
				stationaryStart = false;
			}
			if(firstPoint){
				firstPoint = false;
				if(constMap) glColor4fv(constFlowColor);
			}
			
			//qWarning("point is %f %f %f", *point, *(point+1), *(point+2));
			
			if (!constMap){
				float* rgba = flowLineData->getFlowRGBAs(i,j);
				for (int jj=0; jj<4; jj++ ) {
					vertexArray[curVaIndex].color[jj]= rgba[jj];
				}
			}

			//Use the last point for a stationary marker
			if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
				vcopy(point, vertexArray[curVaIndex].vertex);
				curVaIndex++;
				
				renderStationary(point);
				break;
			}
			myFlowParams->periodicMap(point, mappedPoint, false);
			vcopy(mappedPoint, vertexArray[curVaIndex].vertex);
			curVaIndex++;
		}	
		
		/* render VA for this point set*/
		glDrawArrays(GL_POINTS, 0, curVaIndex);

	}

	/* disable the other client states, so we don't accidentally interfere w/ other renderers */
	glDisableClientState(GL_VERTEX_ARRAY);
	if (!constMap) glDisableClientState(GL_COLOR_ARRAY);
	
}
//  Issue OpenGL calls for a set of lines associated with a number of seed points.
void FlowRenderer::
renderCurves(FlowLineData* flowLineData,float radius, bool isLit, int firstAge, int lastAge, bool constMap){
	float dirVec[3];
	float testVec[3];
	float normVec[3];
	float mappedPoint[3];
	
	int currentCycle[3]; 
	int newCycle[3];
	bool newcycle;
	if (firstAge >= lastAge) return;
	
	/* (re)allocate memory for Vertex Array - Allocate the same memory as needed 
	 for renderTubes, since they use the same type-number. */
	if (lastAge > curVaSize || newType) {
		delete[] vertexArray;
		vertexArray = new flowTubeVertexData[(lastAge+1)*6];
		
		curVaSize = lastAge;
		
		/* reallocate and rebuild index array */
		delete[] indexArray;
		indexArray = new unsigned int [lastAge*2*7];
		
		int ib=0;
		for (int ia=0; ia<curVaSize*7; ia++) {
			if ((ia+1) % 7 == 0) {
				indexArray[2*ia + 0] = (unsigned int) ib - 6;
				indexArray[2*ia + 1] = (unsigned int) ib;
			} else {
				indexArray[2*ia + 0] = (unsigned int) ib;
				indexArray[2*ia + 1] = (unsigned int) ib + 6;
				ib++;
			}
		}
	}
	
	/* enable client states for vertex arrays*/
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+7);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+4);
	
	if (!constMap){ 
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 10*sizeof(float), vertexArray);
	}
	
	glLineWidth(radius);
	if (!isLit) glDisable(GL_LIGHTING);
	
	//Get light direction vector of first light:
	float lightDir[3];
	ViewpointParams* vpParams = myGLWindow->getActiveViewpointParams();
	const float* worldLightDir = vpParams->getLightDirection(0);
	//Transform it by modelview matrix:
	vpParams->transform3Vector(worldLightDir,lightDir);
	for (int i = 0; i< flowLineData->getNumLines(); i++){
		//Assume seed point starts inside region:
		currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
		newcycle = false;
		if (constMap)  glColor4fv(constFlowColor);
		bool endGL = true;
		bool firstPoint = true;
		bool stationaryStart = false;
		curVaIndex = 0;
		
		int firstIndex = Max(firstAge, flowLineData->getStartIndex(i));
		int lastIndex = Min(lastAge, flowLineData->getEndIndex(i));
		for (int j = firstIndex; j<= lastIndex; j++){
			//For each point after first, calc vector from prev vector to this one
			//then calculate corresponding normal
			float* point = flowLineData->getFlowPoint(i,j);
			if (*point == END_FLOW_FLAG) break;
			if (*point == IGNORE_FLAG) continue;
			//Check for an initial STATIONARY_FLAG
			if (firstPoint && (*point == STATIONARY_STREAM_FLAG)){
				stationaryStart = true;
				continue;
			}
			if (stationaryStart){
				if (*point == STATIONARY_STREAM_FLAG) break;
				renderStationary(point);
				stationaryStart = false;
			}
			if(firstPoint){
				firstPoint = false;
				if(constMap) glColor4fv(constFlowColor);

				//Establish the first cycle, use it for first two points:
				bool modifiedCycle = mapPeriodicCycle(point, mappedPoint, currentCycle, newCycle);
				if (modifiedCycle) 
					newcycle = mapPeriodicCycle(point, mappedPoint, newCycle, currentCycle);
				
			}
			else {
				vsub(point, point-3, dirVec);
				float len = vdot(dirVec,dirVec);
				if (len == 0.f){//If 2nd is same as first, set default normal
					vset(dirVec, 0.f,0.f,1.f);
				} else {
					vscale(dirVec, 1.f/sqrt(len));
				} 
				//Project light direction vector orthogonal to dirvec:
				
				vmult(dirVec, vdot(dirVec,lightDir), testVec);
				vsub(lightDir, testVec, normVec);
				//Now normalize it:
				len = vdot(normVec,normVec);
				if (len == 0.f){//  0 projection, take normvec = 0,0,1
					vset(normVec, 0.,0.,1.);
				} 
				
				vcopy(normVec, vertexArray[curVaIndex].normal);
				//only call mapPeriodicCycle to translate, or detect leaving the region, after the first point
				newcycle = mapPeriodicCycle(point, mappedPoint, currentCycle, newCycle);
			}
			if (!constMap){
				float* rgba = flowLineData->getFlowRGBAs(i,j);
				for (int jj=0; jj<4; jj++ ) {
					vertexArray[curVaIndex].color[jj]= rgba[jj];
				}
			}
			
			if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
				vcopy(point, vertexArray[curVaIndex].vertex);
				curVaIndex++;
				renderStationary(mappedPoint);
				break;
			}
			
			vcopy(mappedPoint, vertexArray[curVaIndex].vertex);
			curVaIndex++;
			//Test if the last point ran off the end of the cycle. If so render it again in the new place.
			if (newcycle) {
				//end previous curve, start next one:
				newcycle = mapPeriodicCycle(point, mappedPoint, newCycle, currentCycle);
				assert(!newcycle);

				vcopy(normVec, vertexArray[curVaIndex].normal);
				vcopy(mappedPoint, vertexArray[curVaIndex].vertex);
				curVaIndex++;
			}
		}

		/* render VA for this point set*/
		glDrawArrays(GL_LINE_STRIP, 0, curVaIndex);
		
	}
	/* disable the other client states, so we don't accidentally interfere w/ other renderers */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	if (!constMap) glDisableClientState(GL_COLOR_ARRAY);
	
}
//  Issue OpenGL calls for a cylindrical (hexagonal cross-section) tube 
//  following the stream or Path line
//
void FlowRenderer::
renderTubes(FlowLineData* flowLineData, float radius, bool isLit, int firstAge, int lastAge,  bool constMap ){
	//Constants are needed for cosines and sines, at 60 degree intervals
	const float sines[6] = {0.f, sqrt(3.)/2., sqrt(3.)/2., 0.f, -sqrt(3.)/2., -sqrt(3.)/2.};
	const float coses[6] = {1.f, 0.5, -0.5, -1., -.5, 0.5};
	//Declare values used repeatedly (toggle between even and odd)
	//Tube will actually be hexagonal
	float evenVertex[18];
	float oddVertex[18];
	float evenNormal[18];
	float oddNormal[18];
	float evenU[3];//vector in plane of points, orthog to A
	float oddU[3];
	
	float evenN[3];//vector pointing in direction to next point
	float oddN[3];
	float evenA[3];//normalized average of previous and next N
	float oddA[3];
	float currentB[3];//Binormal, U cross A
	float finalA[3], startA[3];
	
	float len;
	float testVec[3];
	float testVec2[3];
	
	//Vectors to hold the start and end arrow coordinates.
	//They may be translated due to periodic boundary conditions.
	float startPoint[3], endPoint[3];
	int newCycle[3],currentCycle[3];
	bool newcycle;

	if (firstAge >= lastAge) return;

	/* (re)allocate memory for Vertex Array */
	if (lastAge > curVaSize || newType) {
		delete[] vertexArray;
		vertexArray = new flowTubeVertexData[(lastAge+1)*6];

		curVaSize = lastAge;

		/* reallocate and rebuild index array */
		delete[] indexArray;
		indexArray = new unsigned int [lastAge*2*7];

		int ib=0;
		for (int ia=0; ia<curVaSize*7; ia++) {
			/* every 7th index needs to repeat the first 2 verticies of the tube to close it */
			if ((ia+1) % 7 == 0) {
				indexArray[2*ia + 0] = (unsigned int) ib - 6;
				indexArray[2*ia + 1] = (unsigned int) ib;
			} else {
				indexArray[2*ia + 0] = (unsigned int) ib;
				indexArray[2*ia + 1] = (unsigned int) ib + 6;
				ib++;
			}
		}
	}

	/* enable client states for vertex arrays*/
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+7);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+4);

	if (!constMap){ 
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 10*sizeof(float), vertexArray);
	}

	for (int tubeNum = 0; tubeNum < flowLineData->getNumLines(); tubeNum++){
		//Skip the tube entirely if first point is end-flow 
		//This can occur in the middle of a Pathline, 
		//also with flowLineData after it's left the region
		
		int firstIndex = Max(firstAge, flowLineData->getStartIndex(tubeNum));
		int lastIndex = Min(lastAge, flowLineData->getEndIndex(tubeNum));
		if (firstIndex >= lastIndex) continue;
		float* point = flowLineData->getFlowPoint(tubeNum, firstIndex);
		if(*point == END_FLOW_FLAG) continue;
		//assert(*point != END_FLOW_FLAG);
		//Cycle through the points looking for a valid tubeStartIndex.
		//If the first one is at the end, we don't render anything
		//
		int tubeStartIndex;
		bool stationaryStart = false;
		curVaIndex = 0;
		// first loop is just to find a start for the tube:
		for (tubeStartIndex = firstIndex; tubeStartIndex < lastIndex; tubeStartIndex++){
		 
			point = flowLineData->getFlowPoint(tubeNum,tubeStartIndex);
			if (*point == IGNORE_FLAG) assert(0); //continue;  
			if (*point == STATIONARY_STREAM_FLAG) {
				//if Multiple STATIONARY_FLAGS, it's a bug...
				if (stationaryStart) {
					assert(0);
					stationaryStart = false;
					tubeStartIndex = lastIndex;
					break;
				}
				stationaryStart = true;
				continue;
			}
			//At this point we know it's a valid point
			assert (*point != END_FLOW_FLAG);
			break;
		}
		//check if we didn't find anything.  Possibly render a solitary
		//stationary point if we didn't find anything else.
		if (tubeStartIndex == lastIndex){
			if (stationaryStart) {
				point = flowLineData->getFlowPoint(tubeNum, tubeStartIndex);
				renderStationary(point);
			}
			break;
		}
		//Check if the first point was preceded by stationary flag:
		if (stationaryStart){
			point = flowLineData->getFlowPoint(tubeNum, tubeStartIndex);
			renderStationary(point);
		}
		assert(tubeStartIndex < lastIndex);

		//startTube( intput: tubeStartIndex, flowRGBAs constFlowColor, flowDataArray, 

		//output: point [continue, or break??], evenA, len, evenN, evenU

		//OK, we are at the start of a real tube.
		//Start the colors for the start of the tube 
		if (!constMap){
			float* rgba = flowLineData->getFlowRGBAs(tubeNum, tubeStartIndex);
			glColor4fv(rgba);
		} else {
			 glColor4fv(constFlowColor);
		}
		//Check if the second point is a stationary flag:
		if (*(flowLineData->getFlowPoint(tubeNum, (tubeStartIndex+1))) == STATIONARY_STREAM_FLAG) {
			//If so just render the stationary symbol, and we are done.
			float *point = flowLineData->getFlowPoint(tubeNum, (tubeStartIndex));
			renderStationary(point);
			continue;
		} else if (*(flowLineData->getFlowPoint(tubeNum, (tubeStartIndex+1))) == END_FLOW_FLAG) {//render nothing:
				//We could potentially have the second point be an end flow flag
				continue;
		} else {  //OK, now we can render a tube:
			
			
			//data point is three floats starting at data[3*tubeStartIndex]
			//evenA is the direction the line is pointing
			vsub(flowLineData->getFlowPoint(tubeNum, tubeStartIndex+1), flowLineData->getFlowPoint(tubeNum, tubeStartIndex), evenA);
			//Normalize evenA
			len = vdot(evenA,evenA);
			if (len == 0.f){//If 2nd is same as first set default normal
				vset(evenA, 0.f,0.f,1.f);
			} else {
				vscale(evenA, 1.f/sqrt(len));
			}
			//The first time, N is equal to A:
			vcopy(evenA, evenN);
			//Calculate evenU, orthogonal to evenA:
			vset(testVec, 1.,0.,0.);
			vcross(evenA, testVec, evenU);
			len = vdot(evenU,evenU);
			if (len == 0.f){
				vset(testVec, 0.,1.,0.);
				vcross(evenA, testVec, evenU);
				len = vdot(evenU, evenU);
				assert(len != 0.f);
			} 
			vscale( evenU, 1.f/sqrt(len));
			vcross(evenU, evenA, currentB);
			//set up initial even 6 vertices around point P = P(0)
			//These are P + 
			for (int i = 0; i<6; i++){
				vmult(evenU, coses[i], testVec);
				vmult(currentB, sines[i], testVec2);
				//Calc outward normal as a sideEffect..
				vadd(testVec, testVec2, evenNormal+3*i);
				vmult(evenNormal+3*i, radius, evenVertex+3*i);
				vadd(evenVertex+3*i, flowLineData->getFlowPoint(tubeNum, tubeStartIndex), evenVertex+3*i);
			}
			//copy first normal for endcap
			vcopy(evenA, startA);


			//Now render the cylinders:
			//loop over points, starting with no. 1.
			//toggle even and odd.
			float* currentN;
			float* currentU;
			float* currentA = 0;
			float* prevN;
			float* prevA;
			float* prevU;
			float* currentVertex = 0;
			float* currentNormal;
			float* prevVertex;
			float* prevNormal;
			

			//prepare to render the first cylinder in the tube.
			currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
			newcycle = false;

			bool currentIsEven = false;
			int tubeIndex;
			for (tubeIndex = tubeStartIndex+1; tubeIndex <= lastIndex; tubeIndex++){
				point = flowLineData->getFlowPoint(tubeNum, tubeIndex);
				if (*point == END_FLOW_FLAG || *point == STATIONARY_STREAM_FLAG) break;
				//Toggle the meaning of "current" and "prev"
				//First time thru current is odd
				if (currentIsEven) {
					currentN = evenN;
					prevN = oddN;
					currentA = evenA;
					prevA = oddA;
					currentU = evenU;
					prevU = oddU;
					currentVertex = evenVertex;
					prevVertex = oddVertex;
					currentNormal = evenNormal;
					prevNormal = oddNormal;
					currentIsEven = false;
				} else {
					currentN = oddN;
					prevN = evenN;
					currentA = oddA;
					prevA = evenA;
					currentU = oddU;
					prevU = evenU;
					currentVertex = oddVertex;
					prevVertex = evenVertex;
					currentNormal = oddNormal;
					prevNormal = evenNormal;
					currentIsEven = true;
				}
				
				//Calc currentN
				vsub(point, point-3, currentN);
				//Normalize currentN:
				len = vdot(currentN,currentN);
				if (len == 0.f){// keep previous normal
					vcopy(prevN,currentN);
				} else {
					vscale(currentN, 1.f/sqrt(len));
				}
				//Calc currentA, as sum (average) of prevN and currentN:
				vadd(prevN, currentN, currentA);
				//Normalize currentA
				len = vdot(currentA,currentA);
				if (len == 0.f){// keep previous normal
					vcopy(prevA,currentA);
				} else {
					vscale(currentA, 1.f/sqrt(len));
				}
				//Now get next U, by projecting previous U orthog to currentA:
				vmult(currentA, vdot(prevU,currentA), testVec);
				vsub(prevU, testVec, currentU);
				//Now normalize it:
				len = vdot(currentU, currentU);
				if (len == 0.f){
					//If U is in direction of A, the previous A should work
					vcopy(prevA, currentU);
					len = vdot(currentU, currentU);
					assert(len != 0.f);
				} 
				vscale(currentU, 1.f/sqrt(len));
				vcross(currentU, currentA, currentB);
				
				if(tubeIndex > tubeStartIndex+1) {
					//For each tube after the first, 
					// the new tube goes from previous endpoint to mapping of point
					//and map the endPoint in the same cycle as the startPoint:
					vcopy(endPoint, startPoint);
					newcycle = mapPeriodicCycle(point, endPoint, currentCycle, newCycle);
				} else { //first time through, startPoint is point-3, endPoint is point
					//Establish the first cycle, use it for first two points:
					bool modifiedCycle = mapPeriodicCycle(point-3, startPoint, currentCycle, newCycle);
					if (modifiedCycle) {
						//remap start point
						mapPeriodicCycle(point-3, startPoint, newCycle, currentCycle);
						//also translate the prevVertex by the difference we just applied to startPoint
						for (int j = 0; j< 6; j++){
							for (int k = 0; k<3; k++){
								prevVertex[3*j+k] += startPoint[k]-point[k-3];
							}
						}
					}
					newcycle = mapPeriodicCycle(point, endPoint, currentCycle, newCycle);
				}
				drawTube(isLit, flowLineData->getFlowRGBAs(tubeNum,tubeIndex), startPoint, endPoint, currentB, currentU, radius, constMap,
					prevNormal, prevVertex, currentNormal, currentVertex);

				//If the tube exited the region (in cyclic case) need to reset the points.  Note that
				//the direction vectors don't need to be changed:
				if (newcycle){
					float nextStartPoint[3];
					//Establish a new cycle for a translate of the last tube endpoint:
					newcycle = mapPeriodicCycle(point, endPoint, newCycle, currentCycle);
					assert(!newcycle);
					//Also map the startPoint to the same cycle, but we don't use the resulting cycle
					mapPeriodicCycle(point-3, nextStartPoint, currentCycle, newCycle);
					//Render the translation of the previous tube.  Note that we will need to first specify
					//translated values for prevVertex.  prevNormal is OK.  The resulting values
					//in currentVertex are translated appropriately, since they are offset from endPoint
					//The repeated segment needs to translate prevVertex accordingly
					for (int j = 0; j< 6; j++){
						for (int k = 0; k<3; k++){
							prevVertex[3*j+k] += nextStartPoint[k]-startPoint[k];
						}
					}
					drawTube(isLit, flowLineData->getFlowRGBAs(tubeNum, tubeIndex), nextStartPoint, endPoint, currentB, currentU, radius, constMap,
						prevNormal, prevVertex, currentNormal, currentVertex);
				}
			}

			//Draw an end-cap on the cylinder, and potentially a stationary symbol:
			vcopy(currentA, finalA);
			if ((*point) == STATIONARY_STREAM_FLAG)
				renderStationary(point-3);
			
		} //end of one tube rendering.  
		
		/* render VA for this tube*/
		for (int ii = 0; ii+1 < curVaIndex/6; ii++) {
			//glDrawRangeElements(GL_TRIANGLE_STRIP, ii*6, (ii+2)*6-1, 14, GL_UNSIGNED_INT, &indexArray[ii*2*7]);
			glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, &indexArray[ii*2*7]);
		}

		/* render endcaps from first/last hexagon in vertexArray if there's a tube*/
		if (curVaIndex >= 12) { 
			glDisableClientState(GL_NORMAL_ARRAY);
			glNormal3fv(startA);
			glDrawArrays(GL_POLYGON, 0, 6);
			glNormal3fv(finalA);
			glDrawArrays(GL_POLYGON, curVaIndex-6, 6);
			glEnableClientState(GL_NORMAL_ARRAY);
		}
		

		printOpenGLError();
	} //end of loop over seedPoints

	/* disable the other client states, so we don't accidentally interfere w/ other renderers */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	if (!constMap) glDisableClientState(GL_COLOR_ARRAY);
	
}
//  Issue OpenGL calls for a cylindrical (hexagonal cross-section) arrow 
//  following the stream or Path line
//  tubeNum specifies which tube in the flowdata array to render
//
void FlowRenderer::
renderArrows(FlowLineData* flowLineData, float radius, bool isLit, int firstAge, int lastAge,  bool constMap ){
	
	//Declare values used repeatedly (toggle between even and odd)
	//Tube will actually be hexagonal
	
	float evenU[3];//vector in plane of points, orthog to A
	float oddU[3];
	
	float evenN[3];//vector pointing in direction to next point
	float oddN[3];
	
	float currentB[3];//Binormal, U cross A
	
	float len;
	float testVec[3];
	
	if (firstAge >= lastAge) return;

	//Vectors to hold the start and end arrow coordinates.
	//They may be translated due to periodic boundary conditions.
	float startPoint[3], endPoint[3];
	int newCycle[3],currentCycle[3];
	bool newcycle;

	if (lastAge > curVaSize || newType) {
		curVaSize = lastAge;

		/* realloc vertex array */
		delete[] vertexArray;
		vertexArray = new flowTubeVertexData[(curVaSize+1)*(6*2 + 7)]; //12/tube + 7/arrowhead

		/* realloc, rebuild index array */
		delete[] indexArray;
		indexArray = new unsigned int [curVaSize*(2*7+8)]; // 14/tube + 8/arrowhead

		int ib=0;
		/* loop over tube indicies */
		for (int ia=0; ia<curVaSize*7; ia++) {
			if ((ia+1) % 7 == 0) {
				indexArray[2*ia + 0] = (unsigned int) ib - 6;
				indexArray[2*ia + 1] = (unsigned int) ib;
				ib+=6; 
			} else {
				indexArray[2*ia + 0] = (unsigned int) ib;
				indexArray[2*ia + 1] = (unsigned int) ib + 6;
				ib++;
			}
		}
		/* loop over arrowhead indicies */
		ib = (curVaSize+1)*6*2;
		for (int ia=curVaSize*2*7; ia<curVaSize*(2*7+8); ia++) {
			if ((ia-curVaSize*2*7+1) % 8 == 0) {
				indexArray[ia] = (unsigned int) ib - 6;
			} else {
				indexArray[ia] = (unsigned int) ib;
				ib++;
			}
		}
	}

	/* enable client states for vertex arrays*/
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+7);
	glEnableClientState(GL_NORMAL_ARRAY);
	glNormalPointer(GL_FLOAT, 10*sizeof(float), ((float*)vertexArray)+4);

	if (!constMap){ 
		glEnableClientState(GL_COLOR_ARRAY);
		glColorPointer(4, GL_FLOAT, 10*sizeof(float), vertexArray);
	}

	for (int tubeNum = 0; tubeNum < flowLineData->getNumLines(); tubeNum++){
		//Skip the tube entirely if first point is end-flow 
		//This can occur in the middle of a Pathline, but not with flowLineData!
		//float* point = flowDataArray + 3*(startIndex+tubeNum*maxPoints +firstAge);
		int firstIndex = Max(firstAge, flowLineData->getStartIndex(tubeNum));
		int lastIndex = Min(lastAge, flowLineData->getEndIndex(tubeNum));
		if (firstIndex >= lastIndex) continue;
		float* point = flowLineData->getFlowPoint(tubeNum, firstIndex);
		curVaIndex = 0;
		curVaIndex2 = (curVaSize+1)*6*2;
		
		assert(*point != END_FLOW_FLAG);
		//Cycle through the points looking for a valid tubeStartIndex.
		//If the first one is at the end, we don't render anything
		//
		int tubeStartIndex;
		bool stationaryStart = false;
		// first loop is just to find a start for the tube:
		for (tubeStartIndex = firstIndex; tubeStartIndex < lastIndex; tubeStartIndex++){
		 
			point = flowLineData->getFlowPoint(tubeNum,tubeStartIndex);
			if (*point == IGNORE_FLAG) assert(0); //continue;  
			if (*point == STATIONARY_STREAM_FLAG) {
				//if Multiple STATIONARY_FLAGS, it's a bug...
				if (stationaryStart) {
					assert(0);
					stationaryStart = false;
					tubeStartIndex = lastIndex;
					break;
				}
				stationaryStart = true;
				continue;
			}
			//At this point we know it's a valid point
			assert (*point != END_FLOW_FLAG);
			break;
		}
		//check if we didn't find anything.  Possibly render a solitary
		//stationary point if we didn't find anything else.
		if (tubeStartIndex == lastIndex){
			if (stationaryStart) {
				point = flowLineData->getFlowPoint(tubeNum, tubeStartIndex);
				renderStationary(point);
			}
			break;
		}
		//Check if the first point was preceded by stationary flag:
		if (stationaryStart){
			point = flowLineData->getFlowPoint(tubeNum, tubeStartIndex);
			renderStationary(point);
		}
		assert(tubeStartIndex < lastIndex);

		//OK, we are at the start of a real tube.
		//Start the colors for the start of the tube 
		if (!constMap) glColor4fv(flowLineData->getFlowRGBAs(tubeNum, tubeStartIndex));
		else glColor4fv(constFlowColor);

		//Check if the second point is a stationary flag:
		if (*(flowLineData->getFlowPoint(tubeNum, (tubeStartIndex+1))) == STATIONARY_STREAM_FLAG) {
			//If so just render the stationary symbol, and we are done.
			float *point = flowLineData->getFlowPoint(tubeNum, (tubeStartIndex));
			renderStationary(point);
			continue;
		} else if (*(flowLineData->getFlowPoint(tubeNum, (tubeStartIndex+1))) == END_FLOW_FLAG) {//render nothing:
				//We could potentially have the second point be an end flow flag
				continue;
		} else {  //OK, now we can render a tube:

			currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
			newcycle = false;
		
			
			//data point is three floats starting at data[3*tubeStartIndex]
			//evenN is the direction the line is pointing
			vsub(flowLineData->getFlowPoint(tubeNum,(tubeStartIndex+1)), flowLineData->getFlowPoint(tubeNum,tubeStartIndex), evenN);
			//Normalize even
			len = vdot(evenN,evenN);
			if (len == 0.f || len > 1.e36f ){//If 2nd is same as first set default normal
				vset(evenN, 0.f,0.f,1.f);
			} else {
				vscale(evenN, 1.f/sqrt(len));
			}
			
			//Calculate evenU, orthogonal to evenN:
			vset(testVec, 1.,0.,0.);
			vcross(evenN, testVec, evenU);
			len = vdot(evenU,evenU);
			if (len == 0.f){
				vset(testVec, 0.,1.,0.);
				vcross(evenN, testVec, evenU);
				len = vdot(evenU, evenU);
				assert(len != 0.f);
			} 
			vscale( evenU, 1.f/sqrt(len));
			vcross(evenU, evenN, currentB);
			
			//Now loop over points, starting with no. 1.
			//toggle even and odd.
			float* currentN;
			float* currentU;
			float* prevN;
			float* prevU;
			
			//tubeIndex counts the arrow coords along the data,
			//tubeIndex is pointNum +startIndex+tubeNum*maxPoints
			for (int tubeIndex = tubeStartIndex+1; tubeIndex <= lastIndex; tubeIndex++){
				point = flowLineData->getFlowPoint(tubeNum,tubeIndex);
			//for (int pointNum = firstAge+ 1; pointNum <= lastAge; pointNum++){
			//	point = flowDataArray+3*(startIndex+tubeNum*maxPoints+pointNum);
				if (*point == END_FLOW_FLAG || *point == STATIONARY_STREAM_FLAG) break;
				//Toggle the meaning of "current" and "prev"
				if (0 == (tubeIndex-tubeStartIndex)%2) {
					currentN = evenN;
					prevN = oddN;
					currentU = evenU;
					prevU = oddU;
				} else {
					currentN = oddN;
					prevN = evenN;
					currentU = oddU;
					prevU = evenU;
				}
				
				//Calc currentN
				vsub(point, point-3, currentN);
				//Normalize currentN:
				len = vdot(currentN,currentN);
				if (len == 0.f){// keep previous direction vector
					vcopy(prevN,currentN);
				} else {
					vscale(currentN, 1.f/sqrt(len));
				}
				
				//Now get next U, by projecting previous U orthog to currentN:
				vmult(currentN, vdot(prevU,currentN), testVec);
				vsub(prevU, testVec, currentU);
				//Now normalize it:
				len = vdot(currentU,currentU);
				if (len == 0.f){
					//If U is in direction of N, the previous N should work
					vcopy(prevN, currentU);
					len = vdot(currentU, currentU);
					assert(len != 0.f);
				} 
				vscale(currentU, 1.f/sqrt(len));
				vcross(currentU, currentN, currentB);
				
				
				if(tubeIndex > tubeStartIndex+1) {
					//For each arrow after the first, 
					// the new arrow goes from previous endpoint to mapping of point
					//and map the endPoint in the same cycle as the startPoint:
					vcopy(endPoint, startPoint);
					newcycle = mapPeriodicCycle(point, endPoint, currentCycle, newCycle);
				} else { //first time through, startPoint is point-3, endPoint is point
					//Establish the first cycle, use it for first two points:
					bool modifiedCycle = mapPeriodicCycle(point-3, startPoint, currentCycle, newCycle);
					if (modifiedCycle) mapPeriodicCycle(point-3, startPoint, newCycle, currentCycle);
					//Then put the second point into the same cycle
					newcycle = mapPeriodicCycle(point, endPoint, currentCycle, newCycle);
					//vcopy(point-3, startPoint);
					//vcopy(point, endPoint);
				}
				
				drawArrow(isLit, flowLineData->getFlowRGBAs(tubeNum,tubeIndex-1), startPoint, endPoint, currentN, currentB, currentU, radius, constMap);

				//If the arrow exited the region (in cyclic case) need to restart the points.  Note that
				//the direction vectors don't need to be changed:
				if (newcycle){
					//Establish a new cycle for a translate of the last arrow endpoint:
					newcycle = mapPeriodicCycle(point, endPoint, newCycle, currentCycle);
					assert(!newcycle);
					//Also map the startPoint to the same cycle, but we don't use the resulting cycle
					mapPeriodicCycle(point-3, startPoint, currentCycle, newCycle);
					//Then render the new (cyclically offset) arrow:
					drawArrow(isLit, flowLineData->getFlowRGBAs(tubeNum,tubeIndex-1), startPoint, endPoint, currentN, currentB, currentU, radius, constMap);
				}
				
				
				if ((*point) == STATIONARY_STREAM_FLAG)
					renderStationary(point-3);
			} //end of arrows along one flow line
			
		} //legitimate flow line

		//Render tubes
		for (int ii = 0; ii+1 < curVaIndex/12; ii++) 
			glDrawElements(GL_TRIANGLE_STRIP, 14, GL_UNSIGNED_INT, &indexArray[ii*2*7]);

		//Render endcaps
		for (int ii = 0; ii+1 < curVaIndex/12; ii++)
			glDrawArrays(GL_POLYGON, ii*(2*6+0), 6);

		//Render arrowheads
		for (int ii = 0; ii+1 < curVaIndex/12; ii++)
			glDrawElements(GL_TRIANGLE_FAN, 8, GL_UNSIGNED_INT, &indexArray[curVaSize*2*7 + ii*8]);

	} //end of loop over seedPoints
	//Special symbol at stationary flow:

	/* disable the other client states, so we don't accidentally interfere w/ other renderers */
	glDisableClientState(GL_VERTEX_ARRAY);
	glDisableClientState(GL_NORMAL_ARRAY);
	if (!constMap) glDisableClientState(GL_COLOR_ARRAY);
}
//Turn off all the need refresh flags, as when autoRefresh is enabled.
//Return true if anything was dirty, that would indicate the need to enable
//the refresh button.
bool FlowRenderer::
setDirtyNeedsRefresh(FlowParams* fParams) {
	bool isDirty = false;
	if (fParams->getFlowType() != 1){
		for (int i = 0; i<=maxFrame; i++) {
			setNeedOfRefresh(i,false);
			if (flowDataIsDirty(i)) {
				isDirty = true;
			}
		}
	} else {
		isDirty = allFlowDataIsDirty();
		setAllNeedRefresh(false);
	}
	return isDirty;
}
//Capture the flow lines for steady flow, or seeds for FLA
void FlowRenderer::
captureFlow(int timestep){
	FlowLineData* flowData = getSteadyCache(timestep);
	if (!flowData) return;
	//Construct the filename by adjoining the time step
	QString saveFilename = myGLWindow->getFlowFilename();
	FlowParams* fParams = (FlowParams*)getRenderParams();
	int flowType = fParams->getFlowType();
	saveFilename += (QString("%1").arg(timestep)).rightJustified(4,'0');
	saveFilename +=  ".txt";
	FILE* saveFile = fopen((const char*)saveFilename.toAscii(),"w");
	if (!saveFile){
		MyBase::SetErrMsg(VAPOR_ERROR_FLOW_SAVE,"Unable to write to file %s",
			(const char*)saveFilename.toAscii());
		myGLWindow->stopFlowCapture();
		return;
	}

	for (int i = 0; i<flowData->getNumLines(); i++){
		//Determine the seed position in the flow
		//If it's steady, find the start and end position.
		int seedPos = flowData->getSeedPosition();
		int startPos = flowData->getStartIndex(i);
		int endPos = flowData->getEndIndex(i);
		//Move start and end position if END_FLOW_FLAG
		//or STATIONARY_STREAM_FLAG
		if (seedPos > 0 && startPos > 0 &&
			((flowData->getFlowPoint(i,startPos-1)[0] == END_FLOW_FLAG)
			||(flowData->getFlowPoint(i,startPos-1)[0] == STATIONARY_STREAM_FLAG))) startPos--;
		if (seedPos < flowData->getMaxPoints()-1 && 
			endPos < flowData->getMaxPoints()-1 &&
			((flowData->getFlowPoint(i,endPos+1)[0] == END_FLOW_FLAG)
			||(flowData->getFlowPoint(i,endPos+1)[0] == STATIONARY_STREAM_FLAG))) endPos++;
		//Write header line for this flowline:
		if (flowType == 0){
			fprintf(saveFile,"%d %d %d\n",timestep, 
				endPos-startPos+1, seedPos - startPos);
			//then write the points of the flow line
			for (int j = startPos; j<= endPos; j++){
				float* point = flowData->getFlowPoint(i,j);
				int rc = fprintf(saveFile,"%9g %9g %9g\n",
					point[0],point[1],point[2]);
				if (rc <= 0){
					MyBase::SetErrMsg(VAPOR_ERROR_FLOW_SAVE,"Unable to write stream line\nfor time step %d",timestep);
					return;
				}
			}
		} else { //Write just one FLA seed:
			float* point = flowData->getFlowPoint(i, seedPos);
			int rc = fprintf(saveFile,"%9g %9g %9g\n",
					point[0],point[1],point[2]);
			if (rc <= 0){
				MyBase::SetErrMsg(VAPOR_ERROR_FLOW_SAVE,"Unable to write FLA seed\nfor time step %d",timestep);
				return;
			}
		}
	}
	//Done with writing flow lines:
	fclose(saveFile);
	return;
}
