//
// $Id$
//
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <ctime>
#include <cassert>
#include <netcdf.h>
#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/WRF.h>
#include <vapor/WeightTable.h>	

#ifdef _WINDOWS 
#define _USE_MATH_DEFINES
#include <math.h>
#pragma warning(disable : 4996)
#endif
using namespace VAPoR;
using namespace VetsUtil;

WeightTable::WeightTable(
	const float *geo_lat, const float *geo_lon, 
	int ny, int nx,
	const float latexts[2], const float lonexts[2]
) {
	MOMBased = true;
	_nx = nx;
	_ny = ny;
	_lonLatExtents[0] = lonexts[0];
	_lonLatExtents[1] = latexts[0];
	_lonLatExtents[2] = lonexts[1];
	_lonLatExtents[3] = latexts[1];

	_geo_lat = new float [nx*ny];
	_geo_lon = new float [nx*ny];

	_alphas = new float[nx*ny];
	_betas = new float[nx*ny];
	_testValues = new float[nx*ny];
	_cornerLons = new int[nx*ny];
	_cornerLats = new int[nx*ny];

	for (int i = 0; i<nx*ny; i++){
		_testValues[i] = 1.e30f;
		_alphas[i] = - 1.f;
		_betas[i] = -1.f;
		_geo_lat[i] = geo_lat[i];
		_geo_lon[i] = geo_lon[i];
	}

	_deltaLat = (_lonLatExtents[3]-_lonLatExtents[1])/(ny-1);
	_deltaLon = (_lonLatExtents[2]-_lonLatExtents[0])/(nx-1);
	_wrapLon=false;

	//Check for global wrap in longitude:
	
	if (abs(_lonLatExtents[2] - 360.0 - _lonLatExtents[0])< 2.*_deltaLon) {
		_deltaLon = (_lonLatExtents[2]-_lonLatExtents[0])/nx;
		_wrapLon=true;
	}
	//Does the grid go to the north pole?
	if (_lonLatExtents[3] >= 90.-2.*_deltaLat) _wrapLat = true;
	else _wrapLat = false;
	
	// Calc epsilon for converging alpha,beta 
	_epsilon = Max(_deltaLat, _deltaLon)*1.e-7;
	// Calc epsilon for checking outside rectangle
	_epsRect = Max(_deltaLat,_deltaLon)*0.1;

	calcWeights();
}

WeightTable::~WeightTable() {
	if (_alphas) delete [] _alphas;
	if (_betas) delete [] _betas;
	if (_testValues) delete [] _testValues;
	if (_cornerLons) delete [] _cornerLons;
	if (_cornerLats) delete [] _cornerLats;
	if (_geo_lat) delete [] _geo_lat;
	if (_geo_lon) delete [] _geo_lon;
}


//Interpolation functions, can be called after the alphas and betas arrays have been calculated.
//Following can also be used on slices of 3D data
//If the corner latitude is at the top then
void WeightTable::interp2D(const float* sourceData, float* resultData, float srcMV, float dstMV, const size_t* dims){
	int corlon, corlat, corlatp, corlona, corlonb, corlonp;
	
	int latsize = dims[1];
	int lonsize = dims[0];
	float minin = sourceData[0];
	float maxin = sourceData[0];
	assert (latsize <= _ny && lonsize <= _nx);
	for (int i = 0; i< _ny*_nx; i++){
		if (sourceData[i] == srcMV) continue;
		if (sourceData[i]<minin) minin = sourceData[i];
		if (sourceData[i]>maxin) maxin = sourceData[i];
	}
	
	for (int j = 0; j<latsize; j++){
		for (int i = 0; i<lonsize; i++){
			if(_testValues[i+_nx*j] >= 1.)  {
				//Outside of range of mapping.  Provide missing value:
				resultData[i+j*lonsize] = dstMV;
				continue;
			}
			corlon = _cornerLons[i+_nx*j];//Lookup the user grid vertex lowerleft corner that contains (i,j) lon-lat vertex
			corlat = _cornerLats[i+_nx*j];//corlon and corlat are not lon and lat, just x and y 
			corlatp = corlat+1;
			corlona = corlon;
			corlonb = corlon+1;
			corlonp = corlon+1;
			if (MOMBased && corlon == _nx-1){ //Wrap-around longitude.  Note that longitude will not wrap at zipper.
				corlonp = 0;
				corlonb = 0;
			} else if (MOMBased && corlat == _ny-1){ //Get corners on opposite side of zipper:
				corlatp = corlat;
				corlona = _nx - corlon -1;
				corlonb = _nx - corlon -2;
			}
			float alpha = _alphas[i+_nx*j];
			float beta = _betas[i+_nx*j];
			float data0 = sourceData[corlon+_nx*corlat];
			float data1 = sourceData[corlonp+_nx*corlat];
			float data2 = sourceData[corlonb+_nx*corlatp];
			float data3 = sourceData[corlona+_nx*corlatp];
			
			float cf0 = (1.-alpha)*(1.-beta);
			float cf1 = alpha*(1.-beta);
			float cf2 = alpha*beta;
			float cf3 = (1.-alpha)*beta;
			float goodSum = 0.f;
			float mvCoef = 0.f;
			//Accumulate values and coefficients for missing and non-missing values
			//If less than half the weight comes from missing value, then apply weights to non-missing values,
			//otherwise just set result to missing value.
			if (data0 == srcMV) mvCoef += cf0;
			else {
				goodSum += cf0*data0;
			}
			if (data1 == srcMV) mvCoef += cf1;
			else {
				goodSum += cf1*data1;
			}
			if (data2 == srcMV) mvCoef += cf2;
			else {
				goodSum += cf2*data2;
			}
			if (data3 == srcMV) mvCoef += cf3;
			else {
				goodSum += cf3*data3;
			}
			if (mvCoef >= 0.5f) resultData[i+j*lonsize] = dstMV;
			else {
				resultData[i+j*lonsize] = goodSum/(1.-mvCoef);
			}
			
		}
		
	}
}

//For given latitude longitude grid vertex, used for testing.
//check all geolon/geolat quads.  See what ulat/ulon cell contains it.
float WeightTable::bestLatLon(int ilon, int ilat){
	
	
	//Loop over all the lon/lat grid vertices
	
	float testLon = _lonLatExtents[0]+ilon*_deltaLon;
	float testLat = _lonLatExtents[1]+ilat*_deltaLat;
	//Loop over all cells in grid
	float minval = 1.e30;
	int bestulon,bestulat;
	//Loop over the ulat/ulon grid.  For each ulon-ulat grid coord
	//See if ilon,ilat is in cell mapped from ulon,ulat
	for (int qlat = 0; qlat< _ny-1; qlat++){
		for (int plon = 0; plon< _nx; plon++){
			//Test if ilon,ilat coordinates are in quad of ulon,ulat
			float eps = testInQuad(testLon,testLat,plon, qlat);
			if (eps < minval) {
				minval = eps;
				bestulon = plon;
				bestulat = qlat;
			}
		}
	}
	return minval;
}

//check all ulon,ulat quads.  See if lat/lon vertex lies in one of them
float WeightTable::bestLatLonPolar(int ilon, int ilat){
	//Loop over all the lon/lat grid vertices
	
	float testLon = _lonLatExtents[0]+ilon*_deltaLon;
	float testLat = _lonLatExtents[1]+ilat*_deltaLat;
	
	//Loop over all cells in grid
	float minval = 1.e30;
	int bestulon, bestulat;
	float testRad, testThet;
	float bestx, besty;
	
	float bestLat, bestLon, bestThet, bestRad;
	//loop over ulon,ulat cells
	for (int qlat = 0; qlat< _ny; qlat++){
		if (qlat == _ny-1 && !_wrapLat) continue;
		
		for (int plon = 0; plon< _nx; plon++){
			if (qlat == _ny-1 && plon > _nx/2) continue;  // only do first half of zipper
			if (plon == _nx-1 && !_wrapLon) continue;
			
			//Convert lat/lon coordinates to x,y
			testRad = (90.-testLat)*2./M_PI;
			testThet = testLon*M_PI/180.;
			float testx = testRad*cos(testThet);
			float testy = testRad*sin(testThet);
			float eps = testInQuad2(testx,testy,plon, qlat);
			if (eps < minval){
				minval = eps;
				bestulon = plon;
				bestulat = qlat;
				bestLat = testLat;
				bestLon = testLon;
				bestThet = testThet;
				bestRad = testRad;
				bestx = testx;
				besty = testy;
			}
		}
	}
	return minval;
}

	
int WeightTable::calcWeights(){
	
	float eps = Max(_deltaLat,_deltaLon)*1.e-3;

	//Check out the geolat, geolon variables
	for (int i = 0; i<_nx*_ny; i++){
		assert (_geo_lat[i] >= -90. && _geo_lat[i] <= 90.);
		assert (_geo_lon[i] >= -360. && _geo_lon[i] <= 360.);
	}
	
	
	//	Loop over (_nx/_ny) user grid vertices.  These are x and y grid vertex indices
	//  Call them ulat and ulon to suggest "lat and lon" in user coordinates
	float lat[4],lon[4];
	for (int ulat = 0; ulat<_ny; ulat++){
		if (ulat == _ny-1 && !_wrapLat) continue;
		float beglat = _geo_lat[_nx*ulat];
		if(beglat < 0. ){ //below equator
						
			for (int ulon = 0; ulon<_nx; ulon++){
				
				if (ulon == _nx-1 && !_wrapLon) continue;
				if (ulon == _nx-1){
					lat[0] = _geo_lat[ulon+_nx*ulat];
					lat[1] = _geo_lat[_nx*ulat];
					lat[2] = _geo_lat[ulon+_nx*(ulat+1)];
					lat[3] = _geo_lat[_nx*(ulat+1)];
				} else {
					lat[0] = _geo_lat[ulon+_nx*ulat];
					lat[1] = _geo_lat[ulon+1+_nx*ulat];
					lat[2] = _geo_lat[ulon+_nx*(ulat+1)];
					lat[3] = _geo_lat[ulon+1+_nx*(ulat+1)];
				}
				float minlat = Min(Min(lat[0],lat[1]),Min(lat[0],lat[3])) - eps;
				float maxlat = Max(Max(lat[0],lat[1]),Min(lat[2],lat[3])) + eps;

				
				//	For each cell, find min and max longitude and latitude; expand by eps in all directions to define maximal ulon-ulat cell in
				// actual lon/lat coordinates
				float lon0,lon1,lon2,lon3;
				if (ulon == _nx-1){
					lon0 = _geo_lon[ulon+_nx*ulat];
					lon1 = _geo_lon[_nx*ulat];
					lon2 = _geo_lon[ulon+_nx*(ulat+1)];
					lon3 = _geo_lon[_nx*(ulat+1)];
					
				} else {
					lon0 = _geo_lon[ulon+_nx*ulat];
					lon1 = _geo_lon[ulon+1+_nx*ulat];
					lon2 = _geo_lon[ulon+_nx*(ulat+1)];
					lon3 = _geo_lon[ulon+1+_nx*(ulat+1)];
				}
							
				float minlon = Min(Min(lon0,lon1),Min(lon2,lon3))-eps;
				float maxlon = Max(Max(lon0,lon1),Max(lon2,lon3))+eps;

								
				// find all the latlon grid vertices that fit near this rectangle: 
				// Add 1 to the LL corner latitude because it is definitely below and left of the rectangle
				int latInd0 = (int)(1.+((minlat-_lonLatExtents[1])/_deltaLat));
				// Don't add 1 to the longitude corner because in some cases (with wrap) it would eliminate a valid longitude index.
				int lonInd0 = (int)(((minlon-_lonLatExtents[0])/_deltaLon));
				//cover the edges too!
				// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
				bool edgeFlag = false;
				if (ulat == 0 && latInd0 ==1) {latInd0 = 0; edgeFlag = true;}
				if (ulon == 0) {
					lonInd0 = 0; edgeFlag = true;
				}
			
				// Whereas the UR corner vertex may be inside:
				int latInd1 = (int)((maxlat-_lonLatExtents[1])/_deltaLat);
				int lonInd1 = (int)((maxlon-_lonLatExtents[0])/_deltaLon);
			
			
				if (latInd0 <0 || lonInd0 < 0) {
					continue;
				}
				if (latInd1 >= _ny || (lonInd1 > _nx)||(lonInd1 == _nx && !_wrapLon)){
					continue;
				}
						
				//Loop over all the lon/lat grid vertices in the maximized cell:
				for (int qlat = latInd0; qlat<= latInd1; qlat++){
					for (int plon = lonInd0; plon<= lonInd1; plon++){
						
						int plon1 = plon;
						if(plon == _nx) {
							assert(_wrapLon);
							plon1 = 0;
						}
						float testLon = _lonLatExtents[0]+plon1*_deltaLon;
						float testLat = _lonLatExtents[1]+qlat*_deltaLat;
						float eps = testInQuad(testLon,testLat,ulon, ulat);
						if (eps < _epsRect || edgeFlag){  //OK, point is inside, or close enough.
							//Check to make sure eps is smaller than previous entries:
							if (eps > _testValues[plon1+_nx*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							findWeight(testLon, testLat, ulon, ulat,&alph, &beta);
							assert(plon1 >= 0 && plon1 < _nx);
							assert(qlat >= 0 && qlat < _ny);
							_alphas[plon1+_nx*qlat] = alph;
							_betas[plon1+_nx*qlat] = beta;
							_testValues[plon1+_nx*qlat] = eps;
							_cornerLats[plon1+_nx*qlat] = ulat;
							_cornerLons[plon1+_nx*qlat] = ulon;
							
						}
					}
				}
			}
		} else { //provide mapping to polar coordinates of northern hemisphere
			//map to circle of circumference 360 (matching lon/lat projection above at equator)
			//There are two cases to consider:
			//All 4 points lie in a sector of angle less than 180 degrees, or not.
			//To determine if they lie in such a sector, we first check if the
			//difference between the largest and smallest longitude is less than 180 degrees.
			//If that is not the case, it is still possible that they lie in such a sector, when
			//the angles are considered modulo 360 degrees.  So, for each of the 4 points, test if it
			//is the smallest longitude L in such a sector by seeing if all the other 3 points have longitude
			//no more than 180 + L when an appropriate multiple of 360 is added.
			//If that does not work for any of the 4 points, then there is no such sector and the points
			//may circumscribe the north pole.  In that case the boolean "fullCircle" is set to true.
			//
			// In the case that the points do lie in a sector of less than 180 degrees, we also must
			//determine an inner and outer radius (latitude) to search for latlon points.
			//Begin by using the largest and smallest radius of the four points.
			//However, in order to be sure the sector contains all the points inside the rectangle of the
			//four points, the sector's inner radius must be multiplied by the factor cos(theta/2) where
			//theta is the sector's angle.  This factor ensures that the secant crossing the inner
			//radius will be included in the modified sector.
			for (int ulon = 0; ulon<_nx; ulon++){
				
				if (ulat == _ny-1 && ulon > _nx/2) continue;  // only do first half of zipper
				if (ulon == _nx-1 && !_wrapLon) continue;
				
				if (ulon == _nx-1){
					lat[0] = _geo_lat[ulon+_nx*ulat];
					lat[1] = _geo_lat[_nx*ulat];
					lat[2] = _geo_lat[ulon+_nx*(ulat+1)];
					lat[3] = _geo_lat[_nx*(ulat+1)];
					
					lon[0] = _geo_lon[ulon+_nx*ulat];
					lon[1] = _geo_lon[_nx*ulat];
					lon[2] = _geo_lon[ulon+_nx*(ulat+1)];
					lon[3] = _geo_lon[_nx*(ulat+1)];
				} else if (ulat == _ny-1) { //Handle zipper:
					// The point (ulat, ulon) is across from (ulat, _nx-ulon-1)
					// Whenever ulat+1 is used, replace it with ulat, and switch ulon to _nx-ulon-1
					lat[0] = _geo_lat[ulon+_nx*ulat];
					lat[1] = _geo_lat[ulon+1+_nx*ulat];
					lat[2] = _geo_lat[_nx-ulon-2+_nx*ulat];
					lat[3] = _geo_lat[_nx -ulon -1+_nx*ulat];
					lon[0] = _geo_lon[ulon+_nx*ulat];
					lon[1] = _geo_lon[ulon+1+_nx*ulat];
					lon[2] = _geo_lon[_nx-ulon-2+_nx*ulat];
					lon[3] = _geo_lon[_nx-ulon-1+_nx*ulat];
				} else {
					lat[0] = _geo_lat[ulon+_nx*ulat];
					lat[1] = _geo_lat[ulon+1+_nx*ulat];
					lat[2] = _geo_lat[ulon+_nx*(ulat+1)];
					lat[3] = _geo_lat[ulon+1+_nx*(ulat+1)];
					
					lon[0] = _geo_lon[ulon+_nx*ulat];
					lon[1] = _geo_lon[ulon+1+_nx*ulat];
					lon[2] = _geo_lon[ulon+_nx*(ulat+1)];
					lon[3] = _geo_lon[ulon+1+_nx*(ulat+1)];
				}
				
							
				float minlat = 1000., maxlat = -1000.;
				
				//Get min/max latitude
				for (int k=0;k<4;k++) {
					
					if (lat[k]<minlat) minlat = lat[k];
					if (lat[k]>maxlat) maxlat = lat[k];
				}
				
				//Determine a minimal sector (longitude interval)  that contains the 4 corners:
				float maxlon, minlon;
				
				bool fullCircle = false;
				minlon = Min(Min(lon[0],lon[1]),Min(lon[2],lon[3]));
				maxlon = Max(Max(lon[0],lon[1]),Max(lon[2],lon[3]));
				//If the sector width is too great, try other orderings mod 360, or set fullCircle = true.
				//the lon[] array is only used to find max and min.
				if (maxlon - minlon >= 180.) {
					//see if we can rearrange the points modulo 360 degrees to get them to lie in a 180 degree sector
					int firstpoint = -1;
					for (int k = 0; k< 4; k++){
						bool ok = true;
						for( int m = 0; m<4; m++){
							if (m == k) continue;
							if (lon[m]>= lon[k] && lon[m] < lon[k]+180.) continue;
							if (lon[m] <= lon[k] && (lon[m]+360.) < lon[k]+180.) continue;
							//Hmmm.  This point is more than 180 degrees above, when considered modulo 360
							ok = false;
							break;
						}
						if (ok){ firstpoint = k; break;}
					}
					if (firstpoint >= 0) { // we need to consider thet[firstpoint] as the starting angle in the sector
						minlon = lon[firstpoint];
						maxlon = minlon -1000.;
						for (int k = 0; k<4; k++){
							if (k==firstpoint) continue;
							if (lon[k] < lon[firstpoint]) lon[k]+=360.;
							assert(lon[k] >= lon[firstpoint] && lon[k]<(lon[firstpoint]+180.));
							if(lon[k]>maxlon) maxlon = lon[k];
						}
						
					} else fullCircle = true;
							
						
				}
				
				
				//Expand slightly to include points on boundary
				maxlon += eps;
				minlon -= eps;
				
				//If fullCircle is false the sector goes from minlat, minlon to maxlat, maxlon
				//If fullCircle is true, the sector is a full circle, from minrad= 0 to maxrad
				
				//Shrink the inner radius (largest latitude) to be sure to include secant lines between two vertices on max latitude:
				if (!fullCircle){
					float sectorAngle = (maxlon - minlon)*M_PI/180.;
					float shrinkFactor = cos(sectorAngle*0.5);
					maxlat = 90.- (90.-maxlat)*shrinkFactor;
				}
				minlat -= eps;
				maxlat += eps;

				
				// Test all lat/lon pairs that are in the sector, to see if they are inside the rectangle of (x,y) corners.
				// first, discretize the sector extents to match lat/lon grid
				int lonMin, lonMax;
				int latMin = (int)(1.+(minlat-_lonLatExtents[1])/_deltaLat);
				int latMax = (int)((maxlat-_lonLatExtents[1])/_deltaLat);
				if (fullCircle) {
					lonMin = 0; lonMax = _nx;
					latMax = _ny -1;
				}
				else {
					lonMin = (int)(1.+(minlon  -_lonLatExtents[0])/_deltaLon);
					lonMax = (int)((maxlon -_lonLatExtents[0])/_deltaLon);
				}
				if (lonMin < 0) {
					lonMin = 0;
				}
				
				//cover the edges too!
				// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
				bool edgeFlag = false;
				if (ulat == 0 && latMin ==1) {latMin = 0; edgeFlag = true;}
				if (ulon == 0 && lonMin == 1) {
					lonMin = 0; edgeFlag = true;
				}
				
				//slightly enlarge the bounds (??)
				if (latMin > 0) latMin--;
				if (lonMin > 0) lonMin--;
				if (latMax < _ny-1) latMax++;
				if (lonMax < _nx -1) lonMax++;
				if (latMax > _ny-1) latMax = _ny-1;
				
				//Test each point in lon/lat interval:
				for (int qlat = latMin; qlat<= latMax; qlat++){
					for (int plon = lonMin; plon<= lonMax; plon++){
						
						float testLon = _lonLatExtents[0]+plon*_deltaLon;
						int plon1 = plon;
						if(plon >= _nx) {
							assert(_wrapLon);
							plon1 = plon -_nx;
						}
						float testLat = _lonLatExtents[1]+qlat*_deltaLat;
						//Convert lat/lon coordinates to x,y
						float testRad = (90.-testLat)*2./M_PI;
						float testThet = testLon*M_PI/180.;
						float testx = testRad*cos(testThet);
						float testy = testRad*sin(testThet);
						float eps = testInQuad2(testx,testy,ulon, ulat);
						if (eps < _epsRect || edgeFlag){  //OK, point is inside, or close enough.
							assert(plon1 >= 0 && plon1 < _nx);
							assert(qlat >= 0 && qlat < _ny);

							//Check to make sure eps is smaller than previous entries:
							if (eps >= _testValues[plon1+_nx*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							
							findWeight2(testx, testy, ulon, ulat,&alph, &beta);
							
							_alphas[plon1+_nx*qlat] = alph;
							_betas[plon1+_nx*qlat] = beta;
							_testValues[plon1+_nx*qlat] = eps;
							_cornerLats[plon1+_nx*qlat] = ulat;
							_cornerLons[plon1+_nx*qlat] = ulon;
							
						}
					}
				}
				
			} //end ulon loop
		} //end else
	} //end ulat loop
	//Check it out:
#if 0
//#ifdef _DEBUG
	for (int i = 0; i<_ny; i++){
		for (int j = 0; j<_nx; j++){
			if (_testValues[j+_nx*i] <1.){
				if (_alphas[j+_nx*i] > 2.0 || _alphas[j+_nx*i] < -1.0 || _betas[j+_nx*i]>2.0 || _betas[j+_nx*i]< -1.0)
					fprintf(stderr," bad alpha %g beta %g at lon,lat %d , %d, ulon %d ulat %d, testval: %g\n",_alphas[j+_nx*i],_betas[j+_nx*i], j, i, _cornerLons[j+_nx*i], _cornerLats[j+_nx*i],_testValues[j+i*_nx]);
				continue;
			}
			else if (MOMBased) {
				if (_testValues[j+_nx*i] < 10000.) printf( "poor estimate: %f at lon, lat %d %d, ulon, ulat %d %d\n", _testValues[j+_nx*i],j,i, _cornerLons[j+_nx*i], _cornerLats[j+_nx*i]);
				else {
					printf(" Error remapping coordinates.  Unmapped lon, lat: %d, %d\n",j,i);
					float est = bestLatLonPolar(j,i);
					
				}
			}
		}
	}
#endif
	return 0;				
}
float WeightTable::testInQuad(float plon, float plat, int ilon, int ilat){
	//Sides of quad are determined by lines thru the 4 points at user grid corners:
	//(ilon,ilat), (ilon+1,ilat), (ilon+1,ilat+1), (ilon,ilat+1)
	//These points are mapped to 4 (lon,lat) coordinates (xi,yi) using geolon,geolat variables
	//For each pair P=(xi,yi), Q=(xi+1,yi+1) of these points, the formula
	//Fi(x,y) = (yi+1-yi)*(x-xi)-(y-yi)*(xi+1-xi) defines a function that is <0 on the in-side of the line
	//and >0 on the out-side of the line.
	//Return the maximum of the four formulas Fi evaluated at (plon,plat), or the first one that
	//exceeds epsilon
	float lg[4],lt[4];
	if (ilon == _nx-1){//wrap in ilon coord
		lt[0] = _geo_lat[ilon+_nx*ilat];
		lt[1] = _geo_lat[_nx*ilat];
		lt[2] = _geo_lat[_nx*(ilat+1)];
		lt[3] = _geo_lat[ilon+_nx*(ilat+1)];
		lg[0] = _geo_lon[ilon+_nx*ilat];
		lg[1] = _geo_lon[_nx*ilat];
		lg[2] = _geo_lon[_nx*(ilat+1)];
		lg[3] = _geo_lon[ilon+_nx*(ilat+1)];
	} else {
		lt[0] = _geo_lat[ilon+_nx*ilat];
		lt[1] = _geo_lat[ilon+1+_nx*ilat];
		lt[2] = _geo_lat[ilon+1+_nx*(ilat+1)];
		lt[3] = _geo_lat[ilon+_nx*(ilat+1)];
		lg[0] = _geo_lon[ilon+_nx*ilat];
		lg[1] = _geo_lon[ilon+1+_nx*ilat];
		lg[2] = _geo_lon[ilon+1+_nx*(ilat+1)];
		lg[3] = _geo_lon[ilon+_nx*(ilat+1)];
	}
	//Make sure we're not off by 360 degrees..
	for (int k = 0; k<4; k++){
		if (lg[k] > plon + 180.) lg[k] -= 360.;
	}
	float dist = -1.e30;
	for (int j = 0; j<4; j++){
		float dst = (lt[(j+1)%4]-lt[j])*(plon-lg[j]) - (plat-lt[j])*(lg[(j+1)%4] - lg[j]);
		if (dst > dist) dist = dst;

	}
	return dist;
}
float WeightTable::testInQuad2(float x, float y, int ulon, int ulat){
	//Sides of quad are determined by lines thru the 4 points at user grid corners
	//(ulon,ulat), (+1,ilat), (ulon+1,ulat+1), (ulon,ulat+1)
	//These points are mapped to 4 (lon,lat) coordinates  using geolon,geolat variables.
	//Then the 4 points are mapped into a circle of circumference 360. (radius 360/2pi) using polar coordinates
	// lon*180/M_PI is theta, (90-lat)*2/M_PI is radius
	// this results in 4 x,y corner coordinates
	float xc[4],yc[4];
	float rad[4],thet[4];
	if (ulon == _nx-1 ){  //wrap; ie. ulon+1 is replaced by 0
		rad[0] = (90.-_geo_lat[ulon+_nx*ulat])*2./M_PI;
		rad[1] = (90.-_geo_lat[_nx*ulat])*2./M_PI;
		rad[2] = (90.-_geo_lat[_nx*(ulat+1)])*2./M_PI;
		rad[3] = (90.-_geo_lat[ulon+_nx*(ulat+1)])*2./M_PI;
		thet[0] = _geo_lon[ulon+_nx*ulat]*M_PI/180.;
		thet[1] = _geo_lon[_nx*ulat]*M_PI/180.;
		thet[2] = _geo_lon[_nx*(ulat+1)]*M_PI/180.;
		thet[3] = _geo_lon[ulon+_nx*(ulat+1)]*M_PI/180.;
	} else if (ulat == _ny-1) { //Handle zipper:
		// The point (ulat, ulon) is across from (ulat, _nx-ulon-1)
		// Whenever ulat+1 is used, replace it with ulat, and switch ulon to _nx-ulon-1
		rad[0] = (90.-_geo_lat[ulon+_nx*ulat])*2./M_PI;
		rad[1] = (90.-_geo_lat[ulon+1+_nx*ulat])*2./M_PI;
		rad[2] = (90.-_geo_lat[_nx-ulon-2+_nx*ulat])*2./M_PI;
		rad[3] = (90.-_geo_lat[_nx -ulon -1+_nx*ulat])*2./M_PI;
		thet[0] = _geo_lon[ulon+_nx*ulat]*M_PI/180.;
		thet[1] = _geo_lon[ulon+1+_nx*ulat]*M_PI/180.;
		thet[2] = _geo_lon[_nx-ulon-2+_nx*ulat]*M_PI/180.;
		thet[3] = _geo_lon[_nx-ulon-1+_nx*ulat]*M_PI/180.;
	} else {
		rad[0] = (90.-_geo_lat[ulon+_nx*ulat])*2./M_PI;
		rad[1] = (90.-_geo_lat[ulon+1+_nx*ulat])*2./M_PI;
		rad[2] = (90.-_geo_lat[ulon+1+_nx*(ulat+1)])*2./M_PI;
		rad[3] = (90.-_geo_lat[ulon+_nx*(ulat+1)])*2./M_PI;
		thet[0] = _geo_lon[ulon+_nx*ulat]*M_PI/180.;
		thet[1] = _geo_lon[ulon+1+_nx*ulat]*M_PI/180.;
		thet[2] = _geo_lon[ulon+1+_nx*(ulat+1)]*M_PI/180.;
		thet[3] = _geo_lon[ulon+_nx*(ulat+1)]*M_PI/180.;
	}
	for (int i = 0; i<4; i++){
		xc[i] = rad[i]*cos(thet[i]);
		yc[i] = rad[i]*sin(thet[i]);
	}
	//
	//For each pair P=(xi,yi), Q=(xi+1,yi+1) of these points, the formula
	//Fi(x,y) = (yi+1-yi)*(x-xi)-(y-yi)*(xi+1-xi) defines a function that is <0 on the in-side of the line
	//and >0 on the out-side of the line.
	//Return the maximum of the four formulas Fi evaluated at (x,y), or the first one that
	//exceeds epsilon
	float fmax = -1.e30;
	for (int j = 0; j<4; j++){
		float dist = (yc[(j+1)%4]-yc[j])*(x-xc[j]) - (y-yc[j])*(xc[(j+1)%4] - xc[j]);
		//if (dist > epsRect) return dist;
		if (dist > fmax) fmax = dist;
	}
	return fmax;
}
		
void WeightTable::findWeight(float plon, float plat, int ilon, int ilat, float* alpha, float* beta){
	//use iteration to determine alpha and beta that interpolates plon, plat from the user grid
	//corner associated with ilon and ilat.
	//start with prevAlpha=0, prevBeta=0.  
	float th[4],ph[4];
	if (ilon == _nx-1){
		th[0] = _geo_lon[ilon+_nx*ilat];
		ph[0] = _geo_lat[ilon+_nx*ilat];
		th[1] = _geo_lon[_nx*ilat];
		ph[1] = _geo_lat[_nx*ilat];
		th[2] = _geo_lon[_nx*(ilat+1)];
		ph[2] = _geo_lat[_nx*(ilat+1)];
		th[3] = _geo_lon[ilon+_nx*(ilat+1)];
		ph[3] = _geo_lat[ilon+_nx*(ilat+1)];
		
	} else {
		th[0] = _geo_lon[ilon+_nx*ilat];
		ph[0] = _geo_lat[ilon+_nx*ilat];
		th[1] = _geo_lon[ilon+1+_nx*ilat];
		ph[1] = _geo_lat[ilon+1+_nx*ilat];
		th[2] = _geo_lon[ilon+1+_nx*(ilat+1)];
		ph[2] = _geo_lat[ilon+1+_nx*(ilat+1)];
		th[3] = _geo_lon[ilon+_nx*(ilat+1)];
		ph[3] = _geo_lat[ilon+_nx*(ilat+1)];
	}
	for (int k = 0; k<4; k++){
		if (th[k]> plon+180.) th[k] -= 360.;
	}
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	float errp = 100.;
	for (iter = 0; iter < 20; iter++){
		newalpha = NEWA(alph, bet, plon, plat, th, ph);
		newbeta = NEWB(alph, bet, plon, plat, th, ph);
		if (newalpha == 1.e30f || newbeta == 1.e30f){  //Degenerate rectangle; expand slightly
			for (int j = 0; j<4; j++){
				bool lowerth = true;
				bool lowerph = true;
				if (j == 1 || j == 2) lowerth = false;
				if (j > 1 ) lowerph = false;
				if (th[j] < 0.f) lowerth = !lowerth;
				if (ph[j] < 0.f) lowerph = !lowerph;
				if (lowerth) th[j]=0.99999f*th[j]; 
				else th[j] = 1.000001f*th[j];
				if (lowerph) ph[j]=0.99999f*ph[j]; 
				else ph[j] = 1.00001f*ph[j];
			}
			//Try again with expanded coordinates
			continue;
		}
		errp = errP(plon, plat, newalpha, newbeta,th,ph);
		if (errp < 1.e-6) break;
		alph = newalpha;
		bet = newbeta;
	}
	//if(iter >= 20 && errp > 1.f){
	//	fprintf(stderr," Weight nonconvergence, error value: %f , alpha, beta: %f %f\n",errp, newalpha, newbeta);
	//}
	
	*alpha = newalpha;
	*beta = newbeta;
	
}
void WeightTable::findWeight2(float x, float y, int ilon, int ilat, float* alpha, float* beta){
	//use iteration to determine alpha and beta that interpolates x, y from the user grid
	//corners associated with ilon and ilat, mapped into polar coordinates.
	//start with prevAlpha=0, prevBeta=0.  
	float xc[4],yc[4];
	float rad[4],thet[4];
	if (ilon == _nx-1){//wrap 
		rad[0] = (90.-_geo_lat[ilon+_nx*ilat])*2./M_PI;
		rad[1] = (90.-_geo_lat[_nx*ilat])*2./M_PI;
		rad[2] = (90.-_geo_lat[_nx*(ilat+1)])*2./M_PI;
		rad[3] = (90.-_geo_lat[ilon+_nx*(ilat+1)])*2./M_PI;
		thet[0] = _geo_lon[ilon+_nx*ilat]*M_PI/180.;
		thet[1] = _geo_lon[_nx*ilat]*M_PI/180.;
		thet[2] = _geo_lon[_nx*(ilat+1)]*M_PI/180.;
		thet[3] = _geo_lon[ilon+_nx*(ilat+1)]*M_PI/180.;
	} else if (ilat == _ny-1){  //zipper
		rad[0] = (90.-_geo_lat[ilon+_nx*ilat])*2./M_PI;
		rad[1] = (90.-_geo_lat[ilon+1+_nx*ilat])*2./M_PI;
		rad[2] = (90.-_geo_lat[_nx - ilon -2 +_nx*ilat])*2./M_PI;
		rad[3] = (90.-_geo_lat[_nx - ilon - 1 +_nx*ilat])*2./M_PI;
		thet[0] = _geo_lon[ilon+_nx*ilat]*M_PI/180.;
		thet[1] = _geo_lon[ilon+1+_nx*ilat]*M_PI/180.;
		thet[2] = _geo_lon[_nx - ilon -2 +_nx*ilat]*M_PI/180.;
		thet[3] = _geo_lon[_nx - ilon -1 +_nx*ilat]*M_PI/180.;
	} else {
		rad[0] = (90.-_geo_lat[ilon+_nx*ilat])*2./M_PI;
		rad[1] = (90.-_geo_lat[ilon+1+_nx*ilat])*2./M_PI;
		rad[2] = (90.-_geo_lat[ilon+1+_nx*(ilat+1)])*2./M_PI;
		rad[3] = (90.-_geo_lat[ilon+_nx*(ilat+1)])*2./M_PI;
		thet[0] = _geo_lon[ilon+_nx*ilat]*M_PI/180.;
		thet[1] = _geo_lon[ilon+1+_nx*ilat]*M_PI/180.;
		thet[2] = _geo_lon[ilon+1+_nx*(ilat+1)]*M_PI/180.;
		thet[3] = _geo_lon[ilon+_nx*(ilat+1)]*M_PI/180.;
	}
	
	for (int i = 0; i<4; i++){
		xc[i] = rad[i]*cos(thet[i]);
		yc[i] = rad[i]*sin(thet[i]);
	}
	
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	float errp=100.;
	for (iter = 0; iter < 20; iter++){
		newalpha = NEWA(alph, bet, x, y, xc, yc);
		newbeta = NEWB(alph, bet, x, y, xc, yc);
		if (newalpha == 1.e30f || newbeta == 1.e30f) {  //Degenerate rectangle
			for (int j = 0; j<4; j++){
				bool lowerx = true;
				bool lowery = true;
				if (j == 1 || j == 2) lowerx = false;
				if (j > 1 ) lowery = false;
				if (xc[j] < 0.f) lowerx = !lowerx;
				if (yc[j] < 0.f) lowery = !lowery;
				if (lowerx) xc[j]=0.99999f*xc[j]; 
				else xc[j] = 1.000001f*xc[j];
				if (lowery) yc[j]=0.99999f*yc[j]; 
				else yc[j] = 1.00001f*yc[j];
			}
			continue;
		}
		errp = errP(x, y, newalpha, newbeta,xc,yc);
		if (errp < 1.e-6) break;
		alph = newalpha;
		bet = newbeta;
	}
	//if(iter >= 20 && errp > 1.){
	//	fprintf(stderr," Weight nonconvergence, error value: %f , alpha, beta: %f %f\n",errp, newalpha, newbeta);
	//}
	*alpha = newalpha;
	*beta = newbeta;
	
}
		

//Calculate the angle (in radians) that the grid makes with the latitude, at a particular vertex.
//Use the geolat, geolon variables at the vertex, determine the longitude and latitude difference 
//(londiff and latdiff) in going
//to the next vertex over (or previous, if this is on the right edge).  
//the angle is atan(latdiff/londiff). Of course the atan is between -90 and +90.
//If londiff is negative, then the angle is between 90 and 180 (if latdiff is +) or between -90 and -180 (if latdiff is - ).
//If londiff is zero, then the angle is 90 (if latdiff is +) or -90 (if latdiff is - )
float WeightTable::getAngle(int ilon, int ilat) const{
	float londiff, latdiff, angle;
	if (ilon < _nx -1) {
		londiff = _geo_lon[ilon+1+_nx*ilat] - _geo_lon[ilon+_nx*ilat];
		latdiff = _geo_lat[ilon+1+_nx*ilat] - _geo_lat[ilon+_nx*ilat];
	} else {
		londiff = _geo_lon[ilon+_nx*ilat] - _geo_lon[ilon-1 +_nx*ilat];
		latdiff = _geo_lat[ilon+_nx*ilat] - _geo_lat[ilon-1 +_nx*ilat];
	}
	if (londiff == 0.f){
		if (latdiff > 0.f) angle = M_PI*.5;
		else angle = -M_PI*0.5;
	} else {
		angle = atan(latdiff/londiff);
		if (londiff < 0.){ //add PI or subtract PI to get angle between -PI and PI
			if (latdiff > 0.) angle = M_PI + angle;
			else angle = angle - M_PI;
		}
	}
	return angle;
}
