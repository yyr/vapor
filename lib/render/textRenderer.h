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

#ifndef TEXTRENDERER_H
#define TEXTRENDERER_H
#include <FTGL/ftgl.h>
#include <vapor/MyBase.h>
#include <vapor/common.h>
#include <QGLWidget>

class FTPixmapFont;
//class GLWindow;

namespace VAPoR {
 class RENDER_API TextObject {
public:
    TextObject( string inFont,
                string inText,
                int inSize,
                float inCoords[3],
                int inType,
                float bgColor[4],
                float txtColor[4],
                QGLWidget *myWindow);
    ~TextObject();

    void setText(string txt) { _text = txt; }
    void setSize(int sz) { _size = sz; }
    void setType(int t) { _type = t; } 
    void setTextColor(float r, float g, float b) { 
         _txtColor[0] = r; _txtColor[1] = g; _txtColor[2] = b; }
    void setBGColor(float r, float g, float b) { 
         _bgColor[0] = r; _bgColor[1] = g; _bgColor[2] = b; }
    void setCoords(float x, float y, float z=0.0) {
         _coords[0] = x; _coords[1] = y; _coords[2] = z; } 
    
    string  getText() { return _text;}
    int     getSize() { return _size;}
    float*  getTextColor() { return _txtColor; } 
    float*  getBGColor() { return _bgColor; }
    float*  getCoords() { return _coords; }

    int drawMe();
    void findBBoxSize(void);
    void initFrameBuffer(void);
    void applyViewerMatrix(void);
    void removeViewerMatrix();

private:
    void initFrameBufferTexture(void);

    GLuint _fbo;
    GLuint _fboTexture;
    
    int           _width;            // Width of our texture
    int           _height;           // Height of our texture

    QGLWidget     *_myWindow;
    FTPixmapFont  *_pixmap;          // FTGL pixmap object
    string        _font;             // font file
    string        _text;             // text to display
    int           _size;             // font size
    float         _txtColor[4];      // text color
    float         _bgColor[4];       // background color
    float         _coords[3];        // text coordates
    float		  _3Dcoords[3];		 // 3D coordinates used if we draw text within the scene
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
                float bgColor[4], 
                float txtColor[4], 
                int type,
                QGLWidget *myGivenWindow);
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
    QGLWidget *_myWindow;
    float _txtColor[4];
    float _bgColor[4];
};

}; 		// end namespace vapor

#endif  // textrenderer_h
