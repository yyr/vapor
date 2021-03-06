//************************************************************************
//																		*
//		     Copyright (C)  2011										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		boxsliders.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2011
//
//	Description:	Defines the BoxSliderFrame class.  This provides
//		a frame that supports sliders and textboxes to control a box
//
#ifndef BOXSLIDERFRAME_H
#define BOXSLIDERFRAME_H
#include <QFrame>
#include <qwidget.h>
#include "boxframe.h"
#include <vector>


namespace VAPoR {
	
class BoxSliderFrame : public QFrame, public Ui_boxframe {
	Q_OBJECT

public:
	BoxSliderFrame( QWidget * parent = 0);
	~BoxSliderFrame();
	void setBoxExtents(const std::vector<double>& vec);
	void getBoxExtents(double[6]);
	void setFullDomain(const double[6]);
	void setVoxelDims(const int[3]);
	void updateGuiValues(const double mid[3],const double size[3]);
	void setNumRefinements(int numrefs){
		numRefinements = numrefs;
	}
	

public slots:
	void boxTextChanged(const QString&);
	void boxReturnPressed();
	void xSliderCtrChange();
	void ySliderCtrChange();
	void zSliderCtrChange();
	void xSliderSizeChange();
	void ySliderSizeChange();
	void zSliderSizeChange();
	void nudgeXCenter(int);
	void nudgeYCenter(int);
	void nudgeZCenter(int);
	void nudgeXSize(int);
	void nudgeYSize(int);
	void nudgeZSize(int);
signals:
	void extentsChanged();
	
	
protected:
	void confirmText();
	void nudgeCenter(int val, int dir);
	void nudgeSize(int val, int dir);
	bool textChangedFlag;
	bool silenceSignals;
	double boxExtents[6];
	double domainExtents[6];
	int voxelDims[3];
	int numRefinements;
	int lastCenterSlider[3];
	int lastSizeSlider[3];
};
};


#endif

