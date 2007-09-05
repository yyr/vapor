//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		vizfeatureparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		June 2005
//
//	Description:	Defines the VizFeatureParams class.
//		This is not derived from Params class!
//		Provides a temporary container for the state of the VizFeature dialog
//		It launches the dialog, and is deleted after the dialog returns.
//		While the dialog is active, values are saved in this state (using connections from
//		widget).  If the user changes the visualizer under consideration, and if there
//		has been any change in current state, then the user is prompted as to whether
//		to save the changes.
//		undo/redo is supported by cloning this class whenever a visualizer feature changes.
//
#ifndef VIZFEATUREPARAMS_H
#define VIZFEATUREPARAMS_H
 
#include <qwidget.h>

class QWidget;
class VizFeatures;
namespace VAPoR{
class VizFeatureParams : public QObject  {
	Q_OBJECT
public: 
	//Constructor gets starting state from the Session
	VizFeatureParams();
	//Copy constructor, just clones the state:
	//Needed because QObject copy constructor is inaccessible.
	VizFeatureParams(const VizFeatureParams&);
	~VizFeatureParams() {}
	//Launch a VizFeature dialog.  Put its values into visualizer(s) if it succeeds.
	void launch();
	void applyToViz(int viznum);
	
protected slots:
	void visualizerSelected(int comboIndex);
	void panelChanged();
	void selectRegionFrameColor();
	void selectSubregionFrameColor();
	void selectBackgroundColor();
	void selectColorbarBackgroundColor();
	void selectElevGridColor();
	void selectAxisColor();
	void applySettings();
	void annotationChanged();
	void xTicOrientationChanged(int);
	void yTicOrientationChanged(int);
	void zTicOrientationChanged(int);
	
protected:
	//Copy data from vizwin to and from dialog (shadowed in this class)
	void setDialog();
	void copyFromDialog();
	
	//Convert visualizer number to a combo index and vice versa
	int getComboIndex(int vizNum);
	int getVizNum(int comboIndex);
	VizFeatures* vizFeatureDlg;

	//State of one visualizer is saved here:
	int currentComboIndex;
	bool dialogChanged;
	QString vizName;
	bool showBar;
	bool showAxisArrows;
	bool showElevGrid;
	bool showAxisAnnotation;
	bool showRegion;
	bool showSubregion;
	float axisArrowCoords[3];
	float axisOriginCoords[3];
	float minTic[3],maxTic[3],ticLength[3];
	int numTics[3],ticDir[3];
	int labelHeight, labelDigits;
	float ticWidth;
	QColor axisAnnotationColor;

	float colorbarLLCoords[2];
	float colorbarURCoords[2];
	int numColorbarTics;
	int elevGridRefinement;
	
	QColor backgroundColor;
	QColor regionFrameColor;
	QColor subregionFrameColor;
	QColor colorbarBackgroundColor;
	QColor elevGridColor;
	
	
	
	
};
};
#endif //VIZFEATUREPARAMS_H 
