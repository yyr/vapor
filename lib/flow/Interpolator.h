/*************************************************************************
*						OSU Flow Vector Field							 *
*																		 *
*																		 *
*	Created:	Han-Wei Shen, Liya Li									 *
*				The Ohio State University								 *
*	Date:		06/2005													 *
*																		 *
*	Interpolator														 *
*************************************************************************/

#ifndef _INTERPOLATOR_H_
#define _INTERPOLATOR_H_

#include "vapor/MyBase.h"
#include "vaporinternal/common.h"

namespace VAPoR
{
	// linear interpolation
	FLOW_API float Lerp(float x, float y, float ratio);

	// bilinear interpolation
	FLOW_API float BiLerp(float ll, float hl, float hh, float lh, float coeff[2]);

	// barycentric interpolation
	FLOW_API float BaryInterp(float dataValue[4], float coeff[3]);

	// trilinear interpolation
	FLOW_API float TriLerp(float lll, float hll, float lhl, float hhl, float llh, float hlh, float lhh, float hhh, float coeff[3]);
};

#endif