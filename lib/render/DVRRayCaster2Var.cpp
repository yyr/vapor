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

#include "DVRRayCaster2Var.h"
#include "TextureBrick.h"
#include "ShaderProgram.h"
#include "BBox.h"
#include "glutil.h"
#include "params.h"

#include "Matrix3d.h"
#include "Point3d.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
DVRRayCaster2Var::DVRRayCaster2Var(
	int precision, int nvars, ShaderMgr *shadermgr,
	int nthreads
) : DVRRayCaster(precision, nvars, shadermgr, nthreads)
{
	MyBase::SetDiagMsg(
		"DVRRayCaster2Var::DVRRayCaster2Var( %d %d %d)", 
		precision, nvars, nthreads
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


bool DVRRayCaster2Var::setShaderTextures(){

    if (! _shadermgr->defineEffect("Iso", "MAPPED;", instanceName("mapped")))
        return(false);

    if (! _shadermgr->defineEffect(
        "Iso", "MAPPED; LIGHTING;", instanceName("mapped+lighting")
    )) return(false);

	
	if (! _shadermgr->uploadEffectData(instanceName("mapped"), "volumeTexture", 0)) return(false);	

	if (! _shadermgr->uploadEffectData(instanceName("mapped"), "texcrd_buffer", _texcrd_sampler)) return(false);

	if (! _shadermgr->uploadEffectData(instanceName("mapped"), "colormap", 1)) return(false);	

	if (! _shadermgr->uploadEffectData(instanceName("mapped"), "depth_buffer", _depth_sampler)) return(false);	

	if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "volumeTexture", 0)) return(false);	

	if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "colormap", 1)) return(false);	

	if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "texcrd_buffer", _texcrd_sampler)) return(false);	

	if (! _shadermgr->uploadEffectData(instanceName("mapped+lighting"), "depth_buffer", _depth_sampler)) return(false);	

	return true;
}
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
int DVRRayCaster2Var::GraphicsInit() 
{
	glewInit();

	if (initTextures() < 0) return(-1);

	if (! setShaderTextures()) return (-1);
	initShaderVariables();

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
	DVRRayCaster::raycasting_pass(brick, modelview, modelviewInverse, getCurrentEffect());
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
	_shadermgr->uploadEffectData(getCurrentEffect(), "isovalue", _values[0]);
	
	if (_lighting) {		
		_shadermgr->uploadEffectData(getCurrentEffect(), "dimensions", (float)_nx, (float)_ny, (float)_nz);
		_shadermgr->uploadEffectData(getCurrentEffect(), "kd", _kd);
		_shadermgr->uploadEffectData(getCurrentEffect(), "ka", _ka);
		_shadermgr->uploadEffectData(getCurrentEffect(), "ks", _ks);
		_shadermgr->uploadEffectData(getCurrentEffect(), "expS", _expS);
		_shadermgr->uploadEffectData(getCurrentEffect(), "lightDirection", _pos[0], _pos[1], _pos[2]);
		
	}
}

std::string DVRRayCaster2Var::getCurrentEffect(){
	
	if (_lighting) {
		return instanceName("mapped+lighting");
	}
	else {
		return instanceName("mapped");
	}
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
