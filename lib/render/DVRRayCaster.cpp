//--DVRRayCaster.cpp -----------------------------------------------------------
//
// $Id$
//
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#ifdef Darwin
#include <gl.h>
#include <glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

#include <qgl.h>
#include "renderer.h"
#include "DVRRayCaster.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"
#include "params.h"


#include "Matrix3d.h"
#include "Point3d.h"
#include "raycast_shaders.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRRayCaster::DVRRayCaster(
	GLint internalFormat, GLenum format, GLenum type, int nthreads, Renderer* ren
) : DVRShader(internalFormat, format, type, nthreads, ren)
{
	MyBase::SetDiagMsg(
		"DVRRayCaster::DVRRayCaster( %d %d %d %d)", 
		internalFormat, format, type, nthreads
	);
	_lighting = false;
	_framebufferid = 0;
	_backface_texcrd_texid = 0;
	_backface_depth_texid = 0;

	_nisos = 0;

	if (GLEW_VERSION_2_0) {
		_texcrd_texunit = GL_TEXTURE1;
		_depth_texunit = GL_TEXTURE2;
	}
	else {
		_texcrd_texunit = GL_TEXTURE1_ARB;
		_depth_texunit = GL_TEXTURE2_ARB;
	}
	_texcrd_sampler = 1;
	_depth_sampler = 2;
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
}

bool DVRRayCaster::createShader(ShaderType type,
                             const char *vertexCommandLine,
                             const char *vertexSource,
                             const char *fragCommandLine,
                             const char *fragmentSource)
{
	_shaders[type] = new ShaderProgram();
	_shaders[type]->create();

	//
	// Vertex shader
	//
	if (Params::searchCmdLine(vertexCommandLine)) {
		_shaders[type]->loadVertexShader(Params::parseCmdLine(vertexCommandLine));
	} 
	else if (vertexSource) {
		_shaders[type]->loadVertexSource(vertexSource);
	}

	//
	// Fragment shader
	//
	if (Params::searchCmdLine(fragCommandLine)) {
		_shaders[type]->loadFragmentShader(Params::parseCmdLine(fragCommandLine));
	}
	else if (fragmentSource) {
		_shaders[type]->loadFragmentSource(fragmentSource);
	}

	if (!_shaders[type]->compile()) {
		return false;
	}

	//
	// Set up initial uniform values
	//
	if (type != BACKFACE) {
		if (_shaders[type]->enable() < 0) return(false);;

		if (GLEW_VERSION_2_0) {
			glUniform1i(_shaders[type]->uniformLocation("volumeTexture"), 0);
			glUniform1i(
				_shaders[type]->uniformLocation("texcrd_buffer"), 
				_texcrd_sampler
			);
			glUniform1i(
				_shaders[type]->uniformLocation("depth_buffer"), _depth_sampler
			);
		}
		else {
			glUniform1iARB(_shaders[type]->uniformLocation("volumeTexture"), 0);
			glUniform1iARB(
				_shaders[type]->uniformLocation("texcrd_buffer"), 
				_texcrd_sampler
			);
			glUniform1iARB(
				_shaders[type]->uniformLocation("depth_buffer"), _depth_sampler
			);
		}

		_shaders[type]->disable();
	}

	printOpenGLError();
	return true;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::GraphicsInit() 
{
	glewInit();

	if (initTextures() < 0) return(-1);

	//
	// Create, Load & Compile the shader programs
	//

	if (!createShader(
		DEFAULT, 
		"--iso-vertex-shader", vertex_shader_iso,
		"--iso-fragment-shader", fragment_shader_iso)) {

		return -1;
	}
	if (!createShader(
		LIGHT, 
		"--iso-lighting-vertex-shader", vertex_shader_iso_lighting,
		"--iso-lighting-fragment-shader", fragment_shader_iso_lighting)) {

		return -1;
	}

#ifdef	DEAD
	if (!createShader(
		BACKFACE, NULL, NULL,
		"--backface-fragment-shader", fragment_shader_backface)) {

		return -1;
	}
#endif

	//
	// Set the current shader
	//

	_shader = _shaders[DEFAULT];

	return 0;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::Render(const float matrix[16])
{
	if (_shader) if (_shader->enable() < 0) return(0);

	DVRTexture3d::calculateSampling();

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	if (GLEW_VERSION_2_0) {
		glUniform1f(_shader->uniformLocation("delta"), _deltaEye);
		if (_lighting) {
			glUniform2f(
				_shader->uniformLocation("winsize"), viewport[2], viewport[3]
			);
		}
	} else {
		glUniform1fARB(_shader->uniformLocation("delta"), _deltaEye);
		if (_lighting) {
			glUniform2fARB(
				_shader->uniformLocation("winsize"), viewport[2], viewport[3]
			);
		}
	}
	if (_shader) _shader->disable();

	return(DVRShader::Render(matrix));

	return 0;
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


//cerr << "DrawVolumeFaces()\n";
	for (int i=0; i<6; i++) {
		glBegin(GL_QUADS);
		//cerr << "glBegin(GL_QUADS)\n";

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
			//cerr << "glColor3f : " 
			//	<< tbox[planes[i][j]].x << " "
			//	<< tbox[planes[i][j]].y << " " 
			//	<< tbox[planes[i][j]].z << endl;

			// Texture coordinates needed for front facing planes. 
			glTexCoord3f(
				tbox[planes[i][j]].x, 
				tbox[planes[i][j]].y, 
				tbox[planes[i][j]].z
			);
			//cerr << "glTexCoord3f : " 
			//	<< tbox[planes[i][j]].x << " "
			//	<< tbox[planes[i][j]].y << " " 
			//	<< tbox[planes[i][j]].z << endl;

			glVertex3f(
				box[planes[i][j]].x, 
				box[planes[i][j]].y, 
				box[planes[i][j]].z
			);
			//cerr << "glVertex3f : "
			//	<< box[planes[i][j]].x << " "
			//	<< box[planes[i][j]].y << " " 
			//	<< box[planes[i][j]].z << endl;
		}
		//cerr << endl;
		glEnd();
	}
}

// render the backfacing polygons
void DVRRayCaster::render_backface(
	const TextureBrick *brick
) {
	const BBox &volumeBox  = brick->volumeBox();
	const BBox &textureBox = brick->textureBox();

//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

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
	glEnable(GL_BLEND);
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


	if (GLEW_VERSION_2_0) {


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

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);


	if (_shader->enable() == 0) {

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
		_shader->disable();
	}

	glDisable(GL_CULL_FACE);

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

	// Write depth info.
	glDepthMask(GL_TRUE);

	// Parent class enables default shader
	_shader->disable();

	// enable rendering to FBO
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, _framebufferid);

	// No depth comparisons
	glDepthFunc(GL_ALWAYS);

    render_backface(brick);
	 
	// disable rendering to FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

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
		myRenderer->setAllBypass(true);
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
	
	// Per OpenGL spec:
	// If a fragment program specifies the "ARB_fragment_program_shadow"
    // program option, the result returned of a sample from a texture target on
    // a texture image unit is undefined if :
	// the texture target is SHADOW1D, SHADOW2D, SHADOWRECT, and
	// the texture object's internal format is DEPTH_COMPONENT_ARB, and
	// the TEXTURE_COMPARE_MODE_ARB is NONE;
	
	/* 
	 BUGFIX:
	 Author: Yannick Polius
	 Date 05/03/11 
	 Summary: changed the texture params to comply with OpenGL spec */
	
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_NONE);
	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);
	
//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
//	glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_NEVER);

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
		myRenderer->setAllBypass(true);
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

	assert(_shader);

	if (_shader->enable() < 0) return;

	if (GLEW_VERSION_2_0) {

		glUniform4f(
			_shader->uniformLocation("isocolor"), 
			_colors[0], _colors[1], _colors[2], _colors[3]
		);
		glUniform1f(_shader->uniformLocation("isovalue"), _values[0]);
	} else {

		glUniform4fARB(
			_shader->uniformLocation("isocolor"), 
			_colors[0], _colors[1], _colors[2], _colors[3]
		);
		glUniform1fARB(_shader->uniformLocation("isovalue"), _values[0]);
	}

	if (_lighting) {
		if (GLEW_VERSION_2_0) {   
			glUniform3f(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
			glUniform1f(_shader->uniformLocation("kd"), _kd);
			glUniform1f(_shader->uniformLocation("ka"), _ka);
			glUniform1f(_shader->uniformLocation("ks"), _ks);
			glUniform1f(_shader->uniformLocation("expS"), _expS);
			glUniform3f(
				_shader->uniformLocation("lightDirection"), 
				_pos[0], _pos[1], _pos[2]
			);
		}       
		else {   
			glUniform3fARB(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
			glUniform1fARB(_shader->uniformLocation("kd"), _kd);
			glUniform1fARB(_shader->uniformLocation("ka"), _ka);
			glUniform1fARB(_shader->uniformLocation("ks"), _ks);
			glUniform1fARB(_shader->uniformLocation("expS"), _expS);
			glUniform3fARB(
				_shader->uniformLocation("lightDirection"), 
				_pos[0], _pos[1], _pos[2]
			);
		}       
	}

	_shader->disable();
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
//
//----------------------------------------------------------------------------
bool DVRRayCaster::supported()
{
	GLint	ntexunits;
	glGetIntegerv(GL_MAX_TEXTURE_UNITS, &ntexunits);
	return (
		DVRShader::supported() && 
		GLEW_EXT_framebuffer_object && 
		GLEW_ARB_vertex_buffer_object &&
		ntexunits >= 3
	);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ShaderProgram* DVRRayCaster::shader()
{
	if (_lighting) {
		return _shaders[LIGHT];
	}
	else {
		return _shaders[DEFAULT];
	}
}
