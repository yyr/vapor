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
#include "glutil.h"	// Must be included first!!!
#include <qpixmap.h>
#include <qpainter.h>
#include "renderer.h"
#include <vapor/DataMgr.h>
#include "regionparams.h"
#include "viewpointparams.h"
#include "animationparams.h"
#include "dvrparams.h"
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
	int fontsize = myGLWindow->getColorbarFontsize();
	textHeight = (int) (textHeight*fontsize*0.1);
	textFont.setPixelSize(textHeight);
	painter.setFont(textFont);
	int numdigits = myGLWindow->getColorbarDigits();

	//Draw outline:
	painter.drawLine(0,3, imgWidth, 3);
	painter.drawLine(imgWidth-3,0, imgWidth-3, imgHeight);
	painter.drawLine(imgWidth, imgHeight-3, 0, imgHeight-3);
	painter.drawLine(3, imgHeight, 3, 0);

	//Obtain the relevant transfer function:
	TransferFunction* myTransFunc = 
		(TransferFunction*)currentRenderParams->GetMapperFunc();
	
	
	
	for (int i = 0; i< numtics; i++){
		int ticPos = i*(imgHeight/numtics)+(imgHeight/(2*numtics));
		painter.drawLine((int)(imgWidth*.35), ticPos, (int)(imgWidth*.45), ticPos);
		double ycoord = myTransFunc->getMinMapValue() + (1.f - (float)i/(float)(numtics-1.f))*(myTransFunc->getMaxMapValue() -myTransFunc->getMinMapValue());
		QString ytext = QString::number(ycoord,'g',numdigits);
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
	float whitecolor[4] = {1.,1.,1.,1.f};
	if (dorebuild) buildColorscaleImage();

	myGLWindow->setColorbarDirty(false);
	glColor4fv(whitecolor);
	glBindTexture(GL_TEXTURE_2D,_colorbarTexid);
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


void Renderer::enableClippingPlanes(const double extents[6]){

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();

	myGLWindow->getTBall()->TrackballSetMatrix();

    //cerr << "transforming everything to unit box coords :-(\n";
    myGLWindow->TransformToUnitBox();

    const float* scales = DataStatus::getInstance()->getStretchFactors();
    glScalef(scales[0], scales[1], scales[2]);

	GLdouble x0Plane[] = {1., 0., 0., 0.};
	GLdouble x1Plane[] = {-1., 0., 0., 1.0};
	GLdouble y0Plane[] = {0., 1., 0., 0.};
	GLdouble y1Plane[] = {0., -1., 0., 1.};
	GLdouble z0Plane[] = {0., 0., 1., 0.};
	GLdouble z1Plane[] = {0., 0., -1., 1.};//z largest


	x0Plane[3] = -extents[0];
	x1Plane[3] = extents[3];
	y0Plane[3] = -extents[1];
	y1Plane[3] = extents[4];
	z0Plane[3] = -extents[2];
	z1Plane[3] = extents[5];

	glClipPlane(GL_CLIP_PLANE0, x0Plane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, x1Plane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, y0Plane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, y1Plane);
	glEnable(GL_CLIP_PLANE3);
	glClipPlane(GL_CLIP_PLANE4, z0Plane);
	glEnable(GL_CLIP_PLANE4);
	glClipPlane(GL_CLIP_PLANE5, z1Plane);
	glEnable(GL_CLIP_PLANE5);

	glPopMatrix();
}

void Renderer::enableFullClippingPlanes() {

	AnimationParams *myAnimationParams = myGLWindow->getActiveAnimationParams();
    size_t timeStep = myAnimationParams->getCurrentTimestep();
	DataMgr *dataMgr = DataStatus::getInstance()->getDataMgr();

	const vector<double>& extvec = dataMgr->GetExtents(timeStep);
	double extents[6];
	for (int i=0; i<3; i++) {
		extents[i] = extvec[i] - ((extvec[3+i]-extvec[i])*0.001);
		extents[i+3] = extvec[i+3] + ((extvec[3+i]-extvec[i])*0.001);
	}

	enableClippingPlanes(extents);
}
	
void Renderer::disableFullClippingPlanes(){
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);
}

 void Renderer::enable2DClippingPlanes(){
	GLdouble topPlane[] = {0., -1., 0., 1.}; //y = 1
	GLdouble rightPlane[] = {-1., 0., 0., 1.0};// x = 1
	GLdouble leftPlane[] = {1., 0., 0., 0.001};//x = -.001
	GLdouble botPlane[] = {0., 1., 0., 0.001};//y = -.001
	
	const float* sizes = DataStatus::getInstance()->getFullStretchedSizes();
	topPlane[3] = sizes[1]*1.001;
	rightPlane[3] = sizes[0]*1.001;
	
	glClipPlane(GL_CLIP_PLANE0, topPlane);
	glEnable(GL_CLIP_PLANE0);
	glClipPlane(GL_CLIP_PLANE1, rightPlane);
	glEnable(GL_CLIP_PLANE1);
	glClipPlane(GL_CLIP_PLANE2, botPlane);
	glEnable(GL_CLIP_PLANE2);
	glClipPlane(GL_CLIP_PLANE3, leftPlane);
	glEnable(GL_CLIP_PLANE3);
}

void Renderer::enableRegionClippingPlanes() {

	AnimationParams *myAnimationParams = myGLWindow->getActiveAnimationParams();
    size_t timeStep = myAnimationParams->getCurrentTimestep();
	RegionParams* myRegionParams = myGLWindow->getActiveRegionParams();

	double regExts[6];
	myRegionParams->GetBox()->GetUserExtents(regExts,timeStep);

	//
	// add padding for floating point roundoff
	//
	for (int i=0; i<3; i++) {
		regExts[i] = regExts[i] - ((regExts[3+i]-regExts[i])*0.001);
		regExts[i+3] = regExts[i+3] + ((regExts[3+i]-regExts[i])*0.001);
	}

	enableClippingPlanes(regExts);
}

void Renderer::disableRegionClippingPlanes(){
	Renderer::disableFullClippingPlanes();
}
