#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif
#include <QScrollArea>
#include <QScrollBar>
#include <qapplication.h>
#include <qcursor.h>
#include <qdesktopwidget.h>
#include <qrect.h>
#include <qmessagebox.h>
#include <qlineedit.h>
#include <qcombobox.h>
#include <qpushbutton.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qcheckbox.h>
#include <qcolordialog.h>
#include <qlabel.h>
#include <qtimer.h>
#include <qtooltip.h>

#include <qlineedit.h>
#include <qcombobox.h>
#include <qspinbox.h>
#include <qslider.h>
#include <qlabel.h>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

#include "params.h"
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "arrowparams.h"
#include "mousemodeparams.h"
#include "vizwinparams.h"
#include "arrowrenderer.h"
#include "arroweventrouter.h"
#include "arrowAppearance.h"


using namespace VAPoR;

ArrowAppearance::ArrowAppearance(QWidget* parent) : QWidget(parent), Ui_barbAppearance(){
        setupUi(this);
	
}


ArrowAppearance::~ArrowAppearance(){
	
}
/**********************************************************
 * Whenever a new Arrowtab is created it must be hooked up here
 ************************************************************/







