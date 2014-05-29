//
//      $Id$
//
//	File:		ModelScene.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Class to read and write model scene files
//
//

#ifndef _ModelScene3d_h_
#define _ModelScene3d_h_

#include <vapor/ExpatParseMgr.h>
#include <vector>
#include <string>
#include "Transform3d.h"
#include "params.h"

#define DEGREES_TO_RADIANS 0.0174532925199

namespace VAPoR
{

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
class PARAMS_API ModelScene : public ParsedXml
{

 public:

   ModelScene();
   ModelScene(const ModelScene &scene);

   virtual ~ModelScene();   

   int nClones(unsigned int timestep) const;
   const Transform3d* transform(unsigned int timestep, unsigned int clone=0) const;
   const vector<double> Color(unsigned int timestep, unsigned int clone=0) const;
   const string&      modelFile(unsigned int timestep) const;

   ParamNode* buildNode();

   static string xmlTag() { return _tag; }
   
   virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                    const char **attribs);
   virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
   
 protected:

   class PARAMS_API Timestep : public ParsedXml
   {
      public:

      Timestep();
      Timestep(const Timestep &ts);
      virtual ~Timestep();

      //
      // The (optional) transforms for this timestep
      // Each Transform3d, clones the model. 
      // Default: Identity transform
      //
      int                nClones() const;
      const Transform3d* transform(unsigned int clone=0) const;

	  const vector<double> color(unsigned int clone=0) const;

      //
      // The (optional) model file for this timestep
      //
      const string& modelFile() const { return _modelFile; }

      //
      // XML creation/parsing
      //
      ParamNode* buildNode();
      
      static string xmlTag() { return _tag; }
      
      virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                       const char **attribs);
      virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
      
     protected:

      string               _modelFile;
      vector<Transform3d*> _transforms;
      Transform3d          _identity;

      static const string  _tag;
      static const string  _modelFileTag;
   };

   string               _baseModelFile;
   vector<Transform3d*> _transforms;
   vector<Timestep*>    _timesteps;
   Transform3d          _identity;

   static const string  _tag;
   static const string  _modelFileTag;
};
   
};

#endif
