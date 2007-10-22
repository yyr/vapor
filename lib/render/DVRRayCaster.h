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
 private: 
  const static int MAX_ISO_VALUES = 1;

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
  static int GetMaxIsoValues() {return (MAX_ISO_VALUES); };

  virtual void Resize(int width, int height);
  virtual void SetNearFar(GLfloat near, GLfloat far);



protected:

  enum ShaderType
  {
    DEFAULT = 0,
    LIGHT,
	BACKFACE
  };

  virtual int initTextures();
  virtual void initShaderVariables();

  virtual bool createShader(ShaderType,
                    const char *vertexCommandLine,
                    const char *vertexSource,
                    const char *fragCommandLine,
                    const char *fragmentSource);

  ShaderProgram* shader();


  virtual void drawVolumeFaces(const BBox &box, const BBox &tbox);

  virtual void render_backface(const BBox &box, const BBox &tbox); 

  virtual void raycasting_pass(
	const TextureBrick *brick, const BBox &box, const BBox &tbox
  );

  virtual void renderBrick(
	const TextureBrick *brick,
	const Matrix3d &modelview,
	const Matrix3d &modelviewInverse
  );


  GLuint _framebufferid;
  GLuint _backface_texcrd_texid;	// the FBO color buffer
  GLuint _backface_depth_texid;	// the FBO depth buffer

  GLfloat _nearClip;
  GLfloat _farClip;	// near and far clipping planes

  float _values[MAX_ISO_VALUES];
  float _colors[MAX_ISO_VALUES*4];
  int _nisos;

  GLint _colorInternalFormat;	// format/type of interal frame buffer object
  GLenum _colorInternalType;
  GLint _depthInternalFormat;
  GLenum _depthInternalType;


};

};


#endif 
