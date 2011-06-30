//
// $Id$
//
//
//----------------------------------------------------------------------------

#ifndef DVRRayCaster2Var_h
#define DVRRayCaster2Var_h

#include "DVRRayCaster.h"

#include "Vect3d.h"

#include <vector>
#include <QString>
namespace VAPoR {

  class BBox;
  class ShaderProgram;
  class Renderer;


class RENDER_API DVRRayCaster2Var : public DVRRayCaster
{
 private: 
  const static int MAX_ISO_VALUES = 1;

 public:


  DVRRayCaster2Var(GLint internalFormat, GLenum format, GLenum type, int nthreads, Renderer* ren);
  virtual ~DVRRayCaster2Var();

  virtual int GraphicsInit();
  
  static bool supported();

  // Set the isovalues to be displayed. values is an array of isovalues.
  // colors is an array of 4-tupples, each tupple containing r,g,b,a, 
  // normalized in the range [0..1.0]. The number of isovalues/colors 
  // is specified n.  The maximum value of n permitted may be queried with
  // GetMaxIsoValues(). Results are undefined for values of n greater than
  // GetMaxIsoValues()
  // 
  virtual void SetIsoValues(const float *values, const float *colors, int n);

protected:

  virtual void initShaderVariables();
  virtual bool createShader(ShaderType,
                    const char *vertexCommandLine,
                    const char *vertexSource,
                    const char *fragCommandLine,
                    const char *fragmentSource);

  virtual void raycasting_pass(
	const TextureBrick *brick, 
	const Matrix3d &modelview, const Matrix3d &modelviewInverse
  );
private:	
	 QString getCurrentEffect();
	
	 bool setShaderTextures();
};

};


#endif 
