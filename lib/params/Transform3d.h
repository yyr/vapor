//
//      $Id$
//
//	File:		Transform3d.h
//
//	Author:		Kenny Gruchalla
//			National Renewable Energy Laboratory
//
//	Description:	Class to read and write 3d tranformations 
//
//

#ifndef _Transform3d_h_
#define _Transform3d_h_

#include <QMetaType>
#include <QStringList>
#include <QString>
#include <vapor/ExpatParseMgr.h>
#include <vector>
#include "vapor/ParamsBase.h"
#include "params.h"

#define DEGREES_TO_RADIANS 0.0174532925199

namespace VAPoR
{

//----------------------------------------------------------------------------
//
//----------------------------------------------------------------------------
class PARAMS_API Transform3d : public ParamsBase
{

 public:

   Transform3d();
   Transform3d(const Transform3d &transform3d);

   static ParamsBase* CreateDefaultInstance() {return new Transform3d();}

   virtual ParamsBase* deepCopy(ParamNode* newRoot = 0);

   virtual ~Transform3d();   

   class TransformBase;

   const std::vector<TransformBase*>& transformations() const { return _transformations; }

   void clear();

   bool set(const QStringList &strings);
   QStringList get();

   ParamNode* buildNode();

   static string xmlTag() { return _tag; }
   
   virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                    const char **attribs);
   virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
   
 protected:

   std::vector<TransformBase*> _transformations;

   static const string _tag;

 public:

   //----------------------------------------------------------------------------
   //
   //----------------------------------------------------------------------------
   class PARAMS_API TransformBase : public ParsedXml
   {
    public:
      virtual TransformBase* clone() = 0;
      virtual ParamNode* buildNode() = 0;
      virtual QString label() = 0;
      virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, const char**) = 0;
      virtual bool elementEndHandler(ExpatParseMgr*, int, std::string&) = 0;
   };
   
   //----------------------------------------------------------------------------
   //
   //----------------------------------------------------------------------------
   class PARAMS_API Rotate : public TransformBase
   {
    public:
      enum Axis
      {
         X=0,
         Y=1,
         Z=2
      };
      
      Rotate(double x=1, double y=0, double z=0, double degrees=0);

      virtual ~Rotate() {}
      
      virtual TransformBase* clone();

      virtual QString label() { return QString("Rotate %1 %2 %3 %4").arg(axisx()).arg(axisy()).arg(axisz()).arg(degrees()); }

      double axisx() const   { return _rotation[0]; }
      void   axisx(double x) { _rotation[0] = x; }

      double axisy() const   { return _rotation[1]; }
      void   axisy(double y) { _rotation[1] = y; }

      double axisz() const   { return _rotation[2]; }
      void   axisz(double z) { _rotation[2] = z; }

      void degrees(float deg) { _rotation[3] = deg; } 
      float degrees() const   { return _rotation[3]; }
      
      void radians(float rad) { _rotation[3] = rad / DEGREES_TO_RADIANS; }
      float rad() const       { return _rotation[3] * DEGREES_TO_RADIANS; }
      
      ParamNode* buildNode();
      
      static string xmlTag() { return _tag; }
      
      virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                       const char **attribs);
      virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
      
   protected:
      
      std::vector<double> _rotation;
      
      static const string _tag;
   };
   
   
   //----------------------------------------------------------------------------
   //
   //----------------------------------------------------------------------------
   class PARAMS_API Translate : public TransformBase
   {
    public:
      Translate(double x = 0.0, double y = 0.0, double z = 0.0);
      
      virtual ~Translate() {}
      
      virtual TransformBase* clone();

      virtual QString label() { return QString("Translate %1 %2 %3").arg(x()).arg(y()).arg(z()); }

      void  x(double xp) { _translation[0] = xp; }
      double x() const   { return _translation[0]; }
      
      void  y(double yp) { _translation[1] = yp; }
      double y() const   { return _translation[1]; }
      
      void  z(double zp) { _translation[2] = zp; }
      double z() const   { return _translation[2]; }
      
      ParamNode* buildNode();
      
      static string xmlTag() { return _tag; }
      
      virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                       const char **attribs);
      virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
      
   protected:
      
      std::vector<double> _translation;
      
      static const string _tag;
   };
   
   
   //----------------------------------------------------------------------------
   //
   //----------------------------------------------------------------------------
   class PARAMS_API Scale : public TransformBase
   {
   public:
      Scale(double x = 1.0, double y = 1.0, double z = 1.0);
      
      virtual ~Scale() {}
      
      virtual TransformBase* clone();

      virtual QString label() { return QString("Scale %1 %2 %3").arg(x()).arg(y()).arg(z()); }

      void  x(double xp) { _scale[0] = xp; }
      double x() const   { return _scale[0]; }
      
      void  y(double yp) { _scale[1] = yp; }
      double y() const   { return _scale[1]; }
      
      void  z(double zp) { _scale[2] = zp; }
      double z() const   { return _scale[2]; }
      
      ParamNode* buildNode();
      
      static string xmlTag() { return _tag; }
      
      virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                       const char **attribs);
      virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
      
   protected:

      std::vector<double> _scale;
      
      static const string _tag;
   };
   
   
   //----------------------------------------------------------------------------
   //
   //----------------------------------------------------------------------------
   class PARAMS_API Matrix : public TransformBase
   {
   public:
      Matrix();
      Matrix(const std::vector<double> &m);
      Matrix(const Matrix &matrix);
      
      virtual ~Matrix() {}
      
      virtual TransformBase* clone();

      virtual QString label() { return QString("Matrix"); }

      void matrix(const std::vector<double> &m) { _matrix = m;    }
      const std::vector<double>& matrix() const { return _matrix; }
      
      ParamNode* buildNode();
      
      static string xmlTag() { return _tag; }
      
      virtual bool elementStartHandler(ExpatParseMgr*, int, std::string&, 
                                       const char **attribs);
      virtual bool elementEndHandler(ExpatParseMgr*, int depth, std::string &tag);
      
     protected:
      
      std::vector<double> _matrix;
      
      static const string _tag; 
   };

};

};

Q_DECLARE_METATYPE( VAPoR::Transform3d::TransformBase* )

#endif
