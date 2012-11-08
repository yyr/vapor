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
#include <cstdio>
#include <iostream>
#include "DVRDebug.h" 
using namespace VAPoR;

DVRDebug::DVRDebug(
	int *argc,
	char **argv,
	int	nthreads
) {
	int	i;
	fprintf(stdout, "\nDVRDebug::DVRDebug() called\n");

	for(i=0; i<*argc; i++) {
		fprintf(stdout, "argv[%d] = %s\n", i, argv[i]);
	}
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
	const RegularGrid *rg,
	const float range[2],
	int num
) {

	std::cout << *rg;
	std::cout << "quantization range : " << range[0] << " " << range[1] << std::endl;
	std::cout << "region # : " << num << std::endl;


	return(0);
}


int	DVRDebug::Render() {
	fprintf(stdout, "\nDVRDebug::Render() called\n");

	return(0);
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
