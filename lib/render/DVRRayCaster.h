//--DVRRayCaster.h -----------------------------------------------------
//
// $Id$
//
//
//----------------------------------------------------------------------------

#ifndef DVRRayCaster_h
#define DVRRayCaster_h

#include "DVRShader.h"

#include "Vect3d.h"

#include <vector>

namespace VAPoR {

  class BBox;
  class ShaderProgram;


class RENDER_API DVRRayCaster : public DVRShader
{

 public:


  DVRRayCaster(DataType_T type, int nthreads);
  virtual ~DVRRayCaster();

  virtual int GraphicsInit();
  
  virtual int Render(const float matrix[16]);

  virtual int HasType(DataType_T type);
  virtual int HasPreintegration() const { return false; };
  virtual int HasLighting() const { return true; };



  virtual void SetPreintegrationOnOff(int ) {return;}
  virtual void SetPreIntegrationTable(const float tab[256][4], const int nR) {return;}

  virtual void SetLightingOnOff(int on);

  static bool supported();

  // Set the isovalues to be displayed. values is an array of isovalues.
  // colors is an array of 4-tupples, each tupple containing r,g,b,a, 
  // normalized in the range [0..1.0]. The number of isovalues/colors 
  // is specified n.  The maximum value of n permitted may be queried with
  // GetMaxIsoValues(). Results are undefined for values of n greater than
  // GetMaxIsoValues()
  // 
  virtual void SetIsoValues(const float *values, const float *colors, int n);

  // return the maximum number of isovalues that may be set
  static int GetMaxIsoValues() {return (1); };

  virtual void Resize(int width, int height);


protected:

  enum ShaderType
  {
    DEFAULT = 0,
    LIGHT,
	BACKFACE
  };

  void initTextures();

  bool createShader(ShaderType,
                    const char *vertexCommandLine,
                    const char *vertexSource,
                    const char *fragCommandLine,
                    const char *fragmentSource);

  ShaderProgram* shader();


  void drawVolumeFaces(const BBox &box, const BBox &tbox);

  void render_backface(const BBox &box, const BBox &tbox); 

  void raycasting_pass(
	const TextureBrick *brick, const BBox &box, const BBox &tbox
  );

  virtual void renderBrick(
	const TextureBrick *brick,
	const Matrix3d &modelview,
	const Matrix3d &modelviewInverse
  );


  GLuint _framebufferid;
  GLuint _backface_bufferid;	// the FBO buffer

};

};


#endif 
