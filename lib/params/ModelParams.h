//
//      $Id$
//
//	File:		ModelParams.h
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

#ifndef _ModelParams_h_
#define _ModelParams_h_


#include <vapor/XmlNode.h>
#include <vapor/ParamNode.h>
#include "params.h"
#include "datastatus.h"
#include "mapperfunction.h"
#include "Transform3d.h"
#include <vector>

namespace VAPoR
{

//
//! \class ModelParams
//! \brief A class for managing (storing and retrieving) 3D Model parameters.
//! \author Kenny Gruchalla
//! \version $Revision$
//! \date    $Date$
//!
//! 
class PARAMS_API ModelParams : public RenderParams 
{

public: 


 //! Create a ModelParams abstract base class from scratch
 //!
 ModelParams(XmlNode *parent=0, int winnum=0);
 ModelParams(const ModelParams &params): RenderParams(params) { assert(0); }

 virtual ~ModelParams();

 //! Reset parameter state to the default
 //!
 //! This pure virtual method must be implented by the derived class to
 //! restore state to the default
 //
 virtual void restart();
 
 //! Create a default instance.  Required of all params classes
 //!
 //! This static method must be implemented
 //!
 static ParamsBase* CreateDefaultInstance() { return new ModelParams(0,-1); }

 //! Specify a short name for the class.  Will be used to identify
 //! the panel in the tab
 //!
 //! This static method must be implemented by all Params classes
 //!
 const std::string& getShortName() { return _shortName; }

 //! Specify the current number of refinements of this Params. (Not used)
 //!
 //! Pure virtual method required of render params
 //
 virtual void SetRefinementLevel(int) {}

 //! Return the current refinement level. (Not Used)
 //!
 //! Pure virtual method required of render params
 //
 virtual int GetRefinementLevel() { return 0; }

 //! Obtain the current compression level. (Not used)
 //!
 //! Pure virtual method required of render params
 //! \retval level index into the set of available compression ratios
 virtual int GetCompressionLevel() { return 0; }

 //! Set the current compression level. (Not Used)
 //!
 //! Pure virtual method required of render params
 //
 virtual void SetCompressionLevel(int) { return; }

 //! Reinitialize the object for a new dataset.
 //!
 //! Pure virtual method required of Params
 //!
 virtual bool reinit(bool override);

 //!
 //! Pure virtual method required of Params
 //!
 virtual bool usingVariable(const string&) { return false; }

 //!
 //! Pure virtual method required of Params
 //!
 virtual int getSessionVarNum() { return -1; }

 //!
 //! Pure virtual method required of Params
 //!
 virtual void setMinColorMapBound(float) {}

 //!
 //! Pure virtual method required of Params
 //!
 virtual void setMaxColorMapBound(float) {}

 //!
 //! Pure virtual method required of Params
 //!
 virtual void setMinOpacMapBound(float) {}

 //!
 //! Pure virtual method required of Params
 //!
 virtual void setMaxOpacMapBound(float) {}

 //!
 //! Pure virtual method required of Params
 //!
 virtual MapperFunction* getMapperFunc() { return NULL; }


 //! Following Get/Set methods are for 
 //! parameters used in defining an isosurface
 //! All of them are implemented in the XML
 //

 // Model File 
 virtual string GetModelFilename();
 void SetModelFilename(const string filename);

 // Scene File 
 virtual string GetSceneFilename();
 void SetSceneFilename(const string filename);

 // Transformation
 virtual Transform3d* GetTransformation();

 // Constant Color
 void SetConstantColor(const float rgb[3]);
 const float *GetConstantColor();

 //  Set the model preferences to default values. 
 static void setDefaultPrefs();

 // following required of 
 virtual bool IsOpaque() { return true; }

 bool modelIsDirty()       { return _modelDirty; }
 void setModelDirty()      { _modelDirty = true;}
 void clearDirtyModel()    { _modelDirty = false; }

 bool sceneIsDirty()       { return _modelDirty; }
 void setSceneDirty()      { _modelDirty = true;}
 void clearDirtyScene()    { _modelDirty = false; }

 bool transformationIsDirty()   { return _transformationDirty; }
 void setTransformationDirty()  { _transformationDirty = true; }
 void clearDirtyMatrix()        { _transformationDirty = false; }

 static const string _modelParamsTag;

protected:

 static const string _shortName;

 bool         _modelDirty;
 bool         _transformationDirty;

private:
 static const string _transformTag;
 static const string _sceneFilenameTag;
 static const string _modelFilenameTag;
 static const string _constantColorTag;
 float _constcolorbuf[4];
};
};
#endif // _ModelParams_h_
