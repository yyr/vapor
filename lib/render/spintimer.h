#ifndef SPINTIMER_H
#define SPINTIMER_H
#include <qobject.h>

#include <qtimer.h>
#include "glwindow.h"
#include <qgl.h>
using namespace VetsUtil;

namespace VAPoR {


//! \class SpinTimer
//! \brief A class that performs spinning in the glwindow
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!

class RENDER_API SpinTimer: public QObject
{
     Q_OBJECT
	
public:
	SpinTimer() : QObject() {
		myTimer = new QTimer(this);
		connect(myTimer, SIGNAL(timeout()), this, SLOT(updateGLWin()));
	}
	void stop(){
		myTimer->stop();
		delete myTimer;
	}
	void start(int interval){
		myTimer->start(interval);
	}
	void setWindow(GLWindow* win){ myGLWindow = win;}

private:
	GLWindow* myGLWindow;
	QTimer* myTimer;
private slots:
	void updateGLWin(){
		myGLWindow->update();
	}
};
};




#endif