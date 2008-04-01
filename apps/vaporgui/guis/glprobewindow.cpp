//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		glprobewindow.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Nov 2005
//
//	Description:  Implementation of GLProbeWindow class: 
//		This performs the opengl drawing of a texture in the probe frame.
//

#include "glprobewindow.h"
#include "proberenderer.h"

#include "glutil.h"
#include "probeframe.h"
#include "probeparams.h"
#include "probeeventrouter.h"
#include "messagereporter.h"
#include "session.h"
#include <math.h>
#include <qgl.h>
#include <qapplication.h>
#include <qcursor.h>
#include "assert.h"

using namespace VAPoR;


GLProbeWindow::GLProbeWindow( const QGLFormat& fmt, QWidget* parent, const char* name, ProbeFrame* pf )
: QGLWidget(fmt, parent, name)

{
	
	horizTexSize = 1.f;
	vertTexSize = 1.f;
	rectLeft = -1.f;
	rectTop = 1.f;
	probeFrame = pf;
	animatingTexture = false;
	animatingFrameNum = 0;
	animationStarting = true;
	assert(doubleBuffer());
	setAutoBufferSwap(true);
	patternListNum = -1;
	currentAnimationTimestep = 0;
}


/*!
  Release allocated resources.
*/

GLProbeWindow::~GLProbeWindow()
{
    makeCurrent();
	//delete the texture?
}


//
//  Set up the OpenGL view port, matrix mode, etc.
//

void GLProbeWindow::resizeGL( int width, int height )
{
	//update the size of the drawing rectangle
	glViewport( 0, 0, (GLint)width, (GLint)height );
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glMatrixMode(GL_MODELVIEW);
	qglClearColor( QColor(233,236,216) ); 		// same as frame

	//Calculate the lower-left and upper right positions for the 
	//texture to be mapped
	float winAspect = (float)height/(float)width;
	float texAspect = vertTexSize/horizTexSize;
	if (winAspect > texAspect){
		//Window is taller than texture, so fill from left to right:
		rectLeft = -1.f;
		rectTop = texAspect/winAspect;
	} else {
		rectLeft = winAspect/texAspect;
		rectTop = 1.f;
	}

}
	
void GLProbeWindow::setTextureSize(float horiz, float vert){
	vertTexSize = vert;
	horizTexSize = horiz;
	float winAspect = (float)height()/(float)width();
	float texAspect = vertTexSize/horizTexSize;
	if (winAspect > texAspect){
		//Window is taller than texture, so fill from left to right:
		rectLeft = -1.f;
		
		rectTop = texAspect/winAspect;
		
	} else {
		rectLeft = -winAspect/texAspect;
		rectTop = 1.f;
	}
}


void GLProbeWindow::paintGL()
{
	
	ProbeParams* myParams = VizWinMgr::getActiveProbeParams();
	size_t fullHeight = VizWinMgr::getActiveRegionParams()->getFullGridHeight();
	int timestep = VizWinMgr::getInstance()->getActiveAnimationParams()->getCurrentFrameNumber();

    qglClearColor( QColor(233,236,216) ); 		// same as frame
   
	glClearDepth(1);
	glPolygonMode(GL_FRONT,GL_FILL);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

	if (!myParams->isEnabled()) {return;}
	
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_BLEND);
	glTexEnvf(GL_TEXTURE_ENV,GL_TEXTURE_ENV_MODE,GL_REPLACE);
	//get the probe texture:
	unsigned char* probeTexture = 0;
	int imgSize[2];
	if (myParams->getProbeType() == 1) {  //IBFV texture
		
		if (animatingTexture) {
			if (animationStarting) currentAnimationTimestep = timestep;
			if (currentAnimationTimestep == timestep){
				probeTexture = ProbeRenderer::getNextIBFVTexture(fullHeight,myParams, timestep, animatingFrameNum, animationStarting,
					&patternListNum);
				if(probeTexture) animationStarting = false;
				else return; //failure to build texture
			} else {
				return;// timestep changed!
			}
		} else { //not animated.  Calculate it if necessary
			probeTexture = ProbeRenderer::getProbeTexture(myParams, timestep, fullHeight, false);
		}
		myParams->adjustTextureSize(fullHeight, imgSize);
	} else if(myParams ){//data probe
		if (myParams->probeIsDirty(timestep)){
			QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
			probeTexture = ProbeRenderer::getProbeTexture(myParams,timestep,fullHeight,false);
			QApplication::restoreOverrideCursor();
		} else {
			probeTexture = ProbeRenderer::getProbeTexture(myParams,timestep,fullHeight,false);
		}
	}
	myParams->getTextureSize(imgSize);
	
	if(probeTexture) {
		glEnable(GL_TEXTURE_2D);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imgSize[0],imgSize[1], 0, GL_RGBA, GL_UNSIGNED_BYTE, probeTexture);
	} else {
		return;
		//glColor4f(0.,0.,0.,0.);
	}

	glBegin(GL_QUADS);
	//Just specify the rectangle corners
	glTexCoord2f(0.f,0.f); glVertex2f(rectLeft,-rectTop);
	glTexCoord2f(0.f, 1.f); glVertex2f(rectLeft, rectTop);
	glTexCoord2f(1.f,1.f); glVertex2f(-rectLeft,rectTop);
	glTexCoord2f(1.f, 0.f); glVertex2f(-rectLeft, -rectTop);
	glEnd();
	//glFlush();
	if (probeTexture) glDisable(GL_TEXTURE_2D);
	glDisable(GL_BLEND);
	
	if(capturing && animatingTexture){
		doFrameCapture();
	}

	//Now draw the crosshairs
	if (myParams){
		const float* crossHairCoords = myParams->getCursorCoords();
		float crossX = crossHairCoords[0]*rectLeft;
		float crossY = crossHairCoords[1]*rectTop;
		glColor3f(CURSOR_COLOR);
		glLineWidth(2.f);
		glBegin(GL_LINES);
		glVertex2f(crossX-CURSOR_SIZE, crossY);
		glVertex2f(crossX+CURSOR_SIZE, crossY);
		glVertex2f(crossX, crossY-CURSOR_SIZE);
		glVertex2f(crossX, crossY+CURSOR_SIZE);
		glEnd();
	}
	
    
	
	if ((myParams->getProbeType() == 1) && animatingTexture) {  //animating IBFV texture
		delete probeTexture;
	}
}

//
//  Set up the OpenGL rendering state, and define display list
//

void GLProbeWindow::initializeGL()
{
   	makeCurrent();
	
	qglClearColor( QColor(233,236,216) ); 		// same as frame
    glShadeModel( GL_FLAT );
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_LIGHTING);
	//glGenTextures(1, &texName);
	//glBindTexture(GL_TEXTURE_2D, texName);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    
}
//
//To map the window coords, first map the device coords (0..width-1)
// and (0..height-1) to
// float (-1,1).  Then stretch according to rectLeft, rectRight
//
void GLProbeWindow::mapPixelToProbeCoords( int ix, int iy, float* x, float*y){
	float xcoord = 2.f*((float)ix)/((float)(width()-1))-1.f;
	float ycoord = 1.f - 2.f*((float)iy)/((float)(height()-1));
	*x = xcoord/rectLeft;
	*y = ycoord/rectTop;
}
//Produce an array based on current contents of the (front) buffer.
//This is like the similar method in GLWindow.  Must be invoked while rendering.
bool GLProbeWindow::
getPixelData(int minx, int miny, int sizex, int sizey, unsigned char* data){
	// set the current window 
	makeCurrent();
	 // Must clear previous errors first.
	while(glGetError() != GL_NO_ERROR);

	//if (front)
	//
	//glReadBuffer(GL_FRONT);
	//
	//else
	//  {
	glReadBuffer(GL_BACK);
	//  }
	glDisable( GL_SCISSOR_TEST );


 
	// Turn off texturing in case it is on - some drivers have a problem
	// getting / setting pixels with texturing enabled.
	glDisable( GL_TEXTURE_2D );

	assert( 0<=minx && minx < sizex);
	assert(sizex <= width());
	assert (0 <= miny && sizey > miny);
	assert(sizey <= height());
	// Calling pack alignment ensures that we can grab the any size window
	glPixelStorei( GL_PACK_ALIGNMENT, 1 );
	glReadPixels(minx, miny, sizex,sizey, GL_RGB,
				GL_UNSIGNED_BYTE, data);
	if (glGetError() != GL_NO_ERROR)
		return false;
	//Unfortunately gl reads these in the reverse order that jpeg expects, so
	//Now we need to swap top and bottom!
	unsigned char val;
	for (int j = 0; j< sizey/2; j++){
		for (int i = 0; i<sizex*3; i++){
			val = data[i+sizex*3*j];
			data[i+sizex*3*j] = data[i+sizex*3*(sizey-j-1)];
			data[i+sizex*3*(sizey-j-1)] = val;
		}
	}
	
	return true;
}

//Capture the IBFV image.  Only used for capturing sequence of IBFV images
void GLProbeWindow::
doFrameCapture(){
	
	//Create a string consisting of captureName, followed by nnnn (framenum), 
	//followed by .jpg
	QString filename = captureName;
	filename += (QString("%1").arg(captureNum)).rightJustify(4,'0');
	filename +=  ".jpg";
	
	//Now open the jpeg file:
	FILE* jpegFile = fopen(filename.ascii(), "wb");
	if (!jpegFile) {
		MessageReporter::errorMsg("Image Capture Error: Error opening output Jpeg file: %s",filename.ascii());
		capturing = 0;
		return;
	}
	//The window to extract is x from (rectLeft+1)*w/2 to (1-rectLeft)*w/2 -1
	// y goes from ((1-rectTop)*h/2 to (1+rectTop)*h/2 -1
	int minx = ((rectLeft+1.)*width()*.5)+.5;
	int sizex = -rectLeft*width()-1;
	int miny = ((1.-rectTop)*height()*.5)+.5;
	int sizey = rectTop*height()-1;

	unsigned char* pixData = new unsigned char[sizex*sizey*3];
	
	
	if(!getPixelData(minx, miny, sizex,sizey,pixData)){
		MessageReporter::errorMsg("Image Capture Error; error obtaining GL data");
		capturing = 0;
		delete pixData;
		return;
	}
	
	//Now call the Jpeg library to compress and write the file
	//
	int quality = GLWindow::getJpegQuality();
	
	int rc = write_JPEG_file(jpegFile, sizex,sizey, pixData, quality);
	if (rc){
		//Error!
		MessageReporter::errorMsg("Image Capture Error; Error writing jpeg file ");
		delete pixData;
		return;
	}
	delete pixData;
	captureNum++;
}

