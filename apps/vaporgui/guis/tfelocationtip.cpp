//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfelocationtip.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TFELocationTip class.  This provides
//		a tooltip that displays the coordinates of the mouse in the
//		Mapper function editing window.
//
#include "tfelocationtip.h"
#include "mapeditor.h"
using namespace VAPoR;
TFELocationTip::TFELocationTip( QWidget * parent )
    : QToolTip( parent )
{
    editor = 0;
	frame = parent;
}


void TFELocationTip::maybeTip( const QPoint &pos )
{
    QRect r( pos - QPoint(1,1), pos + QPoint(1,1));
    if ( !r.isValid() || !editor)
        return;
	float xOpacCoord = editor->mapOpacWin2Var(r.center().x());
	float xColorCoord = editor->mapColorWin2Var(r.center().x());
	MapperFunction* tf = editor->getMapperFunction();
	float hf,sf,vf;
	int hue, sat, val;
	tf->hsvValue(xColorCoord,&hf,&sf,&vf);
	hue = (int)(hf*359.99f);
	sat = (int)(sf*255.99f);
	val = (int)(vf*255.99f);
	int histcount=0;
	//If there's a histo, it's based on opacity coord
	histcount = editor->getHistoValue(xOpacCoord);
	float opacity = tf->opacityValue(xOpacCoord);
	QString s;
	if (r.center().y() > frame->height()-COORDMARGIN) return;
	//See if this is over the color bar:
	if (r.center().y() > frame->height()-BELOWOPACITY) {
		if (r.center().y() < frame->height() - BARHEIGHT-COORDMARGIN) return;
		if(xOpacCoord == xColorCoord){
			if (histcount >= 0){
				s.sprintf( "Variable: %.4g; Opacity: %.3f;\n Color(HSV): %3d %3d %3d;\nHistogram: %d",
				xOpacCoord, opacity, hue, sat, val, histcount);
			} else {
				s.sprintf( "Variable: %.4g; Opacity: %.3f;\n Color(HSV): %3d %3d %3d",
				xOpacCoord, opacity, hue, sat, val);
			}
		} else {
			s.sprintf( "Opacity Variable: %.4g; Opacity: %.3f;\n Color Variable: %.4g; Color(HSV): %3d %3d %3d",
				xOpacCoord, opacity, xColorCoord, hue, sat, val);
		}
		tip( r, s );
		return;
	}
	
	float ycoord = (float)(frame->height() - r.center().y() - BELOWOPACITY)/(float)(frame->height() - BELOWOPACITY);
	if(xOpacCoord == xColorCoord){
		if (histcount >= 0){
			s.sprintf( "Variable: %.4g; Y-Coord: %.3f;\nOpacity: %.3f; Color(HSV): %3d %3d %3d;\nHistogram: %d",
				xOpacCoord, ycoord, opacity, hue, sat, val, histcount);
		} else {
			s.sprintf( "Variable: %.4g; Y-Coord: %.3f;\nOpacity: %.3f; Color(HSV): %3d %3d %3d",
				xOpacCoord, ycoord, opacity, hue, sat, val);
		}
	} else {
		s.sprintf( "Opacity Variable: %.4g; Y-Coord: %3f; Opacity: %.3f;\n Color Variable: %.4g; Color(HSV): %3d %3d %3d",
				xOpacCoord, ycoord, opacity, xColorCoord, hue, sat, val);
	}

    tip( r, s );
}
