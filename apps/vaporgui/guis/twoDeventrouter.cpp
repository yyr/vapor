//************************************************************************
//																		*
//		     Copyright (C)  2006										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDeventrouter.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		August 2008
//
//	Description:	Implements the TwoDEventRouter class.
//		This class supports routing messages from the gui to the params
//		associated with the TwoD tab
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qfileinfo.h>
#include <qlabel.h>
#include <qapplication.h>
#include <qcursor.h>
#include <qtooltip.h>
#include <qlayout.h>
#include "twoDrenderer.h"
#include "mappingframe.h"
#include "transferfunction.h"
#include "regionparams.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "twodframe.h"
#include "floweventrouter.h"
#include "instancetable.h"
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include <vaporinternal/jpegapi.h>
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "glutil.h"
#include "twoDparams.h"
#include "twoDeventrouter.h"
#include "regioneventrouter.h"
#include "viewpointeventrouter.h"
#include "eventrouter.h"
#include "VolumeRenderer.h"

using namespace VAPoR;


TwoDEventRouter::TwoDEventRouter(): EventRouter(){
	
	savedCommand = 0;
	ignoreListboxChanges = false;
	seedAttached = false;
	notNudgingSliders = false;

    attachedFlow = NULL;
    lastXSizeSlider = 0;
	lastYSizeSlider = 0;
    lastXCenterSlider = 0;
	lastYCenterSlider = 0;
	lastZCenterSlider = 0;
	for (int i=0; i<MAXVIZWINS+1; i++) {
		copyCount[i] = 0; 
	}
}


TwoDEventRouter::~TwoDEventRouter(){
	if (savedCommand) delete savedCommand;
}


//calculate the variable, or rms of the variables, at a specific point.
//Returns the OUT_OF_BOUNDS flag if point is not (in region and in twoD).
//

float TwoDEventRouter::
calcCurrentValue(TwoDParams* pParams, const float point[3], int* sessionVarNums, int numVars){
	double regMin[3],regMax[3];
	DataStatus* ds = DataStatus::getInstance();
	if (!ds || !ds->getDataMgr()) return 0.f;
	if (!pParams->isEnabled()) return 0.f;
	int timeStep = VizWinMgr::getActiveAnimationParams()->getCurrentFrameNumber();
	if (pParams->doBypass(timeStep)) return OUT_OF_BOUNDS;
	
	int arrayCoord[3];
	
	//Get the data dimensions (at current resolution):
	
	int numRefinements = pParams->getNumRefinements();
	
	//Specify the region to initially be just the point:
	for (int i = 0; i<3; i++){
		regMin[i] = point[i];
		regMax[i] = point[i];
	}

	
	//To index into slice, will need to know what coordinate is constant:
	int orientation = pParams->getOrientation();
	int xcrd = 0, ycrd = 1;
	if (orientation < 2) ycrd++;
	if (orientation == 0) xcrd++;
	
	
	size_t blkMin[3], blkMax[3];
	size_t coordMin[3], coordMax[3];
	if (0 > RegionParams::shrinkToAvailableVoxelCoords(numRefinements, coordMin, coordMax, blkMin, blkMax, timeStep,
		sessionVarNums, numVars, regMin, regMax, true)) {
			pParams->setBypass(timeStep);
			return OUT_OF_BOUNDS;
		}
	for (int i = 0; i< 3; i++){
		if (i == orientation) { arrayCoord[i] = 0; continue;}
		if (regMin[i] >= regMax[i]) {arrayCoord[i] = coordMin[i]; continue;}
		arrayCoord[i] = coordMin[i] + (int) (0.5f+((float)(coordMax[i]- coordMin[i]))*(point[i] - regMin[i])/(regMax[i]-regMin[i]));
		//Make sure the transformed coords are in the region of available data
		if (arrayCoord[i] < (int)coordMin[i] || arrayCoord[i] > (int)coordMax[i] ) {
			return OUT_OF_BOUNDS;
		} 
	}
	size_t bSize[3];
	ds->getDataMgr()->GetBlockSize(bSize, numRefinements);

	//Get the block coords (in the full volume) of the desired array coordinate:
	for (int i = 0; i< 3; i++){
		blkMin[i] = arrayCoord[i]/bSize[i];
		blkMax[i] = blkMin[i];
	}
	
	//Specify an array of pointers to the volume(s) mapped.  We'll retrieve one
	//volume for each variable specified, then do rms on the variables (if > 1 specified)
	float** sliceData = new float*[numVars];
	//Now obtain all of the volumes needed for this twoD:
	
	
	QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
	for (int i = 0; i < numVars; i++){
		sliceData[i] = pParams->getContainingVolume(blkMin, blkMax, numRefinements, sessionVarNums[i], timeStep, true);
		if (!sliceData[i]) {
			//failure to get data.  
			QApplication::restoreOverrideCursor();
			pParams->setBypass(timeStep);
			return OUT_OF_BOUNDS;
		}
	}
	QApplication::restoreOverrideCursor();
			
	int xyzCoord = (arrayCoord[xcrd] - blkMin[xcrd]*bSize[xcrd]) +
		(arrayCoord[ycrd] - blkMin[ycrd]*bSize[ycrd])*(bSize[ycrd]*(blkMax[xcrd]-blkMin[xcrd]+1));
	
	float varVal;
	//use the int xyzCoord to index into the loaded data
	if (numVars == 1) varVal = sliceData[0][xyzCoord];
	else { //Add up the squares of the variables
		varVal = 0.f;
		for (int k = 0; k<numVars; k++){
			varVal += sliceData[k][xyzCoord]*sliceData[k][xyzCoord];
		}
		varVal = sqrt(varVal);
	}
	delete [] sliceData;
	return varVal;
}

//Following method sets up (or releases) a connection to the Flow 
void TwoDEventRouter::
guiAttachSeed(bool attach, FlowParams* fParams){
	confirmText(false);
	//Don't capture the attach/detach event.
	//This cannot be easily undone/redone because it requires maintaining the
	//state of both the flowparams and the twoDparams.
	//But we will capture the successive seed moves that occur while
	//the seed is attached.
	if (attach){ 
		attachedFlow = fParams;
		seedAttached = true;
		
	} else {
		seedAttached = false;
		attachedFlow = 0;
	} 
}
