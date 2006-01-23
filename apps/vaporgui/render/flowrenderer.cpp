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

#include "flowrenderer.h"
#include "vapor/VaporFlow.h"
#include "glwindow.h"
#include "trackball.h"
#include "glutil.h"
#include "vizwin.h"
#include <math.h>
#include <qgl.h>
#include <qcolor.h>
#include "renderer.h"
#include "mapperfunction.h"

//Constants used for arrow design:
	//VERTEX_ANGLE = 45 degrees (angle between direction vector and head edge
#define ARROW_LENGTH_FACTOR  0.90f //fraction of full length used by cylinder
#define ARROW_HEAD_WIDTH_FACTOR 3.f //radius of arrowhead compared to cylinder radius
#define MIN_ARROW_HEAD_RADIUS 3.f //minimum in voxels of head radius
#define MIN_STATIONARY_RADIUS 2.f //minimum in voxels of stationary octahedron
/*!
  Create a FlowRenderer widget
*/
using namespace VAPoR;
FlowRenderer::FlowRenderer(VizWin* vw, int flowType )
:Renderer(vw)
{
    maxPoints = 0;
	
	numInjections = 0;
	steadyFlow = (flowType == 0);
	for (int i = 0; i<4; i++) constFlowColor[i] = 1.f;


}


/*!
  Release allocated resources
*/

FlowRenderer::~FlowRenderer()
{
}


// Perform the rendering
//
  

void FlowRenderer::paintGL()
{
	
	int winNum = myVizWin->getWindowNum();
	FlowParams* myFlowParams = VizWinMgr::getInstance()->getFlowParams(winNum);
	AnimationParams* myAnimationParams = VizWinMgr::getInstance()->getAnimationParams(winNum);
	
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	steadyFlow = myFlowParams->flowIsSteady();
	int timeStep = steadyFlow ? currentFrameNum : 0 ;
	//Note:  always check the validity of the data associated with the
	//flowtype that was most recently used to construct the data.
	//If the user changes the flowType to unsteady and a nonzero timestep
	//is being rendered, the flow won't be recalculated until that
	//timestep is invalidated.  Fortunately "refresh" invalidates all
	//timesteps.
	
	//Colors can be changed without regenerating flow data.
	bool constColors = ((myFlowParams->getColorMapEntityIndex() + myFlowParams->getOpacMapEntityIndex()) == 0);
	//Need to do rendering on either the listFlowData or the rakeFlowData or both.
	//
	
	//Do we need to regenerate any flow data?
	if (myFlowParams->flowDataIsDirty(timeStep)){
		//get the necessary state from the flowParams:
		
		
		
		
		
		maxFrame = myFlowParams->getMaxFrame();
		minFrame = myFlowParams->getMinFrame();
		
		if (!steadyFlow){
			//Special parameters used only for unsteady flow:
			seedIncrement = myFlowParams->getSeedingIncrement();
			if (seedIncrement < 1) seedIncrement = 1;
			startSeed = myFlowParams->getSeedStartFrame();
			endSeed = myFlowParams->getLastSeeding();
			firstDisplayAge = myFlowParams->getFirstDisplayAge();
			lastDisplayAge = myFlowParams->getLastDisplayAge();
			float objectsPerFlowline = (float)myFlowParams->getObjectsPerFlowline();
			objectsPerTimestep = (objectsPerFlowline+1.f)/(float)(maxFrame - minFrame);
		}
	}
	
	//Get the rake flow data:
	if (myFlowParams->rakeEnabled()){
		flowDataArray = myFlowParams->getFlowData(timeStep, true);
		if (!flowDataArray)
			flowDataArray = myFlowParams->regenerateFlowData(timeStep, true);
		
		if (!constColors){
			flowRGBAs = myFlowParams->getRGBAs(timeStep, true);
		} else {
			constFlowColor[3] = myFlowParams->getConstantOpacity();
			QRgb constRgb = myFlowParams->getConstantColor();

			constFlowColor[0] = qRed(constRgb)/255.f;
			constFlowColor[1] = qGreen(constRgb)/255.f;
			constFlowColor[2] = qBlue(constRgb)/255.f;
		}
		numSeedPoints = myFlowParams->getNumRakeSeedPointsUsed();
		maxPoints = myFlowParams->getMaxPoints();
		numInjections = 1;
		renderFlowData(constColors,currentFrameNum);
	}
	//Do the same for seed list
	if (myFlowParams->listEnabled()){
		flowDataArray = myFlowParams->getFlowData(timeStep, false);
		if (!flowDataArray)
			flowDataArray = myFlowParams->regenerateFlowData(timeStep, false);
		
		if (!constColors){
			flowRGBAs = myFlowParams->getRGBAs(timeStep, false);
		} else {
			constFlowColor[3] = myFlowParams->getConstantOpacity();
			QRgb constRgb = myFlowParams->getConstantColor();

			constFlowColor[0] = qRed(constRgb)/255.f;
			constFlowColor[1] = qGreen(constRgb)/255.f;
			constFlowColor[2] = qBlue(constRgb)/255.f;
		}
		numSeedPoints = myFlowParams->getNumListSeedPointsUsed(timeStep);
		maxPoints = myFlowParams->getMaxPoints();
		numInjections = 1;
		renderFlowData(constColors,currentFrameNum);
	}
}

void FlowRenderer::
renderFlowData(bool constColors, int currentFrameNum){
	int winNum = myVizWin->getWindowNum();
	RegionParams* myRegionParams = VizWinMgr::getInstance()->getRegionParams(winNum);
	FlowParams* myFlowParams = VizWinMgr::getInstance()->getFlowParams(winNum);
	GLfloat white_light[] = {1.f,1.f,1.f,1.f};
	GLfloat lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
	
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
	const float* fullExtent = Session::getInstance()->getExtents();
	const size_t* fullDims = Session::getInstance()->getFullDataDimensions();
	voxelSize = Max((fullExtent[5]-fullExtent[2])/fullDims[0],
		Max((fullExtent[4]-fullExtent[1])/fullDims[1], (fullExtent[3]-fullExtent[0])/fullDims[2]));
		
	//stationary radius is radius of stationary point symbol in user coords
	if (diam > 2*MIN_STATIONARY_RADIUS)
		stationaryRadius = voxelSize*0.5*diam;
	else stationaryRadius = voxelSize*MIN_STATIONARY_RADIUS;
	float userRadius = 0.5f*diam*voxelSize;
	arrowHeadRadius = ARROW_HEAD_WIDTH_FACTOR*userRadius;
	float minHeadRad = MIN_ARROW_HEAD_RADIUS*voxelSize;
	if (arrowHeadRadius < minHeadRad) arrowHeadRadius = minHeadRad;
	
	

	//Set up lighting, if we are rendering tubes or lines:
	int nLights = 0;
	if (myFlowParams->getShapeType() != 1) {//rendering tubes/lines/arrows
		
		ViewpointParams* vpParams = VizWinMgr::getInstance()->getViewpointParams(winNum);
		nLights = vpParams->getNumLights();
		if (nLights > 0){
			glShadeModel(GL_SMOOTH);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, constFlowColor);
			const float specColor[4] = {.2f,.2f,.2f,1.f};
			//Specify low specular color

			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
			glLightfv(GL_LIGHT0, GL_POSITION, vpParams->getLightDirection(0));
			glLightfv(GL_LIGHT0, GL_DIFFUSE, white_light);
			//glLightfv(GL_LIGHT0, GL_SPECULAR, white_light);
			//glLightfv(GL_LIGHT0, GL_AMBIENT, white_light);
			glLightModelfv(GL_LIGHT_MODEL_AMBIENT, lmodel_ambient);
			glEnable(GL_LIGHTING);
			glEnable(GL_LIGHT0);
			if (nLights > 1){
				glLightfv(GL_LIGHT1, GL_POSITION, vpParams->getLightDirection(1));
				glLightfv(GL_LIGHT1, GL_DIFFUSE, white_light);
				//glLightfv(GL_LIGHT1, GL_SPECULAR, white_light);
				glEnable(GL_LIGHT1);
			}
			if (nLights > 2){
				glLightfv(GL_LIGHT2, GL_POSITION, vpParams->getLightDirection(2));
				glLightfv(GL_LIGHT2, GL_DIFFUSE, white_light);
				//glLightfv(GL_LIGHT2, GL_SPECULAR, white_light);
				glEnable(GL_LIGHT2);
			}
		} else {
			glDisable(GL_LIGHTING); //No lights
			
		}
	} else {//points are not lit..
		glDisable(GL_LIGHTING);
		
	}
	//If we are doing unsteady flow, handle setup differently:
	if (steadyFlow){
		if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
				
			if (diam < 0.01f){//Render as lines, not cylinders
				renderCurves(diam, (nLights>0), 0, maxPoints-1, 0, constColors);
			
			} else { //render as cylinders
				//Determine cylinder radius in actual coords.
				//One voxel is (full region size)/(region array size)
				
				glPolygonMode(GL_FRONT, GL_FILL);
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
			
			
			glPolygonMode(GL_FRONT, GL_FILL);
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
					
				if (diam < 0.01f){//Render as lines, not cylinders
					renderCurves(diam, (nLights>0), firstGeom, lastGeom,
						maxPoints*numSeedPoints*injectionNum,constColors);
				
				} else { //render as cylinders
					
					glPolygonMode(GL_FRONT, GL_FILL);
					//Render all tubes
					renderTubes(userRadius, (nLights > 0), firstGeom, lastGeom,
						maxPoints*numSeedPoints*injectionNum, constColors);
					
				}
			} else if(myFlowParams->getShapeType() == 1) { //rendering points 
				//just convert the flow data to a set of points..
				renderPoints(diam, firstGeom, lastGeom,
						maxPoints*numSeedPoints*injectionNum, constColors);
			} else { //rendering arrows:
				
				glPolygonMode(GL_FRONT, GL_FILL);
				
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
}


/*!
  Set up the OpenGL rendering state, and define display list
*/

void FlowRenderer::initializeGL()
{
	myGLWindow->makeCurrent();
	myGLWindow->qglClearColor( Qt::black ); 		// Let OpenGL clear to black
   
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
			glVertex3fv(point);
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
	if (firstAge >= lastAge) return;
	glLineWidth(radius);
	if (isLit){
		//Get light direction vector of first light:
		ViewpointParams* vpParams = VizWinMgr::getInstance()->getViewpointParams(myVizWin->getWindowNum());
		const float* lightDir = vpParams->getLightDirection(0);
		for (int i = 0; i< numSeedPoints; i++){
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
				}
				else{
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
				}
				if (!constMap){
					float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
					glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, rgba);
				}
				if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
					glVertex3fv(point);//Terminate current curve
					glEnd();
					renderStationary(point);
					endGL = true;
					break;
				}
				glVertex3fv(point);
			}
			if(!endGL) glEnd();
		}
	} else { //No lights
		//just convert the flow data to a set of lines...
		glDisable(GL_LIGHTING);
		for (int i = 0; i< numSeedPoints; i++){
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
					renderStationary(point);
					stationaryStart = false;
				}
				if(firstPoint){
					firstPoint = false;
					if(constMap) glColor4fv(constFlowColor);
					glBegin(GL_LINE_STRIP);
					endGL = false;
				}
				if (!constMap){
					float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
					glColor4fv(rgba);
				}
				if (j<lastAge && *(point+3) == STATIONARY_STREAM_FLAG){
					glVertex3fv(point);
					glEnd();//Terminate current curve
					
					renderStationary(point);
					endGL = true;
					break;
				}
				glVertex3fv(point);
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
			//Draw an end-cap on the cylinder:
			
			glBegin(GL_POLYGON);
			glNormal3fv(evenA);
			for (int k = 0; k<6; k++){
				glVertex3fv(evenVertex+3*k);
			}
			glEnd();
			//Now loop over points, starting with no. 1.
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
			float* prevRGBA, *nextRGBA;


			//float* point;
			//for (int pointNum = firstAge+ 1; pointNum <= lastAge; pointNum++){
			for (int tubeIndex = tubeStartIndex+1; tubeIndex <= startIndex+tubeNum*maxPoints+lastAge;
				tubeIndex++){
				point = flowDataArray+3*tubeIndex;
				if (*point == END_FLOW_FLAG || *point == STATIONARY_STREAM_FLAG) break;
				//Toggle the meaning of "current" and "prev"
				//First time thru current is odd
				if (0 == (tubeIndex - tubeStartIndex)%2) {
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
				len = vdot(currentU,currentU);
				if (len == 0.f){
					//If U is in direction of A, the previous A should work
					vcopy(prevA, currentU);
					len = vdot(currentU, currentU);
					assert(len != 0.f);
				} 
				vscale(currentU, 1.f/sqrt(len));
				vcross(currentU, currentA, currentB);

				//Now calculate 6 points in plane orthog to currentA, in plane of point:
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
					vadd(currentVertex+3*i, point, currentVertex+3*i);
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
			//Draw an end-cap on the cylinder, and potentially a stationary symbol:
			
			if (!constMap){
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
		
			
			//data point is three floats starting at data[3*tubeStartIndex]
			//evenN is the direction the line is pointing
			vsub(flowDataArray+3*(tubeStartIndex+1), flowDataArray+3*tubeStartIndex, evenN);
			//Normalize even
			len = vdot(evenN,evenN);
			if (len == 0.f){//If 2nd is same as first set default normal
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
				

				drawArrow(isLit, tubeIndex-1, currentN, currentB, currentU, radius, constMap);

				
				
				if ((*point) == STATIONARY_STREAM_FLAG)
					renderStationary(point-3);
			} //end of arrows along one flow line
			
		} //legitimate flow line
		

	} //end of loop over seedPoints
	//Special symbol at stationary flow:
	
		
}
//Issue OpenGL calls to draw a cylinder with orthogonal ends from one point to another.
//center points indexed by startIndex and startIndex+1
//Orthogonal frame at first point is (dirVec, UVec, bVec)
//U and B are orthog to direction of cylinder, determine plane for 6 points
//
void FlowRenderer::drawArrow(bool isLit, int firstIndex, float* dirVec, float* bVec, float* uVec, float radius, bool constMap) {
	//Constants are needed for cosines and sines, at 60 degree intervals
	const float sines[6] = {0.f, sqrt(3.)/2., sqrt(3.)/2., 0.f, -sqrt(3.)/2., -sqrt(3.)/2.};
	const float coses[6] = {1.f, 0.5, -0.5, -1., -.5, 0.5};
	//Parameters that control arrow head:

	float* startPoint = flowDataArray+3*(firstIndex);
	float* endPoint = flowDataArray+3*(firstIndex+1);
	
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



