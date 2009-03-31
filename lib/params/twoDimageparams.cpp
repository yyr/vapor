//************************************************************************
//																		*
//		     Copyright (C)  2009										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDimageparams.cpp
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		November 2005
//
//	Description:	Implementation of the twoDImageparams class
//		This contains all the parameters required to support the
//		TwoDImage image renderer. 
//
#ifdef WIN32
//Annoying unreferenced formal parameter warning
#pragma warning( disable : 4100 )
#endif

#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <math.h>
#include "glutil.h"
#include "twoDparams.h"
#include "twoDimageparams.h"
#include "regionparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"

#include <math.h>
#include "vapor/Metadata.h"
#include "vapor/DataMgr.h"
#include "vapor/VDFIOBase.h"
#include "vapor/LayeredIO.h"
#include "vapor/errorcodes.h"
#include "vapor/WRF.h"
//tiff stuff:

#include "geo_normalize.h"
#include "geotiff.h"
#include "xtiffio.h"
#include "proj_api.h"


using namespace VAPoR;
const string TwoDImageParams::_imagePlacementAttr = "ImagePlacement";
const string TwoDImageParams::_cropImageAttr = "CropImage";
const string TwoDImageParams::_georeferencedAttr = "Georeferenced";
const string TwoDImageParams::_resampleRateAttr = "ResamplingRate";
const string TwoDImageParams::_opacityMultAttr = "OpacityMultiplier";
const string TwoDImageParams::_imageFileNameAttr = "ImageFileName";

TwoDImageParams::TwoDImageParams(int winnum) : TwoDParams(winnum){
	thisParamType = TwoDImageParamsType;
	imageExtents = 0;
	textureSizes = 0;
	imageNums = 0;
	twoDDataTextures = 0;
	maxTimestep = 1;
	restart();
	
}
TwoDImageParams::~TwoDImageParams(){
	
	
	if (twoDDataTextures) {
		for (int i = 0; i<= maxTimestep; i++){
			if (twoDDataTextures[i]) delete twoDDataTextures[i];
		}
		delete twoDDataTextures;
		delete textureSizes;
		if (imageExtents){
			delete imageExtents;
		}
		if (imageNums) delete imageNums;
		imageNums = 0;
	}
	
	
	
}


//Deepcopy requires cloning tf 
RenderParams* TwoDImageParams::
deepRCopy(){
	TwoDImageParams* newParams = new TwoDImageParams(*this);
	
	//TwoD texture must be recreated when needed
	newParams->twoDDataTextures = 0;
	newParams->imageExtents = 0;
	newParams->imageNums = 0;
	newParams->textureSizes = 0;
	
	return newParams;
}






//Initialize for new metadata.  
//
bool TwoDImageParams::
reinit(bool doOverride){
	
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getExtents();
	setMaxNumRefinements(ds->getNumTransforms());
	//Set up the numRefinements combo
	
	
	//Either set the twoD bounds to default (full) size in the center of the domain, or 
	//try to use the previous bounds:
	if (doOverride){
		for (int i = 0; i<3; i++){
			float twoDRadius = 0.5f*(extents[i+3] - extents[i]);
			float twoDMid = 0.5f*(extents[i+3] + extents[i]);
			if (i<2) {
				twoDMin[i] = twoDMid - twoDRadius;
				twoDMax[i] = twoDMid + twoDRadius;
			} else {
				twoDMin[i] = twoDMax[i] = twoDMid;
			}
		}
		
		cursorCoords[0] = cursorCoords[1] = 0.0f;
		numRefinements = 0;
	} else {
		//Force the twoD size to be no larger than the domain extents, and 
		//force the twoD center to be inside the domain.  Note that
		//because of rotation, the twoD max/min may not correspond
		//to the same extents.
		float maxExtents = Max(Max(extents[3]-extents[0],extents[4]-extents[1]),extents[5]-extents[2]);
		for (int i = 0; i<3; i++){
			if (twoDMax[i] - twoDMin[i] > maxExtents)
				twoDMax[i] = twoDMin[i] + maxExtents;
			float center = 0.5f*(twoDMin[i]+twoDMax[i]);
			if (center < extents[i]) {
				twoDMin[i] += (extents[i]-center);
				twoDMax[i] += (extents[i]-center);
			}
			if (center > extents[i+3]) {
				twoDMin[i] += (extents[i+3]-center);
				twoDMax[i] += (extents[i+3]-center);
			}
			if(twoDMax[i] < twoDMin[i]) 
				twoDMax[i] = twoDMin[i];
		}
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	
	
	
	//Create new arrays to hold bounds 
	
	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	
	
	minTerrainHeight = extents[2];
	maxTerrainHeight = extents[2];
	int hgtvar = ds->getSessionVariableNum2D("HGT");
	if (hgtvar >= 0)
		maxTerrainHeight = ds->getDefaultDataMax2D(hgtvar);
	
	setEnabled(false);
	// set up the texture cache
	setTwoDDirty();
	if (twoDDataTextures) {
		delete twoDDataTextures;
		delete imageExtents;
		delete textureSizes;
		if(imageNums) delete imageNums;
		imageNums = 0;
	}
	
	maxTimestep = DataStatus::getInstance()->getMaxTimestep();
	twoDDataTextures = 0;
	imageExtents = 0;
	textureSizes = 0;
	initializeBypassFlags();
	return true;
}
//Initialize to default state
//
void TwoDImageParams::
restart(){
	
	imagePlacement = 0;
	
	mapToTerrain = false;
	minTerrainHeight = 0.f;
	maxTerrainHeight = 0.f;
	orientation = 2;
	setTwoDDirty();
	if (twoDDataTextures) {
		delete twoDDataTextures;
		delete imageExtents;
		delete textureSizes;
		if (imageNums) delete imageNums;
	}
	twoDDataTextures = 0;
	imageExtents = 0;
	textureSizes = 0;
	imageNums = 0;
	resampRate = 1.f;
	opacityMultiplier = 1.f;
	
	useGeoreferencing = true;
	cropImage = false;
	imageFileName = "";

	cursorCoords[0] = cursorCoords[1] = 0.0f;
	//Initialize the mapping bounds to [0,1] until data is read
	
	setEnabled(false);
	
	numRefinements = 0;
	maxNumRefinements = 10;
	
	for (int i = 0; i<3; i++){
		if (i < 2) twoDMin[i] = 0.0f;
		else twoDMin[i] = 0.5f;
		if(i<2) twoDMax[i] = 1.0f;
		else twoDMax[i] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	
	
}





//Handlers for Expat parsing.
//
bool TwoDImageParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	
	static int parsedVarNum = -1;
	
	if (StrCmpNoCase(tagString, _twoDImageParamsTag) == 0) {
		//Set defaults 
		resampRate = 1.f;
		opacityMultiplier = 1.f;
		useGeoreferencing = true;
		cropImage = false;
		imageFileName = "";
		orientation = 2; //X-Y aligned
		int newNumVariables = 0;
		imagePlacement = 0;

		
		//If it's a TwoDImage tag, obtain 12 attributes (2 are from Params class)
		//Do this by repeatedly pulling off the attribute name and value
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _vizNumAttr) == 0) {
				ist >> vizNum;
			}
			
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
			}
			
			else if (StrCmpNoCase(attribName, _numTransformsAttr) == 0){
				ist >> numRefinements;
				if (numRefinements > maxNumRefinements) maxNumRefinements = numRefinements;
			}
			
			else if (StrCmpNoCase(attribName, _terrainMapAttr) == 0){
				if (value == "true") setMappedToTerrain(true); 
				else setMappedToTerrain(false);
			}
			
			else if (StrCmpNoCase(attribName, _orientationAttr) == 0) {
				ist >> orientation;
			}
			else if (StrCmpNoCase(attribName, _georeferencedAttr) == 0) {
				if (value == "true") setGeoreferenced(true);
				else setGeoreferenced(false);
			}
			else if (StrCmpNoCase(attribName, _cropImageAttr) == 0) {
				if (value == "true") setImageCrop(true);
				else setImageCrop(false);
			}
			else if (StrCmpNoCase(attribName, _imagePlacementAttr) == 0) {
				ist >> imagePlacement;
			}
			else if (StrCmpNoCase(attribName, _resampleRateAttr) == 0) {
				ist >> resampRate;
			}
			else if (StrCmpNoCase(attribName, _opacityMultAttr) == 0) {
				ist >> opacityMultiplier;
			}
			else if (StrCmpNoCase(attribName, _imageFileNameAttr) == 0) {
				imageFileName = value;
			}
			
			else return false;
		}
		return true;
	}
	
	//Parse the geometry node
	else if (StrCmpNoCase(tagString, _geometryTag) == 0) {
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _twoDMinAttr) == 0) {
				ist >> twoDMin[0];ist >> twoDMin[1];ist >> twoDMin[2];
			}
			else if (StrCmpNoCase(attribName, _twoDMaxAttr) == 0) {
				ist >> twoDMax[0];ist >> twoDMax[1];ist >> twoDMax[2];
			}
			else if (StrCmpNoCase(attribName, _cursorCoordsAttr) == 0) {
				ist >> cursorCoords[0];ist >> cursorCoords[1];
			}
			else return false;
		}
		return true;
	}
	else return false;
}
//The end handler needs to pop the parse stack, not much else
bool TwoDImageParams::
elementEndHandler(ExpatParseMgr* pm, int depth , std::string& tag){
	
	if (StrCmpNoCase(tag, _twoDImageParamsTag) == 0) {
		
		//If this is a TwoDImageparams, need to
		//pop the parse stack.  The caller will need to save the resulting
		//transfer function (i.e. this)
		ParsedXml* px = pm->popClassStack();
		bool ok = px->elementEndHandler(pm, depth, tag);
		return ok;
	} else if (StrCmpNoCase(tag, _geometryTag) == 0){
		return true;
	} else {
		pm->parseError("Unrecognized end tag in TwoDImageParams %s",tag.c_str());
		return false; 
	}
}

//Method to construct Xml for state saving
XmlNode* TwoDImageParams::
buildNode() {
	//Construct the twoD node
	string empty;
	std::map <string, string> attrs;
	attrs.clear();
	
	ostringstream oss;

	oss.str(empty);
	oss << (long)vizNum;
	attrs[_vizNumAttr] = oss.str();

	oss.str(empty);
	oss << (long)numRefinements;
	attrs[_numTransformsAttr] = oss.str();

	oss.str(empty);
	if (isMappedToTerrain())
		oss << "true";
	else 
		oss << "false";
	attrs[_terrainMapAttr] = oss.str();


	oss.str(empty);
	if (isGeoreferenced())
		oss << "true";
	else 
		oss << "false";
	attrs[_georeferencedAttr] = oss.str();

	oss.str(empty);
	if (imageCrop())
		oss << "true";
	else 
		oss << "false";
	attrs[_cropImageAttr] = oss.str();

	oss.str(empty);
	oss << (long)orientation;
	attrs[_orientationAttr] = oss.str();

	oss.str(empty);
	oss << (double)resampRate;
	attrs[_resampleRateAttr] = oss.str();

	oss.str(empty);
	oss << (long)imagePlacement;
	attrs[_imagePlacementAttr] = oss.str();

	oss.str(empty);
	oss << (double) opacityMultiplier;
	attrs[_opacityMultAttr] = oss.str();

	attrs[_imageFileNameAttr] = imageFileName;
	
	XmlNode* twoDNode = new XmlNode(_twoDImageParamsTag, attrs, 3);

	//Now add children:  
	//Now do geometry node:
	attrs.clear();
	oss.str(empty);
	oss << (double)twoDMin[0]<<" "<<(double)twoDMin[1]<<" "<<(double)twoDMin[2];
	attrs[_twoDMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)twoDMax[0]<<" "<<(double)twoDMax[1]<<" "<<(double)twoDMax[2];
	attrs[_twoDMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	twoDNode->NewChild(_geometryTag, attrs, 0);
	return twoDNode;
}



//Clear out the cache.  But in image mode, don't clear out the textures,
//just delete the elevation grid
void TwoDImageParams::setTwoDDirty(){
		setElevGridDirty(true);
		setAllBypass(false);
		return;
}
//In image mode, need to clear out cached info obtained from image file
void TwoDImageParams::setImagesDirty(){
	if (twoDDataTextures){
		for (int i = 0; i<=maxTimestep; i++){
			if (twoDDataTextures[i]) {
				delete twoDDataTextures[i];
				twoDDataTextures[i] = 0;
			}
		}
		twoDDataTextures = 0;
		delete imageExtents;
		delete textureSizes;
		imageExtents = 0;
		textureSizes = 0;
		if (imageNums) delete imageNums;
		imageNums = 0;
	}
	
	setElevGridDirty(true);
	setAllBypass(false);
}


//Calculate the twoD texture (if it needs refreshing).
//It's kept (cached) in the twoD params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
unsigned char* TwoDImageParams::
calcTwoDDataTexture(int ts, int texWidth, int texHeight){
	
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	if (doBypass(ts)) return 0;
	
	//if width and height are 0, then the image will
	//be of the size specified in the 2D params, and the result
	//will be placed in the cache. Otherwise we are just needing
	//an image of specified size for writing to file, not to be put in the cache.
	//
	bool doCache = (texWidth == 0 && texHeight == 0);

	//we need to read the current timestep
	//of image into the texture for this timestep.  If there's only
	//one image, then we shall put it in position 0 in the cache.
	//If a map projection is undefined, invalid imageExts are returned
	// (i.e. imgExts[2]<imgExts[0])
	
		int wid, ht;
		float imgExts[4];
		int imgSize[2];
		unsigned char* img = readTextureImage(ts, &wid, &ht, imgExts);
		if (doCache && img) {
			
			imgSize[0] = wid;
			imgSize[1] = ht;
			setTwoDTexture(img,ts, imgSize, imgExts);
		}
		return img;
}




	

//Get texture from image file, set it in the cache

unsigned char* TwoDImageParams::
readTextureImage(int timestep, int* wid, int* ht, float imgExts[4]){
	
 
	//Initially set imgExts to the TwoDImage extents
	imgExts[0] = twoDMin[0];
	imgExts[1] = twoDMin[1];
	imgExts[2] = twoDMax[0];
	imgExts[3] = twoDMax[1];
	projDefinitionString = "";
    TIFF* tif = XTIFFOpen(imageFileName.c_str(), "r");

	if (!tif) {
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, 
			"Unable to open tiff file: %s\n",
			imageFileName.c_str());
		return 0;
	}
	//Set the tif directory to the one associated with the
	//current frame num.
	if (!imageNums) setupImageNums(tif);
	int currentDir = getImageNum(timestep);
	
	TIFFSetDirectory(tif, currentDir);
	
	uint32 w, h;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	

	if (DataStatus::getProjectionString().size() > 0){  //get a proj4 definition string if it exists, using geoTiff lib
		GTIF* gtifHandle = GTIFNew(tif);
		GTIFDefn* gtifDef = new GTIFDefn();
		int rc1 = GTIFGetDefn(gtifHandle,gtifDef);
		char* proj4String = GTIFGetProj4Defn(gtifDef);
		qWarning("proj4 string: %s",proj4String);
		projDefinitionString = proj4String;
		//Check it out..
		projPJ p = pj_init_plus(proj4String);
		int gotFields = false;
		double* padfTiePoints, *modelPixelScale;
		if (p) pj_free(p);
		if (!p && isGeoreferenced()){
			//Invalid string. Get the error code:
			int *pjerrnum = pj_get_errno_ref();
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Image is not properly geo-referenced\n %s\n",
				pj_strerrno(*pjerrnum));
		} else if (p && isGeoreferenced()) {

			//ModelTiepointTag and modelpixelscale needed to calculate image corners
			uint16 nCount;
			
			gotFields = TIFFGetField(tif,TIFFTAG_GEOTIEPOINTS,&nCount,&padfTiePoints);
		
			if(gotFields) gotFields = TIFFGetField(tif, TIFFTAG_GEOPIXELSCALE, &nCount, &modelPixelScale );
		
			if (gotFields){
				double pixelStart[2];
				pixelStart[0] = padfTiePoints[0];
				pixelStart[1] = h-1 - padfTiePoints[1];
				imgExts[0] = padfTiePoints[3]-pixelStart[0]*modelPixelScale[0];
				imgExts[1] = padfTiePoints[4]-pixelStart[1]*modelPixelScale[1];
				imgExts[2] = padfTiePoints[3]+(w-1-pixelStart[0])*modelPixelScale[0];
				imgExts[3] = padfTiePoints[4]+(h-1-pixelStart[1])*modelPixelScale[1];
			} else {
				//Inadequate georeferencing:
				MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Image is not geo-referenced\n");
			}
		}
	}
	//Following is valid whether or not we are georeferenced:
	size_t npixels = w * h;
	
	uint32* texture = new unsigned int[npixels];
	if (texture != NULL) {
		if (TIFFReadRGBAImage(tif, w, h, texture, 0)) {
			*wid = w;
			*ht = h;
			//May need to resample here!
		}
		else {
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Error reading tiff file:\n %s\n",
				imageFileName.c_str());
			delete texture;
			texture = 0;
		}
	}

	TIFFClose(tif);
	//apply opacity multiplier
	if (opacityMultiplier < 1){
		for (int i = 0; i < w*h; i++){
			unsigned int rgba = texture[i];
			//mask the alpha
			int alpha = (0xff000000 & texture[i])>>24;
			alpha = (int)(alpha*opacityMultiplier);
			alpha <<= 24;
			texture[i] = ((rgba&0xffffff) | alpha);
		}
	}
	return (unsigned char*) texture;
}
void TwoDImageParams::setupImageNums(TIFF* tif){
	//Initialize to zeroes
	imageNums = new int[maxTimestep + 1];
	for (int i = 0; i<=maxTimestep; i++) imageNums[i] = 0;
	int rc;
	char* timePtr = 0;
	int dircount = 0;
	TIME64_T wrfTime;
	//Check if the first time step has a time stamp
	
	//const TIFFFieldInfo*  tfield = TIFFFindFieldInfo(tif, TIFFTAG_DATETIME, TIFF_ASCII);
	//  For some reason the following can crash on windows in TIFFFindFieldInfo:
	bool timesOK = TIFFGetField(tif,TIFFTAG_DATETIME,&timePtr);
	
	vector <TIME64_T> tiffTimes;
	if (timesOK) { //build a list of the times in the tiff
		do {
			dircount++;
			rc = TIFFGetField(tif,TIFFTAG_DATETIME,&timePtr);
			if (!rc) {
				timesOK = false;
				break;
			} else {
				//determine seconds from the time stamp in the tiff
				string tifftime(timePtr);
				TIME64_T seconds = 0;
				rc = WRF::WRFTimeStrToEpoch(tifftime, &seconds);
				if (rc) {
					timesOK = false;
					break;
				}
				else {
					tiffTimes.push_back(seconds);
				}
			}
		} while (TIFFReadDirectory(tif));
	}
	if (timesOK) {
		//get the user times from the metadata,
		//and find the min difference for each usertime.
		
		Metadata* md = DataStatus::getInstance()->getMetadata();
	
		for (int i = 0; i<=maxTimestep; i++){
			const vector<double>& d = md->GetTSUserTime((size_t)i);
			if(d.size()==0){ timesOK = false; break;}
			wrfTime = (TIME64_T)d[0];
			//Find the nearest tifftime:
			TIME64_T minTimeDiff = 123456789123456789LL;
			int bestpos = -1;
			for (int j = 0; j<tiffTimes.size(); j++){
				TIME64_T timediff = (wrfTime > tiffTimes[j]) ? (wrfTime - tiffTimes[j]) : (tiffTimes[j] - wrfTime);
				if (timediff < minTimeDiff){
					bestpos = j;
					minTimeDiff = timediff;
				}
				if (minTimeDiff == 0) break;
			}
			imageNums[i] = bestpos;
		}
	}
	
	if (timesOK){
		qWarning("%d directories in %s\n", dircount, imageFileName.c_str());
		return;
	} else { //Don't use time stamps, just count the images:
		dircount = 0;
		do {
			dircount++;
		} while (TIFFReadDirectory(tif));
		qWarning("%d directories in %s\n", dircount, imageFileName.c_str());
		for (int i = 0; i<= maxTimestep; i++){
			imageNums[i] = Min(dircount-1,i);
		}
	}
		
	return;
}
//Determine the image corners (from lower-left, clockwise) in local coords.
bool TwoDImageParams::getImageCorners(int timestep, double displayCorners[8]){

	
	const float* imgExts = getCurrentTwoDImageExtents(timestep);
	if (!imgExts) return false;
	//Set up proj.4 to convert from image space to VDC coords
	projPJ dst_proj;
	projPJ src_proj; 
	
	src_proj = pj_init_plus(getImageProjectionString().c_str());
	dst_proj = pj_init_plus(DataStatus::getProjectionString().c_str());
	bool doProj = (src_proj != 0 && dst_proj != 0);
	if (!doProj) return false;

	//If a projection string is latlon, the coordinates are in Radians!
	bool latlonSrc = pj_is_latlong(src_proj);
	bool latlonDst = pj_is_latlong(dst_proj);

	static const double RAD2DEG = 180./M_PI;
	static const double DEG2RAD = M_PI/180.0;
	

	//copy x and y coords:
	// 0,1,2,3 go to 0, 0, 2, 2 in x
	// 0,1,2,3 go to 1, 3, 3, 1 in y
	for (int i = 0; i<4; i++){
		displayCorners[2*i] = imgExts[2*(i/2)];
		displayCorners[2*i+1] = imgExts[(1 + 2*((i+1)/2))%4];
	}
	
	//Convert image extents to Vapor coordinates, just to find
	//size of region we are dealing with
	if (latlonSrc){ //need to convert degrees to radians, image exts are in degrees
		for (int i = 0; i<8; i++) displayCorners[i] *= DEG2RAD;
	}
	
	//The above are LL and UR coords.  
	//apply proj4 to transform the four corners (in place):
	int rc = pj_transform(src_proj,dst_proj,4,2, displayCorners,displayCorners+1, 0);

	if (rc){
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, "Error in coordinate projection: \n%s",
			pj_strerrno(rc));
		return false;
	}
	if (latlonDst)  //results are in radians, convert to degrees
		for (int i = 0; i<8; i++) displayCorners[i] *= RAD2DEG;
	
	//Now displayCorners are corners in projection space.  subtract offsets:
	const float* exts = DataStatus::getExtents(timestep);
	const float* globExts = DataStatus::getInstance()->getExtents();
	for (int i = 0; i<8; i++) displayCorners[i] -= (exts[i%2] - globExts[i%2]);
	return true;
	
}