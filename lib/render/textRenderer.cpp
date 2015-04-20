//************************************************************************
//                                                                       *
//           Copyright (C)  2004                                         *
//     University Corporation for Atmospheric Research                   *
//           All Rights Reserved                                         *
//                                                                       *
//************************************************************************/
//
//  File:        textRenderer.cpp
//
//  Author:      Scott Pearse
//               National Center for Atmospheric Research
//               PO 3000, Boulder, Colorado
//
//  Date:        June 2014
//
//  Description: Implementation of text containers that render to 
//               QGLWidgets
//

#include "glutil.h" // Must be included first!!!
#include "params.h"
#include "visualizer.h"
#include "textRenderer.h"
#include "viewpointparams.h"
#include <vapor/MyBase.h>


using namespace VAPoR;
//struct GLWindow;        // just to retrieve window size

TextWriter::TextWriter( string fontFile,
                        int fontSize,
                        float txtColor[4],
                        float bgColor[4],
                        int type,
                        Visualizer *myGivenWindow) {

    _font 		 = fontFile;
    _size 		 = fontSize;
    _type 		 = type;
    _txtColor[0] = txtColor[0];
    _txtColor[1] = txtColor[1];
    _txtColor[2] = txtColor[2];
    _txtColor[3] = txtColor[3];
    _bgColor[0]  = bgColor[0];
    _bgColor[1]  = bgColor[1];
    _bgColor[2]  = bgColor[2];
    _bgColor[3]  = bgColor[3];
    _myWindow    = myGivenWindow;
}

int TextWriter::addText(string text, float coords[3]) {
	TextObject *to = new TextObject();
	to->Initialize(_font, text, _size, coords, _type,
                  _txtColor,_bgColor,_myWindow);
    TextObjects.push_back(to);
                                        
    return 0;
}

void TextWriter::drawText() {
    for (size_t i=0; i<TextObjects.size(); i++){
        TextObjects[i]->drawMe();
    }
}

TextWriter::~TextWriter() {
    for (size_t i=0; i<TextObjects.size(); i++){
        delete TextObjects[i];
    }
}

TextObject::TextObject() {

}

int TextObject::Initialize( string inFont,
                        string inText,
                        int inSize,
                        float inCoords[3],
                        int inType,
                        float txtColor[4],
						float bgColor[4],
                        Visualizer *myGivenWindow) {

    _pixmap 	  = new FTPixmapFont(inFont.c_str());
    _text 		  = inText;
    _size 		  = inSize;
	_type 		  = inType;
    _font 		  = inFont;
    _coords[0] 	  = inCoords[0];
    _coords[1]    = inCoords[1];
    _coords[2]    = inCoords[2];
    _bgColor[0]   = bgColor[0];
    _bgColor[1]   = bgColor[1];
    _bgColor[2]   = bgColor[2];
    _bgColor[3]   = bgColor[3];
    _txtColor[0]  = txtColor[0];
    _txtColor[1]  = txtColor[1];
    _txtColor[2]  = txtColor[2];
    _txtColor[3]  = txtColor[3];
    _myWindow     = myGivenWindow;
    _fbo          = 0;
    _fboTexture   = 0;

	if (inType == 1){
		//Convert user to local/stretched coordinates in cube
		DataStatus* ds = DataStatus::getInstance();
		vector<double>minExts, maxExts;
		//Note:  First argument (timestep) should be passed into this method?????
		ds->GetExtents(0, minExts, maxExts);
		float sceneScaleFactor = 1./DataStatus::getInstance()->getMaxStretchedSize();
		const double* scales = ds->getStretchFactors();
		for (int i = 0; i<3; i++){ 
			_3Dcoords[i] = _coords[i] - minExts[i];
			_3Dcoords[i] *= sceneScaleFactor;
			_3Dcoords[i] *= scales[i];
		}
	}

	_pixmap->FaceSize(_size);
	//Note:  can we do this without QT?
	//glMakeCurrent();
    findBBoxSize();
    initFrameBufferTexture();
    initFrameBuffer();

	return 0;
}

TextObject::~TextObject() {
	delete _pixmap;
//	glDisable(GL_TEXTURE_2D);
	glDeleteTextures(1,&_fboTexture);
	glDeleteBuffers(1,&_fbo);
}

void TextObject::initFrameBufferTexture(void) {
    //glEnable(GL_TEXTURE_2D);
	glGenTextures(1, &_fboTexture);
    glBindTexture(GL_TEXTURE_2D, _fboTexture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, _width, _height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

    glBindTexture(GL_TEXTURE_2D, 0);

    //printOpenGLError();
    GLenum glErr;
    glErr = glGetError();
	char* errString;
    if (glErr != GL_NO_ERROR){
		errString = (char*)gluErrorString(glErr);
        MyBase::SetErrMsg(errString);
	}
}


int TextObject::initFrameBuffer(void) {
    //glEnable(GL_TEXTURE_2D);
    //initFrameBufferTexture();

	//glDisable(GL_LIGHTING);

    glGenFramebuffersEXT(1, &_fbo);
    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, _fbo);

    glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, 
							  GL_COLOR_ATTACHMENT0_EXT, 
							  GL_TEXTURE_2D, _fboTexture, 0);

	// Check that status of our generated frame buffer
    GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);  
    if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {     
        std::cout << "Couldn't create frame buffer" << std::endl;  
        return -1;
    }

    // Save previous color state
    // Then render the background and text to our texture
    // Then finally reset colors to their previous state
    float txColors[4];
    float bgColors[4];
    glGetFloatv(GL_COLOR_CLEAR_VALUE,bgColors);
    glGetFloatv(GL_CURRENT_COLOR,txColors);
    glClearColor(_bgColor[0],_bgColor[1],_bgColor[2],_bgColor[3]);
    glColor4f(_txtColor[0],_txtColor[1],_txtColor[2],_txtColor[3]);
    glWindowPos2f(0,0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    int xRenderOffset = _size/30+1;//_size/24 + 1;
    int yRenderOffset = -_pixmap->BBox(_text.c_str()).Lower().Y()+_size/16+1; //+4
    if (_size <= 40){
		xRenderOffset += 1;
		yRenderOffset += 1;
	}
	else xRenderOffset -= 1;
	FTPoint point;
    point.X(xRenderOffset);
    point.Y(yRenderOffset);
    _pixmap->Render(_text.c_str(),-1,point);
    glColor4f(txColors[0],txColors[1],txColors[2],txColors[3]);
    glClearColor(bgColors[0],bgColors[1],bgColors[2],bgColors[3]);

    glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	//glEnable(GL_LIGHTING);

    //printOpenGLError();
    GLenum glErr;
    glErr = glGetError();
    char* errString;
    if (glErr != GL_NO_ERROR){
        errString = (char*)gluErrorString(glErr);
        MyBase::SetErrMsg(errString);
    }
	return 0;
}


void TextObject::findBBoxSize() {
    FTBBox box = _pixmap->BBox(_text.c_str());
    FTPoint up = box.Upper();
    FTPoint lo = box.Lower();

    int xPadding = _size/12 + 4;
    int yPadding = _size/12 + 4;

    _height = up.Y() - lo.Y() + yPadding;
    _width  = up.X() - lo.X() + xPadding;
}

void TextObject::applyViewerMatrix() {
    if ((_type == 0) || (_type == 1)){ 
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
		glPushAttrib(GL_ALL_ATTRIB_BITS);
		//glDisable(GL_LIGHTING);
		glEnable(GL_TEXTURE_2D);
    }    
    if (_type == 1) { 
        double newCoords[2];
        Visualizer *castWin;
        castWin = dynamic_cast <Visualizer*> (_myWindow);
     
        castWin->projectPointToWin(_3Dcoords, newCoords);
        _coords[0] = newCoords[0];
        _coords[1] = newCoords[1];
	}   
}

//float * TextObject::applyViewerMatrix(float coords[3]) {
int TextObject::applyViewerMatrix(double coords[3]) {
    if ((_type == 0) || (_type == 1)){ 
        glMatrixMode(GL_MODELVIEW);
        glPushMatrix();
        glLoadIdentity();
        glMatrixMode(GL_PROJECTION);
        glPushMatrix();
        glLoadIdentity();
        glPushAttrib(GL_ALL_ATTRIB_BITS);
        //glDisable(GL_LIGHTING);
        glEnable(GL_TEXTURE_2D);
    }    
    if (_type == 1) { 
        double newCoords[3];
		
        Visualizer *castWin;
        castWin = dynamic_cast <Visualizer*> (_myWindow);
		castWin->projectPointToWin(coords, newCoords);
        coords[0] = newCoords[0];
        coords[1] = newCoords[1];
		coords[2] = newCoords[2];
    }
	return 1;
}

void TextObject::removeViewerMatrix() {
    if ((_type == 0) || (_type == 1)){ 
		glPopAttrib();
        glMatrixMode(GL_PROJECTION);
        glPopMatrix();
        glMatrixMode(GL_MODELVIEW);
        glPopMatrix();
    }
}

int TextObject::drawMe(double coords[3], int timestep) {

	if (_type == 1) {
        //Convert user to local/stretched coordinates in cube
        DataStatus* ds = DataStatus::getInstance();
		vector<double> minExts, maxExts;
        ds->GetExtents((size_t)timestep, minExts, maxExts);
        float sceneScaleFactor = 1./DataStatus::getInstance()->getMaxStretchedSize();
        const double* scales = ds->getStretchFactors();
		for (int i = 0; i<3; i++){ 
            coords[i] = coords[i] - minExts[i];
            coords[i] *= sceneScaleFactor;
            coords[i] *= scales[i];
        }  
		applyViewerMatrix(coords);
	}
	else applyViewerMatrix();

    glBindTexture(GL_TEXTURE_2D, _fboTexture);  

    float fltTxtWidth = (float)_width/(float)_myWindow->getWidth();
    float fltTxtHeight = (float)_height/(float)_myWindow->getHeight();
    float llx = 2.*coords[0]/(float)_myWindow->getWidth() - 1.f; 
    float lly = 2.*coords[1]/(float)_myWindow->getHeight() - 1.f; 
    float urx = llx+2.*fltTxtWidth;
    float ury = lly+2.*fltTxtHeight;

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(llx, lly, .0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(llx, ury, .0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(urx, ury, .0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(urx, lly, .0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0); 

    removeViewerMatrix();

    GLenum glErr;
    glErr = glGetError();
    char* errString;
    if (glErr != GL_NO_ERROR){
        errString = (char*) gluErrorString(glErr);
        MyBase::SetErrMsg(errString);
    }   
    return 0;
}

int TextObject::drawMe() {

	applyViewerMatrix();

    glBindTexture(GL_TEXTURE_2D, _fboTexture);  

	cout << "drawMe " <<  _coords[0] << " " << _coords[1] << " " << _coords[2] << endl;

    float fltTxtWidth = (float)_width/(float)_myWindow->getWidth();
    float fltTxtHeight = (float)_height/(float)_myWindow->getHeight();
    float llx = 2.*_coords[0]/(float)_myWindow->getWidth() - 1.f; 
    float lly = 2.*_coords[1]/(float)_myWindow->getHeight() - 1.f; 
    float urx = llx+2.*fltTxtWidth;
    float ury = lly+2.*fltTxtHeight;

    glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
    
    glBegin(GL_QUADS);
    glTexCoord2f(0.0f, 0.0f); glVertex3f(llx, lly, .0f);
    glTexCoord2f(0.0f, 1.0f); glVertex3f(llx, ury, .0f);
    glTexCoord2f(1.0f, 1.0f); glVertex3f(urx, ury, .0f);
    glTexCoord2f(1.0f, 0.0f); glVertex3f(urx, lly, .0f);
    glEnd();

    glBindTexture(GL_TEXTURE_2D, 0);

    removeViewerMatrix();

    GLenum glErr;
    glErr = glGetError();
    char* errString;
    if (glErr != GL_NO_ERROR){
        errString = (char*) gluErrorString(glErr);
        MyBase::SetErrMsg(errString);
    }
	return 0;
}
