//--DVRRayCaster.cpp -----------------------------------------------------------
//
// $Id$
//
//
//----------------------------------------------------------------------------

#include <GL/glew.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <qgl.h>

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
DVRRayCaster::DVRRayCaster(DataType_T type, int nthreads) :
  DVRShader(type, nthreads)
{
	_lighting = false;
	_framebufferid = 0;
	_backface_texcrd_texid = 0;
	_backface_depth_texid = 0;
	_nearClip = 1.0;
	_farClip = 2.0;

	_nisos = 0;

}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRRayCaster::~DVRRayCaster() 
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	if (_framebufferid) glDeleteFramebuffersEXT(1, &_framebufferid);

	if (_backface_texcrd_texid) glDeleteTextures(1, &_backface_texcrd_texid);
	if (_backface_depth_texid) glDeleteTextures(1, &_backface_depth_texid);
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
		_shaders[type]->enable();

		if (GLEW_VERSION_2_0) {
			glUniform1i(_shaders[type]->uniformLocation("volumeTexture"), 0);
			glUniform1i(_shaders[type]->uniformLocation("texcrd_buffer"), 1);
			glUniform1i(_shaders[type]->uniformLocation("depth_buffer"), 2);
		}
		else {
			glUniform1iARB(_shaders[type]->uniformLocation("volumeTexture"), 0);
			glUniform1iARB(_shaders[type]->uniformLocation("texcrd_buffer"), 1);
			glUniform1iARB(_shaders[type]->uniformLocation("depth_buffer"), 2);
		}

		_shaders[type]->disable();
	}

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
	if (_shader) _shader->enable();

	DVRTexture3d::calculateSampling();
	float delta = _delta * 2;

	if (GLEW_VERSION_2_0) {
		glUniform1f(_shader->uniformLocation("delta"), delta);
	} else {
		glUniform1fARB(_shader->uniformLocation("delta"), delta);
	}
	if (_shader) _shader->disable();

	return(DVRShader::Render(matrix));

	return 0;
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
	const BBox &box, const BBox &tbox
) {

//    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

	glDisable(GL_TEXTURE_1D);
	glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_3D);
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glDisable(GL_BLEND);

#ifdef	DEAD
	_shaders[BACKFACE]->enable();
#endif
	drawVolumeFaces(box, tbox);
#ifdef	DEAD
	_shaders[BACKFACE]->disable();
#endif

    glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);
printOpenGLError();

}

// render the front-facing polygons
//
void DVRRayCaster::raycasting_pass(
	const TextureBrick *brick, const BBox &box, const BBox &tbox
) {

	glEnable(GL_TEXTURE_2D);
	glEnable(GL_TEXTURE_3D);

	if (GLEW_VERSION_2_0) {


		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, brick->handle());

	} else {

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);

		glActiveTextureARB(GL_TEXTURE2_ARB);
		glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_3D, brick->handle());
	}

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	_shader->enable();
	drawVolumeFaces(box, tbox);
	_shader->disable();

	glDisable(GL_CULL_FACE);

	if (GLEW_VERSION_2_0) {
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_3D, 0);
	} else {
		glActiveTextureARB(GL_TEXTURE2_ARB);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glBindTexture(GL_TEXTURE_2D, 0);

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glBindTexture(GL_TEXTURE_3D, 0);
	}
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
	BBox volumeBox  = brick->volumeBox();
	BBox textureBox = brick->textureBox();

	// Write depth info.
	glDepthMask(GL_TRUE);

	// Parent class enables default shader
	_shader->disable();

	// enable rendering to FBO
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, _framebufferid);

	// No depth comparisons
	glDepthFunc(GL_ALWAYS);

    render_backface(volumeBox, textureBox);
	 
	// disable rendering to FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	// Restore normal depth comparison for ray casting pass.
	glDepthFunc(GL_LESS);

	raycasting_pass(brick, volumeBox, textureBox);

	printOpenGLError();
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::HasType(DataType_T type) 
{
	if (type == UINT8 || type == UINT16) 
		return(1);
	else 
		return(0);
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

void DVRRayCaster::SetNearFar(GLfloat nearplane, GLfloat farplane) {

	_nearClip = nearplane;
	_farClip = farplane;
	initShaderVariables();
}






//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
int DVRRayCaster::initTextures()
{

	DVRShader::initTextures();

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
	glTexImage2D(
		GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, viewport[2], viewport[3], 0, 
		GL_RGBA, GL_FLOAT, NULL
	);
	glFramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
		_backface_texcrd_texid, 0
	);
printOpenGLError();

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

	glTexImage2D(
		GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT32, viewport[2], viewport[3], 0, 
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL
	);
	glFramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT, GL_TEXTURE_2D, 
		_backface_depth_texid, 0
	);
printOpenGLError();

	GLenum status = (GLenum) glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT) {
		cerr << "Failed to create fbo\n";
		return(-1);
	}

	// Bind the default frame buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	glFlush();	// Necessary???
printOpenGLError();
	return(0);
}

void DVRRayCaster::initShaderVariables() {

	assert(_shader);


	_shader->enable();

	if (GLEW_VERSION_2_0) {
		//glUniform1fv(_shader->uniformLocation("isovalues"), _nisos, _values);
		//glUniform4fv(_shader->uniformLocation("isocolors"), _nisos, _colors);
		//glUniform1i(_shader->uniformLocation("numiso"), _nisos);

		glUniform4f(
			_shader->uniformLocation("isocolors"), 
			_colors[0], _colors[1], _colors[2], _colors[3]
		);
		glUniform1f(_shader->uniformLocation("isovalues"), _values[0]);
		glUniform1f(_shader->uniformLocation("zN"), _nearClip);
		glUniform1f(_shader->uniformLocation("zF"), _farClip);
	} else {
		//glUniform1fvARB(_shader->uniformLocation("isovalues"), _nisos, _values);
		//glUniform4fvARB(_shader->uniformLocation("isocolors"), _nisos, _colors);
		//glUniform1iARB(_shader->uniformLocation("numiso"), _nisos);

		glUniform4fARB(
			_shader->uniformLocation("isocolors"), 
			_colors[0], _colors[1], _colors[2], _colors[3]
		);
		glUniform1fARB(_shader->uniformLocation("isovalues"), _values[0]);
		glUniform1fARB(_shader->uniformLocation("zN"), _nearClip);
		glUniform1fARB(_shader->uniformLocation("zF"), _farClip);
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

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,_framebufferid);

	glBindTexture(GL_TEXTURE_2D, _backface_texcrd_texid);
	glTexImage2D(
		GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, width, height, 0, 
		GL_RGBA, GL_FLOAT, NULL
	);

	glBindTexture(GL_TEXTURE_2D, _backface_depth_texid);
	glTexImage2D(
		GL_TEXTURE_2D, 0,GL_DEPTH_COMPONENT32, width, height, 0, 
		GL_DEPTH_COMPONENT, GL_FLOAT, NULL
	);

	// Bind the default frame buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
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
