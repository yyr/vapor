//
//      $Id$
//
//	File:		ModelScene.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Class to read and write 3d transformations 
//
//

#include "ModelScene.h"
#include <iostream>
#include <fstream>
#include <sstream>

using namespace VAPoR;


const string ModelScene::_tag          = "ModelScene";
const string ModelScene::_modelFileTag = "File";

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelScene::ModelScene() :
        _baseModelFile("")
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelScene::ModelScene(const ModelScene &scene)
{
   _baseModelFile  = scene._baseModelFile;

   //
   // Deep copy tranforms and timesteps
   //
   for (int t=0; t<scene._timesteps.size(); t++)
   {
      _timesteps.push_back(new ModelScene::Timestep(*scene._timesteps[t]));
   }

   for (int i=0; i<scene._transforms.size(); i++)
   {
      _transforms.push_back(new Transform3d(*scene._transforms[i]));
   }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelScene::~ModelScene()
{
   for (int t=0; t<_timesteps.size(); t++)
   {
      delete _timesteps[t]; _timesteps[t] = NULL;
   }

   _timesteps.clear();

   for (int i=0; i<_transforms.size(); i++)
   {
      delete _transforms[i]; _transforms[i] = NULL;
   }

   _transforms.clear();
}

//----------------------------------------------------------------------------
// Return the number of transforms in this scene, indicating how many times
// the model should be cloned in the rendering.
//----------------------------------------------------------------------------
int ModelScene::nClones(unsigned int timestep) const
{
   //
   // if the model timestep exists
   //
   if (timestep < _timesteps.size()) return _timesteps[timestep]->nClones();

   //
   // return the last timestep
   //
   if (_timesteps.size()) return _timesteps.back()->nClones();
   
   //
   // if there are no transformations, a single instance transformed
   // by the identity matrix is implied. 
   //
   return _transforms.size() ? _transforms.size() : 1;
}

//----------------------------------------------------------------------------
// Get the nth transformation at the given timestep (ownership is retained)
//----------------------------------------------------------------------------
const Transform3d* ModelScene::transform(unsigned int timestep, unsigned int clone) const
{
   //
   // if the model timestep exists, return the transform
   //
   if (timestep < _timesteps.size()) return _timesteps[timestep]->transform(clone);
   
   //
   // the model timestep does not exist, return the last timestep
   //
   if (_timesteps.size()) return _timesteps.back()->transform(clone);

   //
   // no transform exists, return the identity matrix
   //
   return &_identity;   
}

//----------------------------------------------------------------------------
// Get the model file at the given timestep
//----------------------------------------------------------------------------
const string& ModelScene::modelFile(unsigned int timestep) const
{
   //
   // if the model timestep exists, return the transform
   //
   if (timestep < _timesteps.size() && !(_timesteps[timestep]->modelFile().empty()))
   {
      return _timesteps[timestep]->modelFile();
   }
   
   //
   // the model timestep does not exist, return the default modelFile
   //
   return _baseModelFile;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
ParamNode* ModelScene::buildNode()
{
   //Construct the scene node
   std::map <string, string> attrs;
   attrs.clear();	
   string empty;
   ostringstream oss;
   oss.str(empty);

   if (!_baseModelFile.empty())
   {
      oss << _baseModelFile;
      attrs[_modelFileTag] = oss.str();
   }

   ParamNode* sceneNode = new ParamNode(_tag, attrs,
                                        _transforms.size()+_timesteps.size());

   for (int i=0; i<_transforms.size(); i++)
   {
      sceneNode->AddChild(_transforms[i]->buildNode());
   }

   for (int t=0; t<_timesteps.size(); t++)
   {
      sceneNode->AddChild(_timesteps[t]->buildNode());
   }

   return sceneNode;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool ModelScene::elementStartHandler(ExpatParseMgr* pm, int level, 
                                     std::string& tagString, 
                                     const char **attrs)
{
   ExpatStackElement *state = pm->getStateStackTop();

   if (level == 0 && StrCmpNoCase(tagString, _tag) == 0) 
   {
      _baseModelFile = "vaporscene.obj";

      return true;
   }
   else if (level == 1 && StrCmpNoCase(tagString, _modelFileTag) == 0)
   {
      state->has_data = 1;
      state->data_type = _stringType;
      
      return true;
   }
   else if (level == 1 && StrCmpNoCase(tagString, ModelScene::Timestep::xmlTag()) == 0) 
   {
      ModelScene::Timestep *timestep = new ModelScene::Timestep();

      _timesteps.push_back(timestep);

      pm->pushClassStack(timestep);

      return timestep->elementStartHandler(pm, level, tagString, attrs);
   }
   else if (level == 1 && StrCmpNoCase(tagString, Transform3d::xmlTag()) == 0) 
   {
      Transform3d *transform = new Transform3d();

      _transforms.push_back(transform);

      pm->pushClassStack(transform);

      return transform->elementStartHandler(pm, level, tagString, attrs);
   }
   
   return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool ModelScene::elementEndHandler(ExpatParseMgr* pm, int depth, 
                                           std::string &tag)
{
   if (StrCmpNoCase(tag, _tag) == 0 && depth == 0) 
   {
      return true;
   } 
   else if (depth == 1 && StrCmpNoCase(tag, _modelFileTag) == 0) 
   {
      _baseModelFile = pm->getStringData();

      return true;
   }
   else if (StrCmpNoCase(tag, Transform3d::xmlTag()) == 0 && depth == 1) 
   {
      return true;
   }
   else if (StrCmpNoCase(tag, ModelScene::Timestep::xmlTag()) == 0 && depth == 1) 
   {
      return true;
   }

   pm->parseError("Unrecognized end tag in ModelScene %s", tag.c_str());

   return false;
}

      
   
//============================================================================
// Class Timestep
//============================================================================
const string ModelScene::Timestep::_tag          = "Timestep";
const string ModelScene::Timestep::_modelFileTag = "File";

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelScene::Timestep::Timestep() :
        _modelFile("")
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelScene::Timestep::Timestep(const ModelScene::Timestep &timestep)
{
   _modelFile = timestep._modelFile;

   //
   // Deep copy transforms
   //
   for (int i=0; i<timestep._transforms.size(); i++)
   {
      _transforms.push_back(new Transform3d(*timestep._transforms[i]));
   }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ModelScene::Timestep::~Timestep()
{
   for (int i=0; i<_transforms.size(); i++)
   {
      delete _transforms[i]; _transforms[i] = NULL;
   }

   _transforms.clear();
}

//----------------------------------------------------------------------------
// Return the number of transforms in this scene, indicating how many times
// the model should be cloned in the rendering.
//----------------------------------------------------------------------------
int ModelScene::Timestep::nClones() const
{
   // if there are no transformations, a single instance transformed
   // by the identity matrix is implied. 
   return _transforms.size() ? _transforms.size() : 1;
}

//----------------------------------------------------------------------------
// Get the nth transformation (ownership is retained)
//----------------------------------------------------------------------------
const Transform3d* ModelScene::Timestep::transform(unsigned int clone) const
{
   //
   // return the nth transform
   //
   if (clone < _transforms.size()) return _transforms[clone];

   //
   // no transform exists, return the identity matrix
   //
   return &_identity;   
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ParamNode* ModelScene::Timestep::buildNode()
{
   //Construct the scene node
   std::map <string, string> attrs;
   attrs.clear();	
   string empty;
   ostringstream oss;
   oss.str(empty);

   if (!_modelFile.empty())
   {
      oss << _modelFile;
      attrs[_modelFileTag] = oss.str();
   }

   ParamNode* tsNode = new ParamNode(_tag, attrs, _transforms.size());

   for (int i=0; i<_transforms.size(); i++)
   {
      tsNode->AddChild(_transforms[i]->buildNode());
   }

   return tsNode;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool ModelScene::Timestep::elementStartHandler(ExpatParseMgr* pm, int depth, 
                                               std::string& tag, 
                                               const char **attrs)
{
   ExpatStackElement *state = pm->getStateStackTop();

   if (StrCmpNoCase(tag, _tag) == 0) 
   {
      return true;
   }
   else if (StrCmpNoCase(tag, _modelFileTag) == 0)
   {
      state->has_data = 1;
      state->data_type = _stringType;
      
      return true;
   }
   else if (StrCmpNoCase(tag, Transform3d::xmlTag()) == 0) 
   {
      Transform3d *transform = new Transform3d();

      _transforms.push_back(transform);

      pm->pushClassStack(transform);

      return transform->elementStartHandler(pm, depth, tag, attrs);
   }
   
   return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool ModelScene::Timestep::elementEndHandler(ExpatParseMgr* pm, int depth, 
                                             std::string &tag)
{
   if (StrCmpNoCase(tag, _tag) == 0)
   {
      if (depth == 0) return true;
   
      ParsedXml* px = pm->popClassStack();
      bool ok = px->elementEndHandler(pm, depth, tag);
      
      return ok;
   } 
   else if (StrCmpNoCase(tag, _modelFileTag) == 0) 
   {
      _modelFile = pm->getStringData();

      return true;
   }
   else if (StrCmpNoCase(tag, Transform3d::xmlTag()) == 0) 
   {
      return true;
   }

   pm->parseError("Unrecognized end tag in ModelScene::Timestep %s", tag.c_str());

   return false;
}

