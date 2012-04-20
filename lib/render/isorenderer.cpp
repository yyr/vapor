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


#include "isorenderer.h"
#include "regionparams.h"
#include "animationparams.h"
#include "viewpointparams.h"
#include "transferfunction.h"

#include "glwindow.h"

//#include "DVRLookup.h"
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
#include <vapor/MyBase.h>

using namespace VAPoR;
using namespace VetsUtil;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
IsoRenderer::IsoRenderer(GLWindow* glw, DvrParams::DvrType type, RenderParams* rp) 
  : VolumeRenderer(glw, type, rp, "IsoRenderer")
	
{
	_myParamsIso = NULL;
	setRenderParams(rp);
    _myParamsIso = (ParamsIso *) rp;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
IsoRenderer::~IsoRenderer()
{
	//
	// Need to unregister these 'cause we're passing a pointer
	// to our internal memory!
	//
	_unregisterDirtyFlags();
}

void IsoRenderer::setRenderParams(RenderParams* rp) {

	Renderer::setRenderParams(rp);

    ParamsIso *myParamsIso = (ParamsIso *) rp;

	// Must unregister any old dirtyflags
	//
	// Tue Jan  3 16:33:42 MST 2012 - this introduces a regression: the
	// ParamsIso instanced pointed bo by _myParmsIso has already been
	// destroyed. 
	//
	//_unregisterDirtyFlags();

	myParamsIso->RegisterIsoValueDirtyFlag(&_isovalueDF);
	myParamsIso->RegisterConstantColorDirtyFlag(&_isovalueDF);
	myParamsIso->RegisterNormalOnOffDirtyFlag(&_lightOnOffDF);
	myParamsIso->RegisterColorMapDirtyFlag(&_colorMapDF);
	myParamsIso->RegisterMapVariableDirtyFlag(&_dataDF);
	myParamsIso->RegisterVariableDirtyFlag(&_dataDF);
	myParamsIso->RegisterMapBoundsDirtyFlag(&_dataDF);
	myParamsIso->RegisterHistoBoundsDirtyFlag(&_dataDF);

	_myParamsIso = myParamsIso;

}

void IsoRenderer::_unregisterDirtyFlags() {

	if (_myParamsIso) {
		_myParamsIso->UnRegisterIsoValueDirtyFlag(&_isovalueDF);
		_myParamsIso->UnRegisterConstantColorDirtyFlag(&_isovalueDF);
		_myParamsIso->UnRegisterNormalOnOffDirtyFlag(&_lightOnOffDF);
		_myParamsIso->UnRegisterColorMapDirtyFlag(&_colorMapDF);
		_myParamsIso->UnRegisterMapVariableDirtyFlag(&_dataDF);
		_myParamsIso->UnRegisterVariableDirtyFlag(&_dataDF);
		_myParamsIso->UnRegisterMapBoundsDirtyFlag(&_dataDF);
		_myParamsIso->UnRegisterHistoBoundsDirtyFlag(&_dataDF);
	}
	_myParamsIso = NULL;
}

int	IsoRenderer::_updateRegion(
	DataMgr *dataMgr, RenderParams *rp, RegionParams *regp,
	size_t ts, string varname, int reflevel, int lod,
	const size_t min[3], const size_t max[3]
) {
    ParamsIso *myParamsIso = (ParamsIso *) rp;

	RegularGrid *rg = dataMgr->GetGrid(
			ts, varname, reflevel, lod, min, max
	);
	if (!rg) return(-1);

	int rc = _driver->SetRegion(rg, myParamsIso->GetHistoBounds(),0);
	delete rg;
	if (rc<0) return(rc);

	if (_type == DvrParams::DVR_RAY_CASTER_2_VAR) {
		string map_varname = myParamsIso->GetMapVariableName();
		rg = dataMgr->GetGrid(
			ts, map_varname, reflevel, lod, min, max
		);
		if (!rg) return(-1);

		rc = _driver->SetRegion(rg, myParamsIso->GetMapBounds(),1);
		delete rg;
		if (rc<0) return(rc);
	}

	return(0);
}

void IsoRenderer::_updateDriverRenderParamsSpec(RenderParams *rp) {

    ParamsIso *myParamsIso = (ParamsIso *) rp;
	DVRRayCaster *driver = (DVRRayCaster *) _driver;


	if (_isovalueDF.Test()) {

		float isoval = myParamsIso->GetIsoValue();
		const float *color = myParamsIso->GetConstantColor();

		// Normalize isovalue
		const float *range = currentRenderParams->getCurrentDatarange();
		isoval = (isoval - range[0]) / (range[1] - range[0]);

		driver->SetIsoValues(&isoval, color, 1);

	}

//	if (_type == DvrParams::DVR_RAY_CASTER_2_VAR && (clutIsDirty() || datarangeIsDirty())) {
	if (_type == DvrParams::DVR_RAY_CASTER_2_VAR && clutIsDirty()) {
		// What the hell does this do?
		myGLWindow->setRenderNew();

		_driver->SetCLUT(myParamsIso->getClut());
	}


	bool lighting = myParamsIso->GetNormalOnOff();
	if (_lightOnOffDF.Test()) {
		driver->SetLightingOnOff(lighting);
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
	_isovalueDF.Clear();
	_lightOnOffDF.Clear();
	//_colorMapDF.Clear(); // cleared by parent class
	//_dataDF.Clear(); // cleared by parent class
}
