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

/*!
  Create a FlowRenderer widget
*/
using namespace VAPoR;
FlowRenderer::FlowRenderer(VizWin* vw )
:Renderer(vw)
{
    maxPoints = 0;
	numSeedPoints = 0;
	numInjections = 0;
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
	GLfloat white_light[] = {1.f,1.f,1.f,1.f};
	GLfloat lmodel_ambient[] = {1.0f, 1.0f, 1.0f, 1.0f};
	float constFlowColor[4];
	int winNum = myVizWin->getWindowNum();
	FlowParams* myFlowParams = VizWinMgr::getInstance()->getFlowParams(winNum);
	AnimationParams* myAnimationParams = VizWinMgr::getInstance()->getAnimationParams(winNum);
	int currentFrameNum = myAnimationParams->getCurrentFrameNumber();
	int timeStep = currentFrameNum;
	if (!myFlowParams->flowIsSteady()) timeStep = 0;
	bool constColors = ((myFlowParams->getColorMapEntityIndex() + myFlowParams->getOpacMapEntityIndex()) == 0);
	//Do we need to regenerate the flow data?
	if (myFlowParams->flowDataIsDirty(timeStep)){
		
		flowDataArray = myFlowParams->regenerateFlowData(timeStep);
		maxPoints = myFlowParams->getMaxPoints();
		firstDisplayFrame = myFlowParams->getFirstDisplayFrame();
		lastDisplayFrame = myFlowParams->getLastDisplayFrame();
		numSeedPoints = myFlowParams->getNumSeedPoints();
		numInjections = myFlowParams->getNumInjections();
		myVizWin->setFlowDirty(false);
	} 
	if (!constColors && myFlowParams->flowMappingIsDirty(timeStep)){
		flowRGBAs = myFlowParams->getRGBAs(timeStep);
	} 

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
	
	//Setup constant color/opacity by default
	
	myFlowParams->getMapperFunc()->mapPointToRGBA(0.0f, constFlowColor);
	
	
	float diam = myFlowParams->getShapeDiameter();
	//Don't allow zero diameter, it causes OpenGL error code 1281
	if (diam < 1.e-10) diam = 1.e-10f;

	//Set up lighting, if we are rendering tubes or lines:
	int nLights = 0;
	if (myFlowParams->getShapeType() != 1) {//rendering tubes/lines/arrows
		
		ViewpointParams* vpParams = VizWinMgr::getInstance()->getViewpointParams(winNum);
		nLights = vpParams->getNumLights();
		if (nLights > 0){
			glShadeModel(GL_SMOOTH);
			glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, constFlowColor);
			glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, constFlowColor);
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
			glColor4fv(constFlowColor);
		}
	} else {//points are not lit..
		glDisable(GL_LIGHTING);
		glColor4fv(constFlowColor);
	}
	//If we are doing unsteady flow, handle setup differently:
	if (myFlowParams->flowIsSteady()){
		if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
				
			if (diam < 0.5f){//Render as lines, not cylinders
				renderCurves(diam, (nLights>0), 0, maxPoints-1, 0, constColors);
			
			} else { //render as cylinders
				//Determine cylinder radius in actual coords.
				//One voxel is (full region size)/(region array size)
				RegionParams* rParams = VizWinMgr::getInstance()->getRegionParams(winNum);
				float rad = 0.5*diam*(rParams->getFullDataExtent(3)- rParams->getFullDataExtent(0))/
					rParams->getFullSize()[0];
				glPolygonMode(GL_FRONT, GL_FILL);
				//Render all tubes
				//Note that lastDisplayFrame is maxPoints -1
				//and firstDisplayFrame is 0
				//
				renderTubes(rad, (nLights > 0), 0, maxPoints-1, 0,constColors);
				
			}
		} else { //rendering points (or arrows?)
			//just convert the flow data to a set of points..
			renderPoints(diam, 0, maxPoints-1, 0, constColors);
		}
	} else { //unsteady flow:
		//Determine first and last seeding that is visible.
		//The portion of a flow that is visible is that portion whose age
		//goes from minAge to maxAge.  In other words, go from the
		//part corresponding to times 
		//currentFrame-maxAge to currentFrame-minAge
		//This is reduced on the front end if currentFrame-maxAge
		//precedes seedStartFrame.
		//It is reduced on the back end if the calculation is not complete.
		
		//Seeding no. K is injected at frame
		//startFrame + (seedingIncrement*(injectionNum -1))
		//Can ignore if this number is > currentFrame
		// I.e., want injectionNum <= 1+ (currentFrame - startFrame)/increment)
		int seedingIncrement = myFlowParams->getSeedingIncrement();
		

		//Check if first injection has happened yet:
		int startFrame = myFlowParams->getStartFrame();
		if (currentFrameNum < startFrame) return;
		int firstInjectionNum = 1;
		int lastInjectionFrame = min(currentFrameNum, myFlowParams->getLastSeeding());
		int lastInjectionNum = 1 + (lastInjectionFrame - startFrame)/seedingIncrement;
		
		int objectsPerTimestep = myFlowParams->getObjectsPerTimestep();

		//Do special case of just one seeding:
		if (seedingIncrement <= 0 || numInjections <= 1){
			firstInjectionNum = 1;
			lastInjectionNum = 1;
		} 
		//Loop over the active seedings.  A value of injectionNum corresponds to an
		//injection at time startFrame + (injectionNum-1)*seedingIncrement
		for (int injectionNum = firstInjectionNum; injectionNum <= lastInjectionNum; injectionNum++){
			int flowStartFrame = myFlowParams->getStartFrame()+(injectionNum-1)*seedingIncrement;
			//Use the display interval to determine what part of the flow is to be visible
			int lastFrame = (currentFrameNum - flowStartFrame) + lastDisplayFrame;
			int firstFrame = currentFrameNum - flowStartFrame - firstDisplayFrame;
			//The rendered geometry depends on how many objects per timestep:
			int firstGeom = (int)(firstFrame*objectsPerTimestep + 0.5f);
			int lastGeom = (int)(lastFrame*objectsPerTimestep+0.5f);
			if (firstGeom < 0) firstGeom = 0;
			if (lastGeom < 0) lastGeom = 0;
			if (lastGeom >= maxPoints) lastGeom = maxPoints-1;
			if (firstGeom > lastGeom) continue;
			
			if (myFlowParams->getShapeType() == 0) {//rendering tubes/lines:
					
				if (diam < 0.5f){//Render as lines, not cylinders
					renderCurves(diam, (nLights>0), firstGeom, lastGeom,
						maxPoints*numSeedPoints*(injectionNum-1),constColors);
				
				} else { //render as cylinders
					//Determine cylinder radius in actual coords.
					//One voxel is (full region size)/(region array size)
					RegionParams* rParams = VizWinMgr::getInstance()->getRegionParams(winNum);
					float rad = 0.5*diam*(rParams->getFullDataExtent(3)- rParams->getFullDataExtent(0))/
						rParams->getFullSize()[0];
					glPolygonMode(GL_FRONT, GL_FILL);
					//Render all tubes
					renderTubes(rad, (nLights > 0), firstGeom, lastGeom,
						maxPoints*numSeedPoints*(injectionNum-1), constColors);
					
				}
			} else { //rendering points (or arrows?)
				//just convert the flow data to a set of points..
				renderPoints(diam, firstGeom, lastGeom,
						maxPoints*numSeedPoints*(injectionNum-1), constColors);
			}
		}
	}

	glDisable(GL_LIGHTING);
	glDisable(GL_BLEND);
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
//  Issue OpenGL calls for point list
void FlowRenderer::
renderPoints(float radius, int firstAge, int lastAge, int startIndex, bool constMap){
	
	//just convert the flow data to a set of points..
	glPointSize(radius);
	glDisable(GL_LIGHTING);
	glBegin (GL_POINTS);
	for (int i = 0; i< numSeedPoints; i++){
		
		for (int j = firstAge; j<=lastAge; j++){
			float* point = flowDataArray+ 3*(startIndex+j+ maxPoints*i);
			if (*point == END_FLOW_FLAG) break;
			qWarning("point is %f %f %f", *point, *(point+1), *(point+2));
			if (!constMap){
				float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
				glColor4fv(rgba);
			}
			glVertex3fv(point);
		}	
	}
	glEnd();
	
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
			glBegin (GL_LINE_STRIP);
			for (int j = firstAge; j<=lastAge; j++){
				//For each point after first, calc vector from prev vector to this one
				//then calculate corresponding normal
				float* point = flowDataArray + 3*(startIndex+ j+ maxPoints*i);
				if (*point == END_FLOW_FLAG) break;
				if (j > firstAge){
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
				glVertex3fv(point);
			}
			glEnd();
		}
	} else { //No lights
		//just convert the flow data to a set of lines...
		glDisable(GL_LIGHTING);
		for (int i = 0; i< numSeedPoints; i++){
			glBegin (GL_LINE_STRIP);
			for (int j = firstAge; j<=lastAge; j++){
				float* point = flowDataArray +  3*(startIndex+ j+ maxPoints*i);
				if (*point == END_FLOW_FLAG) break;
				glVertex3fv(point);
				if (!constMap){
					float* rgba = flowRGBAs + 4*(startIndex + j + maxPoints*i);
					glColor4fv(rgba);
				}
			}
			glEnd();
		}
	}//end no lights
}
//  Issue OpenGL calls for a cylindrical (hexagonal cross-section) tube 
//  following the stream or streak line
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
		//Need at least two points to do anything:
		if ((*(flowDataArray + 3*(startIndex+tubeNum*maxPoints)) == END_FLOW_FLAG) ||
			(*(flowDataArray + 3*(startIndex+tubeNum*maxPoints+1))) == END_FLOW_FLAG) continue;
		
		int tubeStartIndex = 3*(startIndex + tubeNum*maxPoints+firstAge);
		//data point is three floats starting at data[tubeStartIndex]
		//evenA is the direction the line is pointing
		vsub(flowDataArray+(tubeStartIndex+3), flowDataArray+tubeStartIndex, evenA);
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
			vadd(evenVertex+3*i, flowDataArray+tubeStartIndex, evenVertex+3*i);
		}
		//Draw an end-cap on the cylinder:
		if (!constMap){
			float* rgba = flowRGBAs + 4*(startIndex + tubeNum*maxPoints+firstAge);
			if(isLit)
				glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, rgba);
			else
				glColor4fv(rgba);
		}
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



		for (int pointNum = firstAge+ 1; pointNum <= lastAge; pointNum++){
			float* point = flowDataArray+3*(startIndex+tubeNum*maxPoints+pointNum);
			if (*point == END_FLOW_FLAG) break;
			//Toggle the meaning of "current" and "prev"
			if (0 == (pointNum - firstAge)%2) {
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
				prevRGBA = flowRGBAs + 4*(startIndex+tubeNum*maxPoints+pointNum-1);
				nextRGBA = flowRGBAs + 4*(startIndex+tubeNum*maxPoints+pointNum);
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
		//Draw an end-cap on the cylinder:
		
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
	} //end of loop over seedPoints
		
}





