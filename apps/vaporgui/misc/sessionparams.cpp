//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		sessionparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Implements the SessionParams class.
//
#include "sessionparams.h"
#include "session.h"
#include "sessionparameters.h"
#include "mainform.h"
#include "messagereporter.h"
#include "vizwinmgr.h"
#include "vizwin.h"
#include "viewpointparams.h"
#include <qfiledialog.h>
#include <qlineedit.h>
#include <qpushbutton.h>
#include <qmessagebox.h>
#include <qcombobox.h>
#include <qcheckbox.h>
#include <qwhatsthis.h>


using namespace VAPoR;
//Save current session parameters in this class
SessionParams::SessionParams(){
	Session* currentSession = Session::getInstance();
	for (int i = 0; i< 3; i++) stretch[i] = currentSession->getStretch(i);
}
void SessionParams::launch(){
    Session* currentSession = Session::getInstance();	
	
	DataStatus* ds = DataStatus::getInstance();
	
	sessionParamsDlg = new SessionParameters((QWidget*)MainForm::getInstance());
	sessionParamsDlg->stretch0Edit->setText(QString::number(stretch[0]));
	sessionParamsDlg->stretch1Edit->setText(QString::number(stretch[1]));
	sessionParamsDlg->stretch2Edit->setText(QString::number(stretch[2]));

	bool enableStretch = !currentSession->sphericalTransform();
    sessionParamsDlg->stretch0Edit->setEnabled(enableStretch);
    sessionParamsDlg->stretch1Edit->setEnabled(enableStretch);
    sessionParamsDlg->stretch2Edit->setEnabled(enableStretch);

	

	QString str;
	
	
	connect(sessionParamsDlg->buttonHelp, SIGNAL(released()), this, SLOT(doHelp()));

	//set the sessionVariables:
	sessionVariableNum = 0;
	
	bool isLayered = (ds->getMetadata() && (StrCmpNoCase(ds->getMetadata()->GetGridType(),"Layered") == 0));
	sessionParamsDlg->outsideValFrame->setEnabled(isLayered);
	sessionParamsDlg->variableCombo->setCurrentItem(sessionVariableNum);
	if (isLayered) sessionParamsDlg->buttonOk->setDefault(false);
	if (ds->getNumSessionVariables()>0){
		sessionParamsDlg->lowValEdit->setText(QString::number(ds->getBelowValue(sessionVariableNum)));
		sessionParamsDlg->highValEdit->setText(QString::number(ds->getAboveValue(sessionVariableNum)));
		sessionParamsDlg->variableCombo->clear();
		for (int i = 0; i<ds->getNumSessionVariables(); i++){
			sessionParamsDlg->variableCombo->insertItem(ds->getVariableName(i).c_str());
		}
	}
	
	connect(sessionParamsDlg->variableCombo, SIGNAL(activated(int)), this, SLOT(setVariableNum(int)));
	connect(sessionParamsDlg->lowValEdit, SIGNAL(returnPressed()), this, SLOT(setOutsideVal()));
	connect(sessionParamsDlg->highValEdit, SIGNAL(returnPressed()), this, SLOT(setOutsideVal()));
	connect(sessionParamsDlg->highValEdit,SIGNAL(textChanged(const QString&)), this, SLOT(changeOutsideVal(const QString&)));
	connect(sessionParamsDlg->lowValEdit,SIGNAL(textChanged(const QString&)), this, SLOT(changeOutsideVal(const QString&)));
	outValsChanged = false;
	newOutVals = false;
	int rc = sessionParamsDlg->exec();
	if (rc){
		
		
		
		
		
		bool stretchChanged = false;
		float newStretch[3];
		float ratio[3] = { 1.f, 1.f, 1.f };
		newStretch[0] = sessionParamsDlg->stretch0Edit->text().toFloat();
		newStretch[1] = sessionParamsDlg->stretch1Edit->text().toFloat();
		newStretch[2] = sessionParamsDlg->stretch2Edit->text().toFloat();
		float minStretch = 1.e30f;
		for (int i= 0; i<3; i++){
			if (newStretch[i] <= 0.f) newStretch[i] = 1.f;
			if (newStretch[i] < minStretch) minStretch = newStretch[i];
		}
		//Normalize so minimum stretch is 1
		for (int i= 0; i<3; i++){
			if (minStretch != 1.f) newStretch[i] /= minStretch;
			if (newStretch[i] != stretch[i]){
				ratio[i] = newStretch[i]/stretch[i];
				stretchChanged = true;
				currentSession->setStretch(i, newStretch[i]);
			}
		}
		
		if (stretchChanged) {
			
			DataStatus* ds = DataStatus::getInstance();
			ds->stretchExtents(newStretch);
			
			VizWinMgr* vizMgr = VizWinMgr::getInstance();
			//Set the region dirty bit in every window:
			bool firstShared = false;
			for (int j = 0; j< MAXVIZWINS; j++) {
				VizWin* win = vizMgr->getVizWin(j);
				if (!win) continue;
				
				//Only do each viewpoint params once
				ViewpointParams* vpp = vizMgr->getViewpointParams(j);
				
				if (!vpp->isLocal()) {
					if(firstShared) continue;
					else firstShared = true;
				}
				vpp->rescale(ratio);
				vpp->setCoordTrans();
				win->setValuesFromGui(vpp);
				vizMgr->resetViews(vizMgr->getRegionParams(j),vpp);
				vizMgr->setViewerCoordsChanged(vpp);
				win->setDirtyBit(RegionBit, true);
			}
			
		}
		if (newOutVals){
			float aboveVal = sessionParamsDlg->highValEdit->text().toFloat();
			float belowVal = sessionParamsDlg->lowValEdit->text().toFloat();
			DataStatus::getInstance()->setOutsideValues(sessionVariableNum, belowVal, aboveVal);
			outValsChanged = true;
		}
		if (outValsChanged) {
			
			VizWinMgr* vizMgr = VizWinMgr::getInstance();
			DataStatus* ds = DataStatus::getInstance();
			for (int j = 0; j< MAXVIZWINS; j++) {
				VizWin* win = vizMgr->getVizWin(j);
				if (!win) continue;
				//do each dvr

				DvrParams* dp = vizMgr->getDvrParams(j);
				vizMgr->setDatarangeDirty(dp);
			}

			ds->getDataMgr()->SetLowHighVals(
				ds->getVariableNames(),
				ds->getBelowValues(),
				ds->getAboveValues());
		}

	}
	delete sessionParamsDlg;
	sessionParamsDlg = 0;
}


void SessionParams::
setVariableNum(int varNum){
	sessionVariableNum = varNum;
	sessionParamsDlg->lowValEdit->setText(QString::number(DataStatus::getInstance()->getBelowValue(sessionVariableNum)));
	sessionParamsDlg->highValEdit->setText(QString::number(DataStatus::getInstance()->getAboveValue(sessionVariableNum)));
	newOutVals = false;
	
}
void SessionParams::
setOutsideVal(){
	float aboveVal = sessionParamsDlg->highValEdit->text().toFloat();
	float belowVal = sessionParamsDlg->lowValEdit->text().toFloat();
	DataStatus::getInstance()->setOutsideValues(sessionVariableNum, belowVal, aboveVal);
	outValsChanged = true;
}
void SessionParams::
changeOutsideVal(const QString&){
	newOutVals = true;
}

void SessionParams::
doHelp(){
	QWhatsThis::enterWhatsThisMode();
}