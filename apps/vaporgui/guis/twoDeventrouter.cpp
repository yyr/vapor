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
#include <vapor/jpegapi.h>
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
