//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		DVRVolumizer
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		October 2004
//
//	Description:	Definition of the DVRVolumizer class
//		Derived from code by John Clyne
//		Modified to support a volumizer renderer inside a QT window
//
#ifndef	_volumizer_driver_h_
#define	_volumizer_driver_h_
#ifdef VOLUMIZER
#include <stdio.h>
#include <stdlib.h>
//#include <GL/gl.h> //removed so QT can include it
#include <Volumizer2/Version.h>
#include <Volumizer2/Shape.h>
#include <Volumizer2/Block.h>
#include <Volumizer2/Appearance.h>
#include <Volumizer2/ParameterVolumeTexture.h>
#include <Volumizer2/ParameterVec3f.h>
#include <Volumizer2/TMRenderAction.h>
#include <Volumizer2/TMLUTShader.h>
#include <Volumizer2/TMGradientShader.h>
#include <Volumizer2/ParameterLookupTable.h>

#include "DVRBase.h"
namespace VAPoR {
class	DVRVolumizer : public DVRBase {
public:


 DVRVolumizer(
	int *argc,
	char **argv,
	DataType_T type,
	int	nthreads
 );

 virtual ~DVRVolumizer();

 virtual int	GraphicsInit();

 int SetRegion(
	void *data, 
	int nx, int ny, int nz, 
	const int data_roi[6],
	const float extents[6]
 );

 virtual int	Render(
	const float	matrix[16]
 );

 virtual int	HasType(DataType_T type);

 static void	PrintOptions(FILE *fp);

 virtual void	SetCLUT(const float ctab[256][4]);

 virtual void	SetOLUT(const float ftab[256][4], const int numRefinenements);

 virtual int	HasLighting() const { return (1); };

 virtual void	SetLightingOnOff(int on);

 virtual int	HasAmbient() const { return (1); };

 virtual void	SetAmbient(float r, float g, float b);

 virtual int	HasDiffuse() const { return (1); };

 virtual void	SetDiffuse(float r, float g, float b);

 virtual int	HasSpecular() const { return (0); };

 virtual void	SetSpecular(float r, float g, float b);

 virtual int	HasPreclassify() const { return(0); };

 virtual int	HasPerspective() const { return(0); };

 virtual void	SetPerspectiveOnOff(int on);

private:
 int	init_c;		// true if constructor succeeded
 int	is_rendering_c;	// true if Render() method has been called
 int	data_dim_c[3];	// Volume dimensions in voxels
 int	data_roi_c[6];	// Volume subregion coords in voxels
 double	geom_roi_c[6];	// Volume subregion coords in world coords
 float	lut_c[256][4];	// color and opacity lookup table
 float	llut_c[256][4];	// color and opacity lookup table for gradient shading
 float	matrix_c[16];
 unsigned char *grad_data_c;	// gradient data
 unsigned char *data_c;	// shallow copy of scalar data;
 int	grad_data_size_c;	// space allocated to grad_data_c
 int	do_lighting_c;	// true if render with gradiant-based lighting.
 int	nthreads_c;	// max # execution threads
 int	do_frag_prog_c;	// use a fragment program for shading
 

		// state flags
		//
 int	data_dirty_c;
 int	region_dirty_c;
 int	clut_dirty_c;
 int	olut_dirty_c;
 int	light_dirty_c;

 GLfloat	lightdir_c[4];		// lighting direction
 GLfloat	ambient_c[3];		// ambient lighting coefficient/component?
 GLfloat	diffuse_c[3];		// diffuse lighting coefficient/component?

 vzParameterVolumeTexture	*volume_c;

 vzShader	*shader_c;
 vzAppearance	*appearance_c;
 vzShape	*shape_c;
 vzGeometry	*geometry_c;

 vzParameterLookupTable	*table_c;
 vzTMRenderAction	*render_action_c;
 vzParameterVec3f	*lightdir_parm_c;
 vzParameterVec3f	*ambient_parm_c;
 vzParameterVec3f	*diffuse_parm_c;

 int	render();

 vzAppearance	*create_appearance();

 vzParameterVolumeTexture	*compute_gradient_volume(unsigned char *scalars); 

};
};
#endif //VOLUMIZER
#endif	// _volumizer_driver_h_
