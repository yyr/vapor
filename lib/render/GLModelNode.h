//
//      $Id$
//
//	File:		GLModelNode.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	3D model class for importing and rendering models of 3D geometry. This
//                      class uses the Open Asset Import Library (Assimp).
//
//

#ifndef GLModelNode_H
#define GLModelNode_H

#ifdef MODELS

#include <GL/glew.h>
#include <QtOpenGL>
#include <vector>
#include "Matrix3d.h"
#include <assimp/assimp.hpp>


using namespace std;

class aiNode;
class aiScene;

namespace VAPoR 
{

class GLModelNode
{
 public:

   GLModelNode();
   virtual ~GLModelNode();

   bool load(const string &modelFilename);
   void draw(const Matrix3d &transform) const;

   const char* errorString();

 protected:

   GLModelNode(const aiScene *scene, const aiNode *node);

 protected:

   GLuint _displayList;   
   Matrix3d _transform;
      
   vector<GLModelNode*> _children;

   Assimp::Importer _importer;
};

};

#endif // MODELS

#endif // GLModelNode_H
