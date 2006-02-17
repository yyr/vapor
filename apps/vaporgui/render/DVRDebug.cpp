//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		DVRDebug.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Description:	Implementation of the DVRDebug class
//		Modified by A. Norton to support a quick implementation of
//		volumizer renderer
//
#include <stdio.h>
#include "DVRDebug.h" 
using namespace VAPoR;

DVRDebug::DVRDebug(
	int *argc,
	char **argv,
	DataType_T type,
	int	nthreads
) {
	int	i;

	fprintf(stdout, "\nDVRDebug::DVRDebug() called\n");

	for(i=0; i<*argc; i++) {
		fprintf(stdout, "argv[%d] = %s\n", i, argv[i]);
	}
	fprintf(stdout, "\ttype = %d\n", type);
	fprintf(stdout, "\tnthreads = %d\n", nthreads);
}

DVRDebug::~DVRDebug() {

	fprintf(stdout, "\nDVRDebug::~DVRDebug() called\n");
}

 
int	DVRDebug::GraphicsInit(
) {
	fprintf(stdout, "\nDVRDebug::GraphicsInit() called\n");

	return(0);
}

int DVRDebug::SetRegion(
    void *data,
    int nx, int ny, int nz,
    const int data_roi[6],
    const float extents[6],
    const int data_box[6],
    int level
) {
	fprintf(stdout, "\nDVRDebug::SetRegion() called\n");

	fprintf(stdout, "\tnx = %d\n", nx);
	fprintf(stdout, "\tny = %d\n", ny);
	fprintf(stdout, "\tnz = %d\n", nz);
	fprintf(stdout, "\tx0 = %d\n", data_roi[0]);
	fprintf(stdout, "\ty0 = %d\n", data_roi[1]);
	fprintf(stdout, "\tz0 = %d\n", data_roi[2]);
	fprintf(stdout, "\tx1 = %d\n", data_roi[3]);
	fprintf(stdout, "\ty1 = %d\n", data_roi[4]);
	fprintf(stdout, "\tz1 = %d\n", data_roi[5]);

	fprintf(stdout, "\tex0 = %f\n", extents[0]);
	fprintf(stdout, "\tey0 = %f\n", extents[1]);
	fprintf(stdout, "\tez0 = %f\n", extents[2]);
	fprintf(stdout, "\tex1 = %f\n", extents[3]);
	fprintf(stdout, "\tey1 = %f\n", extents[4]);
	fprintf(stdout, "\tez1 = %f\n", extents[5]);

	fprintf(stdout, "\tbox0 = %d\n", data_box[0]);
	fprintf(stdout, "\tbox0 = %d\n", data_box[1]);
	fprintf(stdout, "\tbox0 = %d\n", data_box[2]);
	fprintf(stdout, "\tbox1 = %d\n", data_box[3]);
	fprintf(stdout, "\tbox1 = %d\n", data_box[4]);
	fprintf(stdout, "\tbox1 = %d\n", data_box[5]);

	fprintf(stdout, "\tlevel = %d\n", level);

	return(0);
}

int DVRDebug::SetRegionStretched(
    void *data,
    int nx, int ny, int nz,
    const int data_roi[6],
    const float *xcoords,
    const float *ycoords,
    const float *zcoords
) {
	int	i;

	fprintf(stdout, "\nDVRDebug::SetRegion() called\n");

	fprintf(stdout, "\tnx = %d\n", nx);
	fprintf(stdout, "\tny = %d\n", ny);
	fprintf(stdout, "\tnz = %d\n", nz);
	fprintf(stdout, "\tx0 = %d\n", data_roi[0]);
	fprintf(stdout, "\ty0 = %d\n", data_roi[1]);
	fprintf(stdout, "\tz0 = %d\n", data_roi[2]);
	fprintf(stdout, "\tx1 = %d\n", data_roi[3]);
	fprintf(stdout, "\ty1 = %d\n", data_roi[4]);
	fprintf(stdout, "\tz1 = %d\n", data_roi[5]);

	
	fprintf(stdout, "\txcoords:\n\t\t");
	for(i=data_roi[0];i<=data_roi[3]; i++) {
		fprintf(stdout, "%f ", xcoords[i]);
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "\tycoords:\n\t\t");
	for(i=data_roi[1];i<=data_roi[4]; i++) {
		fprintf(stdout, "%f ", ycoords[i]);
	}
	fprintf(stdout, "\n");

	fprintf(stdout, "\tzcoords:\n\t\t");
	for(i=data_roi[2];i<=data_roi[5]; i++) {
		fprintf(stdout, "%f ", zcoords[i]);
	}
	fprintf(stdout, "\n");

	return(0);
}

int	DVRDebug::Render(
	const float	matrix[16]
) {
	int	i;

	fprintf(stdout, "\nDVRDebug::Render() called\n");
	for(i=0; i<16; i++) {
		fprintf(stdout, "matrix[%d] = %f\n", i, matrix[i]);
	}

	return(0);
}

int	DVRDebug::HasType(DataType_T type) {

	fprintf(stdout, "\nDVRDebug::HasType() called\n");

	fprintf(stdout, "\ttype = %d\n", type);
	return(1);
}


void	DVRDebug::PrintOptions(FILE *fp) {

	fprintf(stdout, "\nDVRDebug::PrintOptions() called\n");
}

void	DVRDebug::SetCLUT(
	const float ctab[256][4]
) {
	int	i;

	fprintf(stdout, "\nDVRDebug::SetCLUT() called\n");

	for(i=0; i<256; i++) {
		fprintf(
			stdout, "\tctab[%d][0..3] = %f, %f, %f, %f\n", 
			i, ctab[i][0], ctab[i][1], ctab[i][2], ctab[i][3]
		);
	}
}

void	DVRDebug::SetOLUT(
	const float atab[256][4],
	const int numRefinements
) {
	int	i;

	fprintf(stdout, "\nDVRDebug::SetOLUT() called with %d refinements\n",numRefinements);

	for(i=0; i<256; i++) {
		fprintf(
			stdout, "\tatab[%d][0..3] = %f, %f, %f, %f\n", 
			i, atab[i][0], atab[i][1], atab[i][2], atab[i][3]
		);
	}
}

int	DVRDebug::HasAmbient() const
{
	fprintf(stdout, "\nDVRDebug::HasAmbient() called\n");
	return(1);
}

void	DVRDebug::SetAmbient(float r, float g, float b)
{
	fprintf(stdout, "\nDVRDebug::SetAmbient() called\n");

	fprintf(stdout, "\tr = %f\n", r);
	fprintf(stdout, "\tg = %f\n", g);
	fprintf(stdout, "\tb = %f\n", b);
}

int	DVRDebug::HasDiffuse() const
{
	fprintf(stdout, "\nDVRDebug::HasDiffuse() called\n");
	return(1);
}

void	DVRDebug::SetDiffuse(float r, float g, float b)
{
	fprintf(stdout, "\nDVRDebug::SetDiffuse() called\n");

	fprintf(stdout, "\tr = %f\n", r);
	fprintf(stdout, "\tg = %f\n", g);
	fprintf(stdout, "\tb = %f\n", b);
}

int	DVRDebug::HasSpecular() const
{
	fprintf(stdout, "\nDVRDebug::HasSpecular() called\n");
	return(1);
}

void	DVRDebug::SetSpecular(float r, float g, float b)
{
	fprintf(stdout, "\nDVRDebug::SetSpecular() called\n");

	fprintf(stdout, "\tr = %f\n", r);
	fprintf(stdout, "\tg = %f\n", g);
	fprintf(stdout, "\tb = %f\n", b);
}

int	DVRDebug::HasSperspective()  const
{
	fprintf(stdout, "\nDVRDebug::HasSperspective() called\n");
	return(1);
}

void	DVRDebug::SetPerspectiveOnOff(int on) {
	fprintf(stdout, "\nDVRDebug::SetPerspectiveOnOff() called\n");

	fprintf(stdout, "\ton = %d\n", on);
}
