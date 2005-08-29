//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfelocationtip.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Defines the TFELocationTip class.  This provides
//		a tooltip that displays the coordinates of the mouse in the
//		transfer function editing window.
//
#ifndef TFELOCATIONTIP_H
#define TFELOCATIONTIP_H

#include <qtooltip.h>
#include <qwidget.h>
#include "tfeditor.h"

namespace VAPoR {
class TFELocationTip : public QToolTip {

public:
    TFELocationTip( QWidget *parent );
	void setEditor(MapEditor* ed) {editor = ed;}
protected:
    void maybeTip( const QPoint& );
	MapEditor* editor;
	QWidget* frame;
};
};

#endif
