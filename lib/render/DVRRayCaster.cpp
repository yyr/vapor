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
	_backface_bufferid = 0;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRRayCaster::~DVRRayCaster() 
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,0);
	if (_framebufferid) glDeleteFramebuffersEXT(1, &_framebufferid);

	if (_backface_bufferid) glDeleteTextures(1, &_backface_bufferid);
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
		float values[] = {0.5};
		float colors[] = {1.0, 0.0, 0.0, 1.0};
		//int n = sizeof(values) / sizeof(values[0]);
		_shaders[type]->enable();

		if (GLEW_VERSION_2_0) {
			glUniform1i(_shaders[type]->uniformLocation("volumeTexture"), 0);
			glUniform1i(_shaders[type]->uniformLocation("backface_buffer"), 1);
			//glUniform1fv(_shaders[type]->uniformLocation("isovalues"), n, values);
			//glUniform4fv(_shaders[type]->uniformLocation("isocolors"), n, colors);
			//glUniform1i(_shaders[type]->uniformLocation("numiso"), n);
			glUniform4f(_shaders[type]->uniformLocation("isocolors"), colors[0], colors[1], colors[2], colors[3]);
			glUniform1f(_shaders[type]->uniformLocation("isovalues"), values[0]);
		}
		else {
			glUniform1iARB(_shaders[type]->uniformLocation("volumeTexture"), 0);
			glUniform1iARB(_shaders[type]->uniformLocation("backface_buffer"), 1);
			//glUniform1fvARB(_shaders[type]->uniformLocation("isovalues"), n, values);
			//glUniform4fvARB(_shaders[type]->uniformLocation("isocolors"), n, colors);
			//glUniform1iARB(_shaders[type]->uniformLocation("numiso"), n);
			glUniform4fARB(_shaders[type]->uniformLocation("isocolors"), colors[0], colors[1], colors[2], colors[3]);
			glUniform1fARB(_shaders[type]->uniformLocation("isovalues"), values[0]);
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

	initTextures();

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

	if (!createShader(
		BACKFACE, NULL, NULL,
		"--backface-fragment-shader", fragment_shader_backface)) {

		return -1;
	}

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
cerr << "Delta hardcoded = " << delta << endl;
cerr << "Dimensions need not be set here - set in DVRShader\n";
	if (GLEW_VERSION_2_0) {
		glUniform1f(_shader->uniformLocation("delta"), delta);
		if (_lighting) {
			glUniform3f(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
		}
	} else {
		glUniform1fARB(_shader->uniformLocation("delta"), delta);
		if (_lighting) {
			glUniform3fARB(_shader->uniformLocation("dimensions"), _nx, _ny, _nz);
		}
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

	_shaders[BACKFACE]->enable();
	drawVolumeFaces(box, tbox);
	_shaders[BACKFACE]->disable();

    glDisable(GL_CULL_FACE);
	glEnable(GL_BLEND);

}

void DVRRayCaster::raycasting_pass(
	const TextureBrick *brick, const BBox &box, const BBox &tbox
) {
	//glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );


	if (GLEW_VERSION_2_0) {

		glActiveTexture(GL_TEXTURE0);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, brick->handle());

		glActiveTexture(GL_TEXTURE1);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_bufferid);

	} else {

		glActiveTextureARB(GL_TEXTURE0_ARB);
		glEnable(GL_TEXTURE_3D);
		glBindTexture(GL_TEXTURE_3D, brick->handle());

		glActiveTextureARB(GL_TEXTURE1_ARB);
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, _backface_bufferid);
	}

#ifdef	DEAD
_shader->disable();
glDisable(GL_TEXTURE_1D);
glDisable(GL_TEXTURE_2D);
glDisable(GL_TEXTURE_3D);
#endif

	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);

	_shader->enable();
	drawVolumeFaces(box, tbox);
	_shader->disable();

	glDisable(GL_CULL_FACE);

	if (GLEW_VERSION_2_0) {
		glActiveTexture(GL_TEXTURE1);
		glDisable(GL_TEXTURE_2D);
		glActiveTexture(GL_TEXTURE0);
		glDisable(GL_TEXTURE_3D);
	} else {
		glActiveTextureARB(GL_TEXTURE1_ARB);
		glDisable(GL_TEXTURE_2D);
		glActiveTextureARB(GL_TEXTURE0_ARB);
		glDisable(GL_TEXTURE_3D);
	}
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

	// Parent class enables default shader
	_shader->disable();

	// enable rendering to FBO
	glBindFramebufferEXT (GL_FRAMEBUFFER_EXT, _framebufferid);

	// Render to texture
    glFramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
		_backface_bufferid, 0
	);

    render_backface(volumeBox, textureBox);

	// disable rendering to FBO
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	raycasting_pass(brick, volumeBox, textureBox);

	printOpenGLError();
}



//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster::HasType(DataType_T type) 
{
  if (type == UINT8) 
    return(1);
  else 
    return(0);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void DVRRayCaster::SetLightingOnOff(int on) 
{
	_lighting = on;

	_shader = shader(); assert(_shader);

}

void DVRRayCaster::SetIsoValues(
	const float *values, const float *colors, int n
) {
	if (n > GetMaxIsoValues()) n = GetMaxIsoValues();

	if (_shader) _shader->enable();
	if (GLEW_VERSION_2_0) {
		//glUniform1fv(_shader->uniformLocation("isovalues"), n, values);
		//glUniform4fv(_shader->uniformLocation("isocolors"), n, colors);
		//glUniform1i(_shader->uniformLocation("numiso"), n);
		glUniform4f(_shader->uniformLocation("isocolors"), colors[0], colors[1], colors[2], colors[3]);
		glUniform1f(_shader->uniformLocation("isovalues"), values[0]);
	} else {
		//glUniform1fvARB(_shader->uniformLocation("isovalues"), n, values);
		//glUniform4fvARB(_shader->uniformLocation("isocolors"), n, colors);
		//glUniform1iARB(_shader->uniformLocation("numiso"), n);
		glUniform4fARB(_shader->uniformLocation("isocolors"), colors[0], colors[1], colors[2], colors[3]);
		glUniform1fARB(_shader->uniformLocation("isovalues"), values[0]);
	}
	if (_shader) _shader->disable();
}



//----------------------------------------------------------------------------
// Initalize the textures (i.e., 3d volume texture and the 1D colormap texture)
//----------------------------------------------------------------------------
void DVRRayCaster::initTextures()
{

	DVRShader::initTextures();

	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	// Create the to FBO - one for the backside of the volumecube
	//
	glGenFramebuffersEXT(1, &_framebufferid);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,_framebufferid);

	glGenTextures(1, &_backface_bufferid);
	glBindTexture(GL_TEXTURE_2D, _backface_bufferid);
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
		_backface_bufferid, 0
	);

	// Bind the default frame buffer
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	glFlush();	// Necessary???
}

void DVRRayCaster::Resize(int width, int height) {

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT,_framebufferid);

	glBindTexture(GL_TEXTURE_2D, _backface_bufferid);
	glTexImage2D(
		GL_TEXTURE_2D, 0,GL_RGBA16F_ARB, width, height, 0, 
		GL_RGBA, GL_FLOAT, NULL
	);
	glFramebufferTexture2DEXT(
		GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, 
		_backface_bufferid, 0
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
		ntexunits >= 2
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
