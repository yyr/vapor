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
#include <QScrollArea>
#include <qdialog.h>
#include "vizfeatures.h"
#include <QResizeEvent>
#include <vector>

class QWidget;
class ScrollContainer;

namespace VAPoR{

class ScrollContainer : public QDialog {
public:
	ScrollContainer(QWidget* parent, const char* name) : QDialog(parent, Qt::Widget) {
		setWindowTitle(QString(name));}
	void setScroller(QScrollArea* sv){scroller = sv;}
	QScrollArea* getScroller(){return scroller;}
protected:
	virtual void resizeEvent(QResizeEvent* event){
		if (scroller) {
			scroller->resize(event->size().width(), event->size().height());
		}
	}
	QScrollArea* scroller;
};
class VizFeatureDialog : public QDialog, public Ui_VizFeatures {
public :
	VizFeatureDialog(QDialog* parent) : QDialog(parent), Ui_VizFeatures() 
	{ setupUi(this); }
};
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
	

	
signals: 
	void doneWithIt();

protected slots:
	void toggleLatLon(bool val);
	void visualizerSelected(int comboIndex);
	void panelChanged();
	void rendererChanged(int);
	
	void selectTimeTextColor();

	void selectColorbarBackgroundColor();
	
	void selectAxisColor();
	void applySettings();
	void annotationChanged();
	void okClicked();
	void reloadShaders();
	void doHelp();
	
	void checkSurface(bool);
	
protected:
	//Copy data from vizwin to and from dialog (shadowed in this class)
	void setDialog();
	void copyFromDialog();
	
	//Convert visualizer number to a combo index and vice versa
	int getComboIndex(int vizNum);
	int getVizNum(int comboIndex);
	VizFeatureDialog* vizFeatureDlg;
	ScrollContainer* featureHolder;

	//State of one visualizer is saved here:
	int colorbarRendererTypeId;
	int currentComboIndex;
	bool dialogChanged;
	QString vizName;
	bool showBar;
	bool showAxisArrows;
	bool useLatLon;
	bool showAxisAnnotation;
	bool enableSpin;
	int timeAnnotType;//0,1,2 

	float axisArrowCoords[3];
	double axisOriginCoords[3];
	double minTic[3],maxTic[3],ticLength[3];
	int numTics[3],ticDir[3];
	int labelHeight, labelDigits;
	int timeAnnotTextSize;
	float ticWidth;
	
	float colorbarLLCoords[2];
	float colorbarURCoords[2];
	int colorbarDigits;
	int colorbarFontsize;
	float timeAnnotCoords[2];
	int numColorbarTics;
	QColor colorbarBackgroundColor;

	QColor timeAnnotColor;
	QColor axisAnnotationColor;
	//Temp versions to remember value set in gui.  This info is lost when the dialog closes.
	QColor tempColorbarBackgroundColor;
	
	QColor tempTimeAnnotColor;
	QColor tempAxisAnnotationColor;

	float stretch[3];
	
	std::vector<int> rendererTypeLookup;
	
	
};


};
#endif //VIZFEATUREPARAMS_H 
