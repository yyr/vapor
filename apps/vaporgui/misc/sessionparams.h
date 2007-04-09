//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		sessionparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2004
//
//	Description:	Defines the SessionParams class.
//		This is not derived from Params class
//		Provides a temporary container for the state of the SessionParameters dialog
//		It launches the dialog, and must be deleted after the dialog returns.
//
#ifndef SESSIONPARAMS_H
#define SESSIONPARAMS_H
 
#include <qwidget.h>

class QWidget;
class SessionParameters;
namespace VAPoR{


	class SessionParams : public QObject  {
	Q_OBJECT
public: 
	//Constructor gets starting state from the Session
	SessionParams();
	
	~SessionParams() {}
	//Launch a SessionParams dialog.  Put its values into session if it succeeds.
	void launch();
	
public slots:
	void logFileChoose();
	void resetCounts();
	
protected:
	
	int jpegQuality;
	int cacheSize;
	int maxPopup[3];
	int maxLog[3];
	SessionParameters* sessionParamsDlg;
	
};
};
#endif //SESSIONPARAMS_H 
