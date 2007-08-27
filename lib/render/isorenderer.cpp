//-- isorendeerr.cpp --------------------------------------------------------
//   
//                   Copyright (C)  2004
//     University Corporation for Atmospheric Research
//                   All Rights Reserved
//
//----------------------------------------------------------------------------
//
//      File:           isorenderer.cpp
//
//      Author:         Alan Norton
//                      National Center for Atmospheric Research
//                      PO 3000, Boulder, Colorado
//
//      Date:           August 2007
//
//      Description:  Implementation of IsoRenderer class
//
// Class is derived from VolumeRenderer, since isosurface rendering
// has a similar implementation
//
//
//
//----------------------------------------------------------------------------

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <sys/types.h>
#include <ctype.h>
#include <errno.h>

#include <qgl.h>
#include <qmessagebox.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qstring.h>
#include <qstatusbar.h>

#include "vapor/errorcodes.h"
#include "isorenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "transferfunction.h"

#include "glwindow.h"

#include "DVRLookup.h"
#include "DVRShader.h"
#include "DVRSpherical.h"
#include "DVRRayCaster.h"

#include "DVRDebug.h"
#include "params.h"
#include "renderer.h"

#include "Stopwatch.h"


#include "glutil.h"
#include "dvrparams.h"
#include "ParamsIso.h"
#include "vapor/MyBase.h"

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
IsoRenderer::IsoRenderer(GLWindow* glw, DvrParams::DvrType type, RenderParams* rp) 
  : VolumeRenderer(glw, type, rp)
	
{
	setRenderParams(rp);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
IsoRenderer::~IsoRenderer()
{
 
}
void IsoRenderer::setRenderParams(RenderParams* rp) {

	Renderer::setRenderParams(rp);

    ParamsIso *myParamsIso = (ParamsIso *) rp;

	myParamsIso->RegisterIsoValueDirtyFlag(&_isovalueDF);
	myParamsIso->RegisterConstantColorDirtyFlag(&_isovalueDF);
	myParamsIso->RegisterNormalOnOffDirtyFlag(&_lightOnOffDF);
}

void IsoRenderer::UpdateDriverRenderParamsSpec(RenderParams *rp) {

    ParamsIso *myParamsIso = (ParamsIso *) rp;
	DVRRayCaster *driver = (DVRRayCaster *) _driver;


	if (_isovalueDF.Test()) {

		float isoval = myParamsIso->GetIsoValue();
		const float *color = myParamsIso->GetConstantColor();

		// Normalize isovalue
		const float *range = currentRenderParams->getCurrentDatarange();
		isoval = (isoval - range[0]) / (range[1] - range[0]);

		driver->SetIsoValues(&isoval, color, 1);

		_isovalueDF.Clear();
	}

	bool lighting = myParamsIso->GetNormalOnOff();
	if (_lightOnOffDF.Test()) {
		driver->SetLightingOnOff(lighting);
		_lightOnOffDF.Clear();
	}

	// No dirty flags for lighting parameters. Sigh.
	//
	if (lighting) {
		ViewpointParams *vpParams = myGLWindow->getActiveViewpointParams(); 

		_driver->SetLightingCoeff(
			vpParams->getDiffuseCoeff(0), vpParams->getAmbientCoeff(),
			vpParams->getSpecularCoeff(0), vpParams->getExponent()
		);

		if (vpParams->getNumLights() >= 1) {
			_driver->SetLightingLocation(vpParams->getLightDirection(0));
		}
	}
}
