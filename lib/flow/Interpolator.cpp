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

#include "Interpolator.h"

using namespace VetsUtil;
using namespace VAPoR;

// barycentric interpolation
float VAPoR::BaryInterp(float dataValue[4], float coeff[3])
{
	return (dataValue[0] + 
			(dataValue[1]-dataValue[0])*coeff[0] +
			(dataValue[2]-dataValue[0])*coeff[1] +
			(dataValue[3]-dataValue[0])*coeff[2]);
}

// trilinear interpolation
float VAPoR::TriLerp(float lll, float hll, float lhl, float hhl,
					float llh, float hlh, float lhh, float hhh,
					float coeff[3])
{
	float temp1, temp2;
	float factor[2];

	factor[0] = coeff[0]; 
	factor[1] = coeff[1];

	temp1 = BiLerp(lll, hll, lhl, hhl, factor);
	temp2 = BiLerp(llh, hlh, lhh, hhh, factor);
	return (Lerp(temp1, temp2, coeff[2]));
}

// bilinear interpolation
float VAPoR::BiLerp(float ll, float hl, float lh, float hh, float coeff[2])
{
	return (Lerp(Lerp(ll, hl, coeff[0]), Lerp(lh, hh, coeff[0]), coeff[1]));
}

// linear interpolation
float VAPoR::Lerp(float x, float y, float ratio)
{
	return (x * (1 - ratio) + y * ratio);
}