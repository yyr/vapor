//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		isosurfaceparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:  Definition of IsosurfaceParams class 
//		Currently this is a stand-in, intended to eventually support
//		all the parameters needed for an isosurface renderer
//		initially this is just rendering a glbox.
//

#ifndef ISOSURFACEPARAMS_H
#define ISOSURFACEPARAMS_H

#include <qwidget.h>
#include "params.h"
class QColor;
class IsoTab;
namespace VAPoR{


class MainForm;
class IsosurfaceParams: public Params {
	
public: 
	IsosurfaceParams(MainForm*, int winnum);
	
	~IsosurfaceParams();

	
	void setTab(IsoTab* tab) {myIsoTab = tab;}
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	//implement change of enablement, or change of local/global
	//
	virtual void updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow);

	void setNumSurfaces(int n) {numSurfaces = n;}
	void setVariable1(int n) {variable1 = n;}
	void setValue1(float val) {value1 = val;}
	void setColor1(QColor* clr) {color1 = clr;}
	float getValue1() {return value1;}
	float getOpac1() {return opac1;}
	void setOpac1(float val) {opac1 = val;}
	void setDiffuseCoeff(float val) {diffuseCoeff = val;}
	void setAmbientCoeff(float val) {ambientCoeff = val;}
	void setSpecularCoeff(float val) {specularCoeff = val;}
	void setExponent(int val) {specularExponent = val;}
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();

	//Methods that record changes in the history:
	//
	virtual void guiSetEnabled(bool);
	void guiSetColor1(QColor*);
	void guiSetOpac1(int val);
	void guiSetValue1(int val);
	void guiSetNumSurfaces(int n);
	void guiSetVariable1(int n);
	
protected:
	
	float diffuseCoeff, ambientCoeff, specularCoeff;
	int specularExponent;
	QColor* color1;
	float value1;
	float opac1;
	int variable1;
	int numSurfaces;
	IsoTab* myIsoTab;
	
	

};
};
#endif //ISOSURFACEPARAMS_H 
