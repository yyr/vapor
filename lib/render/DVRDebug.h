//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		DVRDebug.h
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Description:	Definition of the DVRDebug class
//		Modified by A. Norton to support a quick implementation of
//		volumizer renderer
//
#ifndef	_debug_driver_h_
#define	_debug_driver_h_

#include <cstdio>
#include <vapor/RegularGrid.h>
#include "DVRBase.h"
namespace VAPoR{

class	RENDER_API DVRDebug : public DVRBase {
public:


 DVRDebug(
	int *argc,
	char **argv,
	int	nthreads
 );

 virtual ~DVRDebug();

 virtual int	GraphicsInit();

 int SetRegion(const RegularGrid *rg, const float range[2], int num = 0);


 virtual int	Render();

 static void	PrintOptions(FILE *fp);

 virtual void	SetCLUT(const float ctab[256][4]);

 virtual void	SetOLUT(const float ftab[256][4], const int numRefinements);

 virtual int	HasAmbient() const;

 virtual void	SetAmbient(float r, float g, float b);

 virtual int	HasDiffuse() const;

 virtual void	SetDiffuse(float r, float g, float b);

 virtual int	HasSpecular() const;

 virtual void	SetSpecular(float r, float g, float b);

 virtual int	HasSperspective() const;

 virtual void	SetPerspectiveOnOff(int on);

private:

};
};
#endif	// _debug_driver_h_
