//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		renderer.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		September 2004
//
//	Description:	Implements the Renderer class.
//		A pure virtual class that is implemented for each renderer.
//		Methods are called by the glwindow class as needed.
//

#include "renderer.h"
#include "vapor/DataMgr.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "vizwinmgr.h"
#include "mainform.h"
#include "session.h"
#include "vizwin.h"

using namespace VAPoR;

Renderer::Renderer( VizWin* vw )
{
	//Establish the data sources for the rendering:
	//
	myVizWin = vw;
    myGLWindow = vw->getGLWindow();
	VizWinMgr* vwm = VizWinMgr::getInstance();
	int winNum = vw->getWindowNum();
	myViewpointParams = vwm->getViewpointParams(winNum);
	myRegionParams = vwm->getRegionParams(winNum);
	myDataMgr = Session::getInstance()->getDataMgr();
	myMetadata = Session::getInstance()->getCurrentMetadata();

}
