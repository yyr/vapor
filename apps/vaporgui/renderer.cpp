//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		renderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the Renderer class.
//		A pure virtual class that is implemented for each renderer.
//		Methods are called by the glwindow class as needed.
//
#include <qpixmap.h>
#include <qpainter.h>

#include "renderer.h"
#include "vapor/DataMgr.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "vizwinmgr.h"
#include "mainform.h"
#include "session.h"
#include "vizwin.h"
#include "glutil.h"
#include "transferfunction.h"

using namespace VAPoR;

Renderer::Renderer( VizWin* vw )
{
	//Establish the data sources for the rendering:
	//
	myVizWin = vw;
    myGLWindow = vw->getGLWindow();
	savedNumXForms = -1;
}
//Issue OpenGL commands to draw a grid of lines of the full domain.
//Grid resolution is up to 2x2x2
//
void Renderer::renderDomainFrame(float* extents, float* minFull, float* maxFull){

	int i; 
	int numLines[3];
	float regionSize, fullSize[3], modMin[3],modMax[3];
	setRegionFrameColor( myVizWin->getRegionFrameColor());
	
	
	//Instead:  either have 2 or 1 lines in each dimension.  2 if the size is < 1/3
	for (i = 0; i<3; i++){
		regionSize = extents[i+3]-extents[i];
		//Stretch size by 1%
		fullSize[i] = (maxFull[i] - minFull[i])*1.01;
		float mid = 0.5f*(maxFull[i]+minFull[i]);
		modMin[i] = mid - 0.5f*fullSize[i];
		modMax[i] = mid + 0.5f*fullSize[i];
		if (regionSize < fullSize[i]*.3) numLines[i] = 2;
		else numLines[i] = 1;
	}
	

	glColor3fv(regionFrameColor);	   
    glLineWidth( 2.0 );
	//Now draw the lines.  Divide each dimension into numLines[dim] sections.
	
	int x,y,z;
	//Do the lines in each z-plane
	//Turn on writing to the z-buffer
	glDepthMask(GL_TRUE);
	
	glBegin( GL_LINES );
	for (z = 0; z<=numLines[2]; z++){
		float zCrd = modMin[2] + ((float)z/(float)numLines[2])*fullSize[2];
		//Draw lines in x-direction for each y
		for (y = 0; y<=numLines[1]; y++){
			float yCrd = modMin[1] + ((float)y/(float)numLines[1])*fullSize[1];
			
			glVertex3f(  modMin[0],  yCrd, zCrd );   
			glVertex3f( modMax[0],  yCrd, zCrd );
			
		}
		//Draw lines in y-direction for each x
		for (x = 0; x<=numLines[0]; x++){
			float xCrd = modMin[0] + ((float)x/(float)numLines[0])*fullSize[0];
			
			glVertex3f(  xCrd, modMin[1], zCrd );   
			glVertex3f( xCrd, modMax[1], zCrd );
			
		}
	}
	//Do the lines in each y-plane
	
	for (y = 0; y<=numLines[1]; y++){
		float yCrd = modMin[1] + ((float)y/(float)numLines[1])*fullSize[1];
		//Draw lines in x direction for each z
		for (z = 0; z<=numLines[2]; z++){
			float zCrd = modMin[2] + ((float)z/(float)numLines[2])*fullSize[2];
			
			glVertex3f(  modMin[0],  yCrd, zCrd );   
			glVertex3f( modMax[0],  yCrd, zCrd );
			
		}
		//Draw lines in z direction for each x
		for (x = 0; x<=numLines[0]; x++){
			float xCrd = modMin[0] + ((float)x/(float)numLines[0])*fullSize[0];
		
			glVertex3f(  xCrd, yCrd, modMin[2] );   
			glVertex3f( xCrd, yCrd, modMax[2]);
			
		}
	}
	
	//Do the lines in each x-plane
	for (x = 0; x<=numLines[0]; x++){
		float xCrd = modMin[0] + ((float)x/(float)numLines[0])*fullSize[0];
		//Draw lines in y direction for each z
		for (z = 0; z<=numLines[2]; z++){
			float zCrd = modMin[2] + ((float)z/(float)numLines[2])*fullSize[2];
			
			glVertex3f(  xCrd, modMin[1], zCrd );   
			glVertex3f( xCrd, modMax[1], zCrd );
			
		}
		//Draw lines in z direction for each y
		for (y = 0; y<=numLines[1]; y++){
			float yCrd = modMin[1] + ((float)y/(float)numLines[1])*fullSize[1];
			
			glVertex3f(  xCrd, yCrd, modMin[2] );   
			glVertex3f( xCrd, yCrd, modMax[2]);
			
		}
	}
	
	glEnd();//GL_LINES
	
	
	

}

float* Renderer::cornerPoint(float* extents, int faceNum){
	if(faceNum&1) return extents+3;
	else return extents;
}
bool Renderer::faceIsVisible(float* extents, float* viewerCoords, int faceNum){
	float temp[3];
	//Calc vector from a corner to the viewer.  Face is visible if
	//the outward normal to the face points in same direction as this vector.
	vsub (viewerCoords, cornerPoint(extents, faceNum), temp);
	switch (faceNum) { //visible if temp points in direction of outward normal:
		case (0): //norm is 0,0,-1
			return (temp[2]<0.f);
		case (1):
			return (temp[2]>0.f);
		case (2): //norm is 0,-1,0
			return (temp[1]<0.f);
		case (3):
			return (temp[1]>0.f);
		case (4): //norm is -1,0,0
			return (temp[0]<0.f);
		case (5):
			return (temp[0]>0.f);
		default: 
			assert(0);
			return false;
	}
}
void Renderer::drawSubregionBounds(float* extents) {
	setSubregionFrameColor(myVizWin->getSubregionFrameColor());
	glLineWidth( 2.0 );
	glColor3fv(subregionFrameColor);
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[2]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glEnd();		
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[0], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[2]);
	glVertex3f(extents[3], extents[4], extents[5]);
	glVertex3f(extents[0], extents[4], extents[5]);
	glEnd();
	glBegin(GL_LINE_LOOP);
	glVertex3f(extents[0], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[2]);
	glVertex3f(extents[3], extents[1], extents[5]);
	glVertex3f(extents[0], extents[1], extents[5]);
	glEnd();
}
void Renderer::drawRegionFace(float* extents, int faceNum, bool isSelected){
	glLineWidth( 2.0 );
	glEnable (GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glPolygonMode(GL_FRONT, GL_FILL);
	if (isSelected)
			glColor4f(.8f,.8f,0.f,.6f);
		else 
			glColor4f(.8f,.8f,.8f,.2f);
	switch (faceNum){
		case 4://Do left (x=0)
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		
		case 5:
		//do right 
			glBegin(GL_QUADS);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glEnd();
			break;
		case(3)://top
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			break;
		case(2)://bottom
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glEnd();
			break;
	
		case(0):
			//back
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[2]);
			glVertex3f(extents[3], extents[1], extents[2]);
			glVertex3f(extents[3], extents[4], extents[2]);
			glVertex3f(extents[0], extents[4], extents[2]);
			glEnd();
			break;
		case(1):
			//do the front:
			//
			glBegin(GL_QUADS);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			glColor3fv(subregionFrameColor);
			glBegin(GL_LINE_LOOP);
			glVertex3f(extents[0], extents[1], extents[5]);
			glVertex3f(extents[3], extents[1], extents[5]);
			glVertex3f(extents[3], extents[4], extents[5]);
			glVertex3f(extents[0], extents[4], extents[5]);
			glEnd();
			break;
		default: 
			break;
	}
	glColor4f(1,1,1,1);
	glDisable(GL_BLEND);
}
// This method draws the faces of the subregion-cube.
// The surface of the cube is drawn partially transparent. 
// This is drawn after the cube is drawn.
// If a face is selected, it is drawn yellow
// The z-buffer will continue to be
// read-only, but is left on so that the grid lines will continue to be visible.
// Faces of the cube are numbered 0..5 based on view from pos z axis:
// back, front, bottom, top, left, right
// selectedFace is -1 if nothing selected
//	
// The viewer direction determines which faces are rendered.  If a coordinate
// of the viewDirection is positive (resp., negative), 
// then the back side (resp front side) of the corresponding cube side is rendered

void Renderer::renderRegionBounds(float* extents, int selectedFace, float* camPos, float faceDisplacement){
	setSubregionFrameColor(myVizWin->getSubregionFrameColor());
	//Copy the extents so they can be stretched
	int i;
	float cpExtents[6];
	int stretchCrd = -1;

	//Determine which coord direction is being stretched:
	if (selectedFace >= 0) {
		stretchCrd = (5-selectedFace)/2;
		if (selectedFace%2) stretchCrd +=3;
	}
	for (i = 0; i< 6; i++) {
		cpExtents[i] = extents[i];
		//Stretch the "extents" associated with selected face
		if(i==stretchCrd) cpExtents[i] += faceDisplacement;
	}
	for (i = 0; i< 6; i++){
		if (faceIsVisible(extents, camPos, i)){
			drawRegionFace(cpExtents, i, (i==selectedFace));
		}
	}
}
//Set colors to use in bound rendering:
void Renderer::setSubregionFrameColor(QColor& c){
	subregionFrameColor[0]= (float)c.red()/255.;
	subregionFrameColor[1]= (float)c.green()/255.;
	subregionFrameColor[2]= (float)c.blue()/255.;
}
void Renderer::setRegionFrameColor(QColor& c){
	regionFrameColor[0]= (float)c.red()/255.;
	regionFrameColor[1]= (float)c.green()/255.;
	regionFrameColor[2]= (float)c.blue()/255.;
}
void Renderer::drawAxes(float* extents){
	float origin[3];
	float maxLen = -1.f;
	for (int i = 0; i<3; i++){
		origin[i] = extents[i] + (myVizWin->getAxisCoord(i))*(extents[i+3]-extents[i]);
		if (extents[i+3] - extents[i] > maxLen) {
			maxLen = extents[i+3] - extents[i];
		}
	}
	float len = maxLen*0.2f;
	glColor3f(1.f,0.f,0.f);
	glLineWidth( 4.0 );
	glBegin(GL_LINES);
	glVertex3fv(origin);
	glVertex3f(origin[0]+len,origin[1],origin[2]);
	
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1]+.1*len, origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]+.1*len);

	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]+.1*len);
	glVertex3f(origin[0]+.8*len, origin[1]-.1*len, origin[2]);

	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1]-.1*len, origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]-.1*len);

	glVertex3f(origin[0]+len,origin[1],origin[2]);
	glVertex3f(origin[0]+.8*len, origin[1], origin[2]-.1*len);
	glVertex3f(origin[0]+.8*len, origin[1]+.1*len, origin[2]);
	glEnd();

	glColor3f(0.f,1.f,0.f);
	glBegin(GL_LINES);
	glVertex3fv(origin);
	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0]+.1*len, origin[1]+.8*len, origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]+.1*len);

	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]+.1*len);
	glVertex3f(origin[0]-.1*len, origin[1]+.8*len, origin[2]);

	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0]-.1*len, origin[1]+.8*len, origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]-.1*len);

	glVertex3f(origin[0],origin[1]+len,origin[2]);
	glVertex3f(origin[0], origin[1]+.8*len, origin[2]-.1*len);
	glVertex3f(origin[0]+.1*len, origin[1]+.8*len, origin[2]);
	glEnd();
	glColor3f(0.f,0.3f,1.f);
	glBegin(GL_LINES);
	glVertex3fv(origin);
	glVertex3f(origin[0],origin[1],origin[2]+len);
	glEnd();
	glBegin(GL_TRIANGLES);
	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0]+.1*len, origin[1], origin[2]+.8*len);
	glVertex3f(origin[0], origin[1]+.1*len, origin[2]+.8*len);

	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0], origin[1]+.1*len, origin[2]+.8*len);
	glVertex3f(origin[0]-.1*len, origin[1], origin[2]+.8*len);

	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0]-.1*len, origin[1], origin[2]+.8*len);
	glVertex3f(origin[0], origin[1]-.1*len, origin[2]+.8*len);

	glVertex3f(origin[0],origin[1],origin[2]+len);
	glVertex3f(origin[0], origin[1]-.1*len, origin[2]+.8*len);
	glVertex3f(origin[0]+.1*len, origin[1], origin[2]+.8*len);
	glEnd();
}
//Following methods are to support display of a colorscale in front of the data.
//
void Renderer::
buildColorscaleImage(){
	//Get the image size from the VizWin:
	float fwidth = myVizWin->getColorbarURCoord(0) - myVizWin->getColorbarLLCoord(0);
	float fheight = myVizWin->getColorbarURCoord(1) - myVizWin->getColorbarLLCoord(1);
	imgWidth = (int)(fwidth*myVizWin->width());
	imgHeight = (int)(fheight*myVizWin->height());
	if (imgWidth < 2 || imgHeight < 2) return;

	//Now push up to the next power of two
	int pow2 = 0;
	while (imgWidth>>pow2) {pow2++;}
	imgWidth = 1<<(pow2+1);
	pow2= 0;
	while (imgHeight>>pow2) {pow2++;}
	imgHeight = 1<<(pow2+1);

	
	
	//First, create a QPixmap (specified background color) and draw the coordinates on it.
	QPixmap colorbarPixmap(imgWidth, imgHeight);

	QColor bgColor = myVizWin->getColorbarBackgroundColor();
	colorbarPixmap.fill(bgColor);
	//assert(colorbarPixmap.depth()==32);
	
	QPainter painter(&colorbarPixmap);
	QColor penColor(255-bgColor.red(), 255-bgColor.green(),255-bgColor.blue());
	QPen myPen(penColor, 6);
	painter.setPen(myPen);

	//Setup font:
	int numtics = myVizWin->getColorbarNumTics();
	int textHeight = imgHeight/(2*numtics);
	if (textHeight > imgHeight/15) textHeight = imgHeight/15;
	QFont textFont;
	textFont.setPixelSize(textHeight);
	painter.setFont(textFont);

	//Draw outline:
	painter.drawLine(0,3, imgWidth, 3);
	painter.drawLine(imgWidth-3,0, imgWidth-3, imgHeight);
	painter.drawLine(imgWidth, imgHeight-3, 0, imgHeight-3);
	painter.drawLine(3, imgHeight, 3, 0);

	//Obtain the relevant transfer function:
	TransferFunction* myTransFunc = 
		VizWinMgr::getInstance()->getDvrParams(myVizWin->getWindowNum())->getTransFunc();
	
	
	
	for (int i = 0; i< numtics; i++){
		int ticPos = i*(imgHeight/numtics)+(imgHeight/(2*numtics));
		painter.drawLine((int)(imgWidth*.35), ticPos, (int)(imgWidth*.45), ticPos);
		double ycoord = myTransFunc->getMinMapValue() + (1.f - (float)i/(float)(numtics-1.f))*(myTransFunc->getMaxMapValue() -myTransFunc->getMinMapValue());
		QString ytext = QString::number(ycoord);
		painter.drawText(imgWidth/2 , ticPos - textHeight/2, imgWidth/2, textHeight, Qt::AlignLeft, ytext);
	}
	
	//Then, convert the pxmap to a QImage and draw the colormap colors on it.
	QImage colorbarImage = colorbarPixmap.convertToImage();

	//Calculate coefficients that convert screen coords to ycoords, 
	//Inverting above calc of ycoord.
	//
	double A = (myTransFunc->getMaxMapValue() - myTransFunc->getMinMapValue())*(double)(numtics)/
		((double)(1.-numtics)*(double)imgHeight);
	double B = myTransFunc->getMaxMapValue() - A*(double)imgHeight*.5/(double)(numtics);
	
	//check it out, should work at top and bottom:
	/*
	int topTicPosn = imgHeight/(2*numtics);
	int botTicPosn = (numtics-1)*(imgHeight/numtics)+(imgHeight/(2*numtics));
	double topFloat = A*(double)topTicPosn + B;
	double botFloat = A*(double)botTicPosn + B;
	*/


	for (int line = imgHeight-16; line>=16; line--){
		float ycoord = A*(float)line + B;
		QRgb clr = myTransFunc->getRgbValue(ycoord);
		for (int col = 16; col<(int)(imgWidth*.35); col++){
			colorbarImage.setPixel(col, line, clr);
		}
	}
	
	//Finally create the gl-formatted texture 
	//assert(colorbarImage.depth()==32);
	glColorbarImage = QGLWidget::convertToGLFormat(colorbarImage);
	
	
}
void Renderer::
renderColorscale(bool dorebuild){
	if (dorebuild) buildColorscaleImage();
	myVizWin->setColorbarDirty(false);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	//Disable z-buffer compare, always overwrite:
	glDepthFunc(GL_ALWAYS);
    glEnable( GL_TEXTURE_2D );
	
	//create a polygon appropriately positioned in the scene.  It's inside the unit cube--
	glTexImage2D(GL_TEXTURE_2D, 0, 3, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		glColorbarImage.bits());
	
	float llx = 2.f*myVizWin->getColorbarLLCoord(0) - 1.f; 
	float lly = 2.f*myVizWin->getColorbarLLCoord(1) - 1.f; 
	float urx = 2.f*myVizWin->getColorbarURCoord(0) - 1.f; 
	float ury = 2.f*myVizWin->getColorbarURCoord(1) - 1.f; 
	glBegin(GL_QUADS);
	glTexCoord2f(0.0f, 0.0f); glVertex3f(llx, lly, 0.0f);
	glTexCoord2f(0.0f, 1.0f); glVertex3f(llx, ury, 0.0f);
	glTexCoord2f(1.0f, 1.0f); glVertex3f(urx, ury, 0.0f);
	glTexCoord2f(1.0f, 0.0f); glVertex3f(urx, lly, 0.0f);
	glEnd();
	glDisable(GL_TEXTURE_2D);
	//Reset to default:
	glDepthFunc(GL_LESS);
}
