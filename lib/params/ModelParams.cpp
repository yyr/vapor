//
//      $Id$
//
//	File:		ModelParams.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Params class to capture 3d geometric model state. 
//                      Currently model filename and the model transform(s). 
//                      The actual models are stored as vertex buffer objects, 
//                      requiring an OpenGL context and are therefore stored 
//                      in the ModelRenderer.
//
//


#include "ModelParams.h"
#include "mapperfunction.h"
#include "transferfunction.h"
#include <vapor/Version.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace VetsUtil;
using namespace VAPoR;

//
// Static initialization
//
const string ModelParams::_shortName = "Model";
const string ModelParams::_modelParamsTag = "ModelParams";
const string ModelParams::_sceneFilenameTag = "SceneFilename";
const string ModelParams::_modelFilenameTag = "ModelFilename";
const string ModelParams::_transformTag = "Transformation";
const string ModelParams::_constantColorTag = "ConstantColor";

namespace 
{
   const string ModelName = "ModelParams";
};

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelParams::ModelParams(XmlNode *parent, int winnum) : 
        RenderParams(parent, _modelParamsTag, winnum)
{
   restart();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelParams::~ModelParams() 
{
}

//----------------------------------------------------------------------------
// Initialize for new metadata.  Keep old transfer functions
//
// For each new variable in the metadata create a variable child node, and 
// build the associated isoControl and transfer function nodes.
//----------------------------------------------------------------------------
bool ModelParams::reinit(bool doOverride)
{
   if (doOverride) 
   {
      const float col[3] = {1.f, 1.f, 1.f};
      SetConstantColor(col);

      GetTransformation()->clear();
   }

   return true;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelParams::restart() 
{
   vector<string> emptyModels;
   vector<string> emptyMatrices;

   SetModelFilename("");
   SetSceneFilename("");

   if (!GetRootNode()->HasChild(Transform3d::xmlTag()))
   {
      Transform3d *trans = new Transform3d();
      ParamNode   *node  = trans->GetRootNode();

      GetRootNode()->AddRegisteredNode(Transform3d::xmlTag(), node, trans);
   }

   GetTransformation()->clear();

   const float default_color[3] = {1.0, 1.0, 1.0};
   SetConstantColor(default_color);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelParams::setDefaultPrefs()
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d* ModelParams::GetTransformation()
{
   if (GetRootNode() && (Transform3d*)GetRootNode()->GetNode(Transform3d::xmlTag()))
   {
      return (Transform3d*)GetRootNode()->GetNode(Transform3d::xmlTag())->GetParamsBase();
   }
   
   return NULL;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelParams::SetModelFilename(const string filename)
{
   GetRootNode()->SetElementString(_modelFilenameTag, filename);
   setModelDirty();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
string ModelParams::GetModelFilename()
{
   string filename = GetRootNode()->GetElementString(_modelFilenameTag);
   return filename;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelParams::SetSceneFilename(const string filename)
{
   GetRootNode()->SetElementString(_sceneFilenameTag, filename);
   setModelDirty();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
string ModelParams::GetSceneFilename()
{
   string filename = GetRootNode()->GetElementString(_sceneFilenameTag);
   return filename;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void ModelParams::SetConstantColor(const float rgb[3]) 
{
   vector <double> valvec(3,0);
   for (int i=0; i<3; i++) 
   {
      valvec[i] = rgb[i];
   }
   GetRootNode()->SetElementDouble(_constantColorTag, valvec);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
const float *ModelParams::GetConstantColor() 
{
   vector <double> valvec = GetRootNode()->GetElementDouble(_constantColorTag);

   for (int i=0; i<3; i++) 
   {
      _constcolorbuf[i] = valvec[i];
   }

   return(_constcolorbuf);
}
