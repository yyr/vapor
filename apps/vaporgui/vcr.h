//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//					
//	File:		vcr.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2004
//
//	Description:	Defines the Vcr class
//		Eventually this will be used for animation control
//
#ifndef VCR_H
#define VCR_H

#include <qwidget.h>
class QVBoxLayout;
namespace VAPoR {
class Vcr : public QWidget
{
    Q_OBJECT

public:
    Vcr( QWidget *parent = 0, const char *name = 0 );
    ~Vcr() {}
private:
	QVBoxLayout* vlayout;
signals:
    void rewind();
    void play();
    void next();
    void stop();
};
};
#endif //VCR_H

