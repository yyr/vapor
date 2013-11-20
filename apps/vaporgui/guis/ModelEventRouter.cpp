//
//      $Id$
//
//	File:		ModelEventRouter.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Event router for geometric model control
//
//

#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <QtGui>

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <GL/glew.h>

#ifdef MODELS
#include <assimp/assimp.hpp>
#endif

#include "instancetable.h"
#include "mainform.h"
#include "vizwinmgr.h"
#include "session.h"
#include "panelcommand.h"
#include "messagereporter.h"
#include "params.h"
#include <vapor/XmlNode.h>
#include "tabmanager.h"
#include "ModelParams.h"
#include "ModelEventRouter.h"
#include "eventrouter.h"
#include "ModelRenderer.h"
#include "TransformListModel.h"
#include "TransformWidget.h"
#include "TransformDelegate.h"

using namespace VAPoR;
using namespace VAPoR;
const char* ModelEventRouter::webHelpText[] = 
{
	"Model Rendering Overview",
	"Renderer control",
	"Assimp: Open Asset Import Library",
	"<>"
};
const char* ModelEventRouter::webHelpURL[] =
{

	"http://www.vapor.ucar.edu/docs/vapor-how-guide/model-tab-3d-geometry-display",
	"http://www.vapor.ucar.edu/docs/vapor-how-guide/renderer-instances",
	"http://assimp.sourceforge.net"
};
//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
ModelEventRouter::ModelEventRouter(QWidget* parent): 
        QWidget(parent), 
        Ui_Model(), 
        EventRouter()
{
   setupUi(this);
   myParamsBaseType = Params::GetTypeFromTag(ModelParams::_modelParamsTag);
   myWebHelpActions = makeWebHelpActions(webHelpText,webHelpURL);
   savedCommand = 0;

   QStringList transforms;
   TransformListModel *model = new TransformListModel(transforms);

   transformView->setModel(model);
   transformView->setDragDropMode(QAbstractItemView::InternalMove);
   transformView->setViewMode(QListView::ListMode);
   transformView->setMovement(QListView::Snap);
   transformView->setDropIndicatorShown(true);
   transformView->setDragDropOverwriteMode(false);

   TransformDelegate *delegate = new TransformDelegate();
   transformView->setItemDelegate(delegate);
   transformView->installEventFilter(this);

   connect(delegate, SIGNAL(transformChanged()), this, SLOT(guiTransformsChanged()));
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
ModelEventRouter::~ModelEventRouter()
{
   if (savedCommand) delete savedCommand;
}

//----------------------------------------------------------------------------
// Whenever a new Modeltab is created it must be hooked up here
//----------------------------------------------------------------------------
void ModelEventRouter::hookUpTab()
{
   // 
   // the following are needed for any renderer eventrouter:
   //
   connect (refinementCombo,SIGNAL(activated(int)), this, SLOT(guiSetNumRefinements(int)));
   connect (lodCombo,SIGNAL(activated(int)), this, SLOT(guiSetCompRatio(int)));
   connect (instanceTable, SIGNAL(changeCurrentInstance(int)), this, SLOT(guiChangeInstance(int)));
   connect (copyCombo, SIGNAL(activated(int)), this, SLOT(guiCopyInstanceTo(int)));
   connect (newInstanceButton, SIGNAL(clicked()), this, SLOT(guiNewInstance()));
   connect (deleteInstanceButton, SIGNAL(clicked()),this, SLOT(guiDeleteInstance()));
   connect (instanceTable, SIGNAL(enableInstance(bool,int)), this, SLOT(setModelEnabled(bool,int)));
	
   // 
   // Unique connections for ModelTab:
   //
   connect (modelLoad, SIGNAL(clicked()), this, SLOT(guiLoadModel()));
   connect (colorSelect, SIGNAL(pressed()), this, SLOT(guiSelectColor()));
   connect (clipping, SIGNAL(toggled(bool)), this, SLOT(guiApplyClipping(bool)));

   connect (addTranslate, SIGNAL(clicked()), this, SLOT(guiAddTranslate()));
   connect (addScale, SIGNAL(clicked()), this, SLOT(guiAddScale()));
   connect (addRotate, SIGNAL(clicked()), this, SLOT(guiAddRotate()));
   connect (deleteItem, SIGNAL(clicked()), this, SLOT(guiDeleteItem()));
}

//----------------------------------------------------------------------------
// 
//----------------------------------------------------------------------------
void ModelEventRouter::guiChangeInstance(int newCurrent)
{
   performGuiChangeInstance(newCurrent);
}

	
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiNewInstance()
{
   performGuiNewInstance();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiDeleteInstance()
{
   performGuiDeleteInstance();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiCopyInstanceTo(int toViz)
{
   if (toViz == 0) return; 
   if (toViz == 1) 
   {
      performGuiCopyInstance(); 
      return;
   }
	
   int viznum = copyCount[toViz];
   copyCombo->setCurrentIndex(0);
   performGuiCopyInstanceToViz(viznum);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiSelectColor()
{
   confirmText(true);
   QPalette pal(colorBox->palette());
   QColor newColor = QColorDialog::getColor(pal.color(QPalette::Base), this);

   if (!newColor.isValid()) return;

   pal.setColor(QPalette::Base, newColor);
   colorBox->setPalette(pal);
	
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);

   qreal rgb[3];
   float rgbf[3];

   PanelCommand* cmd = PanelCommand::captureStart(modelParams, "change model color");

   newColor.getRgbF(rgb,rgb+1,rgb+2);

   for (int i = 0; i<3; i++)
   {
      rgbf[i] = (float)rgb[i];
   }

   modelParams->SetConstantColor(rgbf);
   PanelCommand::captureEnd(cmd, modelParams);

   VizWinMgr::getInstance()->forceRender(modelParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiDeleteItem()
{
   QModelIndex index = transformView->currentIndex();

   if (index.isValid())
   {
      ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
      PanelCommand* cmd = PanelCommand::captureStart(modelParams, "delete transform item");
      
      TransformListModel *model = dynamic_cast<TransformListModel*>(transformView->model());
      
      model->removeRow(index.row(), index.parent());

      modelParams->GetTransformation()->set(model->stringList());
      modelParams->setTransformationDirty();

      PanelCommand::captureEnd(cmd, modelParams);
      VizWinMgr::getInstance()->forceRender(modelParams);
   }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiAddTranslate()
{
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   PanelCommand* cmd = PanelCommand::captureStart(modelParams, "add model translation");
   
   TransformListModel *model = dynamic_cast<TransformListModel*>(transformView->model());
      
   model->add(new Transform3d::Translate(0,0,0));
   modelParams->GetTransformation()->set(model->stringList());
   modelParams->setTransformationDirty();
      
   PanelCommand::captureEnd(cmd, modelParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiAddScale()
{
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   PanelCommand* cmd = PanelCommand::captureStart(modelParams, "add model scale");
   
   TransformListModel *model = dynamic_cast<TransformListModel*>(transformView->model());
      
   model->add(new Transform3d::Scale(1,1,1));
   modelParams->GetTransformation()->set(model->stringList());
   modelParams->setTransformationDirty();
      
   PanelCommand::captureEnd(cmd, modelParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiAddRotate()
{
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   PanelCommand* cmd = PanelCommand::captureStart(modelParams, "add model rotation");
   
   TransformListModel *model = dynamic_cast<TransformListModel*>(transformView->model());
      
   model->add(new Transform3d::Rotate(1,0,0,0));
   modelParams->GetTransformation()->set(model->stringList());
   modelParams->setTransformationDirty();
   
   PanelCommand::captureEnd(cmd, modelParams);
}

//----------------------------------------------------------------------------
// signals QListView::indexesMoved and QAbstractItemModel::LayoutChanged are not
// being emitted. This is a kludgy workaround to catch the corresponding event. 
//----------------------------------------------------------------------------
bool ModelEventRouter::eventFilter(QObject *object, QEvent *event)
{
   if (event->type() == QEvent::ChildRemoved)
   {
      guiTransformsReordered();
   }

   return QObject::eventFilter(object, event);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiTransformsReordered()
{
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   PanelCommand* cmd = PanelCommand::captureStart(modelParams, "transforms reordering");
   TransformListModel *model = dynamic_cast<TransformListModel*>(transformView->model());
      
   modelParams->GetTransformation()->set(model->stringList());
   modelParams->setTransformationDirty();

   PanelCommand::captureEnd(cmd, modelParams);

   VizWinMgr::getInstance()->forceRender(modelParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiTransformsChanged()
{
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);

   QStringListModel *model = (QStringListModel*)transformView->model();

   modelParams->GetTransformation()->set(model->stringList());
   modelParams->setTransformationDirty();

   VizWinMgr::getInstance()->forceRender(modelParams);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiApplyClipping(bool flag)
{
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);

   modelParams->SetClipping(flag);

   VizWinMgr::getInstance()->forceRender(modelParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::setModelEnabled(bool val, int instance)
{
   VizWinMgr* vizMgr = VizWinMgr::getInstance();
   int activeViz = vizMgr->getActiveViz();
	
   ModelParams* mp = (ModelParams*)(Params::GetParamsInstance(ModelParams::_modelParamsTag, 
                                                              activeViz, instance));

   // Make sure this is a change:
   if (mp->isEnabled() == val ) return;

   // If we are enabling, also make this the current instance:
   if (val) 
   {
      if (mp->GetModelFilename().empty() && mp->GetSceneFilename().empty())
      {
         instanceTable->checkEnabledBox(false,instance);
         MessageReporter::errorMsg("No model or scene file have been specified.\nModel display will not be enabled.");
         return;
      }

      performGuiChangeInstance(instance);
   }

   guiSetEnabled(val,instance);	
}


//----------------------------------------------------------------------------
// Insert values from params into tab panel
//----------------------------------------------------------------------------
void ModelEventRouter::updateTab()
{
   if(!MainForm::getInstance()->getTabManager()->isFrontTab(this)) return;
   MainForm::getInstance()->buildWebHelpMenu(myWebHelpActions);
   MessageReporter::infoMsg("ModelEventRouter::updateTab()");
	
   if (!isEnabled()) return;
	
   Session *session = Session::getInstance();
   session->blockRecording();

   //
   // Set up the instance table:
   //
   DataStatus* ds = DataStatus::getInstance();
   if (ds->getDataMgr()) instanceTable->setEnabled(true);
   else instanceTable->setEnabled(false);
	
   instanceTable->rebuild(this);
	
   VizWinMgr* vizMgr = VizWinMgr::getInstance();
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   int winnum = vizMgr->getActiveViz();
   int numViz = vizMgr->getNumVisualizers();

   copyCombo->clear();
   copyCombo->addItem("Duplicate In:");
   copyCombo->addItem("This visualizer");

   if (numViz > 1) 
   {
      int copyNum = 2;
      for (int i = 0; i<MAXVIZWINS; i++)
      {
         if (vizMgr->getVizWin(i) && winnum != i)
         {
            copyCombo->addItem(vizMgr->getVizWinName(i));
            //Remember the viznum corresponding to a combo item:
            copyCount[copyNum++] = i;
         }
      }
   }
	
   deleteInstanceButton->setEnabled(vizMgr->getNumInstances(winnum, 
                                    Params::GetTypeFromTag(ModelParams::_modelParamsTag)) > 1);

   // 
   // Set up refinements and LOD combos:
   //
   int numRefs = modelParams->GetRefinementLevel();

   if(numRefs <= refinementCombo->count()) refinementCombo->setCurrentIndex(numRefs);

   lodCombo->setCurrentIndex(modelParams->GetCompressionLevel());
	
   // 
   // Set model and scene file names
   //
   modelFile->setText(modelParams->GetSceneFilename().c_str());

   if (modelFile->text().isEmpty())
   {
      modelFile->setText(modelParams->GetModelFilename().c_str());
   }

   //
   // Set the transform
   //
   TransformListModel *model = dynamic_cast<TransformListModel*>(transformView->model());
   model->setStringList(modelParams->GetTransformation()->get());

   //
   // Setup the color box
   //
   const float* clr = modelParams->GetConstantColor();
   QColor newColor;
   newColor.setRgbF((qreal)clr[0],(qreal)clr[1],(qreal)clr[2]);
   QPalette pal(colorBox->palette());
   pal.setColor(QPalette::Base, newColor);
   colorBox->setPalette(pal);

   //
   // Set up clipping
   //
   clipping->setChecked(modelParams->GetClipping());
	
   update();

   Session::getInstance()->unblockRecording();
   vizMgr->getTabManager()->update();
   vizMgr->forceRender(modelParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::confirmText(bool /*render*/)
{
   return;
}

//----------------------------------------------------------------------------
// Reinitialize Model tab settings, session has changed.
// Note that this is called after the globalModelParams are set up, but before
// any of the localModelParams are setup.
//----------------------------------------------------------------------------
void ModelEventRouter::reinitTab(bool doOverride)
{
   DataStatus* ds = DataStatus::getInstance();

   if (ds->dataIsPresent3D()) 
   {
      setEnabled(true);
      instanceTable->setEnabled(true);
      instanceTable->rebuild(this);
   }
   else setEnabled(false);

   int i;
	
   //
   // Set up the refinement combo:
   //
   const DataMgr *dataMgr = ds->getDataMgr();
   
   int numRefinements = dataMgr->GetNumTransforms();
   refinementCombo->setMaxCount(numRefinements+1);
   refinementCombo->clear();
   for (i = 0; i<= numRefinements; i++)
   {
      refinementCombo->addItem(QString::number(i));
   }
	
   if (dataMgr)
   {
      vector<size_t> cRatios = dataMgr->GetCRatios();
      lodCombo->clear();
      lodCombo->setMaxCount(cRatios.size());

      for (int i = 0; i<cRatios.size(); i++)
      {
         QString s = QString::number(cRatios[i]);
         s += ":1";
         lodCombo->addItem(s);
      }
   }
	
   updateTab();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiSetCompRatio(int num)
{
   confirmText(false);
   //make sure we are changing it
   ModelParams* dParams = (ModelParams*)VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   if (num == dParams->GetCompressionLevel()) return;
   
   PanelCommand* cmd = PanelCommand::captureStart(dParams, "set compression level");
   
   dParams->SetCompressionLevel(num);
   lodCombo->setCurrentIndex(num);
   PanelCommand::captureEnd(cmd, dParams);
   VizWinMgr::getInstance()->forceRender(dParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiSetNumRefinements(int num)
{
   confirmText(false);
   //make sure we are changing it
   ModelParams* dParams = (ModelParams*)VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);
   if (num == dParams->GetRefinementLevel()) return;
	
   PanelCommand* cmd = PanelCommand::captureStart(dParams, "set number of refinements");
   
   dParams->SetRefinementLevel(num);
   refinementCombo->setCurrentIndex(num);
   PanelCommand::captureEnd(cmd, dParams);
   VizWinMgr::getInstance()->forceRender(dParams);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiSetEnabled(bool value, int instance, bool undoredo)
{
   VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
   int winnum = vizWinMgr->getActiveViz();
   //Ignore spurious clicks.
   ModelParams* dParams = (ModelParams*)(Params::GetParamsInstance(ModelParams::_modelParamsTag, winnum, instance));
   
   if (value == dParams->isEnabled()) return;
   confirmText(false);
   PanelCommand* cmd;
   if(undoredo) cmd = PanelCommand::captureStart(dParams, "toggle model enabled", instance);
   dParams->setEnabled(value);
   if(undoredo) PanelCommand::captureEnd(cmd, dParams);
   //Make the change in enablement occur in the rendering window, 
   // Local/Global is not changing.
   updateRenderer(dParams,!value, false);
   
   updateTab();
   vizWinMgr->forceRender(dParams);
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelEventRouter::guiLoadModel()
{
#ifdef MODELS
   Assimp::Importer importer;
   ModelParams* modelParams = (ModelParams*) VizWinMgr::getActiveParams(ModelParams::_modelParamsTag);

   QString sceneExtension;
   sceneExtension.prepend("Vapor Model Scene (");
   sceneExtension.append("*.vms)");

   string extensionsString;
   importer.GetExtensionList(extensionsString);   

   QString modelExtensions = extensionsString.c_str();
   modelExtensions.replace(';', ' ');
   modelExtensions.prepend("3D Models (");
   modelExtensions.append(")");

   QString extensions;
   extensions.append(sceneExtension);
   extensions.append(";;");
   extensions.append(modelExtensions);

   QString selected;
   
   QString filename = QFileDialog::getOpenFileName(this, "Specify Model File",
                                                   ".", extensions, &selected);

   if (!filename.isNull())
   {
      if (selected == sceneExtension)
      {
         PanelCommand* cmd = PanelCommand::captureStart(modelParams, "Load scene file");
         modelParams->SetSceneFilename(filename.toStdString());
         modelParams->SetModelFilename("");
         modelFile->setText(filename);
         PanelCommand::captureEnd(cmd, modelParams);
      }
      else // Model extensions
      {
         PanelCommand* cmd = PanelCommand::captureStart(modelParams, "Load model file");
         modelParams->SetModelFilename(filename.toStdString());
         modelParams->SetSceneFilename("");
         modelFile->setText(filename);
         PanelCommand::captureEnd(cmd, modelParams);
      }
   }
#endif
}


//----------------------------------------------------------------------------
// Handle the change of status associated with change of enablement 
//
// If the window is new, (i.e. we are just creating a new window, use 
//  prevEnabled = false
//----------------------------------------------------------------------------
void ModelEventRouter::updateRenderer(RenderParams* rParams, bool prevEnabled, bool newWindow)
{
#ifdef MODELS   
   ModelParams* dParams = (ModelParams*)rParams;
   VizWinMgr* vizWinMgr = VizWinMgr::getInstance();
   int winnum = dParams->getVizNum();
   
   if (newWindow) 
   {
      prevEnabled = false;
   }
   
   bool nowEnabled = rParams->isEnabled();
   
   if (prevEnabled == nowEnabled) return;
   
   
   VizWin* viz = 0;

   if(winnum >= 0)
   {
      // Find the viz that this applies to:
      // Note that this is only for the cases below where one particular
      // visualizer is needed
      viz = vizWinMgr->getVizWin(winnum);
   } 
	
   // cases to consider:
   // 1.  unchanged disabled renderer; do nothing.
   //     enabled renderer, just force refresh:
   
   if (prevEnabled == nowEnabled) 
   {
      if (!prevEnabled) return;
      VizWinMgr::getInstance()->forceRender(dParams);
      return;
   }
	
   // 2.  Change of disable->enable .  Create a new renderer in active window.
   // 6.  Change of enable->disable with local renderer.  Delete renderer in local window same as:
   // For a new renderer

   if (nowEnabled && !prevEnabled )
   {
      //For case 2.:  create a renderer in the active window:
      Renderer* model = new ModelRenderer(viz->getGLWindow(), dParams);
      
      viz->getGLWindow()->insertSortedRenderer(dParams, model);
      
      //force the renderer to refresh 
      
      VizWinMgr::getInstance()->forceRender(dParams);
      
      //Quit 
      return;
   }
	
   assert(prevEnabled && !nowEnabled); //case 6, disable 
   viz->getGLWindow()->removeRenderer(dParams);

#endif // MODELS

   return;
}


//----------------------------------------------------------------------------
// Method called when undo/redo changes params.  If prevParams is null, the
// vizwinmgr will create a new instance.
//----------------------------------------------------------------------------
void ModelEventRouter::makeCurrent(Params* prevParams, Params* newParams,
                                   bool newWin, int instance, bool reEnable) 
{
   assert(instance >= 0);
   
   ModelParams* dParams = (ModelParams*)(newParams->deepCopy());
   int vizNum = dParams->getVizNum();
	
   // If we are creating one, it should be the first missing instance:
   if (!prevParams) assert(VizWinMgr::getInstance()->getNumInstances(vizNum,Params::GetTypeFromTag(ModelParams::_modelParamsTag)) == instance);
   VizWinMgr::getInstance()->setParams(vizNum, dParams, Params::GetTypeFromTag(ModelParams::_modelParamsTag), instance);
	
   
   ModelParams* formerParams = (ModelParams*)prevParams;
   if (reEnable)
   {
      // Need to disable the current instance
      assert(dParams->isEnabled() && formerParams->isEnabled());
      assert(!newWin);
      dParams->setEnabled(false);

      // Install dParams disabled:
      updateRenderer(dParams, true, false);

      // Then enable it
      dParams->setEnabled(true);
      updateRenderer(dParams, false, false);

   } 
   else 
   {
      bool wasEnabled = false;
      if (formerParams) wasEnabled = formerParams->isEnabled();

      // Check if the enabled and/or Local settings changed:
      if (newWin || (formerParams->isEnabled() != dParams->isEnabled()))
      {
         updateRenderer(dParams, wasEnabled,  newWin);
      }
   }
   
   updateTab();
}

