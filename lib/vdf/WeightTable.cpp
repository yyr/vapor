//
// $Id$
//
#include <iostream>
#include <cstdio>
#include <cstring>
#include <vector>
#include <sstream>
#include <ctime>
#include <netcdf.h>
#include "assert.h"
#include "proj_api.h"
#include <vapor/CFuncs.h>
#include <vapor/OptionParser.h>
#include <vapor/MOM.h>
#include <vapor/ROMS.h>
#include <vapor/WRF.h>
#include <vapor/weightTable.h>	

#ifdef _WINDOWS 
#define _USE_MATH_DEFINES
#include <math.h>
#pragma warning(disable : 4996)
#endif
using namespace VAPoR;
using namespace VetsUtil;
#define NC_ERR_READ(X) \
    { \
    int rc = (X); \
    if (rc != NC_NOERR) { \
        MyBase::SetErrMsg("Error reading netCDF file at line %d : %s", \
		__LINE__,  nc_strerror(rc)) ; \
        return(-1); \
    } \
    }

WeightTable::WeightTable(MOM* mom, int latvar, int lonvar){
	MOMBased = true;
	geoLatVarName = mom->getGeoLatVars()[latvar];
	geoLonVarName = mom->getGeoLonVars()[lonvar];
	size_t dims[3];
	mom->GetDims(dims);
	nlon = dims[0];
	nlat = dims[1];
	
	const float *llexts = mom->GetLonLatExtents();
	for (int i = 0; i<4; i++) lonLatExtents[i] = llexts[i];
		
	alphas = new float[nlon*nlat];
	betas = new float[nlon*nlat];
	testValues = new float[nlon*nlat];
	cornerLons = new int[nlon*nlat];
	cornerLats = new int[nlon*nlat];
	for (int i = 0; i<nlon*nlat; i++){
		testValues[i] = 1.e30f;
		alphas[i] = - 1.f;
		betas[i] = -1.f;
	}
	geo_lat = 0;
	geo_lon = 0;
	deltaLat = (lonLatExtents[3]-lonLatExtents[1])/(nlat-1);
	deltaLon = (lonLatExtents[2]-lonLatExtents[0])/(nlon-1);
	wrapLon=false;
	//Check for global wrap in longitude:
	
	
	if (abs(lonLatExtents[2] - 360.0 - lonLatExtents[0])< 2.*deltaLon) {
		deltaLon = (lonLatExtents[2]-lonLatExtents[0])/nlon;
		wrapLon=true;
	}
	//Does the grid go to the north pole?
	if (lonLatExtents[3] >= 90.-2.*deltaLat) wrapLat = true;
	else wrapLat = false;
	
	// Calc epsilon for converging alpha,beta 
	epsilon = Max(deltaLat, deltaLon)*1.e-7;
	// Calc epsilon for checking outside rectangle
	epsRect = Max(deltaLat,deltaLon)*0.1;
}
WeightTable::WeightTable(ROMS* roms, int latlonvar){
	MOMBased = false;
	geoLatVarName = roms->getGeoLatVars()[latlonvar];
	geoLonVarName = roms->getGeoLonVars()[latlonvar];
	size_t dims[3];
	roms->GetDims(dims);
	nlon = dims[0];
	nlat = dims[1];
	if (latlonvar == 1 || latlonvar == 3) // u-grid or rho-grid
		nlat++;
	if (latlonvar == 2 || latlonvar == 3) // v-grid or rho-grid
		nlon++;
	
	const float *llexts = roms->GetLonLatExtents();
	for (int i = 0; i<4; i++) lonLatExtents[i] = llexts[i];
		
	alphas = new float[nlon*nlat];
	betas = new float[nlon*nlat];
	testValues = new float[nlon*nlat];
	cornerLons = new int[nlon*nlat];
	cornerLats = new int[nlon*nlat];
	for (int i = 0; i<nlon*nlat; i++){
		testValues[i] = 1.e30f;
		alphas[i] = - 1.f;
		betas[i] = -1.f;
	}
	geo_lat = 0;
	geo_lon = 0;
	deltaLat = (lonLatExtents[3]-lonLatExtents[1])/(nlat-1);
	deltaLon = (lonLatExtents[2]-lonLatExtents[0])/(nlon-1);

	//With ROMS, do not support wrap in lat or lon
	wrapLon=false;
	wrapLat = false;
	
	// Calc epsilon for converging alpha,beta 
	epsilon = Max(deltaLat, deltaLon)*1.e-7;
	// Calc epsilon for checking outside rectangle
	epsRect = Max(deltaLat,deltaLon)*0.1;
}
//Interpolation functions, can be called after the alphas and betas arrays have been calculated.
//Following can also be used on slices of 3D data
//If the corner latitude is at the top then
void WeightTable::interp2D(const float* sourceData, float* resultData, float missingValue, const size_t* dims){
	int corlon, corlat, corlatp, corlona, corlonb, corlonp;
	
	int latsize = dims[1];
	int lonsize = dims[0];
	float minin = 1.e38f;
	float maxin = -1.e38f;
	assert (latsize <= nlat && lonsize <= nlon);
	for (int i = 0; i< nlat*nlon; i++){
		if (sourceData[i] == missingValue) continue;
		if (sourceData[i] < -1.e10 || sourceData[i] > 1.e10){
			float foo = sourceData[i];
		}
		if (sourceData[i]<minin) minin = sourceData[i];
		if (sourceData[i]>maxin) maxin = sourceData[i];
	}
	for (int j = 0; j<latsize; j++){
		for (int i = 0; i<lonsize; i++){
			corlon = cornerLons[i+nlon*j];//Lookup the user grid vertex lowerleft corner that contains (i,j) lon-lat vertex
			corlat = cornerLats[i+nlon*j];//corlon and corlat are not lon and lat, just x and y 
			corlatp = corlat+1;
			corlona = corlon;
			corlonb = corlon+1;
			corlonp = corlon+1;
			if (MOMBased && corlon == nlon-1){ //Wrap-around longitude.  Note that longitude will not wrap at zipper.
				corlonp = 0;
				corlonb = 0;
			} else if (MOMBased && corlat == nlat-1){ //Get corners on opposite side of zipper:
				corlatp = corlat;
				corlona = nlon - corlon -1;
				corlonb = nlon - corlon -2;
			}
			float alpha = alphas[i+nlon*j];
			float beta = betas[i+nlon*j];
			float data0 = sourceData[corlon+nlon*corlat];
			float data1 = sourceData[corlonp+nlon*corlat];
			float data2 = sourceData[corlonb+nlon*corlatp];
			float data3 = sourceData[corlona+nlon*corlatp];
			
			float cf0 = (1.-alpha)*(1.-beta);
			float cf1 = alpha*(1.-beta);
			float cf2 = alpha*beta;
			float cf3 = (1.-alpha)*beta;
			float goodSum = 0.f;
			float mvCoef = 0.f;
			//Accumulate values and coefficients for missing and non-missing values
			//If less than half the weight comes from missing value, then apply weights to non-missing values,
			//otherwise just set result to missing value.
			if (data0 == missingValue) mvCoef += cf0;
			else {
				assert (data0 > -1.e10 && data0 < 1.e10);
				goodSum += cf0*data0;
			}
			if (data1 == missingValue) mvCoef += cf1;
			else {
				assert (data1 > -1.e10 && data1 < 1.e10);
				goodSum += cf1*data1;
			}
			if (data2 == missingValue) mvCoef += cf2;
			else {
				assert (data2 > -1.e10 && data2 < 1.e10);
				goodSum += cf2*data2;
			}
			if (data3 == missingValue) mvCoef += cf3;
			else {
				assert (data3 > -1.e10 && data3 < 1.e10);
				goodSum += cf3*data3;
			}
			if (mvCoef >= 0.5f) resultData[i+j*lonsize] = (float)MOM::vaporMissingValue();
			else {
				resultData[i+j*lonsize] = goodSum/(1.-mvCoef);
				assert(resultData[i+j*lonsize] < 1.e10 && resultData[i+j*lonsize] > -1.e10);
			}
			
		}
		
	}
	
}
void WeightTable::interp2D(const double* sourceData, float* resultData, double missingValue, const size_t* dims){
	int corlon, corlat, corlatp, corlona, corlonb, corlonp;
	
	int latsize = dims[1];
	int lonsize = dims[0];
	float minin = 1.e38f;
	float maxin = -1.e38f;
	assert (latsize <= nlat && lonsize <= nlon);
	for (int i = 0; i< nlat*nlon; i++){
		if (sourceData[i] == missingValue) continue;
		if (sourceData[i] < -1.e10 || sourceData[i] > 1.e10){
			float foo = sourceData[i];
		}
		if (sourceData[i]<minin) minin = sourceData[i];
		if (sourceData[i]>maxin) maxin = sourceData[i];
	}
	for (int j = 0; j<latsize; j++){
		for (int i = 0; i<lonsize; i++){
			corlon = cornerLons[i+nlon*j];//Lookup the user grid vertex lowerleft corner that contains (i,j) lon-lat vertex
			corlat = cornerLats[i+nlon*j];//corlon and corlat are not lon and lat, just x and y 
			corlatp = corlat+1;
			corlona = corlon;
			corlonb = corlon+1;
			corlonp = corlon+1;
			if (MOMBased && corlon == nlon-1){ //Wrap-around longitude.  Note that longitude will not wrap at zipper.
				corlonp = 0;
				corlonb = 0;
			} else if (MOMBased && corlat == nlat-1){ //Get corners on opposite side of zipper:
				corlatp = corlat;
				corlona = nlon - corlon -1;
				corlonb = nlon - corlon -2;
			}
			float alpha = alphas[i+nlon*j];
			float beta = betas[i+nlon*j];
			float data0 = sourceData[corlon+nlon*corlat];
			float data1 = sourceData[corlonp+nlon*corlat];
			float data2 = sourceData[corlonb+nlon*corlatp];
			float data3 = sourceData[corlona+nlon*corlatp];
			
			float cf0 = (1.-alpha)*(1.-beta);
			float cf1 = alpha*(1.-beta);
			float cf2 = alpha*beta;
			float cf3 = (1.-alpha)*beta;
			float goodSum = 0.f;
			float mvCoef = 0.f;
			//Accumulate values and coefficients for missing and non-missing values
			//If less than half the weight comes from missing value, then apply weights to non-missing values,
			//otherwise just set result to missing value.
			if (data0 == missingValue) mvCoef += cf0;
			else {
				assert (data0 > -1.e10 && data0 < 1.e10);
				goodSum += cf0*data0;
			}
			if (data1 == missingValue) mvCoef += cf1;
			else {
				assert (data1 > -1.e10 && data1 < 1.e10);
				goodSum += cf1*data1;
			}
			if (data2 == missingValue) mvCoef += cf2;
			else {
				assert (data2 > -1.e10 && data2 < 1.e10);
				goodSum += cf2*data2;
			}
			if (data3 == missingValue) mvCoef += cf3;
			else {
				assert (data3 > -1.e10 && data3 < 1.e10);
				goodSum += cf3*data3;
			}
			if (mvCoef >= 0.5f) resultData[i+j*lonsize] = (float)MOM::vaporMissingValue();
			else {
				resultData[i+j*lonsize] = goodSum/(1.-mvCoef);
				assert(resultData[i+j*lonsize] < 1.e10 && resultData[i+j*lonsize] > -1.e10);
			}
			
		}
		
	}
	
}
//For given latitude longitude grid vertex, used for testing.
//check all geolon/geolat quads.  See what ulat/ulon cell contains it.
float WeightTable::bestLatLon(int ilon, int ilat){
	
	
	//Loop over all the lon/lat grid vertices
	
	float testLon = lonLatExtents[0]+ilon*deltaLon;
	float testLat = lonLatExtents[1]+ilat*deltaLat;
	//Loop over all cells in grid
	float minval = 1.e30;
	int bestulon,bestulat;
	//Loop over the ulat/ulon grid.  For each ulon-ulat grid coord
	//See if ilon,ilat is in cell mapped from ulon,ulat
	for (int qlat = 0; qlat< nlat-1; qlat++){
		for (int plon = 0; plon< nlon; plon++){
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
	
	float testLon = lonLatExtents[0]+ilon*deltaLon;
	float testLat = lonLatExtents[1]+ilat*deltaLat;
	
	//Loop over all cells in grid
	float minval = 1.e30;
	int bestulon, bestulat;
	float testRad, testThet;
	float bestx, besty;
	
	float bestLat, bestLon, bestThet, bestRad;
	//loop over ulon,ulat cells
	for (int qlat = 0; qlat< nlat; qlat++){
		if (qlat == nlat-1 && !wrapLat) continue;
		
		for (int plon = 0; plon< nlon; plon++){
			if (qlat == nlat-1 && plon > nlon/2) continue;  // only do first half of zipper
			if (plon == nlon-1 && !wrapLon) continue;
			
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

	
int WeightTable::calcWeights(int ncid){
	
	//	Allocate arrays for geolat and geolon.
	geo_lat = new float[nlat*nlon];
	
	//Find the geo_lat and geo_lon variable id's in the file
	int geolatvarid;
	NC_ERR_READ(nc_inq_varid (ncid, geoLatVarName.c_str(), &geolatvarid));
	//	Read the geolat and geolon variables into arrays.
	// Note that lon increases fastest
	NC_ERR_READ(nc_get_var_float(ncid, geolatvarid, geo_lat));
	if (MOMBased)
		geo_lon = MOM::getMonotonicLonData(ncid, geoLonVarName.c_str(), nlon, nlat);
	else 
		geo_lon = ROMS::getMonotonicLonData(ncid, geoLonVarName.c_str(), nlon, nlat);
	if (!geo_lon) return -1;
	float eps = Max(deltaLat,deltaLon)*1.e-3;

	//Check out the geolat, geolon variables
	for (int i = 0; i<nlon*nlat; i++){
		assert (geo_lat[i] >= -90. && geo_lat[i] <= 90.);
		assert (geo_lon[i] >= -360. && geo_lon[i] <= 360.);
	}
	if (!MOMBased){
		assert  (!wrapLat && !wrapLon);
	}
	
	//float tst = bestLatLon(0,0);
	
	//	Loop over (nlon/nlat) user grid vertices.  These are x and y grid vertex indices
	//  Call them ulat and ulon to suggest "lat and lon" in user coordinates
	float lat[4],lon[4];
	for (int ulat = 0; ulat<nlat; ulat++){
		if (ulat == nlat-1 && !wrapLat) continue;
		float beglat = geo_lat[nlon*ulat];
		if(beglat < 0. ){ //below equator
						
			for (int ulon = 0; ulon<nlon; ulon++){
				
				if (ulon == nlon-1 && !wrapLon) continue;
				if (ulon == nlon-1){
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[nlon*(ulat+1)];
				} else {
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[ulon+1+nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[ulon+1+nlon*(ulat+1)];
				}
				float minlat = Min(Min(lat[0],lat[1]),Min(lat[0],lat[3])) - eps;
				float maxlat = Max(Max(lat[0],lat[1]),Min(lat[2],lat[3])) + eps;

				
				//	For each cell, find min and max longitude and latitude; expand by eps in all directions to define maximal ulon-ulat cell in
				// actual lon/lat coordinates
				float lon0,lon1,lon2,lon3;
				if (ulon == nlon-1){
					lon0 = geo_lon[ulon+nlon*ulat];
					lon1 = geo_lon[nlon*ulat];
					lon2 = geo_lon[ulon+nlon*(ulat+1)];
					lon3 = geo_lon[nlon*(ulat+1)];
					
				} else {
					lon0 = geo_lon[ulon+nlon*ulat];
					lon1 = geo_lon[ulon+1+nlon*ulat];
					lon2 = geo_lon[ulon+nlon*(ulat+1)];
					lon3 = geo_lon[ulon+1+nlon*(ulat+1)];
				}
							
				float minlon = Min(Min(lon0,lon1),Min(lon2,lon3))-eps;
				float maxlon = Max(Max(lon0,lon1),Max(lon2,lon3))+eps;

								
				// find all the latlon grid vertices that fit near this rectangle: 
				// Add 1 to the LL corner latitude because it is definitely below and left of the rectangle
				int latInd0 = (int)(1.+((minlat-lonLatExtents[1])/deltaLat));
				// Don't add 1 to the longitude corner because in some cases (with wrap) it would eliminate a valid longitude index.
				int lonInd0 = (int)(((minlon-lonLatExtents[0])/deltaLon));
				//cover the edges too!
				// edgeflag is used to enable extrapolation when points are slightly outside of the grid extents
				bool edgeFlag = false;
				if (ulat == 0 && latInd0 ==1) {latInd0 = 0; edgeFlag = true;}
				if (ulon == 0) {
					lonInd0 = 0; edgeFlag = true;
				}
			
				// Whereas the UR corner vertex may be inside:
				int latInd1 = (int)((maxlat-lonLatExtents[1])/deltaLat);
				int lonInd1 = (int)((maxlon-lonLatExtents[0])/deltaLon);
			
			
				if (latInd0 <0 || lonInd0 < 0) {
					continue;
				}
				if (latInd1 >= nlat || (lonInd1 > nlon)||(lonInd1 == nlon && !wrapLon)){
					continue;
				}
						
				//Loop over all the lon/lat grid vertices in the maximized cell:
				for (int qlat = latInd0; qlat<= latInd1; qlat++){
					for (int plon = lonInd0; plon<= lonInd1; plon++){
						
						int plon1 = plon;
						if(plon == nlon) {
							assert(wrapLon);
							plon1 = 0;
						}
						float testLon = lonLatExtents[0]+plon1*deltaLon;
						float testLat = lonLatExtents[1]+qlat*deltaLat;
						float eps = testInQuad(testLon,testLat,ulon, ulat);
						if (eps < epsRect || edgeFlag){  //OK, point is inside, or close enough.
							//Check to make sure eps is smaller than previous entries:
							if (eps > testValues[plon1+nlon*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							findWeight(testLon, testLat, ulon, ulat,&alph, &beta);
							assert(plon1 >= 0 && plon1 < nlon);
							assert(qlat >= 0 && qlat < nlat);
							alphas[plon1+nlon*qlat] = alph;
							betas[plon1+nlon*qlat] = beta;
							testValues[plon1+nlon*qlat] = eps;
							cornerLats[plon1+nlon*qlat] = ulat;
							cornerLons[plon1+nlon*qlat] = ulon;
							
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
			for (int ulon = 0; ulon<nlon; ulon++){
				
				if (ulat == nlat-1 && ulon > nlon/2) continue;  // only do first half of zipper
				if (ulon == nlon-1 && !wrapLon) continue;
				
				if (ulon == nlon-1){
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[nlon*(ulat+1)];
					
					lon[0] = geo_lon[ulon+nlon*ulat];
					lon[1] = geo_lon[nlon*ulat];
					lon[2] = geo_lon[ulon+nlon*(ulat+1)];
					lon[3] = geo_lon[nlon*(ulat+1)];
				} else if (ulat == nlat-1) { //Handle zipper:
					// The point (ulat, ulon) is across from (ulat, nlon-ulon-1)
					// Whenever ulat+1 is used, replace it with ulat, and switch ulon to nlon-ulon-1
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[ulon+1+nlon*ulat];
					lat[2] = geo_lat[nlon-ulon-2+nlon*ulat];
					lat[3] = geo_lat[nlon -ulon -1+nlon*ulat];
					lon[0] = geo_lon[ulon+nlon*ulat];
					lon[1] = geo_lon[ulon+1+nlon*ulat];
					lon[2] = geo_lon[nlon-ulon-2+nlon*ulat];
					lon[3] = geo_lon[nlon-ulon-1+nlon*ulat];
				} else {
					lat[0] = geo_lat[ulon+nlon*ulat];
					lat[1] = geo_lat[ulon+1+nlon*ulat];
					lat[2] = geo_lat[ulon+nlon*(ulat+1)];
					lat[3] = geo_lat[ulon+1+nlon*(ulat+1)];
					
					lon[0] = geo_lon[ulon+nlon*ulat];
					lon[1] = geo_lon[ulon+1+nlon*ulat];
					lon[2] = geo_lon[ulon+nlon*(ulat+1)];
					lon[3] = geo_lon[ulon+1+nlon*(ulat+1)];
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
				int latMin = (int)(1.+(minlat-lonLatExtents[1])/deltaLat);
				int latMax = (int)((maxlat-lonLatExtents[1])/deltaLat);
				if (fullCircle) {
					lonMin = 0; lonMax = nlon;
					latMax = nlat -1;
				}
				else {
					lonMin = (int)(1.+(minlon  -lonLatExtents[0])/deltaLon);
					lonMax = (int)((maxlon -lonLatExtents[0])/deltaLon);
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
				if (latMax < nlat-1) latMax++;
				if (lonMax < nlon -1) lonMax++;
				//Test each point in lon/lat interval:
				for (int qlat = latMin; qlat<= latMax; qlat++){
					for (int plon = lonMin; plon<= lonMax; plon++){
						float testLon = lonLatExtents[0]+plon*deltaLon;
						int plon1 = plon;
						if(plon >= nlon) {
							assert(wrapLon);
							plon1 = plon -nlon;
						}
						float testLat = lonLatExtents[1]+qlat*deltaLat;
						//Convert lat/lon coordinates to x,y
						float testRad = (90.-testLat)*2./M_PI;
						float testThet = testLon*M_PI/180.;
						float testx = testRad*cos(testThet);
						float testy = testRad*sin(testThet);
						float eps = testInQuad2(testx,testy,ulon, ulat);
						if (eps < epsRect || edgeFlag){  //OK, point is inside, or close enough.
							assert(plon1 >= 0 && plon1 < nlon);
							assert(qlat >= 0 && qlat < nlat);

							//Check to make sure eps is smaller than previous entries:
							if (eps >= testValues[plon1+nlon*qlat]) continue;
							//It's OK, stuff it in the weights arrays:
							float alph, beta;
							
							findWeight2(testx, testy, ulon, ulat,&alph, &beta);
							
							alphas[plon1+nlon*qlat] = alph;
							betas[plon1+nlon*qlat] = beta;
							testValues[plon1+nlon*qlat] = eps;
							cornerLats[plon1+nlon*qlat] = ulat;
							cornerLons[plon1+nlon*qlat] = ulon;
							
						}
					}
				}
				
			} //end ulon loop
		} //end else
	} //end ulat loop
	//Check it out:
	
	for (int i = 0; i<nlat; i++){
		for (int j = 0; j<nlon; j++){
			if (testValues[j+nlon*i] <1.){
				if (alphas[j+nlon*i] > 2.0 || alphas[j+nlon*i] < -1.0 || betas[j+nlon*i]>2.0 || betas[j+nlon*i]< -1.0)
					fprintf(stderr," bad alpha %g beta %g at lon,lat %d , %d, ulon %d ulat %d, testval: %g\n",alphas[j+nlon*i],betas[j+nlon*i], j, i, cornerLons[j+nlon*i], cornerLats[j+nlon*i],testValues[j+i*nlon]);
				continue;
			}
			else {
				if (testValues[j+nlon*i] < 10000.) printf( "poor estimate: %f at lon, lat %d %d, ulon, ulat %d %d\n", testValues[j+nlon*i],j,i, cornerLons[j+nlon*i], cornerLats[j+nlon*i]);
				else {
					MyBase::SetErrMsg(" Error remapping coordinates.  Unmapped lon, lat: %d, %d\n",j,i);
					//exit (-1);
				}
			}
		}
	}
	 
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
	if (ilon == nlon-1){//wrap in ilon coord
		lt[0] = geo_lat[ilon+nlon*ilat];
		lt[1] = geo_lat[nlon*ilat];
		lt[2] = geo_lat[nlon*(ilat+1)];
		lt[3] = geo_lat[ilon+nlon*(ilat+1)];
		lg[0] = geo_lon[ilon+nlon*ilat];
		lg[1] = geo_lon[nlon*ilat];
		lg[2] = geo_lon[nlon*(ilat+1)];
		lg[3] = geo_lon[ilon+nlon*(ilat+1)];
	} else {
		lt[0] = geo_lat[ilon+nlon*ilat];
		lt[1] = geo_lat[ilon+1+nlon*ilat];
		lt[2] = geo_lat[ilon+1+nlon*(ilat+1)];
		lt[3] = geo_lat[ilon+nlon*(ilat+1)];
		lg[0] = geo_lon[ilon+nlon*ilat];
		lg[1] = geo_lon[ilon+1+nlon*ilat];
		lg[2] = geo_lon[ilon+1+nlon*(ilat+1)];
		lg[3] = geo_lon[ilon+nlon*(ilat+1)];
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
	if (ulon == nlon-1 ){  //wrap; ie. ulon+1 is replaced by 0
		rad[0] = (90.-geo_lat[ulon+nlon*ulat])*2./M_PI;
		rad[1] = (90.-geo_lat[nlon*ulat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon*(ulat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ulon+nlon*(ulat+1)])*2./M_PI;
		thet[0] = geo_lon[ulon+nlon*ulat]*M_PI/180.;
		thet[1] = geo_lon[nlon*ulat]*M_PI/180.;
		thet[2] = geo_lon[nlon*(ulat+1)]*M_PI/180.;
		thet[3] = geo_lon[ulon+nlon*(ulat+1)]*M_PI/180.;
	} else if (ulat == nlat-1) { //Handle zipper:
		// The point (ulat, ulon) is across from (ulat, nlon-ulon-1)
		// Whenever ulat+1 is used, replace it with ulat, and switch ulon to nlon-ulon-1
		rad[0] = (90.-geo_lat[ulon+nlon*ulat])*2./M_PI;
		rad[1] = (90.-geo_lat[ulon+1+nlon*ulat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon-ulon-2+nlon*ulat])*2./M_PI;
		rad[3] = (90.-geo_lat[nlon -ulon -1+nlon*ulat])*2./M_PI;
		thet[0] = geo_lon[ulon+nlon*ulat]*M_PI/180.;
		thet[1] = geo_lon[ulon+1+nlon*ulat]*M_PI/180.;
		thet[2] = geo_lon[nlon-ulon-2+nlon*ulat]*M_PI/180.;
		thet[3] = geo_lon[nlon-ulon-1+nlon*ulat]*M_PI/180.;
	} else {
		rad[0] = (90.-geo_lat[ulon+nlon*ulat])*2./M_PI;
		rad[1] = (90.-geo_lat[ulon+1+nlon*ulat])*2./M_PI;
		rad[2] = (90.-geo_lat[ulon+1+nlon*(ulat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ulon+nlon*(ulat+1)])*2./M_PI;
		thet[0] = geo_lon[ulon+nlon*ulat]*M_PI/180.;
		thet[1] = geo_lon[ulon+1+nlon*ulat]*M_PI/180.;
		thet[2] = geo_lon[ulon+1+nlon*(ulat+1)]*M_PI/180.;
		thet[3] = geo_lon[ulon+nlon*(ulat+1)]*M_PI/180.;
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
	if (ilon == nlon-1){
		th[0] = geo_lon[ilon+nlon*ilat];
		ph[0] = geo_lat[ilon+nlon*ilat];
		th[1] = geo_lon[nlon*ilat];
		ph[1] = geo_lat[nlon*ilat];
		th[2] = geo_lon[nlon*(ilat+1)];
		ph[2] = geo_lat[nlon*(ilat+1)];
		th[3] = geo_lon[ilon+nlon*(ilat+1)];
		ph[3] = geo_lat[ilon+nlon*(ilat+1)];
		
	} else {
		th[0] = geo_lon[ilon+nlon*ilat];
		ph[0] = geo_lat[ilon+nlon*ilat];
		th[1] = geo_lon[ilon+1+nlon*ilat];
		ph[1] = geo_lat[ilon+1+nlon*ilat];
		th[2] = geo_lon[ilon+1+nlon*(ilat+1)];
		ph[2] = geo_lat[ilon+1+nlon*(ilat+1)];
		th[3] = geo_lon[ilon+nlon*(ilat+1)];
		ph[3] = geo_lat[ilon+nlon*(ilat+1)];
	}
	for (int k = 0; k<4; k++){
		if (th[k]> plon+180.) th[k] -= 360.;
	}
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	for (iter = 0; iter < 10; iter++){
		newalpha = NEWA(alph, bet, plon, plat, th, ph);
		newbeta = NEWB(alph, bet, plon, plat, th, ph);
		
		if (errP(plon, plat, newalpha, newbeta,th,ph) < 1.e-9) break;
		alph = newalpha;
		bet = newbeta;
	}
	if(iter > 10){
		printf(" Weight nonconvergence, errp: %f , alpha, beta: %f %f\n",errP(plon, plat, newalpha, newbeta,th,ph), newalpha, newbeta);
	}
	
	*alpha = newalpha;
	*beta = newbeta;
	
}
void WeightTable::findWeight2(float x, float y, int ilon, int ilat, float* alpha, float* beta){
	//use iteration to determine alpha and beta that interpolates x, y from the user grid
	//corners associated with ilon and ilat, mapped into polar coordinates.
	//start with prevAlpha=0, prevBeta=0.  
	float xc[4],yc[4];
	float rad[4],thet[4];
	if (ilon == nlon-1){//wrap 
		rad[0] = (90.-geo_lat[ilon+nlon*ilat])*2./M_PI;
		rad[1] = (90.-geo_lat[nlon*ilat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon*(ilat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ilon+nlon*(ilat+1)])*2./M_PI;
		thet[0] = geo_lon[ilon+nlon*ilat]*M_PI/180.;
		thet[1] = geo_lon[nlon*ilat]*M_PI/180.;
		thet[2] = geo_lon[nlon*(ilat+1)]*M_PI/180.;
		thet[3] = geo_lon[ilon+nlon*(ilat+1)]*M_PI/180.;
	} else if (ilat == nlat-1){  //zipper
		rad[0] = (90.-geo_lat[ilon+nlon*ilat])*2./M_PI;
		rad[1] = (90.-geo_lat[ilon+1+nlon*ilat])*2./M_PI;
		rad[2] = (90.-geo_lat[nlon - ilon -2 +nlon*ilat])*2./M_PI;
		rad[3] = (90.-geo_lat[nlon - ilon - 1 +nlon*ilat])*2./M_PI;
		thet[0] = geo_lon[ilon+nlon*ilat]*M_PI/180.;
		thet[1] = geo_lon[ilon+1+nlon*ilat]*M_PI/180.;
		thet[2] = geo_lon[nlon - ilon -2 +nlon*ilat]*M_PI/180.;
		thet[3] = geo_lon[nlon - ilon -1 +nlon*ilat]*M_PI/180.;
	} else {
		rad[0] = (90.-geo_lat[ilon+nlon*ilat])*2./M_PI;
		rad[1] = (90.-geo_lat[ilon+1+nlon*ilat])*2./M_PI;
		rad[2] = (90.-geo_lat[ilon+1+nlon*(ilat+1)])*2./M_PI;
		rad[3] = (90.-geo_lat[ilon+nlon*(ilat+1)])*2./M_PI;
		thet[0] = geo_lon[ilon+nlon*ilat]*M_PI/180.;
		thet[1] = geo_lon[ilon+1+nlon*ilat]*M_PI/180.;
		thet[2] = geo_lon[ilon+1+nlon*(ilat+1)]*M_PI/180.;
		thet[3] = geo_lon[ilon+nlon*(ilat+1)]*M_PI/180.;
	}
	
	for (int i = 0; i<4; i++){
		xc[i] = rad[i]*cos(thet[i]);
		yc[i] = rad[i]*sin(thet[i]);
	}
	
	float alph = 0.f, bet = 0.f;  //first guess
	float newalpha, newbeta;
	int iter;
	for (iter = 0; iter < 10; iter++){
		newalpha = NEWA(alph, bet, x, y, xc, yc);
		newbeta = NEWB(alph, bet, x, y, xc, yc);
		
		if (errP(x, y, newalpha, newbeta,xc,yc) < 1.e-9) break;
		alph = newalpha;
		bet = newbeta;
	}
	if(iter > 10){
		fprintf(stderr," Weight nonconvergence, errp: %f , alpha, beta: %f %f\n",errP(x, y, newalpha, newbeta, xc,yc), newalpha, newbeta);
	}
	*alpha = newalpha;
	*beta = newbeta;
	
}
		

