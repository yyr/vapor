//
//      $Id$
//
//	File:		GLModelNode.cpp
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	3D model class for importing and rendering models of 3D geometry. This
//                      class uses the Open Asset Import Library (Assimp).
//
//

#ifdef MODELS

#include <GLModelNode.h>
#include "glutil.h"

#include <assimp.h>
#include <aiScene.h>
#include <aiPostProcess.h>
#include <assert.h>


using namespace VAPoR;

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
GLModelNode::GLModelNode() :
        _displayList(0)
{
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
GLModelNode::GLModelNode(const aiScene *scene, const aiNode *node) :
        _displayList(0)
{
   //
   // Copy the transformation matrix
   //
   aiMatrix4x4 m = node->mTransformation;
   aiTransposeMatrix4(&m);
   _transform = m[0]; 
   
   //
   // Build the display list
   //
   _displayList = glGenLists(1);

   glNewList(_displayList, GL_COMPILE);
   glBegin(GL_TRIANGLES);

   for (unsigned int m=0; m < node->mNumMeshes; m++)
   {
      const aiMesh *mesh = scene->mMeshes[node->mMeshes[m]];

      for (unsigned int f=0; f < mesh->mNumFaces; f++)
      {
         const aiFace *face = &mesh->mFaces[f];

         assert(face->mNumIndices == 3);

         for (int i=0; i<3; i++)
         {
            int index = face->mIndices[i];

            glVertex3f(mesh->mVertices[index].x,
                       mesh->mVertices[index].y,
                       mesh->mVertices[index].z);

            glNormal3f(mesh->mNormals[index].x,
                       mesh->mNormals[index].y,
                       mesh->mNormals[index].z);
         }
      }
   }

   glEnd();
   glEndList();

   printOpenGLError();

   //
   // Build the children
   //
   for (unsigned int n=0; n < node->mNumChildren; n++)
   {
      _children.push_back(new GLModelNode(scene, node->mChildren[n]));
   }

   /* 
      Initially I implemented this via vertex buffer object, but large 
      models posed problems on lower-end hardware due to buffer limits.
      Punted in favor of display lists. 
   //
   // Construct the VBOs
   //
   glGenBuffers(1, &_verticesVBO);
   glBindBuffer(GL_ARRAY_BUFFER, _verticesVBO);
   glBufferData(GL_ARRAY_BUFFER, _vertices.size()*sizeof(float), &_vertices[0],
                GL_STATIC_DRAW);

   // TBD -- Place holder for future work: colormapped geometry -- TBD
   //
   //glGenBuffers(1, &_texcoordsVBO);
   //glBindBuffer(GL_ARRAY_BUFF || _models[framenum]ER, _texcoordsVBO);
   //glBufferData(GL_ARRAY_BUFFER, _texcoords.size()*sizeof(float), 
   //             &_texcoords[0], GL_STATIC_DRAW);

   glGenBuffers(1, &_normalsVBO);
   glBindBuffer(GL_ARRAY_BUFFER, _normalsVBO);
   glBufferData(GL_ARRAY_BUFFER, _normals.size()*sizeof(float), &_normals[0],
                GL_STATIC_DRAW);
   */
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
bool GLModelNode::load(const string &filename)
{

   QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));

   const aiScene *scene = _importer.ReadFile(filename,
                                             aiProcess_Triangulate |
                                             aiProcess_JoinIdenticalVertices |
                                             aiProcess_OptimizeGraph |
                                             aiProcess_GenSmoothNormals);
   QApplication::restoreOverrideCursor();

   if (!scene)
   {
      return false;
   }
   
   _children.push_back(new GLModelNode(scene, scene->mRootNode));

   return true;
}

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
const char* GLModelNode::errorString()
{
   return _importer.GetErrorString();
}


//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
GLModelNode::~GLModelNode()
{
   glDeleteLists(1, _displayList);

   for (unsigned int n=0; n < _children.size(); n++)
   {
      delete _children[n];
   }

   _children.clear();

   _importer.FreeScene();
}
      
//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
void GLModelNode::draw(const Matrix3d &transform) const
{
   printOpenGLError();

   glPushMatrix();

   glMultMatrixf(transform.data());
   glMultMatrixf(_transform.data());

   if (_displayList) glCallList(_displayList);

   for (unsigned int n=0; n < _children.size(); n++)
   {
      _children[n]->draw(Matrix3d());
   }
   
   glPopMatrix();

   printOpenGLError();
}

#endif MODELS
