//
//      $Id$
//
//	File:		ModelRenderer.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	3D Model rendering class
//
//

#ifdef MODELS

#include "params.h"
#include "ModelRenderer.h"
#include "ModelScene.h"
#include "GLModelNode.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"
#include "glutil.h"

#include <vapor/errorcodes.h>
#include <vapor/DataMgr.h>
#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include <fstream>
#include "renderer.h"
#include "proj_api.h"

using namespace VAPoR;

//
// Static initialization
//
map<string, GLModelNode*> ModelRenderer::_ourModelCache;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelRenderer::ModelRenderer(GLWindow* glw, ModelParams* mParams) :
        Renderer(glw, mParams, "ModelRenderer"),
        _model(NULL),
        _scene(NULL)

{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelRenderer::~ModelRenderer()
{
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelRenderer::initializeGL()
{
   printOpenGLError();
   myGLWindow->makeCurrent();
   myGLWindow->qglClearColor(Qt::black); // Let OpenGL clear to black
   initialized = true;
   printOpenGLError();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelRenderer::paintGL()
{
   ModelParams* params = (ModelParams*)getRenderParams();

   myGLWindow->makeCurrent();
   printOpenGLError();

   glPushMatrix();

   //
   // Perform setup of OpenGL transform matrix
   //
   float sceneScaleFactor = 1.f/ViewpointParams::getMaxStretchedCubeSide();
   glScalef(sceneScaleFactor, sceneScaleFactor, sceneScaleFactor);
   float* transVec = ViewpointParams::getMinStretchedCubeCoords();
   glTranslatef(-transVec[0],-transVec[1], -transVec[2]);

   // 
   // Set up lighting and color
   //
   ViewpointParams* vpParams =  myGLWindow->getActiveViewpointParams();
   int nLights = vpParams->getNumLights();
   const float *constColor = params->GetConstantColor();
	
   if (nLights == 0) 
   {
      glDisable(GL_LIGHTING);
   }
   else 
   {
      glShadeModel(GL_SMOOTH);
      glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, constColor);
      glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, vpParams->getExponent());

      //All the geometry will get a white specular color:
      const float specColor[4] = { 0.8, 0.8, 0.8, 1.0 };
      glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specColor);
      glEnable(GL_LIGHTING);
      glEnable(GL_COLOR_MATERIAL);

   }
   
   glColor3fv(constColor);

   const float* scales = DataStatus::getInstance()->getStretchFactors();
   glScalef(scales[0], scales[1], scales[2]);

   //
   // Retrieve and render the model
   //
   AnimationParams* animationParams = myGLWindow->getActiveAnimationParams();
   int framenum = animationParams->getCurrentFrameNumber();

   const ModelScene *scene = getModelScene(params);
   const GLModelNode *model = getModel(params, scene, framenum);

   if (model)
   {
      for (int clone=0; clone<scene->nClones(framenum); clone++)
      {
         Matrix3d matrix(params->GetTransformation());

         matrix *= Matrix3d(scene->transform(framenum, clone));

         // column-major for OpenGL
         matrix.transpose();

         model->draw(matrix);
      }
   }

   glPopMatrix();
   printOpenGLError();
}

//----------------------------------------------------------------------------
// Method to load the 3D model
//----------------------------------------------------------------------------
const GLModelNode* ModelRenderer::getModel(ModelParams *mParams, 
                                           const ModelScene *scene, int framenum)
{
   if (!scene) return NULL;

   string modelFile = scene->modelFile(framenum);

   if (modelFile.empty()) modelFile = mParams->GetModelFilename();

   //
   // Check the cache for duplicates
   //
   if (_ourModelCache.find(modelFile) != _ourModelCache.end())
   {
      _model = _ourModelCache[modelFile];
      return _model;
   }

   //
   // Create a new model
   //
   _model = _ourModelCache[modelFile] = new GLModelNode();

   if (!_model->load(modelFile))
   {
      //
      // Load failed. Report error and clean up. 
      //
      MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, 
                        _model->errorString());
      
      delete _model; 
      _model = NULL;
      _ourModelCache.erase(modelFile);
   }

   return _model;
}

//----------------------------------------------------------------------------
// Method to retrieve the model scene
//----------------------------------------------------------------------------
const ModelScene* ModelRenderer::getModelScene(ModelParams *mParams)
{
   if (!mParams->sceneIsDirty() && _scene)
   {
      return _scene;
   }

   delete _scene;
   _scene = new ModelScene();

   const string filename = mParams->GetSceneFilename();

   if (filename.empty()) return _scene;

   ifstream istr(filename.c_str(), ios::in);

   if (istr.good())
   {
      ExpatParseMgr *parseMgr = new ExpatParseMgr(_scene);

      parseMgr->parse(istr);
   }
   else
   {
      QString msg = QString("Error reading model scene file: ") + 
                    QString(filename.c_str());

      MyBase::SetErrMsg(VAPOR_ERROR_DATA_UNAVAILABLE, msg.toAscii().data());
   }

   return _scene;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelRenderer::setAllDataDirty()
{
   ((ModelParams*)getRenderParams())->setModelDirty();
   ((ModelParams*)getRenderParams())->setTransformationDirty();
}

#endif MODELS
