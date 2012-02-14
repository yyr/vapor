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
#include <vector>
#include <vapor/MyBase.h>
#include <vapor/RegularGrid.h>
#ifdef	DEAD
#include <vapor/SphericalGrid.h>
#endif
#include <vapor/errorcodes.h>

using namespace VetsUtil;
namespace VAPoR {
class	RENDER_API DVRBase : public MyBase {
public:


 // Construct driver. Destructively parse any options passed by 'argv' and 
 // 'argc' and recogized by this driver.
 //
 // The 'type' specifies the representation type of the data volume elements
 // to be rendered by the driver.
 //
 //DVRBase(
//	int *argc,
//	char **argv,
 //);

 DVRBase();
 //Providing a virtual destructor causes a crash when volumizer exits:
 virtual ~DVRBase() {}  

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
 // Modification by AN, 2/10/05:  'extents' is subvolume of the unit cube
 // (0,1)x(0,1)x(0,1).  Actual mapping to real coords is done in application.
 //
 virtual int SetRegion(
	const RegularGrid *rg, const float range[2], int num = 0
 ) = 0;
	
#ifdef	DEAD

 // Specify the region of data to render with a grid permutation and
 // clipping information (currently only used for spherical rendering). 
 //
 // The type of each data element (voxel) is as specified by the constructor.
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
 // Modification by AN, 2/10/05:  'extents' is subvolume of the unit cube
 // (0,1)x(0,1)x(0,1).  Actual mapping to real coords is done in application.
 //
 virtual int SetRegionSpherical(const SphericalGrid *rg,
								const float range[2], int num,
                                const std::vector<long> &/*permutation*/,
                                const std::vector<bool> &/*clipping*/)
 { 
	SetErrMsg(VAPOR_ERROR_SPHERICAL,"Driver does not support Spherical Grids"); 
	return(-1); 
 }
#endif

 // Returns true if the driver supports "stretched" Cartesian grids
 //
 virtual int	HasStretchedGrid() const { return (0); };



 // Render the volume 
 //
 virtual int	Render() = 0;

 // Prints a description of the options supported by the driver's constructor
 // to the file pointer indicated by 'fp'.
 //
 static void	PrintOptions(FILE *) {return; }

 // Sets the color lookup table. 
 // 
 virtual void	SetCLUT(const float ctab[256][4]) = 0;

 // Sets the alpha lookup table, performing opacity correction 
 // 
 virtual void	SetOLUT(const float ftab[256][4], const int numRefinements) = 0;

 // Sets the view direction
 //
 virtual void SetView(const float* /*pos*/, const float* /*dir*/) {}

 // Sets the preintegration table
 //
 virtual void SetPreIntegrationTable(const float /*tab*/[256][4], const int /*nR*/) {}


 // Returns true if the driver supports a lighting model
 //
 virtual int	HasLighting() const { return (0); };

 // Returns true if the driver supports pre-integrated transfer functions
 //
 virtual int	HasPreintegration() const { return (0); };

 // Turn pre-integrated volume rendering on/off
 //
 virtual void	SetPreintegrationOnOff(int ) { return; } ;

 // Turn gradient-based lighting on/off
 //
 virtual void	SetLightingOnOff(int ) { return; } ;

 // Set the lighting coefficents
 //
 virtual void   SetLightingCoeff(float /*kdiff*/, float /*kamb*/, float /*kspec*/, float /*exponentS*/) {};

 // Set the light position
 //
 virtual void   SetLightingLocation(const float * /*position vector*/) {};

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

 // Get/Set rendering to "fast" mode
 // 
 virtual bool   GetRenderFast() const { return false; }
 virtual void   SetRenderFast(bool)   { return; }

 // Notifiy the driver that the window size has changed.
 virtual void Resize(int /*width*/, int /*height*/) {return;};

 // Set maximum
 virtual void SetMaxTexture(int texsize) {_max_texture = texsize;};

 virtual void calculateSampling() {}

 virtual void SetQuantizationRange(const float range[2]) {
	_range[0] = range[0]; _range[1] = range[1];
 }


protected:
  int _max_texture;
  float _range[2];

private:

};
};

#endif	// dvrbase_h
