//************************************************************************
//                                                                       *
//           Copyright (C)  2004                                         *
//     University Corporation for Atmospheric Research                   *
//           All Rights Reserved                                         *
//                                                                       *
//************************************************************************/
//
//  File:        textRenderer.h
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

#ifndef _TextRenderer_H_
#define _TextRenderer_H_
#include <FTGL/ftgl.h>
#include <vapor/MyBase.h>
#include <vapor/common.h>

class FTPixmapFont;
//class GLWindow;

//
//! \class TextObject
//! \A container class for an FTGL font and its texture for display
//! \author Scott Pearse
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides an API for displaying a single texture that
//! contains text generated in FTGL.

namespace VAPoR {
 class RENDER_API TextObject {
 public:

//! Default constructor.
    TextObject();

//! Initialize a text object after creation.
	int Initialize(
			    string inFont,
                string inText,
                int inSize,
                float inCoords[3],
                int inType,
                float txtColor[4],
				float bgColor[4],
                Visualizer *myWindow);
    ~TextObject();

//    void setText(string txt) { _text = txt; }
//    void setSize(int sz) { _size = sz; }
//    void setType(int t) { _type = t; } 
//    void setTextColor(float r, float g, float b) { 
//         _txtColor[0] = r; _txtColor[1] = g; _txtColor[2] = b; }
//    void setBGColor(float r, float g, float b) { 
//         _bgColor[0] = r; _bgColor[1] = g; _bgColor[2] = b; }
//    void setCoords(float x, float y, float z=0.0) {
//         _coords[0] = x; _coords[1] = y; _coords[2] = z; 
//		cout << "setCoords " << _coords[0] << " " << _coords[1] << " " << _coords[2] << endl;} 
    
//    string  getText() { return _text;}
//    int     getSize() { return _size;}
//    float*  getTextColor() { return _txtColor; } 
//    float*  getBGColor() { return _bgColor; }
//    float*  getCoords() { return _coords; }

//! Draw Text Object at default coordinates specified in TextObject::Initialize()
    int drawMe();
//! Draw Text Object at specified x, y, z coordinate, at specified time step
	int drawMe(double coords[3], int timestep);

//! Sets the variables \p _width and \p _height.
//! These define the size of the texture box that text will be drawn on to
    void findBBoxSize(void);

//! Generates a new framebuffer object and assigns it a unique identifier
    int initFrameBuffer(void);

//! Saves the current GL state and applies transformations
//! to draw the appropriate type of text object.
    void applyViewerMatrix();
	//float * applyViewerMatrix(float coords[3]);
	int applyViewerMatrix(double coords[3]);

//! Resets the state of the GL machine to what it was before
//! our text rendering.
    void removeViewerMatrix();

private:
    void initFrameBufferTexture(void);

    GLuint _fbo;
    GLuint _fboTexture;
    
    int           _width;            // Width of our texture
    int           _height;           // Height of our texture

    Visualizer     *_myWindow;
    FTPixmapFont  *_pixmap;          // FTGL pixmap object
    string        _font;             // font file
    string        _text;             // text to display
    int           _size;             // font size
    float         _txtColor[4];      // text color
    float         _bgColor[4];       // background color
    double         _coords[3];        // text coordates
    double		  _3Dcoords[3];		 // 3D coordinates used if we draw text within the scene
	int           _type;             // in front of scene, following a point in domain, or within
                                     // type 0 - text drawn in 2d on top of the scene
                                     // type 1 - text drawn in 2d, following a point in the scene
                                     // type 2 - text drawn within the scene, bumped up towards the 
                                     // type 3 - text drawn within the scene
};

class RENDER_API TextWriter {
public:
    TextWriter( string fontFile, 
                int fontSize,
                float txtColor[4], 
				float bgColor[4], 
                int type,
                Visualizer *myGivenWindow);
    ~TextWriter();

    int addText(string text, 
                float coords[3]);
    void deleteText();
    void drawText();

private:
    string _font;
    int _size;
    int _type;
    vector<TextObject*> TextObjects;
    Visualizer *_myWindow;
    float _txtColor[4];
    float _bgColor[4];
};

}; 		// end namespace vapor

#endif  // _TextRenderer_H_
