//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		contourparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Defines the ContourParams class
//		This is derived from the Params class
//		It contains all the parameters required for a contour plane
//		rendering.  
//		Initial version only contains a few parameters, just to
//		give some idea of what it will eventually look like
//
#ifndef CONTOURPARAMS_H
#define CONTOURPARAMS_H

#include <qwidget.h>
#include "params.h"

class ContourPlaneTab;

namespace VAPoR {


class MainForm;
class ContourParams : public Params {
	
public: 
	ContourParams(MainForm* mainWin, int winnum);
	~ContourParams();
	//Update the dialog with values from this:
	void updateDialog();
	//And vice-versa:
	virtual void updatePanelState();

	virtual Params* deepCopy();

	virtual void makeCurrent(Params* p, bool newWin);
	
	void setLighting(bool val) {lightingOn = val;}

	void setTab(ContourPlaneTab* tab) {myContourTab = tab;}
	
	void setVarNum(int val) {varNum = val;}
	
	//Following are set by gui, result in saving history state:
	virtual void guiSetEnabled(bool value);
	void guiSetLighting(bool val);
	void guiSetVarNum(int val);
	
	
protected:
	bool enabled;
	int varNum;
	bool lightingOn;
	float diffuseCoeff, ambientCoeff, specularCoeff;
	int specularExponent;
	float normal[3];
	float pointInPlane[3];
	ContourPlaneTab* myContourTab;
	

};
};
#endif //CONTOURPARAMS_H 
