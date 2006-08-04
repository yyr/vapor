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
#include "renderer.h"
#include "mapperfunction.h"

//Constants used for arrow design:
//VERTEX_ANGLE = 45 degrees (angle between direction vector and head edge
#define ARROW_LENGTH_FACTOR  0.90f //fraction of full length used by cylinder
#define MIN_STATIONARY_RADIUS 2.f //minimum in voxels of stationary octahedron
/*!
  Create a FlowRenderer 
*/
using namespace VAPoR;
FlowRenderer::FlowRenderer(GLWindow* glw, FlowParams* fParams )
:Renderer(glw)
{
    myFlowParams = fParams;
	numInjections = 0;
	steadyFlow = fParams->flowIsSteady();
	for (int i = 0; i<4; i++) constFlowColor[i] = 1.f;

	//Set up flow data cache arrays, potentially for all
	//timesteps in data
	numFrames = DataStatus::getInstance()->getNumTimesteps();
	rakeFlowData = new float*[numFrames];
	listFlowData = new float*[numFrames];
	numListSeedPointsUsed = new int[numFrames];
	rakeFlowRGBAs = new float*[numFrames];
	listFlowRGBAs = new float*[numFrames];
	flowDataDirty = new bool[numFrames];
	needRefreshFlag = new bool[numFrames];
	flowMapDirty = new bool[numFrames];
	numRakeSeedPointsUsed = 0;
	for (int i = 0; i<numFrames; i++){
		rakeFlowData[i] = 0;
		listFlowData[i] = 0;
		numListSeedPointsUsed[i] = 0;
		rakeFlowRGBAs[i] = 0;
		listFlowRGBAs[i] = 0;
		flowDataDirty[i] = 0;
		needRefreshFlag[i] = false;
		flowMapDirty[i] = 0;
	}
	setRegionValid(true);
	lastTimeStep = -1;
}


/*!
  Release allocated resources
*/

FlowRenderer::~FlowRenderer()
{
	for (int i = 0; i<numFrames; i++){
		if(rakeFlowData[i]) delete rakeFlowData[i];
		if(listFlowData[i]) delete listFlowData[i];
		if(rakeFlowRGBAs[i]) delete rakeFlowRGBAs[i];
		if(listFlowRGBAs[i]) delete listFlowRGBAs[i];
	}
	delete rakeFlowData;
	delete listFlowData;
	delete rakeFlowRGBAs;
	delete listFlowRGBAs;

	delete numListSeedPointsUsed;
	delete needRefreshFlag;
	delete flowDataDirty;
	delete flowMapDirty;
}


// Perform the rendering
//
  

void FlowRenderer::paintGL()
{
	//If the regionValid flag is off, need a change before we try to render again..
	if (!regionValid()) return;
	
	AnimationParams* myAnimationParams = myGLWindow->getAnimationParams();
	
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	steadyFlow = myFlowParams->flowIsSteady();
	int timeStep = steadyFlow ? currentFrameNum : 0;
	
	bool constColors = ((myFlowParams->getColorMapEntityIndex() + myFlowParams->getOpacMapEntityIndex()) == 0);
	
	

	constFlowColor[3] = myFlowParams->getConstantOpacity();
	QRgb constRgb = myFlowParams->getConstantColor();
	constFlowColor[0] = qRed(constRgb)/255.f;
	constFlowColor[1] = qGreen(constRgb)/255.f;
	constFlowColor[2] = qBlue(constRgb)/255.f;

	//First render the rake flow:
	//May first need to rebuild it:
	bool didRebuild = false;
	if (myFlowParams->rakeEnabled()){ 
		//Either we just need to refresh graphics, or we need both data and graphics
		if (flowMapDirty[timeStep] ||(needRefreshFlag[timeStep] && flowDataDirty[timeStep])){
			if(!rebuildFlowData(timeStep, true)) return;
			didRebuild = true;
		}
		//Now render the rake data
		flowDataArray = getFlowData(timeStep, true);
		flowRGBAs = getRGBAs(timeStep, true); 
		renderFlowData(constColors,currentFrameNum);
	}
	//Next render the list data
	//May first need to rebuild it:
	if (myFlowParams->listEnabled()){ 
		if (flowMapDirty[timeStep] ||(needRefreshFlag[timeStep] && flowDataDirty[timeStep])){
			if(!rebuildFlowData(timeStep, false)) return;
			didRebuild = true;
		}
		//Now render the rake data
		flowDataArray = getFlowData(timeStep, false);
		flowRGBAs = getRGBAs(timeStep, false); 
		renderFlowData(constColors,currentFrameNum);
	}
	//Reset the dirty flags:
	//The color map was already made current
	flowMapDirty[timeStep] = false;
	//If we rebuilt the data, then reset its dirty flag too:
	//Don't change the needRefreshFlag 
	if (didRebuild){
		flowDataDirty[timeStep] = false;
		myGLWindow->setRenderNew();
	}
}

void FlowRenderer::
renderFlowData(bool constColors, int currentFrameNum){
	
	RegionParams* myRegionParams = myGLWindow->getRegionParams();
	FlowParams* myFlowParams = myGLWindow->getFlowParams();
	
	
	
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
	int nLights = 0;
	if (myFlowParams->getShapeType() != 1) {//rendering tubes/lines/arrows
		float diffColor[4], specColor[4], ambColor[4];
		GLfloat lmodel_ambient[4];
		specColor[0]=specColor[1]=specColor[2]=0.2f;
		ambColor[0]=ambColor[1]=ambColor[2]=0.f;
		diffColor[3]=specColor[3]=ambColor[3]=lmodel_ambient[3]=1.f;
		
		ViewpointParams* vpParams =  myGLWindow->getViewpointParams();
		nLights = vpParams->getNumLights();
		if (nLights > 0){
			glPushMatrix();
			glLoadIdentity();
			glShadeModel(GL_SMOOTH);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, constFlowColor);
			glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());
			lmodel_ambient[0]=lmodel_ambient[1]=lmodel_ambient[2] = vpParams->getAmbientCoeff();

			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
			glLightfv(GL_LIGHT0, GL_POSITION, vpParams->getLightDirection(0));
			
			specColor[0] = specColor[1] = specColor[2] = vpParams->getSpecularCoeff(0);
			diffColor[0] = diffColor[1] = diffColor[2] = vpParams->getDiffuseCoeff(0);
			glLightfv(GL_LIGHT0, GL_DIFFUSE, diffColor);
			glLightfv(GL_LIGHT0, GL_SPECULAR, specColor);
			glLightfv(GL_LIGHT0, GL_AMBIENT, ambColor);
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			if (nLights > 1){
				glLightfv(GL_LIGHT1, GL_POSITION, vpParams->getLightDirection(1));
				specColor[0] = specColor[1] = specColor[2] = vpParams->getSpecularCoeff(1);
				diffColor[0] = diffColor[1] = diffColor[2] = vpParams->getDiffuseCoeff(1);
				glLightfv(GL_LIGHT1, GL_DIFFUSE, diffColor);
				glLightfv(GL_LIGHT1, GL_SPECULAR, specColor);
				glLightfv(GL_LIGHT1, GL_AMBIENT, ambColor);
				glEnable(GL_LIGHT1);
			}
			if (nLights > 2){
				glLightfv(GL_LIGHT2, GL_POSITION, vpParams->getLightDirection(2));
				specColor[0] = specColor[1] = specColor[2] = vpParams->getSpecularCoeff(2);
				diffColor[0] = diffColor[1] = diffColor[2] = vpParams->getDiffuseCoeff(2);
				glLightfv(GL_LIGHT2, GL_DIFFUSE, diffColor);
				glLightfv(GL_LIGHT2, GL_SPECULAR, specColor);
				glLightfv(GL_LIGHT1, GL_AMBIENT, ambColor);
				glEnable(GL_LIGHT2);
			}
		} else {
			glDisable(GL_LIGHTING); //No lights
		}
		glPopMatrix();
	} else {//points are not lit..
		glDisable(GL_LIGHTING);
		
	}
	//Apply a coord transform that moves the full region to the unit cube.
	
	glPushMatrix();
	

	//scale:
	float scaleFactor = 1.f/ViewpointParams::getMaxCubeSide();
	glScalef(scaleFactor, scaleFactor, scaleFactor);

	//translate to put origin at corner:
	float* transVec = ViewpointParams::getMinCubeCoords();
	glTranslatef(-transVec[0],-transVec[1], -transVec[2]);
	
	//Set up clipping planes
	topPlane[3] = myRegionParams->getRegionMax(1);
	botPlane[3] = -myRegionParams->getRegionMin(1);
	leftPlane[3] = -myRegionParams->getRegionMin(0);
	rightPlane[3] = myRegionParams->getRegionMax(0);
	frontPlane[3] = myRegionParams->getRegionMax(2);
	backPlane[3] = -myRegionParams->getRegionMin(2);
	
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
	//voxelSize is actually the max of the sides of the voxel in user coords
	const float* fullExtent = DataStatus::getInstance()->getExtents();
	const size_t* fullDims = DataStatus::getInstance()->getFullDataSize();
	voxelSize = Max((fullExtent[5]-fullExtent[2])/fullDims[0],
		Max((fullExtent[4]-fullExtent[1])/fullDims[1], (fullExtent[3]-fullExtent[0])/fullDims[2]));
		
	//stationary radius is radius of stationary point symbol in user coords
	if (diam > 2*MIN_STATIONARY_RADIUS)
		stationaryRadius = voxelSize*0.5*diam;
	else stationaryRadius = voxelSize*MIN_STATIONARY_RADIUS;
	float userRadius = 0.5f*diam*voxelSize;
	arrowHeadRadius = (myFlowParams->getArrowDiameter())*userRadius;

	
	//If we are doing unsteady flow, handle setup differently:
	if (steadyFlow){
		if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
				
			if (diam < 2.f){//Render as lines, not cylinders
				renderCurves(diam, (nLights>0), 0, maxPoints-1, 0, constColors);
			
			} else { //render as cylinders
				//Determine cylinder radius in actual coords.
				//One voxel is (full region size)/(region array size)
				
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				//Render all tubes
				//Note that lastDisplayFrame is maxPoints -1
				//and firstDisplayFrame is 0
				//
				renderTubes(userRadius, (nLights > 0), 0, maxPoints-1, 0,constColors);
				
			}
		} else if (myFlowParams->getShapeType() == 1 ){ //rendering points 
			//just convert the flow data to a set of points..
			renderPoints(diam, 0, maxPoints-1, 0, constColors);
		} else { //render arrows
			
			
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
			//Render arrows
			//Note that lastDisplayFrame is maxPoints -1
			//and firstDisplayFrame is 0
			//
			renderArrows(userRadius, (nLights > 0), 0, maxPoints-1, 0, constColors);
		}

	} else { //unsteady flow:
		//Determine first and last seeding that is visible.
		//The portion of a flow that is visible is that portion whose age
		//goes from currentFrame - firstDisplayAge to currentFrame + lastDisplayAge.  
		
		//This is reduced on the front end if currentFrame-firstDisplayAge
		//precedes seedStartFrame.
		//It shouldn't need to be reduced on the back end since all Pathlines are calculated
		//potentially to the endFrame

		
		for (int seedFrame = startSeed; seedFrame <= endSeed; seedFrame+= seedIncrement){
			//Count the seedings:
			int injectionNum = (seedFrame-startSeed)/seedIncrement;
			//Find the display window for this seeding associated with the current frame:
			int minDisplayFrame = Max( seedFrame, currentFrameNum + firstDisplayAge);
			int maxDisplayFrame = currentFrameNum + lastDisplayAge;
			//Make sure that the current frame is in the display window for this seeding.
			//It gets excluded if current seeding hasn't started by the maxDisplayFrame,
			
			if (seedFrame > maxDisplayFrame) continue;
			//firstFrame and lastFrame translate the display interval to be relative to the 
			//current seeding:
			int firstFrame = minDisplayFrame - seedFrame;
			int lastFrame = maxDisplayFrame - seedFrame;
			//firstGeom and lastGeom provide the interval of instances of points along
			//the flowline that is to be converted to geometry:
			int firstGeom = (int)(firstFrame*objectsPerTimestep + 0.5f);
			int lastGeom = (int)(lastFrame*objectsPerTimestep+0.5f);
			if (firstGeom < 0) firstGeom = 0;
			if (lastGeom < 0) lastGeom = 0;
			if (lastGeom >= maxPoints) lastGeom = maxPoints-1;
			if (firstGeom > lastGeom) continue;

			//Now do the rendering of this interval:
			if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
					
				if (diam < 2.f){//Render as lines, not cylinders
					renderCurves(diam, (nLights>0), firstGeom, lastGeom,
						maxPoints*numSeedPoints*injectionNum,constColors);
				
				} else { //render as cylinders
					
					glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
					//Render all tubes
					renderTubes(userRadius, (nLights > 0), firstGeom, lastGeom,
						maxPoints*numSeedPoints*injectionNum, constColors);
					
				}
			} else if(myFlowParams->getShapeType() == 1) { //rendering points 
				//just convert the flow data to a set of points..
				renderPoints(diam, firstGeom, lastGeom,
						maxPoints*numSeedPoints*injectionNum, constColors);
			} else { //rendering arrows:
				
				glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
				
				renderArrows(userRadius, (nLights > 0), firstGeom, lastGeom,
					maxPoints*numSeedPoints*injectionNum, constColors);
			}
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
	glPopMatrix();

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
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
	setRegionValid(true);
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
void FlowRenderer::
renderPoints(float radius, int firstAge, int lastAge, int startIndex, bool constMap){
	float mappedPoint[3];
	//just convert the flow data to a set of points..
	glPointSize(radius);
	glDisable(GL_LIGHTING);
	
	for (int i = 0; i< numSeedPoints; i++){
		//glBegin (GL_POINTS);
		bool endGL = true;
		bool firstPoint = true;
		bool stationaryStart = false;
		
		for (int j = firstAge; j<=lastAge; j++){
			float* point = flowDataArray+ 3*(startIndex+j+ maxPoints*i);
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
				glBegin(GL_POINTS);
				endGL = false;
			}
			
			//qWarning("point is %f %f %f", *point, *(point+1), *(point+2));
			
			if (!constMap){
				float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
				glColor4fv(rgba);
			}

			//Use the last point for a stationary marker
			if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
				glVertex3fv(point);
				glEnd();
				
				renderStationary(point);
				endGL = true;
				break;
			}
			myFlowParams->periodicMap(point, mappedPoint);
			glVertex3fv(mappedPoint);
		}	
		if(!endGL) glEnd();
	}
	
	
}
//  Issue OpenGL calls for a set of lines associated with a number of seed points.
void FlowRenderer::
renderCurves(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap){
	float dirVec[3];
	float testVec[3];
	float normVec[3];
	float mappedPoint[3];
	int currentCycle[3]; 
	int newCycle[3];
	bool newcycle;
	if (firstAge >= lastAge) return;
	glLineWidth(radius);
	
	if (isLit){
		//Get light direction vector of first light:
		ViewpointParams* vpParams = myGLWindow->getViewpointParams();
		const float* lightDir = vpParams->getLightDirection(0);
		for (int i = 0; i< numSeedPoints; i++){
			//Assume seed point starts inside region:
			currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
			newcycle = false;
			if (constMap)glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE, constFlowColor);
			//glBegin (GL_LINE_STRIP);
			bool endGL = true;
			bool firstPoint = true;
			bool stationaryStart = false;
			
			for (int j = firstAge; j<=lastAge; j++){
				//For each point after first, calc vector from prev vector to this one
				//then calculate corresponding normal
				float* point = flowDataArray + 3*(startIndex+ j+ maxPoints*i);
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
					glBegin(GL_LINE_STRIP);
					endGL = false;
					mappedPoint[0]=point[0];
					mappedPoint[1]=point[1];
					mappedPoint[2]=point[2];
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
					glNormal3fv(normVec);
					//only call mapPeriodicCycle to translate, or detect leaving the region, after the first point
					newcycle = mapPeriodicCycle(point, mappedPoint, currentCycle, newCycle);
				}
				if (!constMap){
					float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, rgba);
				}
				
				if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
					glVertex3fv(mappedPoint);//Terminate current curve
					glEnd();
					renderStationary(mappedPoint);
					endGL = true;
					break;
				}
				
				glVertex3fv(mappedPoint);
				//Test if the last point ran off the end of the cycle. If so render it again in the new place.
				if (newcycle) {
					//end previous curve, start next one:
					glEnd();
					newcycle = mapPeriodicCycle(point, mappedPoint, newCycle, currentCycle);
					assert(!newcycle);
					glBegin(GL_LINE_STRIP);
					glVertex3fv(mappedPoint);
				}
			}
			if(!endGL) glEnd();
		}
	} else { //No lights
		//just convert the flow data to a set of lines...
		glDisable(GL_LIGHTING);
		for (int i = 0; i< numSeedPoints; i++){
			//first point starts in base region:
			currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
			newcycle = false;
			glColor4fv(constFlowColor);
			
			//glBegin (GL_LINE_STRIP);
			bool endGL = true;
			bool firstPoint = true;
			bool stationaryStart = false;
			for (int j = firstAge; j<=lastAge; j++){
				float* point = flowDataArray +  3*(startIndex+ j+ maxPoints*i);
				if (*point == END_FLOW_FLAG) break;
				if (*point == IGNORE_FLAG) continue;
				
				//Check for an initial STATIONARY_FLAG
				if (firstPoint && (*point == STATIONARY_STREAM_FLAG)){
					stationaryStart = true;
					continue;
				}
				if (stationaryStart){
					if (*point == STATIONARY_STREAM_FLAG) break;
					//Render the stationary symbol, then continue on to start the streamline
					renderStationary(point);
					stationaryStart = false;
				}
				if(firstPoint){
					firstPoint = false;
					if(constMap) glColor4fv(constFlowColor);
					glBegin(GL_LINE_STRIP);
					endGL = false;
					mappedPoint[0]=point[0];
					mappedPoint[1]=point[1];
					mappedPoint[2]=point[2];
				} else {  //Call mapPeriodiccycle for every point after the first:
					newcycle = mapPeriodicCycle(point, mappedPoint, currentCycle, newCycle);
				}
				if (!constMap){
					float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
					glColor4fv(rgba);
				}
				if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
					glVertex3fv(mappedPoint);
					glEnd();//Terminate current curve
					
					renderStationary(mappedPoint);
					endGL = true;
					break;
				}
				glVertex3fv(mappedPoint);
				//Test if the last point ran off the end of the cycle. If so render it again in the new place.
				if (newcycle) {
					//end previous curve, start next one:
					glEnd();
					newcycle = mapPeriodicCycle(point, mappedPoint, newCycle, currentCycle);
					assert(!newcycle);
					glBegin(GL_LINE_STRIP);
					glVertex3fv(mappedPoint);
				}

			}
			if(!endGL) glEnd();
		}
	}//end no lights
}
//  Issue OpenGL calls for a cylindrical (hexagonal cross-section) tube 
//  following the stream or Path line
//  tubeNum specifies which tube in the flowdata array to render
//
void FlowRenderer::
renderTubes(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap ){
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
	
	float len;
	float testVec[3];
	float testVec2[3];
	//Vectors to hold the start and end arrow coordinates.
	//They may be translated due to periodic boundary conditions.
	float startPoint[3], endPoint[3];
	int newCycle[3],currentCycle[3];
	bool newcycle;

	if (firstAge >= lastAge) return;
	
	
	
	for (int tubeNum = 0; tubeNum < numSeedPoints; tubeNum++){
		//Skip the tube entirely if first point is end-flow 
		//This can occur in the middle of a Pathline
		float* point = flowDataArray + 3*(startIndex+tubeNum*maxPoints +firstAge); 
		if(*point == END_FLOW_FLAG) continue;
		//Cycle through the points looking for a valid tubeStartIndex.
		//If the first one is at the end, we don't render anything
		//
		int tubeStartIndex;
		bool stationaryStart = false;
		
		for (tubeStartIndex = (startIndex + tubeNum*maxPoints+firstAge);
		 tubeStartIndex < (startIndex + tubeNum*maxPoints+lastAge);
		 tubeStartIndex++){
			point = flowDataArray + 3*tubeStartIndex;
			if (*point == IGNORE_FLAG) continue;
			if (*point == STATIONARY_STREAM_FLAG) {
				//Multiple STATIONARY_FLAGS, render nothing
				if (stationaryStart) {
					stationaryStart = false;
					tubeStartIndex = (startIndex + tubeNum*maxPoints+lastAge);
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
		if (tubeStartIndex == (startIndex + tubeNum*maxPoints+lastAge)){
			if (stationaryStart) {
				point = flowDataArray + 3*tubeStartIndex;
				renderStationary(point);
			}
			break;
		}
		//Check if the first point was preceded by stationary flag:
		if (stationaryStart){
			point = flowDataArray + 3*tubeStartIndex;
			renderStationary(point);
		}
		assert(tubeStartIndex < startIndex + tubeNum*maxPoints + lastAge);

		//startTube( intput: tubeStartIndex, flowRGBAs constFlowColor, flowDataArray, 

		//output: point [continue, or break??], evenA, len, evenN, evenU

		//OK, we are at the start of a real tube.
		//Start the colors for the start of the tube 
		if (!constMap){
			float* rgba = flowRGBAs + 4*(tubeStartIndex);
			if(isLit)
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, rgba);
			else
				glColor4fv(rgba);
		} else {
			if(isLit) glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE, constFlowColor);
			else glColor4fv(constFlowColor);
		}
		//Check if the second point is a stationary flag:
		if ((*(flowDataArray + 3*(tubeStartIndex+1))) == STATIONARY_STREAM_FLAG) {
			//If so just render the stationary symbol, and we are done.
			float *point = flowDataArray+3*tubeStartIndex;
			renderStationary(point);
			continue;
		} else if ((*(flowDataArray + 3*(tubeStartIndex+1))) == END_FLOW_FLAG) {//render nothing:
				//We could potentially have the second point be an end flow flag
				continue;
		} else {  //OK, now we can render a tube:
			
			
			//data point is three floats starting at data[3*tubeStartIndex]
			//evenA is the direction the line is pointing
			vsub(flowDataArray+3*(tubeStartIndex+1), flowDataArray+3*tubeStartIndex, evenA);
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
				vadd(evenVertex+3*i, flowDataArray+3*tubeStartIndex, evenVertex+3*i);
			}
			//Draw a  starting cap on the cylinder:
			
			glBegin(GL_POLYGON);
			glNormal3fv(evenA);
			for (int k = 0; k<6; k++){
				glVertex3fv(evenVertex+3*k);
			}
			glEnd();


			//Now render the cylinders:
			//loop over points, starting with no. 1.
			//toggle even and odd.
			float* currentN;
			float* currentU;
			float* currentA;
			float* prevN;
			float* prevA;
			float* prevU;
			float* currentVertex;
			float* currentNormal;
			float* prevVertex;
			float* prevNormal;
			

			//prepare to render the first cyclinder in the tube.
			currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
			newcycle = false;

			bool currentIsEven = true;
			int tubeIndex;
			for (tubeIndex = tubeStartIndex+1; tubeIndex <= startIndex+tubeNum*maxPoints+lastAge;
				tubeIndex++){
				point = flowDataArray+3*tubeIndex;
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
					vcopy(point-3, startPoint);
					vcopy(point, endPoint);
				}
				drawTube(isLit, tubeIndex, startPoint, endPoint, currentB, currentU, radius, constMap,
					prevNormal, prevVertex, currentNormal, currentVertex);

				//If the arrow exited the region (in cyclic case) need to reset the points.  Note that
				//the direction vectors don't need to be changed:
				if (newcycle){
					//Establish a new cycle for a translate of the last arrow endpoint:
					newcycle = mapPeriodicCycle(point, endPoint, newCycle, currentCycle);
					assert(!newcycle);
					//Also map the startPoint to the same cycle, but we don't use the resulting cycle
					mapPeriodicCycle(point-3, startPoint, currentCycle, newCycle);
					//Render the translation of the previous tube.  Note that we will need to first specify
					//translated values for prevVertex.  prevNormal is OK.  The resulting values
					//in currentVertex are translated appropriately, since they are offset from endPoint
					for (int j = 0; j< 6; j++){
						for (int k = 0; k<3; k++){
							prevVertex[3*j+k] += startPoint[k]-point[k];
						}
					}
					drawTube(isLit, tubeIndex, startPoint, endPoint, currentB, currentU, radius, constMap,
						prevNormal, prevVertex, currentNormal, currentVertex);
				}
			}
			//Draw an end-cap on the cylinder, and potentially a stationary symbol:
			
			if (!constMap){
				float* nextRGBA = flowRGBAs + 4*(tubeIndex);
				if(isLit)
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, nextRGBA);
				else
					glColor4fv(nextRGBA);
			}
			glBegin(GL_POLYGON);
			glNormal3fv(currentA);
			for (int ka = 5; ka>=0; ka--){
				glVertex3fv(currentVertex+3*ka);
			}
			glEnd();
			if ((*point) == STATIONARY_STREAM_FLAG)
				renderStationary(point-3);
			
		} //end of one tube rendering.  
		

	} //end of loop over seedPoints
	
	
		
}
//  Issue OpenGL calls for a cylindrical (hexagonal cross-section) arrow 
//  following the stream or Path line
//  tubeNum specifies which tube in the flowdata array to render
//
void FlowRenderer::
renderArrows(float radius, bool isLit, int firstAge, int lastAge, int startIndex, bool constMap ){
	
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

	for (int tubeNum = 0; tubeNum < numSeedPoints; tubeNum++){
		//Skip the tube entirely if first point is end-flow 
		//This can occur in the middle of a Pathline
		float* point = flowDataArray + 3*(startIndex+tubeNum*maxPoints +firstAge); 
		if(*point == END_FLOW_FLAG) continue;
		//Cycle through the points looking for a valid tubeStartIndex.
		//If the first one is at the end, we don't render anything
		//
		int tubeStartIndex;
		bool stationaryStart = false;
		for (tubeStartIndex = (startIndex + tubeNum*maxPoints+firstAge);
		 tubeStartIndex < (startIndex + tubeNum*maxPoints+lastAge);
		 tubeStartIndex++){
			point = flowDataArray + 3*tubeStartIndex;
			if (*point == IGNORE_FLAG) continue;
			if (*point == STATIONARY_STREAM_FLAG) {
				//Multiple STATIONARY_FLAGS, render nothing
				if (stationaryStart) {
					stationaryStart = false;
					tubeStartIndex = (startIndex + tubeNum*maxPoints+lastAge);
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
		if (tubeStartIndex == (startIndex + tubeNum*maxPoints+lastAge)){
			if (stationaryStart) {
				point = flowDataArray + 3*tubeStartIndex;
				renderStationary(point);
			}
			break;
		}
		//Check if the first point was preceded by stationary flag:
		if (stationaryStart){
			point = flowDataArray + 3*tubeStartIndex;
			renderStationary(point);
		}
		assert(tubeStartIndex < startIndex + tubeNum*maxPoints + lastAge);
		//OK, we are at the start of a real tube.
		//Start the colors for the start of the tube 
		if (!constMap){
			float* rgba = flowRGBAs + 4*(tubeStartIndex);
			if(isLit)
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, rgba);
			else
				glColor4fv(rgba);
		} else {
			if(isLit) glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE, constFlowColor);
			else glColor4fv(constFlowColor);
		}
		//Check if the second point is a stationary flag:
		if ((*(flowDataArray + 3*(tubeStartIndex+1))) == STATIONARY_STREAM_FLAG) {
			//If so just render the stationary symbol, and we are done.
			float *point = flowDataArray+3*tubeStartIndex;
			renderStationary(point);
			continue;
		} else if ((*(flowDataArray + 3*(tubeStartIndex+1))) == END_FLOW_FLAG) {//render nothing:
				//We could potentially have the second point be an end flow flag
				continue;
		} else {  //OK, legitimate flow line

			currentCycle[0]=currentCycle[1]=currentCycle[2]=0;
			newcycle = false;
		
			
			//data point is three floats starting at data[3*tubeStartIndex]
			//evenN is the direction the line is pointing
			vsub(flowDataArray+3*(tubeStartIndex+1), flowDataArray+3*tubeStartIndex, evenN);
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
			for (int tubeIndex = tubeStartIndex+1; tubeIndex <= startIndex+tubeNum*maxPoints+lastAge;
				tubeIndex++){
				point = flowDataArray+3*tubeIndex;
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
					vcopy(point-3, startPoint);
					vcopy(point, endPoint);
				}
				//float* startPoint = flowDataArray+3*(firstIndex);
				//float* endPoint = flowDataArray+3*(firstIndex+1);
				drawArrow(isLit, tubeIndex-1, startPoint, endPoint, currentN, currentB, currentU, radius, constMap);

				//If the arrow exited the region (in cyclic case) need to restart the points.  Note that
				//the direction vectors don't need to be changed:
				if (newcycle){
					//Establish a new cycle for a translate of the last arrow endpoint:
					newcycle = mapPeriodicCycle(point, endPoint, newCycle, currentCycle);
					assert(!newcycle);
					//Also map the startPoint to the same cycle, but we don't use the resulting cycle
					mapPeriodicCycle(point-3, startPoint, currentCycle, newCycle);
					//Then render the new (cyclically offset) arrow:
					drawArrow(isLit, tubeIndex-1, startPoint, endPoint, currentN, currentB, currentU, radius, constMap);
				}
				
				
				if ((*point) == STATIONARY_STREAM_FLAG)
					renderStationary(point-3);
			} //end of arrows along one flow line
			
		} //legitimate flow line
		

	} //end of loop over seedPoints
	//Special symbol at stationary flow:
	
		
}
//Issue OpenGL calls to draw a cylinder with orthogonal ends from one point to another.
//color indexed by startIndex and startIndex+1
//Orthogonal frame at first point is (dirVec, UVec, bVec)
//U and B are orthog to direction of cylinder, determine plane for 6 points
//
void FlowRenderer::drawArrow(bool isLit, int firstIndex, float* startPoint, float *endPoint, float* dirVec, float* bVec, float* uVec, float radius, bool constMap) {
	//Constants are needed for cosines and sines, at 60 degree intervals
	const float sines[6] = {0.f, sqrt(3.)/2., sqrt(3.)/2., 0.f, -sqrt(3.)/2., -sqrt(3.)/2.};
	const float coses[6] = {1.f, 0.5, -0.5, -1., -.5, 0.5};
	//Parameters that control arrow head:

	//float* startPoint = flowDataArray+3*(firstIndex);
	//float* endPoint = flowDataArray+3*(firstIndex+1);
	
	float* nextRGBA = flowRGBAs + 4*(firstIndex+1);
	float nextPoint[3];
	float vertexPoint[3];
	float headCenter[3];
	float startNormal[18];
	float nextNormal[18];
	float startVertex[18];
	float nextVertex[18];
	float testVec[3];
	float testVec2[3];
	int i;
	//Calculate nextPoint and vertexPoint, for arrowhead
	
	for (i = 0; i< 3; i++){
		nextPoint[i] = (1. - ARROW_LENGTH_FACTOR)*startPoint[i]+ ARROW_LENGTH_FACTOR*endPoint[i];
		//Assume a vertex angle of 45 degrees:
		vertexPoint[i] = nextPoint[i] + dirVec[i]*radius;
		headCenter[i] = nextPoint[i] - dirVec[i]*(arrowHeadRadius-radius); 
	}
	//calculate 6 points in plane orthog to dirVec, in plane of point
	for (i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		vmult(uVec, coses[i], testVec);
		vmult(bVec, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, startNormal+3*i);
		//stretch by radius to get current displacement
		vmult(startNormal+3*i, radius, startVertex+3*i);
		//add to current point
		vadd(startVertex+3*i, startPoint, startVertex+3*i);
	}
	//send nonconstant colors, will apply to entire arrow:
	if (!constMap){
		if (isLit) glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, nextRGBA);
		else glColor4fv(nextRGBA);
	}
	
	glBegin(GL_POLYGON);
	glNormal3fv(dirVec);
	for (int k = 0; k<6; k++){
		glVertex3fv(startVertex+3*k);
	}
	glEnd();
			
	//calc for endpoints:
	for (i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		vmult(uVec, coses[i], testVec);
		vmult(bVec, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, nextNormal+3*i);
		//stretch by radius to get current displacement
		vmult(nextNormal+3*i, radius, nextVertex+3*i);
		//add to current point
		vadd(nextVertex+3*i, nextPoint, nextVertex+3*i);
					
	}
	
	//Now make a triangle strip for cylinder sides:
	glBegin(GL_TRIANGLE_STRIP);
	if (constMap){
		for (i = 0; i< 6; i++){

			glNormal3fv(nextNormal+3*i);
			glVertex3fv(nextVertex+3*i);
			glNormal3fv(startNormal+3*i);
			glVertex3fv(startVertex+3*i);
		}
		//repeat first two vertices to close cylinder:
		glNormal3fv(nextNormal);
		glVertex3fv(nextVertex);
		glNormal3fv(startNormal);
		glVertex3fv(startVertex);
	} else if (isLit){
		for (i = 0; i< 6; i++){
			
			glNormal3fv(nextNormal+3*i);
			glVertex3fv(nextVertex+3*i);
			
			glNormal3fv(startNormal+3*i);
			glVertex3fv(startVertex+3*i);
		}
		//repeat first two vertices to close cylinder:
		
		glNormal3fv(nextNormal);
		glVertex3fv(nextVertex);
		
		glNormal3fv(startNormal);
		glVertex3fv(startVertex);
	} else {//unlit, color mapped
		for (i = 0; i< 6; i++){
			
			glVertex3fv(nextVertex+3*i);
			
			glVertex3fv(startVertex+3*i);
		}
		//repeat first two vertices to close cylinder:
		
		glVertex3fv(nextVertex);
		
		glVertex3fv(startVertex);
	}
	glEnd();
	//Now draw the arrow head.  First calc 6 vertices at back of arrowhead
	//Reuse startNormal and startVertex vectors
	//calc for endpoints:
	for (i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		//Can reuse them from previous (cylinder end) calculation
		vmult(uVec, coses[i], testVec);
		vmult(bVec, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, startNormal+3*i);
		//stretch by radius to get current displacement
		vmult(startNormal+3*i, arrowHeadRadius, startVertex+3*i);
		//add to current point
		vadd(startVertex+3*i, headCenter, startVertex+3*i);
		//Now tilt normals in direction of arrow:
		for (int k = 0; k<3; k++){
			startNormal[3*i+k] = 0.5*startNormal[3*i+k] + 0.5*dirVec[k];
		}
	}
	//Create a triangle fan from these 6 vertices.  Continue to use
	//previous color settings
	
	glBegin(GL_TRIANGLE_FAN);
	glNormal3fv(dirVec);
	glVertex3fv(vertexPoint);
	for (i = 0; i< 6; i++){
		glNormal3fv(startNormal+3*i);
		glVertex3fv(startVertex+3*i);
	}
	//Repeat first point to close fan:
	glNormal3fv(startNormal);
	glVertex3fv(startVertex);
	glEnd();
}
//Issue OpenGL calls to draw a cylinder from one point to another.
//center points indexed by startIndex and startIndex+1
//Orthogonal frame at first point is (dirVec, UVec, bVec)
//U and B are orthog to direction of cylinder, determine plane for 6 points
//

void FlowRenderer::drawTube(bool isLit, int tubeIndex, float* startPoint, float* endPoint, float* currentB, float* currentU, float radius, bool constMap,
							float* prevNormal, float* prevVertex, float* currentNormal, float* currentVertex) {

	//Constants are needed for cosines and sines, at 60 degree intervals
	const float sines[6] = {0.f, sqrt(3.)/2., sqrt(3.)/2., 0.f, -sqrt(3.)/2., -sqrt(3.)/2.};
	const float coses[6] = {1.f, 0.5, -0.5, -1., -.5, 0.5};
	float testVec[3],testVec2[3];
	float* prevRGBA, *nextRGBA;
	//calculate 6 points in plane orthog to currentA, in plane of endPoint:
	for (int i = 0; i<6; i++){
		//testVec and testVec2 are components of point in plane
		vmult(currentU, coses[i], testVec);
		vmult(currentB, sines[i], testVec2);
		//Calc outward normal as a sideEffect..
		//It is the vector sum of x,y components (norm 1)
		vadd(testVec, testVec2, currentNormal+3*i);
		//stretch by radius to get current displacement
		vmult(currentNormal+3*i, radius, currentVertex+3*i);
		//add to current point
		vadd(currentVertex+3*i, endPoint, currentVertex+3*i);
		//qWarning(" current Vertex, normal: %f %f %f, %f %f %f",
			//currentVertex[3*i],currentVertex[3*i+1],currentVertex[3*i+2],
			//currentNormal[3*i],currentNormal[3*i+1],currentNormal[3*i+2]);
	}
	
	if (!constMap){
		prevRGBA = flowRGBAs + 4*(tubeIndex-1);
		nextRGBA = flowRGBAs + 4*(tubeIndex);
	}
	
	//Now make a triangle strip:
	glBegin(GL_TRIANGLE_STRIP);
	if (constMap){
		for (int i = 0; i< 6; i++){

			glNormal3fv(currentNormal+3*i);
			glVertex3fv(currentVertex+3*i);
			glNormal3fv(prevNormal+3*i);
			glVertex3fv(prevVertex+3*i);
		}
		//repeat first two vertices to close cylinder:
		glNormal3fv(currentNormal);
		glVertex3fv(currentVertex);
		glNormal3fv(prevNormal);
		glVertex3fv(prevVertex);
	} else if (isLit){
		for (int i = 0; i< 6; i++){
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, nextRGBA);
			glNormal3fv(currentNormal+3*i);
			glVertex3fv(currentVertex+3*i);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, prevRGBA);
			glNormal3fv(prevNormal+3*i);
			glVertex3fv(prevVertex+3*i);
		}
		//repeat first two vertices to close cylinder:
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, nextRGBA);
		glNormal3fv(currentNormal);
		glVertex3fv(currentVertex);
		glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, prevRGBA);
		glNormal3fv(prevNormal);
		glVertex3fv(prevVertex);
	} else {//unlit, mapped
		for (int i = 0; i< 6; i++){
			glColor4fv(nextRGBA);
			glVertex3fv(currentVertex+3*i);
			glColor4fv(prevRGBA);
			glVertex3fv(prevVertex+3*i);
		}
		//repeat first two vertices to close cylinder:
		glColor4fv(nextRGBA);
		glVertex3fv(currentVertex);
		glColor4fv(prevRGBA);
		glVertex3fv(prevVertex);
	}
	glEnd();
}

//Render a symbol for stationary flowline (octahedron?)
void FlowRenderer::renderStationary(float* point){
	
	const float stationaryColor[4] = {.5f,.5f,.5f,1.f};
	
	glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, stationaryColor);
	glColor4fv(stationaryColor);
	glBegin(GL_TRIANGLES);
	glNormal3f(0.5f,.5f,.707f);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glNormal3f(-0.5f,.5f,.707f);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	
	glNormal3f(-0.5f,-.5f,.707f);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glNormal3f(0.5f,-.5f,.707f);
	glVertex3f(point[0],point[1],point[2]+stationaryRadius);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	
	glNormal3f(0.5f,.5f,-.707f);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	glNormal3f(-0.5f,.5f,-.707f);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]+stationaryRadius, point[2]);
	glNormal3f(-0.5f,-.5f,-.707f);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glVertex3f(point[0]-stationaryRadius, point[1], point[2]);
	glNormal3f(0.5f,-.5f,-.707f);
	glVertex3f(point[0],point[1],point[2]-stationaryRadius);
	glVertex3f(point[0]+stationaryRadius, point[1], point[2]);
	glVertex3f(point[0], point[1]-stationaryRadius, point[2]);
	glEnd();
	
}

//Virtual method to set dirty bits.  called by GLWindow
void FlowRenderer::setDirty(DirtyBitType type){
	
	if (type == FlowDataBit){
		setRegionValid(true);  // reset this bit so we will try to render again...
		//set all the dirty flags for all the frames
		//set the needRefresh flags too if autoRefresh is on.
		bool doRefresh = myFlowParams->refreshIsAuto();
		for (int i = 0; i<numFrames; i++){
			flowDataDirty[i] = true;
			if (doRefresh) needRefreshFlag[i] = true; 
			else needRefreshFlag[i] = false;
		}
	}
	else if (type == FlowGraphicsBit){
		//the graphics bit shouldn't be set if we need to 
		//calculate speeds:
		assert((myFlowParams->getOpacMapEntityIndex() != 2)&&(myFlowParams->getColorMapEntityIndex() != 2));
			
		for (int i = 0; i< numFrames; i++){
			flowMapDirty[i] = true;
		}
	}
}



//Rebuild the data cache, or the 
//RGBA mapping data 
//Return false on failure.
bool FlowRenderer::rebuildFlowData(int timeStep, bool doRake){
	//Establish parameters that will be saved with the cache:
	doList = myFlowParams->listEnabled();
	doRake = myFlowParams->rakeEnabled();
	bool doSpeeds = myFlowParams->getColorMapEntityIndex() == 2 || 
		myFlowParams->getOpacMapEntityIndex() == 2;
	steadyFlow = myFlowParams->flowIsSteady();
	
	minFrame = myGLWindow->getAnimationParams()->getStartFrameNumber();
	maxFrame = myGLWindow->getAnimationParams()->getEndFrameNumber();
	maxPoints = myFlowParams->calcMaxPoints();
	RegionParams* rParams = myGLWindow->getRegionParams();
	//Check if we are just doing graphics (not reintegrating flow)
	//that occurs if the map bit id dirty, but there's no need to do data.
	bool graphicsOnly = (flowMapDirty[timeStep] && 
		!(flowDataDirty[timeStep] && needRefreshFlag[timeStep]) );
	bool constColors = ((myFlowParams->getColorMapEntityIndex() + myFlowParams->getOpacMapEntityIndex())== 0);
	//Clean the cache, unless this is just a rebuild of the graphics:
	if (!graphicsOnly){
		if (rakeFlowData[timeStep]) {
			delete rakeFlowData[timeStep]; 
			rakeFlowData[timeStep] = 0;
		}
		if (listFlowData[timeStep]) {
			delete listFlowData[timeStep]; 
			listFlowData[timeStep] = 0;
		}
	}
	//rebuild the graphics arrays
	if (!constColors){
		if (rakeFlowRGBAs[timeStep]) {
			delete rakeFlowRGBAs[timeStep]; 
			rakeFlowRGBAs[timeStep] = 0;
		}
		if (listFlowRGBAs[timeStep]) {
			delete listFlowRGBAs[timeStep]; 
			listFlowRGBAs[timeStep] = 0;
		}
	}
	if (!steadyFlow){
		//Set values for special parameters used only for unsteady flow:
		seedIncrement = myFlowParams->getSeedTimeIncrement();
		if (seedIncrement < 1) seedIncrement = 1;
		startSeed = myFlowParams->getSeedTimeStart();
		endSeed = myFlowParams->getSeedTimeEnd();
		firstDisplayAge = myFlowParams->getFirstDisplayFrame();
		lastDisplayAge = myFlowParams->getLastDisplayFrame();
		float objectsPerFlowline = (float)myFlowParams->getObjectsPerFlowline();
		objectsPerTimestep = (objectsPerFlowline+1.f)/(float)(maxFrame - minFrame);
		numInjections = 1+ (endSeed - startSeed)/seedIncrement;
	} else {
		numInjections = 1;
	}
	
	//Get the rake flow data if needed:
	if (doRake){
		
		numSeedPoints = myFlowParams->calcNumSeedPoints(true, timeStep);
		int flowDataSize = maxPoints*numSeedPoints*numInjections;
		if (!graphicsOnly){
			//reallocate flowDataArray and pass it in:
			if (!rakeFlowData[timeStep]){
				rakeFlowData[timeStep] = new float[3*flowDataSize];
			}
			if ((!rakeFlowRGBAs[timeStep])&&!constColors){
				rakeFlowRGBAs[timeStep] = new float[4*flowDataSize];
			}
			bool OK = myFlowParams->regenerateFlowData(timeStep, minFrame, true, rParams, rakeFlowData[timeStep],rakeFlowRGBAs[timeStep]);
			if (!OK) {
				setRegionValid(false);
				delete rakeFlowData[timeStep];
				rakeFlowData[timeStep] = 0;
				if (rakeFlowRGBAs[timeStep]) delete rakeFlowRGBAs[timeStep];
				rakeFlowRGBAs[timeStep] = 0;
				return false;
			}
		} else { //just map colors
		
			//Note that colors can't be mapped here if the speeds are needed
			assert (!doSpeeds);
			if (!constColors){
				rakeFlowRGBAs[timeStep] = new float[flowDataSize*4];
				myFlowParams->mapColors(0, timeStep, minFrame, numSeedPoints, rakeFlowData[timeStep], rakeFlowRGBAs[timeStep], true);
			}
		}
	} else { //do seed list.  Same as above, but with seed list parameter
		
		numSeedPoints = myFlowParams->calcNumSeedPoints(false, timeStep);
		int flowDataSize = maxPoints*numSeedPoints*numInjections;
		if (!graphicsOnly){
			//reallocate flowDataArray and pass it in:
			if (!listFlowData[timeStep]){
				listFlowData[timeStep] = new float[3*flowDataSize];
			}
			if ((!listFlowRGBAs[timeStep])&&!constColors){
				listFlowRGBAs[timeStep] = new float[4*flowDataSize];
			}
			bool OK = myFlowParams->regenerateFlowData(timeStep, minFrame, false, rParams, listFlowData[timeStep],listFlowRGBAs[timeStep]);
			if (!OK) {
				setRegionValid(false);
				delete listFlowData[timeStep];
				listFlowData[timeStep] = 0;
				if (listFlowRGBAs[timeStep]) delete listFlowRGBAs[timeStep];
				listFlowRGBAs[timeStep] = 0;
				return false;
			}
		} else { //just map colors
		
			//Note that colors can't be mapped here if the speeds are needed
			assert (!doSpeeds);
			if (!constColors){
				listFlowRGBAs[timeStep] = new float[flowDataSize*4];
				myFlowParams->mapColors(0, timeStep, minFrame, numSeedPoints, listFlowData[timeStep], listFlowRGBAs[timeStep], false);
			}
		}
	}
	return true;
}

void FlowRenderer::
setAllNeedRefresh(bool value){
	int mxframe = DataStatus::getInstance()->getNumTimesteps();
	for (int i = 0; i< mxframe; i++){
		needRefreshFlag[i] = value;
	}
}
//Map periodic coords into data extents.
//This is called each time a new point is rendered along a stream line.  oldcycle is initially (0,0,0)
//establishing that the first point is rendered inside the base cycle (even if it is outside, as with a pre-point).
//If the first point is out, then the second point must be in, so oldcycle = 0,0,0 is also passed in with the
//second point.
//If the first point is inside, the second point will pass in oldcycle from the first call (which must be (0,0,0)).
//In any case, this function should not be called for the first point of the streamline, but it must be called for the
//second and subsequent points.  
//Whenever a streamline exits the region, this returns true, indicating that the current streamline must be stopped
//and then the next streamline is started by calling this function again.
//
bool FlowRenderer::mapPeriodicCycle(float origCoord[3], float mappedCoord[3], int oldcycle[3], int newcycle[3]){
	const float* extents = DataStatus::getInstance()->getExtents();
	bool changed = false;
	for (int i = 0; i<3; i++){
		mappedCoord[i] = origCoord[i];
		newcycle[i] = oldcycle[i];
		if (myFlowParams->getPeriodicDim(i)){
			//correct according to oldcycle:
			mappedCoord[i] -= (((float)oldcycle[i])*(extents[i+3]-extents[i]));
			//Then make newcycle for additional correction:
			float mapCoord = mappedCoord[i];
			while (mapCoord < extents[i]) {
				mapCoord += (extents[i+3]-extents[i]); 
				newcycle[i]--; 
				changed=true;
			}
			while (mapCoord > extents[i+3]) {
				mapCoord -= (extents[i+3]-extents[i]); 
				newcycle[i]++; 
				changed=true;
			}
		}
	}
	return changed;
}

