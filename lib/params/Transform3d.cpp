//
//      $Id$
//
//	File:		Transform3d.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Class to read and write 3d transformations 
//
//

#include "Transform3d.h"
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>

using namespace VAPoR;


//============================================================================
// Class Rotate
//============================================================================
const string Transform3d::Rotate::_tag          = "Rotate";

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Rotate::Rotate(double x, double y, double z, double deg) :
        _rotation(4, 0.0)
{
   _rotation[0] = x;
   _rotation[1] = y;
   _rotation[2] = z;
   _rotation[3] = deg;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::TransformBase* Transform3d::Rotate::clone()
{
   return new Transform3d::Rotate(_rotation[0], _rotation[1], 
                                  _rotation[2], _rotation[3]);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
ParamNode* Transform3d::Rotate::buildNode()
{
   //Construct the rotate node
   ParamNode* rotateNode = new ParamNode(_tag, 0);

   rotateNode->SetElementDouble(_tag, _rotation);

   return rotateNode;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool Transform3d::Rotate::elementStartHandler(ExpatParseMgr* pm, int depth, 
                                              std::string& tagString, 
                                              const char **attrs)
{
   ExpatStackElement *state = pm->getStateStackTop();

   if (StrCmpNoCase(tagString, _tag) == 0) 
   {
      state->has_data  = 1;
      state->data_type = _doubleType;

      return true;
   }
   
   return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool Transform3d::Rotate::elementEndHandler(ExpatParseMgr* pm, int depth, 
                                           std::string &tag)
{
   //Check only for the rotation tag, ignore others.
   if (StrCmpNoCase(tag, _tag) != 0) return true;
  
   _rotation = pm->getDoubleData();
   
   ParsedXml* px = pm->popClassStack();
   bool ok = px->elementEndHandler(pm, depth, tag);
   
   return ok;
}
      
   
//============================================================================
// Class Translate
//============================================================================
const string Transform3d::Translate::_tag   = "Translate";

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Translate::Translate(double x, double y, double z) :
        _translation(3,0)
{
   _translation[0] = x;
   _translation[1] = y;
   _translation[2] = z;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::TransformBase* Transform3d::Translate::clone()
{
   return new Transform3d::Translate(_translation[0], _translation[1], _translation[2]);
}
      
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
ParamNode* Transform3d::Translate::buildNode()
{
   //Construct the translate node
   std::map <string, string> attrs;
   attrs.clear();

   ParamNode* translateNode = new ParamNode(_tag, attrs, 0);

   translateNode->SetElementDouble(_tag, _translation);

   return translateNode;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool Transform3d::Translate::elementStartHandler(ExpatParseMgr* pm, int depth, 
                                                 std::string& tagString, 
                                                 const char **attrs)
{
   ExpatStackElement *state = pm->getStateStackTop();

   if (StrCmpNoCase(tagString, _tag) == 0) 
   {
      state->has_data  = 1;
      state->data_type = _doubleType;

      return true;
   }
   
   return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool Transform3d::Translate::elementEndHandler(ExpatParseMgr* pm, int depth, 
                                           std::string &tag)
{
   //Check only for the transform tag, ignore others.
   if (StrCmpNoCase(tag, _tag) != 0) return true;
  
   _translation = pm->getDoubleData();
   
   ParsedXml* px = pm->popClassStack();
   bool ok = px->elementEndHandler(pm, depth, tag);
   
   return ok;
}


//============================================================================
// Class Scale
//============================================================================
const string Transform3d::Scale::_tag   = "Scale";

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Scale::Scale(double x, double y, double z) :
        _scale(3, 1.0)
{
   _scale[0] = x;
   _scale[1] = y;
   _scale[2] = z;
}
      
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::TransformBase* Transform3d::Scale::clone()
{
   return new Transform3d::Scale(_scale[0], _scale[1], _scale[2]);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
ParamNode* Transform3d::Scale::buildNode()
{
   //Construct the rotate node
   std::map <string, string> attrs;
   attrs.clear();

   ParamNode* scaleNode = new ParamNode(_tag, attrs, 0);

   scaleNode->SetElementDouble(_tag, _scale);

   return scaleNode;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool Transform3d::Scale::elementStartHandler(ExpatParseMgr* pm, int depth, 
                                             std::string& tagString, 
                                             const char **attrs)
{
   ExpatStackElement *state = pm->getStateStackTop();

   if (StrCmpNoCase(tagString, _tag) == 0) 
   {
      state->has_data  = 1;
      state->data_type = _doubleType;

      return true;
   }
   
   return false;

}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool Transform3d::Scale::elementEndHandler(ExpatParseMgr* pm, int depth, 
                                           std::string &tag)
{
   //Check only for the transferfunction tag, ignore others.
   if (StrCmpNoCase(tag, _tag) != 0) return true;
     
   _scale = pm->getDoubleData();
   
   ParsedXml* px = pm->popClassStack();
   bool ok = px->elementEndHandler(pm, depth, tag);
   
   return ok;
}

   
   
//============================================================================
// class Matrix
//============================================================================   
const string Transform3d::Matrix::_tag     = "matrix";
   
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Matrix::Matrix() :
   _matrix(16, 0.0)
{
   _matrix[0] = 1;  _matrix[1] = 0;   _matrix[2] = 0;  _matrix[3] = 0;
   _matrix[4] = 0;  _matrix[5] = 1;   _matrix[6] = 0;  _matrix[7] = 0;
   _matrix[8] = 0;  _matrix[9] = 0;  _matrix[10] = 1; _matrix[11] = 0;
   _matrix[12]= 0; _matrix[13] = 0;  _matrix[14] = 0; _matrix[15] = 1;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Matrix::Matrix(const vector<double> &m)
{
   _matrix = m;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Matrix::Matrix(const Matrix &matrix)
{
   _matrix = matrix._matrix;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::TransformBase* Transform3d::Matrix::clone()
{
   return new Transform3d::Matrix(_matrix);
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
ParamNode* Transform3d::Matrix::buildNode()
{
   //Construct the matrix node
   ParamNode* matrixNode = new ParamNode(_tag, 0);

   matrixNode->SetElementDouble(_tag, _matrix);

   return matrixNode;
}
      
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool Transform3d::Matrix::elementStartHandler(ExpatParseMgr *pm, int, 
                                              std::string& tagString, 
                                              const char **attrs)
{
   if (StrCmpNoCase(tagString, _tag) == 0) 
   {
      ExpatStackElement *state = pm->getStateStackTop();
      state->has_data = 1;
      state->data_type = "double";
      
      return true;
   }

   return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------      
bool Transform3d::Matrix::elementEndHandler(ExpatParseMgr* pm, 
                                            int depth, std::string &tag)
{
   if (StrCmpNoCase(tag, _tag) == 0) 
   {
      _matrix = pm->getDoubleData();

      //If this is a regionparams, need to
      //pop the parse stack.  
      ParsedXml* px = pm->popClassStack();
      bool ok = px->elementEndHandler(pm, depth, tag);
      return ok;
   }

   pm->parseError("Unrecognized end tag in Transform3d::Matrix %s", tag.c_str());
   return false;
}
      

//============================================================================
// Class Transform3d
//============================================================================
const string Transform3d::_tag     = "Transform";

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Transform3d() :
        ParamsBase(0, _tag),
        _transformations(0,NULL)
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::Transform3d(const Transform3d &transform3d) :
        ParamsBase(transform3d)
{
   for (int i=0; i<transform3d._transformations.size(); i++)
   {
      _transformations.push_back(transform3d._transformations[i]->clone());
   }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ParamsBase* Transform3d::deepCopy(ParamNode *node)
{
   Transform3d *t = new Transform3d(*this);

   t->SetRootParamNode(node);

   if (node) node->SetParamsBase(t);
   return t;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool Transform3d::set(const QStringList &strings)
{
   clear();

   bool ok[4];
   
   for (int i=0; i<strings.size(); i++)
   {
      QStringList data = strings[i].split(" ");

      if (data[0] == "Translate")
      {
         if (data.size() != 4) 
         {
            clear(); return false;
         }

         double x = data[1].toDouble(&ok[0]);
         double y = data[2].toDouble(&ok[1]);
         double z = data[3].toDouble(&ok[2]);

         if (!ok[0] || !ok[1] || !ok[2])
         {
            clear();
            return false;
         }

         _transformations.push_back(new Translate(x, y, z));
      }

      else if (data[0] == "Scale")
      {
         if (data.size() != 4) 
         {
            clear(); return false;
         }

         double x = data[1].toDouble(&ok[0]);
         double y = data[2].toDouble(&ok[1]);
         double z = data[3].toDouble(&ok[2]);

         if (!ok[0] || !ok[1] || !ok[2])
         {
            clear();
            return false;
         }

         _transformations.push_back(new Scale(x,y,z));
      }

      else if (data[0] == "Rotate")
      {
         if (data.size() != 5) 
         {
            clear(); return false;
         }

         double x = data[1].toDouble(&ok[0]);
         double y = data[2].toDouble(&ok[1]);
         double z = data[3].toDouble(&ok[2]);
         double t = data[4].toDouble(&ok[3]);

         if (!ok[0] || !ok[1] || !ok[2] || !ok[3])
         {
            clear();
            return false;
         }

         _transformations.push_back(new Rotate(x,y,z,t));
      }
   }
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
QStringList Transform3d::get()
{
   QStringList strings;

   for (int i=0; i<_transformations.size(); i++)
   {
      strings << _transformations[i]->label();
   }

   return strings;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
Transform3d::~Transform3d()
{
   clear();
}

//----------------------------------------------------------------------------
// Resets to identity
//----------------------------------------------------------------------------
void Transform3d::clear()
{
   for (int i=0; i<_transformations.size(); i++)
   {
      delete _transformations[i]; _transformations[i] = NULL;
   }

   _transformations.clear();
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
ParamNode* Transform3d::buildNode()
{
   string empty;
   std::map <string, string> attrs;
   attrs.empty();
   ostringstream oss;
   
   attrs[_typeAttr] = ParamNode::_paramsBaseAttr;

   //Construct the trasnform node
   ParamNode* transformNode = new ParamNode(_tag, attrs, _transformations.size());

   for (int i=0; i<_transformations.size(); i++)
   {
      transformNode->AddChild(_transformations[i]->buildNode());
   }

   return transformNode;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool Transform3d::elementStartHandler(ExpatParseMgr* pm, int depth, 
                                      std::string &tagString,
                                      const char **attrs)
{
   if (StrCmpNoCase(tagString, _tag) == 0) 
   {
      return true;
   }
   else if (StrCmpNoCase(tagString, Transform3d::Translate::xmlTag()) == 0) 
   {
      Transform3d::Translate *translate = new Transform3d::Translate();

      _transformations.push_back(translate);

      pm->pushClassStack(translate);

      return translate->elementStartHandler(pm, depth, tagString, attrs);
   }
   else if (StrCmpNoCase(tagString, Transform3d::Rotate::xmlTag()) == 0) 
   {
      Transform3d::Rotate *rotate = new Transform3d::Rotate();

      _transformations.push_back(rotate);

      pm->pushClassStack(rotate);

      return rotate->elementStartHandler(pm, depth, tagString, attrs);
   }
   else if (StrCmpNoCase(tagString, Transform3d::Scale::xmlTag()) == 0) 
   {
      Transform3d::Scale *scale = new Transform3d::Scale();

      _transformations.push_back(scale);

      pm->pushClassStack(scale);

      return scale->elementStartHandler(pm, depth, tagString, attrs);
   }
   else if (StrCmpNoCase(tagString, Transform3d::Matrix::xmlTag()) == 0) 
   {
      Transform3d::Matrix *matrix = new Transform3d::Matrix();

      _transformations.push_back(matrix);

      pm->pushClassStack(matrix);

      return matrix->elementStartHandler(pm, depth, tagString, attrs);
   }

   return false;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool Transform3d::elementEndHandler(ExpatParseMgr* pm, 
                                    int depth, std::string &tagString)
{
   if (StrCmpNoCase(tagString, _tag) == 0) 
   {
      //If depth is 0, this is a color map file; otherwise need to
      //pop the parse stack.  The caller will need to save the resulting
      //transfer function (i.e. this)
      if (depth == 0) return true;
   
      ParsedXml* px = pm->popClassStack();
      bool ok = px->elementEndHandler(pm, depth, tagString);
      
      return ok;
   }
   else if (StrCmpNoCase(tagString, Transform3d::Translate::xmlTag()) == 0) 
   {
      return true;
   }
   else if (StrCmpNoCase(tagString, Transform3d::Rotate::xmlTag()) == 0) 
   {
      return true;
   }
   else if (StrCmpNoCase(tagString, Transform3d::Scale::xmlTag()) == 0) 
   {
      return true;
   }
   else if (StrCmpNoCase(tagString, Transform3d::Matrix::xmlTag()) == 0) 
   {
      return true;
   }

   pm->parseError("Unrecognized end tag in Transform3d %s", tagString.c_str());

   return false;
}
