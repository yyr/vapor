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

#ifndef ModelRenderer_H
#define ModelRenderer_H

#ifdef MODELS

#include <GL/glew.h>
#include "assert.h"
#include "renderer.h"
#include "ModelParams.h"

namespace VAPoR 
{

class Matrix3d; 
class GLModelNode;
class ModelScene;

class RENDER_API ModelRenderer : public Renderer
{

public:

   ModelRenderer(GLWindow*, ModelParams*);
   ~ModelRenderer();
	
   virtual void	initializeGL();
   virtual void paintGL();

   const GLModelNode* getModel(ModelParams*, const ModelScene*, int frameNum);
   const ModelScene*  getModelScene(ModelParams*);

   void setAllDataDirty();
	
protected:

   GLModelNode      *_model;
   ModelScene       *_scene;

   // Keep a cache of models to limit memory and load times for 
   // scenes with duplicate models. 
   static map<string, GLModelNode*> _ourModelCache;
};
};

#endif // MODELS

#endif // ModelRenderer_H
