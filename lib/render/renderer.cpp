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
#include "glwindow.h"
#include "renderer.h"
#include <vapor/DataMgr.h>
#include "regionparams.h"
#include "viewpointparams.h"
#include "dvrparams.h"
#include "glutil.h"
#include "transferfunction.h"
#include "Colormap.h"

using namespace VAPoR;

Renderer::Renderer( GLWindow* glw, RenderParams* rp, string name)
{
	//Establish the data sources for the rendering:
	//
	
	_myName = name;
    myGLWindow = glw;
	savedNumXForms = -1;
	currentRenderParams = rp;
	initialized = false;
	clutDirtyBit = false;
	
	
	rp->initializeBypassFlags();
}



// Destructor
Renderer::~Renderer()
{
   //
   // Ensure that we have the correct graphics context before
   // any graphics objects (e.g., textures, shaders, ...) in that
   // context are deleted.
   //
   if (myGLWindow) myGLWindow->makeCurrent();
}

//Following methods are to support display of a colorscale in front of the data.
//
void Renderer::
buildColorscaleImage(){
	//Get the image size from the VizWin:
	float fwidth = myGLWindow->getColorbarURCoord(0) - myGLWindow->getColorbarLLCoord(0);
	float fheight = myGLWindow->getColorbarURCoord(1) - myGLWindow->getColorbarLLCoord(1);
	imgWidth = (int)(fwidth*myGLWindow->width());
	imgHeight = (int)(fheight*myGLWindow->height());
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

	QColor bgColor = myGLWindow->getColorbarBackgroundColor();
	colorbarPixmap.fill(bgColor);
	//assert(colorbarPixmap.depth()==32);
	
	QPainter painter(&colorbarPixmap);
	QColor penColor(255-bgColor.red(), 255-bgColor.green(),255-bgColor.blue());
	QPen myPen(penColor, 6);
	painter.setPen(myPen);

	//Setup font:
	int textHeight;
	int numtics = myGLWindow->getColorbarNumTics();
	if (numtics == 0) textHeight = imgHeight;  
	else  textHeight = imgHeight/(2*numtics);
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
		(TransferFunction*)currentRenderParams->getMapperFunc();
	
	
	
	for (int i = 0; i< numtics; i++){
		int ticPos = i*(imgHeight/numtics)+(imgHeight/(2*numtics));
		painter.drawLine((int)(imgWidth*.35), ticPos, (int)(imgWidth*.45), ticPos);
		double ycoord = myTransFunc->getMinMapValue() + (1.f - (float)i/(float)(numtics-1.f))*(myTransFunc->getMaxMapValue() -myTransFunc->getMinMapValue());
		QString ytext = QString::number(ycoord);
		painter.drawText(imgWidth/2 , ticPos - textHeight/2, imgWidth/2, textHeight, Qt::AlignLeft, ytext);
	}
	
	//Then, convert the pxmap to a QImage and draw the colormap colors on it.
	QImage colorbarImage = colorbarPixmap.toImage();

	//Calculate coefficients that convert screen coords to ycoords, 
	//Inverting above calc of ycoord.
	//
	//With no tics, use the whole scale
	if (numtics == 0) numtics = 1000;
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
        
        QRgb clr = myTransFunc->colorValue(ycoord);

		for (int col = 16; col<(int)(imgWidth*.35); col++){
			colorbarImage.setPixel(col, line, clr);
		}
	}
	
	//Finally create the gl-formatted texture 
	//assert(colorbarImage.depth()==32);
	glColorbarImage = QGLWidget::convertToGLFormat(colorbarImage);
	painter.end();
	
}
void Renderer::
renderColorscale(bool dorebuild){
	if (dorebuild) buildColorscaleImage();
	myGLWindow->setColorbarDirty(false);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	//Disable z-buffer compare, always overwrite:
	glDepthFunc(GL_ALWAYS);
    glEnable( GL_TEXTURE_2D );
	
	//create a polygon appropriately positioned in the scene.  It's inside the unit cube--
	glTexImage2D(GL_TEXTURE_2D, 0, 3, imgWidth, imgHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE,
		glColorbarImage.bits());
	
	float llx = 2.f*myGLWindow->getColorbarLLCoord(0) - 1.f; 
	float lly = 2.f*myGLWindow->getColorbarLLCoord(1) - 1.f; 
	float urx = 2.f*myGLWindow->getColorbarURCoord(0) - 1.f; 
	float ury = 2.f*myGLWindow->getColorbarURCoord(1) - 1.f; 
	glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
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
void Renderer::enableFullClippingPlanes(){
	GLdouble topPlane[] = {0., -1., 0., 1.}; //y = 1
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};// x = 1
	GLdouble leftPlane[] = {1., 0., 0., 0.001};//x = -.001
	GLdouble botPlane[] = {0., 1., 0., 0.001};//y = -.001
	GLdouble frontPlane[] = {0., 0., -1., 1.};//z =1
	GLdouble backPlane[] = {0., 0., 1., 0.001};//z = -.001
	const float* extents = DataStatus::getInstance()->getStretchedExtents();
	topPlane[3] = extents[4]*1.001;
	rightPlane[3] = extents[3]*1.001;
	frontPlane[3] = extents[5]*1.001;
	
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

}
void Renderer::disableFullClippingPlanes(){
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
}

 
