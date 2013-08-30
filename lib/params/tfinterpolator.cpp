//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		tfinterpolator.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2004
//
//	Description:	Implements the TFInterpolator class:   
//		A class to interpolate transfer function values
//		Currently only supports linear interpolation
//
#include <cmath>
#include <cassert>
#include <vapor/tfinterpolator.h>
using namespace VAPoR;
	// Determine the interpolated value at intermediate value 0<=r<=1
	// where the value at left and right endpoint is known
	// This method is just a stand-in until we get more sophistication
	//
float TFInterpolator::interpolate(TFInterpolator::type t, float leftVal, float rightVal, float r){
	if (t == TFInterpolator::discrete){
		if (r < 0.5) return leftVal;
		else return rightVal;
	}
	float val = (float)(leftVal*(1.-r) + r*rightVal);
	//if (val < 0.f || val > 1.f){
		//assert(val <= 1.f && val >= 0.f);
	//}
	return val;
}
	//Linear interpolation for circular (hue) fcn.  values in [0,1).
	//If it's closer to go around 1, then do so
	//
float TFInterpolator::interpCirc(type t, float leftVal, float rightVal, float r){
	if (t == TFInterpolator::discrete){
		if (r < 0.5) return leftVal;
		else return rightVal;
	}
	if (fabs(rightVal - leftVal) <= 0.5f)
		return interpolate(t, leftVal, rightVal, r);
	//replace smaller by 1+smaller, interpolate, then fit back into interval
	//
	float interpVal;
	if (leftVal <= rightVal) {
		interpVal = interpolate(t, leftVal+1.f, rightVal, r);
	} else interpVal = interpolate(t, leftVal, rightVal+1.f, r);

	if(interpVal >= 1.f) interpVal -= 1.f;
	if (interpVal < 0.f || interpVal > 1.f){
		assert(interpVal <= 1.f && interpVal >= 0.f);
	}
	return interpVal;
}
