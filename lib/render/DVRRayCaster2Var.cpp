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
#include "DVRRayCaster2Var.h"
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
DVRRayCaster2Var::DVRRayCaster2Var(
	GLint internalFormat, GLenum format, GLenum type, int nthreads, Renderer* ren
) : DVRRayCaster(internalFormat, format, type, nthreads, ren)
{
	MyBase::SetDiagMsg(
		"DVRRayCaster2Var::DVRRayCaster2Var( %d %d %d %d)", 
		internalFormat, format, type, nthreads
	);

	if (GLEW_VERSION_2_0) {
		_texcrd_texunit = GL_TEXTURE2;
		_depth_texunit = GL_TEXTURE3;
	}
	else {
		_texcrd_texunit = GL_TEXTURE2_ARB;
		_depth_texunit = GL_TEXTURE3_ARB;
	}
	_texcrd_sampler = 2;
	_depth_sampler = 3;
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
DVRRayCaster2Var::~DVRRayCaster2Var() 
{
	MyBase::SetDiagMsg("DVRRayCaster2Var::~DVRRayCaster2Var()");
}

bool DVRRayCaster2Var::createShader(ShaderType type,
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
			glUniform1i(_shaders[type]->uniformLocation("colormap"), 1);
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
			glUniform1iARB(_shaders[type]->uniformLocation("colormap"), 1);
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

	return true;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster2Var::GraphicsInit() 
{
	glewInit();

	if (initTextures() < 0) return(-1);

	//
	// Create, Load & Compile the shader programs
	//

	if (!createShader(
		DEFAULT, 
		"--iso-color-vertex-shader", vertex_shader_iso_color,
		"--iso-color-fragment-shader", fragment_shader_iso_color)) {

		return -1;
	}
	if (!createShader(
		LIGHT, 
		"--iso-color-lighting-vertex-shader", vertex_shader_iso_color_lighting,
		"--iso-color-lighting-fragment-shader", fragment_shader_iso_color_lighting)) {

		return -1;
	}

	//
	// Set the current shader
	//

	_shader = _shaders[DEFAULT];

	return 0;
}

// render the front-facing polygons
//
void DVRRayCaster2Var::raycasting_pass(
	const TextureBrick *brick, 
	const Matrix3d &modelview, const Matrix3d &modelviewInverse
) {

	// enable color map
  if (_preintegration) {
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
    glEnable(GL_TEXTURE_2D);  
  }
  else {
    if (GLEW_VERSION_2_0) {
      glActiveTexture(GL_TEXTURE1);
    }
    else {
      glActiveTextureARB(GL_TEXTURE1_ARB);
    }
    glEnable(GL_TEXTURE_1D);
  }
	DVRRayCaster::raycasting_pass(brick, modelview, modelviewInverse);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------

void DVRRayCaster2Var::SetIsoValues(
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


void DVRRayCaster2Var::initShaderVariables() {

	assert(_shader);

	if (_shader->enable() < 0) return;

	if (GLEW_VERSION_2_0) {
		//glUniform1fv(_shader->uniformLocation("isovalues"), _nisos, _values);
		//glUniform1i(_shader->uniformLocation("numiso"), _nisos);

		glUniform1f(_shader->uniformLocation("isovalues"), _values[0]);
		glUniform1f(_shader->uniformLocation("zN"), _nearClip);
		glUniform1f(_shader->uniformLocation("zF"), _farClip);
	} else {
		//glUniform1fvARB(_shader->uniformLocation("isovalues"), _nisos, _values);
		//glUniform1iARB(_shader->uniformLocation("numiso"), _nisos);

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

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool DVRRayCaster2Var::supported()
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
