//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		DVRBase.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Description:	Definition of the DVRBase class
//		Modified by A. Norton to support a quick implementation of
//		volumizer renderer
//
#ifndef	dvrbase_h
#define	dvrbase_h

#include <stdio.h>
#include "vapor/MyBase.h"
#include "renderer.h"
using namespace VetsUtil;
namespace VAPoR {
class	DVRBase : public MyBase {
public:

 // Enumerated type representing type of data elements (e.g. byte, short)
 //
 enum DataType_T {
	UINT8,
	UINT16,
	FLOAT32
 };


 // Construct driver. Destructively parse any options passed by 'argv' and 
 // 'argc' and recogized by this driver.
 //
 // The 'type' specifies the representation type of the data volume elements
 // to be rendered by the driver.
 //
 //DVRBase(
//	int *argc,
//	char **argv,
//	DataType_T type,
 //);

 DVRBase();

 // Called once after OpenGL has been initialied so that the driver can
 // initialize the graphics state. The results of calling GraphicsInit
 // more than once are undefined. The results of calling non-const
 // memebers before calling GraphicsInit are similarly undefined.
 //
 virtual int	GraphicsInit() = 0;


 // Specify the region of data to render. The type of each data
 // element (voxel) is as specified by the constructor.
 // The 'n?' parameters
 // specify the dimension of the volume in voxels. The 'data_roi' array
 // specifies the extents of a subregion of interest contained within
 // the volume. The units are specified in voxels. To specify the
 // entire volume, the contents of data_roi should be set to
 // {0,0,0,nx-1,ny-1,nz-1}.
 // The 'extents' arrays specifies the geometic extents of the volume in
 // world coordinates. The volumetric extents of the data_roi, specified
 // in integer coordinates, will be mapped (translated and scaled) to the 
 // world coordinates
 // specified by 'extents'
 //
 virtual int SetRegion(
	void *data,
	int nx, int ny, int nz,
	const int data_roi[6],
	const float extents[6]
 ) = 0;

	
 // This version of the SetRegion method permits the non-uniform 
 // spacing of voxels. Three coordinate arrays, xcoords, ycoords, and
 // zcoords, specify the X,Y,Z coordinates of the voxels making up
 // the subregion of interest. The coordinate arrays are specied in 
 // world coordinates. The dimension of each coordinate array is determined
 // by the dimension of the region of interest indicated by the data_roi
 // parameter. For example, the dimension of the xcoord array is given
 // by: 
 //		data_roi[3] - data_roi[0] + 1
 //
 // Furthermore, the first element of {x,y,z}coord gives the world 
 // coordinate of first voxel in the subregion, and the last element 
 // of {x,y,z}coord gives the
 // world coordinate of the last voxel in the subregion
 //
 // Note: not all drivers are required to support Stretched Grids. 
 // The HasStretchedGrid() method can be used to determine if support is
 // available or not.
 //
 virtual int SetRegionStretched(
	void *,
	int , int , int ,
	const int [6],
	const float *,
	const float *,
	const float *
 ) {SetErrMsg("Driver does not support Stretched Grids"); return(-1); }

 // Returns true if the driver supports "stretched" Cartesian grids
 //
 virtual int	HasStretchedGrid() const { return (0); };



 // Render the volume after applying the OpenGL transfomation matrix, 
 // 'matrix'
 //
 virtual int	Render(
	const float	matrix[16]
 ) = 0;

 // Return's true if the specified data type, 'type' is supported by the
 // driver.
 //
 virtual int	HasType(DataType_T ) {return (0); }

 // Prints a description of the options supported by the driver's constructor
 // to the file pointer indicated by 'fp'.
 //
 static void	PrintOptions(FILE *) {return; }

 // Sets the color lookup table. 
 // 
 virtual void	SetCLUT(const float ctab[256][4]) = 0;

 // Sets the alpha lookup table. 
 // 
 virtual void	SetOLUT(const float ftab[256][4]) = 0;

 // Returns true if the driver supports a lighting model
 //
 virtual int	HasLighting() const { return (0); };

 // Turn gradient-based lighting on/off
 //
 virtual void	SetLightingOnOff(int ) { return; } ;

 // Returns true if the driver supports ambient lighting
 //
 virtual int	HasAmbient() const { return (0); };

 // Set ambient lighting coefficients. Each coefficient is in the 
 // range [0.0..1.0]
 //
 virtual void	SetAmbient(float , float , float ) { return; };

 // Returns true if the driver supports diffuse lighting
 //
 virtual int	HasDiffuse() const { return (0); };

 // Set diffuse lighting coefficients. Each coefficient is in the 
 // range [0.0..1.0]
 //
 virtual void	SetDiffuse(float , float , float ) { return; };

 // Returns true if the driver supports specular lighting
 //
 virtual int	HasSpecular() const { return (0); };

 // Set specular lighting coefficients. Each coefficient is in the 
 // range [0.0..1.0]
 //
 virtual void	SetSpecular(float , float , float ) { return; };

 // Return true if the driver supports perspective rendering
 //
 virtual int	HasSperspective() const { return(0); };

 // Enable/disable perspective rendering
 //
 virtual void	SetPerspectiveOnOff(int )  { return; };

private:

};
};

#endif	// dvrbase_h
