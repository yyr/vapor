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

#include "glutil.h"	// Must be included first!!!
#include <qgl.h>
#include "params.h"
#include "ModelRenderer.h"
#include "ModelScene.h"
#include "GLModelNode.h"
#include "animationparams.h"
#include "regionparams.h"
#include "viewpointparams.h"
#include "datastatus.h"
#include "glwindow.h"

#include <vapor/errorcodes.h>
#include <vapor/CFuncs.h>
#include <qgl.h>
#include <qcolor.h>
#include <qapplication.h>
#include <qcursor.h>
#include <fstream>
#include "renderer.h"
#include "proj_api.h"

using namespace VAPoR;

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

	map<string, GLModelNode*>::iterator p;
    for(p = _ourModelCache.begin(); p!=_ourModelCache.end(); ++p) {
        if (p->second) delete p->second;
    }
    _ourModelCache.clear();

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
   RegionParams* regionParams = myGLWindow->getActiveRegionParams();

   myGLWindow->makeCurrent();
   printOpenGLError();

   glPushMatrix();

   //
   // Perform setup of OpenGL transform matrix
   //
   //cerr << "transforming everything to unit box coords :-(\n";
   myGLWindow->TransformToUnitBox();

   const float* scales = DataStatus::getInstance()->getStretchFactors();
   glScalef(scales[0], scales[1], scales[2]);


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

   //
   // Get timestep information
   //
   AnimationParams* animationParams = myGLWindow->getActiveAnimationParams();
   int timestep = animationParams->getCurrentTimestep();

   if (params->GetClipping())
   {
      //
      // Setup clipping planes
      //
      GLdouble topPlane[] = {0., -1., 0., 1.};
      GLdouble rightPlane[] = {-1., 0., 0., 1.0};
      GLdouble leftPlane[] = {1., 0., 0., 0.};
      GLdouble botPlane[] = {0., 1., 0., 0.};
      GLdouble frontPlane[] = {0., 0., -1., 1.};//z largest
      GLdouble backPlane[] = {0., 0., 1., 0.};
      
      //Set up clipping planes
      float extents[6];
      regionParams->GetBox()->GetLocalExtents(extents, timestep);
      
      topPlane[3] = extents[4]*scales[1];
      botPlane[3] = -extents[1]*scales[1];
      leftPlane[3] = -extents[0]*scales[0];
      rightPlane[3] = extents[3]*scales[0];
      frontPlane[3] = extents[5]*scales[2];
      backPlane[3] = -extents[2]*scales[2];
      
      
      glClipPlane(GL_CLIP_PLANE0, topPlane);
      glEnable(GL_CLIP_PLANE0);
      glClipPlane(GL_CLIP_PLANE1, rightPlane);
      glEnable(GL_CLIP_PLANE1);
      glClipPlane(GL_CLIP_PLANE2, botPlane);
      glEnable(GL_CLIP_PLANE2);
      glClipPlane(GL_CLIP_PLANE3, leftPlane);
      glEnable(GL_CLIP_PLANE3);
      glClipPlane(GL_CLIP_PLANE4, frontPlane);
      glEnable(GL_CLIP_PLANE4);
      glClipPlane(GL_CLIP_PLANE5, backPlane);
      glEnable(GL_CLIP_PLANE5);
   }
   
   //
   // Retrieve and render the model
   //
   const ModelScene *scene = getModelScene(params);
   const GLModelNode *model = getModel(params, scene, timestep);

   if (model)
   {
      for (int clone=0; clone<scene->nClones(timestep); clone++)
      {
         Matrix3d matrix(params->GetTransformation());

         matrix *= Matrix3d(scene->transform(timestep, clone));

         // column-major for OpenGL
         matrix.transpose();

         model->draw(matrix);
      }
   }

   if (params->GetClipping())
   {
      glDisable(GL_CLIP_PLANE0);
      glDisable(GL_CLIP_PLANE1);
      glDisable(GL_CLIP_PLANE2);
      glDisable(GL_CLIP_PLANE3);
      glDisable(GL_CLIP_PLANE4);
      glDisable(GL_CLIP_PLANE5);
   }

   glPopMatrix();
   printOpenGLError();
}

//----------------------------------------------------------------------------
// Method to load the 3D model
//----------------------------------------------------------------------------
const GLModelNode* ModelRenderer::getModel(ModelParams *mParams, 
                                           const ModelScene *scene, int timestep)
{
   if (!scene) return NULL;

   string modelFile = scene->modelFile(timestep);

   if (modelFile.empty()) modelFile = mParams->GetModelFilename();

	// if modelFile is not an absolute path, construct a path
	// based on the path to the sceneFile
	//
	//Even on windows, we use a forward slash here because that 
    //is the path string that this Qt open file dialog returns.
	const string separator = "/";
#ifdef WIN32
	// On windows, an absolute path is of the form Q:/..."
	if (! modelFile.empty() && 
		(modelFile.compare(1,1, ":") != 0)) {
#else
	if (! modelFile.empty() && 
		(modelFile.compare(0,separator.length(), separator) != 0)) {
#endif
		string scenefile = mParams->GetSceneFilename();
		string newpath = Dirname(scenefile);
		newpath += separator;
		newpath += modelFile;
		modelFile = newpath;
	}



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

   if (_scene) delete _scene;
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

   mParams->clearDirtyScene();
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

#endif // MODELS
