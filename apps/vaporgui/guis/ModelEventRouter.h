//
//      $Id$
//
//	File:		ModelEventRouter.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Event router for geometric model control
//
//

#ifndef MODELEVENTROUTER_H
#define MODELEVENTROUTER_H

#include <qobject.h>
#include <QItemDelegate>
#include <QStringListModel>
#include "params.h"
#include "ModelParams.h"
#include "eventrouter.h"
#include <vapor/MyBase.h>
#include "model.h"
#include "Transform3d.h"


class QDoubleSpinBox;

using namespace VetsUtil;
QT_USE_NAMESPACE

namespace VAPoR {


class XmlNode;
class ParamNode;
class PanelCommand;
class Params;
class ModelEventRouter : public QWidget, public Ui_Model, public EventRouter 
{
   Q_OBJECT

public: 
	
   ModelEventRouter(QWidget* parent);
   virtual ~ModelEventRouter();
   
   static EventRouter* CreateTab()
   {
      TabManager* tMgr = VizWinMgr::getInstance()->getTabManager();
      return (EventRouter*)(new ModelEventRouter((QWidget*)tMgr));
   }

   //Connect signals and slots from tab
   virtual void hookUpTab();
   virtual void makeCurrent(Params* prev, Params* next, bool newWin, 
                            int instance = -1, bool reEnable = false);
   
   virtual void confirmText(bool /*render*/);
   virtual void updateTab();
   
   virtual void guiSetEnabled(bool value, int instance, bool undoredo = true);
   
   virtual void updateRenderer(RenderParams* dParams, bool prevEnabled,  bool newWindow);
   virtual void reinitTab(bool doOverride);
   
   //Following needed if tab has embedded opengl frames
   //virtual void refreshTab(){}

protected:
	static const char* webHelpText[];
	static const char* webHelpURL[];
   bool eventFilter(QObject *obj, QEvent *event);
   
protected slots:

   void setModelEnabled(bool on, int instance);
   void guiChangeInstance(int);
   void guiNewInstance();
   void guiDeleteInstance();
   void guiCopyInstanceTo(int toViz);

   void guiDeleteItem();
   void guiAddTranslate();
   void guiAddScale();
   void guiAddRotate();

   void guiApplyClipping(bool);
   
   void guiLoadModel();
   void guiSelectColor();
   
   void guiTransformsChanged();
   void guiTransformsReordered();

protected:
   int copyCount[MAXVIZWINS+1];

};

};

#endif //MODELEVENTROUTER_H 



