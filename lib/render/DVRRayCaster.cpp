//--DVRRayCaster.cpp -----------------------------------------------------------
//
// $Id$
//
//
//----------------------------------------------------------------------------

#include "glutil.h"	// Must be included first!!!

#include <qgl.h>
#include "DVRRayCaster.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "params.h"

#include "Matrix3d.h"
#include "Point3d.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRRayCaster::DVRRayCaster(
			   int precision, int nvars, GLWindow* glw, ShaderMgr *shadermgr,
	int nthreads
) : DVRShader(precision, nvars, shadermgr, nthreads)
{
	MyBase::SetDiagMsg(
		"DVRRayCaster::DVRRayCaster( %d %d %d)", 
		precision, nvars, nthreads
	);
	_mapped = (nvars == 2);
	_vidx = 0;
	_lighting = false;
	_framebufferid = 0;
	_backface_texcrd_texid = 0;
	_backface_depth_texid = 0;
	_initialized = false;
	myGLWindow = glw;
	_nisos = 0;

	if (GLEW_VERSION_2_0) {
		// if mapped, parent class (DVRShader) uses tex unit 0
		// for the volume texture, texture unit 1 for
		// the color map texture, and unit 2 for the coordinate map
		//
		_texcrd_texunit = GL_TEXTURE3;
		_depth_texunit = GL_TEXTURE4;
	}
	else {
		_texcrd_texunit = GL_TEXTURE3_ARB;
		_depth_texunit = GL_TEXTURE4_ARB;
	}

	_texcrd_sampler = 3;
	_depth_sampler = 4;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRRayCaster::~DVRRayCaster() 
{
	MyBase::SetDiagMsg( "DVRRayCaster::~DVRRayCaster()");

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	if (_framebufferid) glDeleteFramebuffersEXT(1, &_framebufferid);

	if (_backface_texcrd_texid) glDeleteTextures(1, &_backface_texcrd_texid);
	if (_backface_depth_texid) glDeleteTextures(1, &_backface_depth_texid);
	printOpenGLError();

	_shadermgr->undefEffect(instanceName("default"));
	_shadermgr->undefEffect(instanceName("lighting"));
	_shadermgr->undefEffect(instanceName("mapped"));
	_shadermgr->undefEffect(instanceName("mapped+lighting"));
	printOpenGLError();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::GraphicsInit() 
{
	if (_initialized) return(0);
	glewInit();
	if(myGLWindow->isDepthPeeling()){
	  if (! _shadermgr->defineEffect("Iso", "DEPTHPEEL;", instanceName("default")))
	    return(-1);

	  if (! _shadermgr->defineEffect("Iso", "LIGHTING; DEPTHPEEL;", instanceName("lighting")))
	    return(-1);

	  if (! _shadermgr->defineEffect("Iso", "MAPPED; DEPTHPEEL;", instanceName("mapped")))
	    return(-1);

	  if (! _shadermgr->defineEffect("Iso", "MAPPED; LIGHTING; DEPTHPEEL;", instanceName("mapped+lighting")))
	    return(-1);
	  _shadermgr->uploadEffectData(instanceName("default"), std::string("previousPass"), myGLWindow->depthTexUnit);
	  _shadermgr->uploadEffectData(instanceName("default"), std::string("height"), (float)myGLWindow->depthHeight);
	  _shadermgr->uploadEffectData(instanceName("default"), std::string("width"), (float)myGLWindow->depthWidth);	
	  _shadermgr->uploadEffectData(instanceName("lighting"), std::string("previousPass"), myGLWindow->depthTexUnit);
	  _shadermgr->uploadEffectData(instanceName("lighting"), std::string("height"), (float)myGLWindow->depthHeight);
	  _shadermgr->uploadEffectData(instanceName("lighting"), std::string("width"), (float)myGLWindow->depthWidth);	
	  _shadermgr->uploadEffectData(instanceName("mapped"), std::string("previousPass"), myGLWindow->depthTexUnit);
	  _shadermgr->uploadEffectData(instanceName("mapped"), std::string("height"), (float)myGLWindow->depthHeight);
	  _shadermgr->uploadEffectData(instanceName("mapped"), std::string("width"), (float)myGLWindow->depthWidth);	
	  _shadermgr->uploadEffectData(instanceName("mapped+lighting"), std::string("previousPass"), myGLWindow->depthTexUnit);
	  _shadermgr->uploadEffectData(instanceName("mapped+lighting"), std::string("height"), (float)myGLWindow->depthHeight);
	  _shadermgr->uploadEffectData(instanceName("mapped+lighting"), std::string("width"), (float)myGLWindow->depthWidth);
#ifdef DEBUG
	  std::cout << "DVRRaycaster DEPTH PEELING\n" << std::endl;
#endif
	}
	else{
	  if (! _shadermgr->defineEffect("Iso", "", instanceName("default")))
	    return(-1);

	  if (! _shadermgr->defineEffect("Iso", "LIGHTING;", instanceName("lighting")))
	    return(-1);

	  if (! _shadermgr->defineEffect("Iso", "MAPPED;", instanceName("mapped")))
	    return(-1);

	  if (! _shadermgr->defineEffect("Iso", "MAPPED; LIGHTING;", instanceName("mapped+lighting")))
	    return(-1);
#ifdef DEBUG
	  std::cout << "DVRRaycaster NO PEEL\n" << std::endl;
#endif
	} 

	if (initTextures() < 0) return(-1);

	if (! _shadermgr->uploadEffectData(instanceName("default"), "volumeTexture", 0)) return(-1);	
	if (! _shadermgr->uploadEffectData(instanceName("default"), "coordmap", 2)) return(-1);	
	if (! _shadermgr->uploadEffectData(instanceName("default"), "texcrd_buffer", _texcrd_sampler)) return(-1);	
	if (! _shadermgr->uploadEffectData(instanceName("default"), "depth_buffer", _depth_sampler)) return(-1);	

	if (! _shadermgr->uploadEffectData(instanceName("lighting"), "volumeTexture", 0)) return(-1);	
	if (! _shadermgr->uploadEffectData(instanceName("lighting"), "coordmap", 2)) return(-1);	
	if (! _shadermgr->uploadEffectData(instanceName("lighting"), "texcrd_buffer", _texcrd_sampler)) return(-1);	
	if (! _shadermgr->uploadEffectData(instanceName("lighting"), "depth_buffer", _depth_sampler)) return(-1);

    if (! _shadermgr->uploadEffectData(instanceName("mapped"), "volumeTexture", 0)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped"), "colormap", 1)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped"), "coordmap", 2)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped"), "texcrd_buffer", _texcrd_sampler)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped"), "depth_buffer", _depth_sampler)) return(-1);

    if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "volumeTexture", 0)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "colormap", 1)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "coordmap", 2)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "texcrd_buffer", _texcrd_sampler)) return(-1);
    if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "depth_buffer", _depth_sampler)) return(-1);

	_initialized = true;
	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::Render()
{
	MyBase::SetDiagMsg("DVRRayCaster::Render()");

	DVRTexture3d::calculateSampling();
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);
	if (_lighting) {		
		_shadermgr->uploadEffectData(
			getCurrentEffect(), "winsize", (float)viewport[2], 
			(float)viewport[3]
		);	
	}
	_shadermgr->uploadEffectData(getCurrentEffect(), "nsamples", (int) _samples);	

	return(DVRShader::Render());

	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::SetRegion(
	const RegularGrid *rg, const float range[2], int num
) {
	_vidx = 0;
	if (_mapped) {

		bool layered = ((dynamic_cast<const LayeredGrid *>(rg)) != NULL);
		bool missing = rg->HasMissingData();

		if (missing && layered) {
			_vidx = 3;
		}
		else if (missing || layered) {
			_vidx = 2;
		}
		else {
			_vidx = 3; // Alpha channel
		}
	}
	return(DVRShader::SetRegion(rg,range,num));
}



//----------------------------------------------------------------------------
// Draw the proxy geometry
//----------------------------------------------------------------------------
void DVRRayCaster::drawFrontPlane(
	const BBox &volumeBox, const BBox &textureBox,
	const Matrix3d &modelview,
	const Matrix3d &modelviewInverse
) {
  //
  //  
  //
  BBox rotatedBox(volumeBox);

  //
  // transform the volume into world coordinates
  //
  rotatedBox.transform(modelview);

  //
  // Define the slice view aligned slice plane. The plane will be positioned
  // at the volume corner furthest from the eye. (model coordinates)
  //
  Vect3d sp(0,0,-0.01,1); // slice point in model coordinates
  Vect3d slicePoint =  modelviewInverse * sp;

  //
  // Calculate the slice plane normal (i.e., the view plane normal transformed
  // into model space). 
  //
  // slicePlaneNormal = modelviewInverse * viewPlaneNormal; 
  //                                       (0,0,1);
  //
  Vect3d slicePlaneNormal(modelviewInverse(2,0), 
                          modelviewInverse(2,1),
                          modelviewInverse(2,2)); 
  slicePlaneNormal.unitize();

  //
  // Calculate edge intersections between the plane and the boxes
  //
  Vect3d verts[6];   // for edge intersections
  Vect3d tverts[6];  // for texture intersections
  Vect3d rverts[6];  // for transformed edge intersections

  { 
    int order[6] ={0,1,2,3,4,5};
    int size = 0;

    //
    // Intersect the slice plane with the bounding boxes
    //
    size = intersect(slicePoint, slicePlaneNormal, 
                     volumeBox, verts, 
                     textureBox, tverts, 
                     rotatedBox, rverts);
    //
    // Calculate the convex hull of the vertices (world coordinates)
    //
    findVertexOrder(rverts, order, size);

    //
    // Draw slice and texture map it
    //
	glBegin(GL_POLYGON); 
	{
      for(int j=0; j< size; ++j)
      {
		glColor3f(tverts[order[j]].x(), 
			 tverts[order[j]].y(), 
			 tverts[order[j]].z()
		);
        
        glTexCoord3f(tverts[order[j]].x(), 
                     tverts[order[j]].y(), 
                     tverts[order[j]].z());

        glVertex3f(verts[order[j]].x(), 
                   verts[order[j]].y(), 
                   verts[order[j]].z());
      }
	}
	glEnd();
  }
}


void DVRRayCaster::drawVolumeFaces(
	const BBox &box, const BBox &tbox
) {

	// Indecies, in proper winding order, of vertices making up planes 
	// of BBox.
	//
	//       6*--------*7
	//       /|       /|
	//      / |      / |
	//     /  |     /  |
	//    /  4*----/---*5
	//  2*--------*3  /
	//   |  /     |  /
	//   | /      | /
	//   |/       |/
	//  0*--------*1
	//
	static const int planes[6][4] = {
		{4,5,7,6},	// front
		{0,2,3,1},	// back
		{5,1,3,7},	// right
		{4,6,2,0},	// left
		{6,7,3,2},	// top
		{4,0,1,5}	// bottom
	};

	for (int i=0; i<6; i++) {
		glBegin(GL_QUADS);

		for (int j=0; j<4; j++) {

			// Color is set to texture coordinates. Thus the frame buffer 
			// will contain the texture coordinates of the pixels on each
			// face. Think we only need color for rendering of back
			// facing faces.
			//
			glColor3f(
				tbox[planes[i][j]].x, 
				tbox[planes[i][j]].y, 
				tbox[planes[i][j]].z
					  );
			// Texture coordinates needed for front facing planes. 
			glTexCoord3f(
				tbox[planes[i][j]].x, 
				tbox[planes[i][j]].y, 
				tbox[planes[i][j]].z
						 );
			glVertex3f(
				box[planes[i][j]].x, 
				box[planes[i][j]].y, 
				box[planes[i][j]].z
					   );
		}
		glEnd();
	}
}

// render the backfacing polygons
void DVRRayCaster::render_backface(
	const TextureBrick *brick
) {
	const BBox &volumeBox  = brick->volumeBox();
	const BBox &textureBox = brick->textureBox();
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

	// Ugh. DVRShader activates texture units 0,1 and 2. We need to
	// disable them or they will be used by the fixed-function pipeline
	// rendering performed here
	//
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE2);
    }
    else {
      glActiveTextureARB(GL_TEXTURE2_ARB);
    }
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);

    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);

    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE0);
    }
    else {
      glActiveTextureARB(GL_TEXTURE0_ARB);
    }
	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);

	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);

	drawVolumeFaces(volumeBox, textureBox);

    glDisable(GL_CULL_FACE);
    if(!myGLWindow->isDepthPeeling()){
	glEnable(GL_BLEND);
    }
	printOpenGLError();

}
// render the front-facing polygons
//
void DVRRayCaster::raycasting_pass(
								   const TextureBrick *brick,
								   const Matrix3d &modelview,
								   const Matrix3d &modelviewInverse
								   ) {

	const BBox &volumeBox  = brick->volumeBox();
	const BBox &textureBox = brick->textureBox();

//#define NOSHADER
#ifndef	NOSHADER
	bool ok = _shadermgr->enableEffect(getCurrentEffect());
	if (! ok) return;

	if (GLEW_VERSION_2_0) {
		
		if (_mapped) {
			glActiveTexture(GL_TEXTURE1);
			glEnable(GL_TEXTURE_1D);
		}
		
		glActiveTexture(_texcrd_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);
		
		glActiveTexture(_depth_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);
		
		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, brick->handle());
		
	} else {
		if (_mapped) {
			glActiveTextureARB(GL_TEXTURE1);
			glEnable(GL_TEXTURE_1D);
		}
		
		glActiveTextureARB(_texcrd_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);
		
		glActiveTextureARB(_depth_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);
		
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, brick->handle());
	}
#endif
	
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	
	printOpenGLError();

	if (_stretched) {
		//
		// If stretched we need to load the inverse lookup table for
		// mapping user coordinates to texture coordinates. Should be
		// smarter about this and only do it when the brick has changed
		//
		size_t x0 = brick->xoffset();
		size_t x1 = x0 + brick->nx() - 1;
		size_t y0 = brick->yoffset();
		size_t y1 = y0 + brick->ny() - 1;
		size_t z0 = brick->zoffset();
		size_t z1 = z0 + brick->nz() - 1;

		loadCoordMap(brick, x0, x1, y0, y1, z0, z1);
	}

	BBox rotatedBox(volumeBox);
	rotatedBox.transform(modelview);
	Point3d camera(0,0,0);
	
	// Call different proxy drawing method depending on whether
	// camera is inside or outside the volume's bounding box
	//
	if (rotatedBox.insideBox(camera)) {
		drawFrontPlane(volumeBox, textureBox, modelview, modelviewInverse);
	}
	else {
		drawVolumeFaces(volumeBox, textureBox);
	}

	_shadermgr->disableEffect();
	
	glDisable(GL_CULL_FACE);
	
#ifndef	NOSHADER

	if (GLEW_VERSION_2_0) {
		glActiveTexture(_depth_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		
		glActiveTexture(_texcrd_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, 0);
	} else {
		glActiveTextureARB(_depth_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		
		glActiveTextureARB(_texcrd_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);
		
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_3D, 0);
	}
	glDisable(GL_TEXTURE_3D);
#endif
	printOpenGLError();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRRayCaster::renderBrick(
	const TextureBrick *brick,
	const Matrix3d &modelview,
	const Matrix3d &modelviewInverse
) {

    _shadermgr->uploadEffectData(
        getCurrentEffect(), "dimensions",
        (float) brick->nx(), (float) brick->ny(), (float) brick->nz()
    );

	// Write depth info.
	glDepthMask(GL_TRUE);

	// Parent class enables default shader
	//_shader->disable();
	_shadermgr->disableEffect();

	// enable rendering to FBO
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, _framebufferid);

	// No depth comparisons
	glDepthFunc(GL_ALWAYS);

	//
	// Need to disable user clipping planes :-(
	//
	glDisable(GL_CLIP_PLANE0);
	glDisable(GL_CLIP_PLANE1);
	glDisable(GL_CLIP_PLANE2);
	glDisable(GL_CLIP_PLANE3);
	glDisable(GL_CLIP_PLANE4);
	glDisable(GL_CLIP_PLANE5);

    render_backface(brick);
    if(myGLWindow->isDepthPeeling()){
      glDisable(GL_BLEND);
      glEnable(GL_DEPTH_TEST);
      //render to previously used fbo
      glBindFramebuffer(GL_FRAMEBUFFER, myGLWindow->currentBuffer);
      
      _shadermgr->uploadEffectData(instanceName("default"), std::string("currentLayer"), myGLWindow->currentLayer);
      _shadermgr->uploadEffectData(instanceName("lighting"), std::string("currentLayer"), myGLWindow->currentLayer);
      _shadermgr->uploadEffectData(instanceName("mapped"), std::string("currentLayer"), myGLWindow->currentLayer);
      _shadermgr->uploadEffectData(instanceName("mapped+lighting"), std::string("currentLayer"), myGLWindow->currentLayer);
#ifdef DEBUG
      std::cout << "DVR current layer: " << myGLWindow->currentLayer << std::endl;
#endif
    }
    else{
      // disable rendering to FBO
      glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }	 
    // Restore normal depth comparison for ray casting pass.
    glDepthFunc(GL_LESS);


    raycasting_pass(brick, modelview, modelviewInverse);

    printOpenGLError();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

void DVRRayCaster::SetIsoValues(
	const float *values, const float *colors, int n
) {

	if (n > GetMaxIsoValues()) n = GetMaxIsoValues();

	_nisos = n;
	for (int i=0; i<n; i++) {
		_values[i] = values[i];
		_colors[i*4+0] = colors[i*4+0];
		_colors[i*4+1] = colors[i*4+1];
		_colors[i*4+2] = colors[i*4+2];
		_colors[i*4+3] = colors[i*4+3];
	}

	initShaderVariables();

}


//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
int DVRRayCaster::initTextures()
{
	// List of acceptable frame buffer object internal types 
	//
	GLint colorInternalFormats[] = {
		GL_RGBA16F_ARB, GL_RGBA16, GL_RGBA16, GL_RGBA8, GL_RGBA
	};
	GLenum colorInternalTypes[] = {
		GL_FLOAT, GL_INT, GL_INT, GL_INT, GL_INT
	};
	int num_color_fmts = 
		sizeof(colorInternalFormats)/sizeof(colorInternalFormats[0]);

	GLint depthInternalFormats[] = {
		GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT, 
		GL_DEPTH_COMPONENT32, GL_DEPTH_COMPONENT24, GL_DEPTH_COMPONENT
	};
	GLenum depthInternalTypes[] = {
		GL_FLOAT, GL_FLOAT, GL_FLOAT,
		GL_INT, GL_INT, GL_INT,
	};
	int num_depth_fmts = 
		sizeof(depthInternalFormats)/sizeof(depthInternalFormats[0]);

	if (DVRShader::initTextures() < 0) return(-1);

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Create the to FBO - one for the backside of the volumecube
	//
	glGenFramebuffersEXT(1, &_framebufferid);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,_framebufferid);

	glGenTextures(1, &_backface_texcrd_texid);
	glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);

	// Try to find a supported format
	//
	GLenum status;
	for (int i=0; i<num_color_fmts; i++) {
		glTexImage2D(
			GL_TEXTURE_2D, 0,colorInternalFormats[i], 
			viewport[2], viewport[3], 0, GL_RGBA, colorInternalTypes[i], NULL
		);
		glFramebufferTexture2DEXT(
			GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
			_backface_texcrd_texid, 0
		);
		status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
			_colorInternalFormat = colorInternalFormats[i];
			_colorInternalType = colorInternalTypes[i];
			break;
		}
	}
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		SetErrMsg(VAPOR_ERROR_DRIVER_FAILURE,
			"Failed to create OpenGL color framebuffer_object : %d", status
		);
		return(-1);
	}

	glGenTextures(1, &_backface_depth_texid);
	glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);
	glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameterf( GL_TEXTURE_2D, GL_DEPTH_TEXTURE_MODE, GL_LUMINANCE);
	
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);

	for (int i=0; i<num_depth_fmts; i++) {
		glTexImage2D(
			GL_TEXTURE_2D, 0,depthInternalFormats[i], viewport[2], viewport[3],
			0, GL_DEPTH_COMPONENT, depthInternalTypes[i], NULL
		);

		glFramebufferTexture2DEXT(
			GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 
			_backface_depth_texid, 0
		);

		status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status == GL_FRAMEBUFFER_COMPLETE_EXT) {
			_depthInternalFormat = depthInternalFormats[i];
			_depthInternalType = depthInternalTypes[i];
			break;
		}
	}
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		SetErrMsg(VAPOR_ERROR_DRIVER_FAILURE,
			"Failed to create OpenGL depth framebuffer_object : %d", status
		);
		return(-1);
	}

	// Bind the default frame buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	if (printOpenGLError() != 0) return(-1);
	return(0);
}

void DVRRayCaster::initShaderVariables() {	

	if (! _mapped) {
		_shadermgr->uploadEffectData(
			getCurrentEffect(), "isocolor", (float)_colors[0], 
			(float)_colors[1], (float)_colors[2], (float)_colors[3]
		);
	}
	else {
		_shadermgr->uploadEffectData(getCurrentEffect(), "vidx", _vidx);
	}

	_shadermgr->uploadEffectData(getCurrentEffect(), "isovalue", _values[0]);

	DVRShader::initShaderVariables();

}

void DVRRayCaster::Resize(int width, int height) {

	if (GLEW_VERSION_2_0) {

		glActiveTexture(_texcrd_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);

		glActiveTexture(_depth_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);

	} else {

		glActiveTextureARB(_texcrd_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);

		glActiveTextureARB(_depth_texunit);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);

	}

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,_framebufferid);

	glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);
	glTexImage2D(
		GL_TEXTURE_2D, 0,_colorInternalFormat, width, height, 0, 
		GL_RGBA, _colorInternalType, NULL
	);

	glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);
	glTexImage2D(
		GL_TEXTURE_2D, 0,_depthInternalFormat, width, height, 0, 
		GL_DEPTH_COMPONENT, _depthInternalType, NULL
	);

	// Bind the default frame buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	if (GLEW_VERSION_2_0) {
		glActiveTexture(_depth_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		glActiveTexture(_texcrd_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

	} else {
		glActiveTextureARB(_depth_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

		glActiveTextureARB(_texcrd_texunit);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

	}
}


//----------------------------------------------------------------------------
// Returns if the system supports iso shading
//----------------------------------------------------------------------------
bool DVRRayCaster::supported()
{
	GLint	ntexunits;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &ntexunits);
	return (
		DVRShader::supported() && 
		GLEW_EXT_framebuffer_object && 
		GLEW_ARB_vertex_buffer_object &&
		ntexunits >= 4
	);
}
//----------------------------------------------------------------------------
// Returns the currently active shader effect
//----------------------------------------------------------------------------
std::string DVRRayCaster::getCurrentEffect()
{
	if (_lighting && _mapped) {
		return instanceName("mapped+lighting");
	}
	else if (_lighting) {
		return instanceName("lighting");
	}
	else if (_mapped) {
		return instanceName("mapped");
	}
	else {
		return instanceName("default");
	}
	
}
