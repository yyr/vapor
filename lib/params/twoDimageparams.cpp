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
#ifdef _WINDOWS
#define _USE_MATH_DEFINES
#pragma warning(disable : 4251 4100)
#endif
#include <cmath>
//#include <netcdf.h>
#include "twoDparams.h"
#include "twoDimageparams.h"
#include "regionparams.h"
#include "params.h"
#include "transferfunction.h"

#include "histo.h"
#include "animationparams.h"
#include "viewpointparams.h"

#include <vapor/DataMgr.h>
#include <vapor/errorcodes.h>
#include <vapor/UDUnitsClass.h>
#include "sys/stat.h"
#include "GeoTileEquirectangular.h"
//tiff stuff:

#include "geo_normalize.h"
#include "geotiff.h"
#include "xtiffio.h"
#include "proj_api.h"


using namespace VAPoR;
const string TwoDImageParams::_shortName = "Image";
const string TwoDImageParams::_imagePlacementAttr = "ImagePlacement";
const string TwoDImageParams::_cropImageAttr = "CropImage";
const string TwoDImageParams::_georeferencedAttr = "Georeferenced";
const string TwoDImageParams::_resampleRateAttr = "ResamplingRate";
const string TwoDImageParams::_opacityMultAttr = "OpacityMultiplier";
const string TwoDImageParams::_imageFileNameAttr = "ImageFileName";

TwoDImageParams::TwoDImageParams(int winnum) : TwoDParams(winnum, Params::_twoDImageParamsTag){
	
	_imageExtents[0] = _imageExtents[1] = _imageExtents[2] = _imageExtents[3] = 0.0;
	_timestep = -1;
	_lonlatexts[0] = _lonlatexts[1] = _lonlatexts[2] = _lonlatexts[3] = 0.0;
	_textureSizes[0] = _textureSizes[1] = 0;
	_texBuf = NULL;
	_geotiffImage = false;

	imageNums = 0;
	maxTimestep = 1;
	linearInterp = false;
	restart();
	
}

TwoDImageParams::~TwoDImageParams(){
	
	
	if (_texBuf) delete [] _texBuf;
	if (imageNums) delete [] imageNums;
	imageNums = 0;
}


//Deepcopy requires cloning tf 
Params* TwoDImageParams::
deepCopy(ParamNode*){
	TwoDImageParams* newParams = new TwoDImageParams(*this);
	ParamNode* pNode = new ParamNode(*(myBox->GetRootNode()));
	newParams->myBox = (Box*)myBox->deepCopy(pNode);
	//TwoD texture must be recreated when needed
	newParams->imageNums = 0;
	_timestep = -1;
	_geotiffImage = false;
	newParams->_texBuf = NULL;
	
	return newParams;
}







//Initialize for new metadata.  
//
bool TwoDImageParams::
reinit(bool doOverride){
	
	DataStatus* ds = DataStatus::getInstance();
	const float* extents = ds->getLocalExtents();
	setMaxNumRefinements(ds->getNumTransforms());
	//Set up the numRefinements combo
	//Either set the twoD bounds to default (full) size in the center of the domain, or 
	//try to use the previous bounds:
	double twoDExtents[6];
	if (doOverride){
		for (int i = 0; i<3; i++){
			float twoDRadius = 0.5f*(extents[i+3]);
			float twoDMid = 0.5f*(extents[i+3]);
			if (i<2) {
				twoDExtents[i] = 0.;
				twoDExtents[i+3] = twoDMid + twoDRadius;
			} else {
				twoDExtents[i] = twoDExtents[i+3] = twoDMid;
			}
		}
		
		cursorCoords[0] = cursorCoords[1] = 0.0f;
		numRefinements = 0;
	} else {
		//Just force the mins to be less than the max's
		//There is no constraint on size or position
		GetBox()->GetLocalExtents(twoDExtents);
		if (DataStatus::pre22Session()){
			//In old session files, the coordinate of box extents were not 0-based
			float * offset = DataStatus::getPre22Offset();
			for (int i = 0; i<3; i++) {
				twoDExtents[i] -= offset[i];
				twoDExtents[i+3] -= offset[i];
			}
		}
		for (int i = 0; i<3; i++){
			if(twoDExtents[i+3] < twoDExtents[i]) 
				twoDExtents[i+3] = twoDExtents[i];
		}
		if (numRefinements > maxNumRefinements) numRefinements = maxNumRefinements;
	}
	GetBox()->SetLocalExtents(twoDExtents);
	
	
	//Create new arrays to hold bounds 
	
	//If we are overriding previous values, delete the transfer functions, create new ones.
	//Set the map bounds to the actual bounds in the data
	//Use HGT as the height variable name, if it's there. If not just use the first 2d variable.
	minTerrainHeight = extents[2];
	maxTerrainHeight = extents[2];
	int hgtvarindex;
	
	if (doOverride){
		string varname = "HGT";
		hgtvarindex = ds->getActiveVarNum2D(varname);
		if (hgtvarindex < 0 && ds->getNumActiveVariables2D()>0) {
			varname = ds->getVariableName2D(0);
			hgtvarindex=0;
		}
		SetHeightVariableName(varname);
	} else {
		string varname = GetHeightVariableName();
		hgtvarindex = ds->getActiveVarNum2D(varname);
		if (hgtvarindex < 0 && ds->getNumActiveVariables2D()>0) {
			varname = ds->getVariableName2D(0);
			SetHeightVariableName(varname);
			hgtvarindex = 0;
		}
	}
	maxTerrainHeight = ds->getDefaultDataMax2D(hgtvarindex);
	if (ds->getNumActiveVariables2D() <= 0) setMappedToTerrain(false);
	
	
	
	// set up the texture cache
	setTwoDDirty();
	if(imageNums) delete [] imageNums;
	imageNums = 0;
	
	maxTimestep = DataStatus::getInstance()->getNumTimesteps()-1;
	initializeBypassFlags();
	return true;
}
//Initialize to default state
//
void TwoDImageParams::
restart(){
	if (!myBox){
		myBox = new Box();
	}
	transparentAlpha = false;
	imagePlacement = 0;
	singleImage = false;
	mapToTerrain = false;
	minTerrainHeight = 0.f;
	maxTerrainHeight = 0.f;
	orientation = 2;
	compressionLevel = 0;
	setTwoDDirty();
	if (imageNums) delete [] imageNums;
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
	double twoDExtents[6];
	for (int i = 0; i<3; i++){
		if (i < 2) twoDExtents[i] = 0.0f;
		else twoDExtents[i] = 0.5f;
		if(i<2) twoDExtents[i+3] = 1.0f;
		else twoDExtents[i+3] = 0.5f;
		selectPoint[i] = 0.5f;
	}
	GetBox()->SetLocalExtents(twoDExtents);
	heightVariableName = "HGT";
}

//Handlers for Expat parsing.
//
bool TwoDImageParams::
elementStartHandler(ExpatParseMgr* pm, int depth , std::string& tagString, const char **attrs){
	
	if (StrCmpNoCase(tagString, _twoDImageParamsTag) == 0) {
		//Set defaults 
		SetIgnoreFidelity(true);
		SetFidelityLevel(0.5f);
		resampRate = 1.f;
		opacityMultiplier = 1.f;
		useGeoreferencing = true;
		cropImage = false;
		imageFileName = "";
		orientation = 2; //X-Y aligned
		imagePlacement = 0;
		heightVariableName = "HGT";

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
			else if (StrCmpNoCase(attribName, _CompressionLevelTag) == 0){
				ist >> compressionLevel;
			}
			else if (StrCmpNoCase(attribName, _FidelityLevelTag) == 0){
				float fid;
				ist >> fid;
				SetFidelityLevel(fid);
			}
			else if (StrCmpNoCase(attribName, _IgnoreFidelityTag) == 0){
				if (value == "true") SetIgnoreFidelity(true); 
				else SetIgnoreFidelity(false);
			}
			else if (StrCmpNoCase(attribName, _terrainMapAttr) == 0){
				if (value == "true") setMappedToTerrain(true); 
				else setMappedToTerrain(false);
			}
			else if (StrCmpNoCase(attribName, _heightVariableAttr) == 0) {
				heightVariableName = value;
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
			
		}
		return true;
	}
	
	//Parse the geometry node
	else if (StrCmpNoCase(tagString, _geometryTag) == 0) {
		float box[6];
		GetBox()->GetLocalExtents(box);
		while (*attrs) {
			string attribName = *attrs;
			attrs++;
			string value = *attrs;
			attrs++;
			istringstream ist(value);
			if (StrCmpNoCase(attribName, _twoDMinAttr) == 0) {
				ist >> box[0];ist >> box[1];ist >> box[2];
				GetBox()->SetLocalExtents(box);
			}
			else if (StrCmpNoCase(attribName, _twoDMaxAttr) == 0) {
				ist >> box[3];ist >> box[4];ist >> box[5];
				GetBox()->SetLocalExtents(box);
			}
			else if (StrCmpNoCase(attribName, _cursorCoordsAttr) == 0) {
				ist >> cursorCoords[0];ist >> cursorCoords[1];
			}
		}
		return true;
	}
	pm->skipElement(tagString, depth);
	return true;
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
ParamNode* TwoDImageParams::
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
	oss << (long)compressionLevel;
	attrs[_CompressionLevelTag] = oss.str();
	oss.str(empty);
	oss << (double)GetFidelityLevel();
	attrs[_FidelityLevelTag] = oss.str();
	oss.str(empty);
	if (GetIgnoreFidelity())
		oss << "true";
	else 
		oss << "false";
	attrs[_IgnoreFidelityTag] = oss.str();
	oss.str(empty);
	if (isMappedToTerrain())
		oss << "true";
	else 
		oss << "false";
	attrs[_terrainMapAttr] = oss.str();

	attrs[_heightVariableAttr] = heightVariableName;

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
	
	ParamNode* twoDNode = new ParamNode(_twoDImageParamsTag, attrs, 3);

	//Now add children:  
	//Now do geometry node:
	attrs.clear();
	oss.str(empty);
	const vector<double>& twoDExtents = GetBox()->GetLocalExtents();
	oss << (double)twoDExtents[0]<<" "<<(double)twoDExtents[1]<<" "<<(double)twoDExtents[2];
	attrs[_twoDMinAttr] = oss.str();
	oss.str(empty);
	oss << (double)twoDExtents[3]<<" "<<(double)twoDExtents[4]<<" "<<(double)twoDExtents[5];
	attrs[_twoDMaxAttr] = oss.str();
	oss.str(empty);
	oss << (double)cursorCoords[0]<<" "<<(double)cursorCoords[1];
	attrs[_cursorCoordsAttr] = oss.str();
	twoDNode->NewChild(_geometryTag, attrs, 0);
	return twoDNode;
}

//Clear out the cache.  But don't clear out the textures,
//just delete the elevation grid
void TwoDImageParams::setTwoDDirty(){
		setElevGridDirty(true);
		setAllBypass(false);
		return;
}
//clear out cached images
void TwoDImageParams::setImagesDirty(){
	if (imageNums) delete [] imageNums;
	imageNums = 0;
	setElevGridDirty(true);
	setAllBypass(false);
	singleImage = false;
	transparentAlpha = false;
	_timestep = -1;
	_lonlatexts[0] = _lonlatexts[1] = _lonlatexts[2] = _lonlatexts[3] = 0.0;
}


//Calculate the twoD texture (if it needs refreshing).
//It's kept (cached) in the twoD params
//If nonzero texture dimensions are provided, then the cached image
//is not affected 
const unsigned char* TwoDImageParams::
calcTwoDDataTexture(int ts, int &texWidth, int &texHeight){
	
	if (!isEnabled()) return 0;
	DataStatus* ds = DataStatus::getInstance();
	if (!ds->getDataMgr()) return 0;
	if (doBypass(ts)) return 0;

	if (! twoDIsDirty(ts)) {
		texWidth = _textureSizes[0];
		texHeight = _textureSizes[1];
		return(_texBuf); 
	}
	
	//we need to read the current timestep
	//of image into the texture for this timestep.  
	//If a map projection is undefined, invalid imageExts are returned
	// (i.e. imgExts[2]<imgExts[0])
	
	_texBuf = readTextureImage(
		ts, &_textureSizes[0], &_textureSizes[1], _imageExtents
	);
	_timestep = ts;
	texWidth = _textureSizes[0];
	texHeight = _textureSizes[1];
	return _texBuf;
}

//Get texture from image file, set it in the cache

const unsigned char* TwoDImageParams::
readTextureImage(int timestep, int* wid, int* ht, float imgExts[4]){
	
	static const basic_string <char>::size_type npos = (size_t)-1;
	const vector<double>& boxExts = GetBox()->GetLocalExtents();
	//Initially set imgExts to the TwoDImage extents
	imgExts[0] = boxExts[0];
	imgExts[1] = boxExts[1];
	imgExts[2] = boxExts[3];
	imgExts[3] = boxExts[4];
	projDefinitionString = "";
	//Check for a valid file name (this avoids Linux crash):
	struct STAT64 statbuf;
	if (STAT64(imageFileName.c_str(), &statbuf) < 0) {
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, 
			"Invalid tiff file: %s\n",
			imageFileName.c_str());
		return 0;
	}
	//Not using memory-mapped IO (m) is reputed to help plug leaks (but doesn't do any good on windows for me)
    TIFF* tif = XTIFFOpen(imageFileName.c_str(), "rm");
	_geotiffImage = isGeoTIFF(tif);
	
	if (!tif) {
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, 
			"Unable to open tiff file: %s\n",
			imageFileName.c_str());
		return 0;
	}
	
	char emsg[1000];
	int ok = TIFFRGBAImageOK(tif,emsg);
	if (!ok){
		MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Unable to process tiff file:\n %s\nError message: %s",
				imageFileName.c_str(),emsg);
		return 0;
	} 
	//Check compression.  Some compressions, e.g. jpeg, cause crash on Linux
	short compr = 1;
	int rc = TIFFGetField(tif, TIFFTAG_COMPRESSION, &compr);
	if (rc){
		if (compr != COMPRESSION_NONE &&
			compr != COMPRESSION_LZW &&
			compr != COMPRESSION_JPEG &&
			compr != COMPRESSION_CCITTRLE){
				MyBase::SetErrMsg(VAPOR_ERROR_TWO_D,
					"Unsupported Tiff compression");
				return 0;
			}
	}
	//Set the tif directory to the one associated with the
	//current frame num.
	if (!imageNums) setupImageNums(tif);
	int currentDir = getImageNum(timestep);
	
	TIFFSetDirectory(tif, currentDir);
	
	uint32 w, h;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	
	size_t npixels = w * h;
	*wid = w;
	*ht = h;
	uint32* texture = new uint32[npixels];
	//Check if this is a 2-component 8-bit image.  These are read by scanline since TIFFReadRGBAImage
	//apparently does not know how to get the alpha channel
	short nsamples, nbitspersample;
	TIFFGetField(tif, TIFFTAG_SAMPLESPERPIXEL, &nsamples);
	TIFFGetField(tif, TIFFTAG_BITSPERSAMPLE, &nbitspersample);
	if (nsamples == 2 && nbitspersample == 8) {
		
		tdata_t buf;
		uint32 row;
		short config;
		short photometric;
		TIFFGetField(tif, TIFFTAG_PLANARCONFIG, &config);
		TIFFGetField(tif, TIFFTAG_PHOTOMETRIC,&photometric);
		buf = _TIFFmalloc(TIFFScanlineSize(tif));
		unsigned char* charArray = (unsigned char*)buf;
		int scanlength = TIFFScanlineSize(tif)/2;
		
		if (config == PLANARCONFIG_CONTIG) {
			
			for (row = 0; row < h; row++){
				int revrow = h-row-1;  //reverse, go bottom up
				TIFFReadScanline(tif, buf, row);
				for (int k = 0; k<scanlength; k++){
					unsigned char greyval = charArray[2*k];
					//If white is zero, reverse it:
					if (!photometric) greyval = 255 - greyval;
					unsigned char alphaval = charArray[2*k+1];
					if (alphaval != 0xff) transparentAlpha = true;
					texture[revrow*w+k] = alphaval<<24 | greyval<<16 | greyval <<8 | greyval;
				}
			}
		} else if (config == PLANARCONFIG_SEPARATE) {
			uint16 s;
			//Note: following loop (adapted from tiff docs) has not been tested.  Are there tiff
			//examples with separate alpha channel?
			for (s = 0; s < nsamples; s++){
				for (row = 0; row < h; row++){
					TIFFReadScanline(tif, buf, row, s);
					int revrow = h-row-1;  //reverse, go bottom up
					if (!(s%2)){ //color
						for (int k = 0; k<h; k++){
							unsigned char greyval = charArray[k];
							//If white is zero, reverse it:
							if (!photometric) greyval = 255 - greyval;
							texture[revrow*w+k] = greyval<<16 | greyval <<8 | greyval;
						}
					} else { // alpha
						for (int k = 0; k<h; k++){
							unsigned char alphaval = charArray[k];
							if (alphaval != 0xff) transparentAlpha = true;
							texture[revrow*w+k] = alphaval<<24 | (texture[revrow*w+k] && 0xffffff);
						}
					}
				}
			}
		}
		_TIFFfree(buf);
	

	} else {
	//Read pixels, whether or not we are georeferenced:
	
		if (texture != NULL) {
			
			if (TIFFReadRGBAImage(tif, w, h, texture, 0)) {
				
				*wid = w;
				*ht = h;
				//May need to resample here!
			}
			else {
				MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Error reading tiff file:\n %s\n",
					imageFileName.c_str());
				delete [] texture;
				return 0;
			}
		}
		//Check for nontrivial alpha:
		if (!transparentAlpha){
			for (int i = 0; i<npixels; i++){
				//Check if leading byte is not 255
				if (texture[i] < (255<<24)) {
					transparentAlpha = true;
					break;
				}
			}
		}
	
	}
	
	
	//Check for georeferencing:
	projPJ p = 0;
	if (DataStatus::getProjectionString().size() > 0 && _geotiffImage){  //get a proj4 definition string if it exists, using geoTiff lib
		GTIF* gtifHandle = GTIFNew(tif);
		GTIFDefn* gtifDef = new GTIFDefn();
		//int rc1 = 
		GTIFGetDefn(gtifHandle,gtifDef);
		GTIFFree(gtifHandle);
		const char* proj4String = GTIFGetProj4Defn(gtifDef);
		const char* newString;
		// If there's no "ellps=" in the string, force it to be spherical,
		// This avoids a bug in the geotiff routines
		std::string p4String(proj4String);
		if (npos == p4String.find("ellps=")){
			p4String += " +ellps=sphere";
			newString = p4String.c_str();
		} else {
			newString = proj4String;
		}

		//qWarning("proj4 string: %s",newString);
		MyBase::SetDiagMsg("proj4 string: %s in image file %s",newString,imageFileName.c_str());
		setImageProjectionString(newString);

		//Check it out..
		p = pj_init_plus(newString);
		int gotFields = false;
		double* padfTiePoints, *modelPixelScale;
		if (!p && isGeoreferenced()){
			//Invalid string. Get the error code:
			int *pjerrnum = pj_get_errno_ref();
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Image at timestep %d is not properly georeferenced,\nand will not be displayed in georeferenced mode. \nError message:\n %s\n",
				timestep, pj_strerrno(*pjerrnum));
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
				if (pj_is_latlong(p)){
					if (imgExts[1]<= -89.9999) imgExts[1] = -89.9999;
					if (imgExts[3] >= 89.9999) imgExts[3] = 89.9999;
				}
				//Don't allow latlong image to go all the way to the poles
			} else {
				//Inadequate georeferencing:
				MyBase::SetErrMsg(VAPOR_WARNING_TWO_D, "Image is not geo-referenced\n");
			}
		}
	}
	

	XTIFFClose(tif);
	//apply opacity multiplier
	if (texture && opacityMultiplier < 1){
		for (int i = 0; i < w*h; i++){
			unsigned int rgba = texture[i];
			//mask the alpha
			int alpha = (0xff000000 & texture[i])>>24;
			alpha = (int)(alpha*opacityMultiplier);
			alpha <<= 24;
			texture[i] = ((rgba&0xffffff) | alpha);
		}
	}
	//Check if we should extract a smaller tile;
	if(p && isGeoreferenced()&& pj_is_latlong(p) && 
		(imgExts[0] < -179. && imgExts[1]<-88. && imgExts[2]>179. && imgExts[3]>88.)){
	
		if (getLonLatExts((size_t)timestep, _lonlatexts)){
			//const size_t* dataSize = DataStatus::getInstance()->getFullDataSize();
			
			//OK, we need to extract a sub-image, that maps to the current lon-lat extents
			int wid2 = *wid, ht2 = *ht;
			//Choose a level of detail so the image is not too large. 
			//If we do no reduction of level of detail the image size will be proportional to the fraction of latlonexts:
			//int xdim = (int) ((_lonlatexts[2]-_lonlatexts[0])*(float)w/360.);
			//int ydim = (int) ((_lonlatexts[3]-_lonlatexts[1])*(float)h/180.);
			//int minMag = (int)Min((float)xdim/(float)dataSize[0],(float)ydim/(float)dataSize[1]);
			int lev = 0;
			//if (minMag > 1) lev = VetsUtil::ILog2(minMag);
			unsigned char* subTexture = extractSubtexture((unsigned char*)texture, _lonlatexts, &wid2, &ht2, lev);
			if (subTexture){
				*wid = wid2;
				*ht = ht2;
				for (int i = 0; i<4; i++) imgExts[i] = _lonlatexts[i];
				delete [] texture;
				if (p) pj_free(p);
				return subTexture;
			}
		}

		
	}
	if (p) pj_free(p);
	return (unsigned char*) texture;
}

bool TwoDImageParams::twoDIsDirty(int timestep) {
	if (timestep != _timestep) return (true);
		
	if (DataStatus::getProjectionString().size() > 0 && _geotiffImage){
		float lonlatexts[4];
		if (getLonLatExts((size_t)timestep, lonlatexts)){
			for (int i=0; i<4; i++) {
				if (lonlatexts[i] != _lonlatexts[i]) return(true);
			}
		}
	}
	return(false);
}
bool TwoDImageParams::isGeoTIFF(TIFF *tif) const {
		GTIF* gtifHandle = GTIFNew(tif);
		GTIFDefn gtifDef;
		GTIFGetDefn(gtifHandle,&gtifDef);
		GTIFFree(gtifHandle);
		string proj4String = GTIFGetProj4Defn(&gtifDef);

		return(! proj4String.empty());
}


unsigned char* TwoDImageParams::extractSubtexture(unsigned char* texture, float lonlatexts[4], int* wid2, int* ht2, int lev){
	GeoTileEquirectangular geotile(*wid2, *ht2,4);
	geotile.Insert("",texture);
	size_t pixelSW[2];
	size_t pixelNE[2];
	size_t nx,ny;

	geotile.LatLongToPixelXY((double)lonlatexts[0],(double)lonlatexts[1],lev, pixelSW[0], pixelSW[1]);
	geotile.LatLongToPixelXY((double)lonlatexts[2],(double)lonlatexts[3],lev, pixelNE[0], pixelNE[1]);
	int rc = geotile.MapSize(pixelSW[0],pixelSW[1],pixelNE[0],pixelNE[1],lev,nx, ny);
	if (rc != 0) return 0;
	unsigned char* dstImage = new unsigned char[nx*ny*4];
	rc = geotile.GetMap(pixelSW[0],pixelSW[1],pixelNE[0],pixelNE[1],lev,dstImage);
	if (rc != 0) return 0;
	*wid2 = (int)nx;
	*ht2 = (int)ny;
	return dstImage;
}
bool TwoDImageParams::getLonLatExts(size_t timestep, float lonlatexts[4]){
	for (int j = 0; j<4; j++) lonlatexts[j] = 0.;
	Box* box = GetBox();
	double extents[6];
	box->GetUserExtents(extents, timestep);
	//Set up proj.4 to convert from VDC coords to latlon (image space)
	projPJ dst_proj;
	projPJ src_proj; 
	
	dst_proj = pj_init_plus(getImageProjectionString().c_str());
	src_proj = pj_init_plus(DataStatus::getProjectionString().c_str());
	bool doProj = (src_proj != 0 && dst_proj != 0);
	if (!doProj) {
		MyBase::SetErrMsg(VAPOR_ERROR_GEOREFERENCE, "Invalid Proj string in VDC or image");
		return false;
	}
	if (!pj_is_latlong(dst_proj)) return false;

	//If a projection string is latlon, or the coordinates are in Radians!
	bool radDst = true;
	bool radSrc = pj_is_latlong(src_proj)||(string::npos != DataStatus::getProjectionString().find("ob_tran"));

	static const double RAD2DEG = 180./M_PI;
	static const double DEG2RAD = M_PI/180.0;

	double interpPoints[128];
	//interpolate 8 points on each row of image
	for (int row = 0; row <8; row++){
		for (int col = 0; col<8; col++){
			double u = ((double)col)/7.;
			double v = ((double)row)/7.;
			interpPoints[16*row + 2*col] = (1.-v)*extents[0]+v*extents[3];
			interpPoints[16*row + 2*col+1] = (1.-u)*extents[1]+u*extents[4];

		}
	}
	
	if (radSrc){ //need to convert meters to degrees, then degrees to radians, 
		for (int i = 0; i<128; i++) interpPoints[i] *= (DEG2RAD/111177.);
	}
	//apply proj4 to transform the points(in place):
	int rc = pj_transform(src_proj,dst_proj,64,2, interpPoints,interpPoints+1, 0);

	if (rc){
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, "Error in coordinate projection: \n%s",
			pj_strerrno(rc));
		return false;
	}
	if (radDst) { //results are in radians, convert to degrees
		for (int i = 0; i<128; i++) interpPoints[i] *= RAD2DEG;
	}

	//Now find the extents, by looking at min, max x and y in projected space:
	double minx = 1.e30, miny = 1.e30;
	double maxx = -1.e30, maxy = -1.e30;
	for (int i = 0; i<64; i++){
		if (minx > interpPoints[2*i]) minx = interpPoints[2*i];
		if (miny > interpPoints[2*i+1]) miny = interpPoints[2*i+1];
		if (maxx < interpPoints[2*i]) maxx = interpPoints[2*i];
		if (maxy < interpPoints[2*i+1]) maxy = interpPoints[2*i+1];
	}
	if (minx < -1.e30 || miny < -1.e30 || maxx > 1.e30 || maxy > 1.e30){
		MyBase::SetErrMsg(VAPOR_ERROR_GEOREFERENCE,
			"Map projection error:  current projection space won't map to degrees \n%s",
			"Image extents may be too great.");  
		return false;
	}

	lonlatexts[0] = minx;
	lonlatexts[1] = miny;
	lonlatexts[2] = maxx;
	lonlatexts[3] = maxy;
	return true;

}
void TwoDImageParams::setupImageNums(TIFF* tif){
	//Initialize to zeroes
	imageNums = new int[maxTimestep + 1];
	//Set up array to hold time differences:
	TIME64_T *timeDifference = new TIME64_T[maxTimestep+1];
	for (int i = 0; i<=maxTimestep; i++) {
		imageNums[i] = 0;
		timeDifference[i] = 0;
	}
	
	singleImage = false;
	int rc;
	char* timePtr = 0;
	int dircount = 0;
	int numDiffImages = 0;
	TIME64_T wrfTime;
	//Check if the first time step has a time stamp
	
	//const TIFFFieldInfo*  tfield = TIFFFindFieldInfo(tif, TIFFTAG_DATETIME, TIFF_ASCII);
	//  For some reason the following can crash on windows in TIFFFindFieldInfo:
	bool timesOK = TIFFGetField(tif,TIFFTAG_DATETIME,&timePtr);
	
	vector <TIME64_T> tiffTimes;
	if (timesOK) { //build a list of the times in the tiff
		UDUnits udunits;
		udunits.Initialize();
		do {
			dircount++;
			rc = TIFFGetField(tif,TIFFTAG_DATETIME,&timePtr);
			if (!rc) {
				timesOK = false;
				break;
			} 

			// determine seconds from the time stamp in the tiff
			// convert tifftags to use WRF style date/time strings
			//
			const char *format = "%4d-%2d-%2d_%2d:%2d:%2d";
			int year, mon, mday, hour, min, sec;
			rc = sscanf(timePtr, format, &year, &mon, &mday, &hour, &min, &sec);
            if (rc != 6) {
				timesOK = false;
				break;
            }
            double seconds = udunits.EncodeTime(year, mon, mday, hour, min,sec);
			tiffTimes.push_back(seconds);

		} while (TIFFReadDirectory(tif));
	}
	if (timesOK) {
		//get the user times from the metadata,
		//and find the min difference for each usertime.
		
		DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	
		for (int i = 0; i<=maxTimestep; i++){
			double d = dataMgr->GetTSUserTime((size_t)i);
			wrfTime = (TIME64_T)d;
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
			timeDifference[i] = minTimeDiff;
		}
		//Count the number of different images that are referenced:
		
		for (int i = 0; i< maxTimestep; i++){
			if (imageNums[i] != imageNums[i+1]) numDiffImages++;
		}
		if (numDiffImages == 0) singleImage = true;
	}
	
	if (timesOK){
		//Issue a warning if there is only one image used, or if there are more images than timesteps.
		//Also if there are multiple images and some do not exactly match the times
		if (dircount > 1 && numDiffImages == 0)
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D,"%d images found in %s, but only 1 was matched to time stamps ", dircount, imageFileName.c_str());
		else
			MyBase::SetDiagMsg("%d images (%d mapped to time steps) in %s\n", dircount, numDiffImages+1, imageFileName.c_str());
		if (dircount > maxTimestep +1) {
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D,"Number of images %d in %s is greater than %d time steps in data.\n%s", dircount, imageFileName.c_str(),
				maxTimestep+1, "Only one image will be displayed at each time step");
		}
		if (dircount > 1 && numDiffImages > 0){//look for time mismatches
			int numUnmatched = 0;
			TIME64_T maxDiff = 0;
			for (int i = 0; i<maxTimestep; i++){
				if (timeDifference[i] != 0) numUnmatched++;
				if (timeDifference[i] > maxDiff) maxDiff = timeDifference[i];
			}
			if (maxDiff != 0){
				MyBase::SetErrMsg(VAPOR_WARNING_TWO_D,"Time mismatch in images from %s.\n %d time steps do not match image times.\n Maximum mismatch is %d seconds", 
					imageFileName.c_str(),
					numUnmatched, maxDiff);
			}
		}

	} else { //Don't use time stamps, just count the images:
		dircount = 0;
		do {
			dircount++;
		} while (TIFFReadDirectory(tif));
		MyBase::SetDiagMsg("%d images, unmatched to time stamps in %s\n", dircount, imageFileName.c_str());
		if (dircount > 1) {
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D,"Time stamps not found in file %s\n", imageFileName.c_str(),
				"Images will be applied in time step order");
		}
		for (int i = 0; i<= maxTimestep; i++){
			imageNums[i] = Min(dircount-1,i);
		}
		if(dircount<2) singleImage = true;
		if (dircount > maxTimestep +1) {
			MyBase::SetErrMsg(VAPOR_WARNING_TWO_D,"Number of images (%d) in %s is greater than %d time steps in data.\n%s", dircount, imageFileName.c_str(),
				maxTimestep+1, "At most one image will be displayed per time step");
		}
	}
	delete [] timeDifference;
	return;
}
//Determine the image corners (from lower-left, clockwise) in user coordinates.
bool TwoDImageParams::getImageCorners(double displayCorners[8]){

	
	const float* imgExts = getCurrentTwoDImageExtents();
	if (!imgExts) return false;
	//Set up proj.4 to convert from image space to VDC coords
	projPJ dst_proj;
	projPJ src_proj; 
	
	src_proj = pj_init_plus(getImageProjectionString().c_str());
	dst_proj = pj_init_plus(DataStatus::getProjectionString().c_str());
	bool doProj = (src_proj != 0 && dst_proj != 0);
	if (!doProj) {
		MyBase::SetErrMsg(VAPOR_ERROR_GEOREFERENCE, "Invalid Proj string in VDC or image");
		return false;
	}

	//If a projection string is latlon, or the coordinates are in Radians!
	bool radSrc = pj_is_latlong(src_proj)||(string::npos != getImageProjectionString().find("ob_tran"));
	bool radDst = pj_is_latlong(dst_proj)||(string::npos != DataStatus::getProjectionString().find("ob_tran"));

	static const double RAD2DEG = 180./M_PI;
	static const double DEG2RAD = M_PI/180.0;
	

	//In order to be sure that the image extents contain the full
	//image, we take 8 sample points on each edge of the image
	//and determine where they go in map projection space.  The
	//image extents are defined to be the smallest rectangle in 
	//projection space that contains all 64 sample points.
	//We can't just take the corners because sometimes their convex hull
	//in projection space does not contain the projected image.
	//For example in a polar stereo projection, one may choose an image
	//that goes around the earth from longitude -180 to 180.  The corners
	//of this image would all lie on the international date line.

	double interpPoints[64];
	//interpolate 8 points on each edge of image
	//First interpolate the y coordinate along left image edge,
	//then interpolate x coordinate along top, etc.
	for (int i = 0; i<8; i++){
		double u = ((double)i)/8.;
		//x coords are constant
		interpPoints[2*i] = imgExts[0];
		interpPoints[2*i+1] = (1.-u)*imgExts[1]+u*imgExts[3];
	}
	for (int i = 0; i<8; i++){
		double u = ((double)i)/8.;
		//y coords are constant
		interpPoints[16+2*i+1] = imgExts[3];
		interpPoints[16+2*i] = (1.-u)*imgExts[0]+u*imgExts[2];
	}
	for (int i = 0; i<8; i++){
		double u = ((double)i)/8.;
		//x coords are constant
		interpPoints[32+2*i] = imgExts[2];
		interpPoints[32+2*i+1] = (1.-u)*imgExts[3]+u*imgExts[1];
	}
	for (int i = 0; i<8; i++){
		double u = ((double)i)/8.;
		//y coords are constant
		interpPoints[48+2*i+1] = imgExts[1];
		interpPoints[48+2*i] = (1.-u)*imgExts[2]+u*imgExts[0];
	}
	if (radSrc){ //need to convert degrees to radians, image exts are in degrees
		for (int i = 0; i<64; i++) interpPoints[i] *= DEG2RAD;
	}
	//apply proj4 to transform the four corners (in place):
	int rc = pj_transform(src_proj,dst_proj,32,2, interpPoints,interpPoints+1, 0);

	if (rc){
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, "Error in coordinate projection: \n%s",
			pj_strerrno(rc));
		return false;
	}
	if (radDst) { //results are in radians, convert to degrees, then convert to meters at equator
		for (int i = 0; i<64; i++) interpPoints[i] *= (RAD2DEG*111177.);
	}

	//Now find the extents, by looking at min, max x and y in projected space:
	double minx = 1.e30, miny = 1.e30;
	double maxx = -1.e30, maxy = -1.e30;
	for (int i = 0; i<32; i++){
		if (minx > interpPoints[2*i]) minx = interpPoints[2*i];
		if (miny > interpPoints[2*i+1]) miny = interpPoints[2*i+1];
		if (maxx < interpPoints[2*i]) maxx = interpPoints[2*i];
		if (maxy < interpPoints[2*i+1]) maxy = interpPoints[2*i+1];
	}
	if (minx < -1.e30 || miny < -1.e30 || maxx > 1.e30 || maxy > 1.e30){
		MyBase::SetErrMsg(VAPOR_ERROR_GEOREFERENCE,
			"Map projection error:  Image cannot be re-mapped to current projection space. \n%s",
			"Image extents may be too great.");  
		return false;
	}

	displayCorners[0] = minx;
	displayCorners[1] = miny;
	displayCorners[2] = minx;
	displayCorners[3] = maxy;
	displayCorners[4] = maxx;
	displayCorners[5] = maxy;
	displayCorners[6] = maxx;
	displayCorners[7] = miny;

	return true;
}

//Map a single point into user coordinates. 
//Input values are using [-1,1] as coordinates in image
bool TwoDImageParams::mapGeorefPoint(int timestep, double pt[2]){

	//obtain the extents of the image in the projected space.
	//the point pt is relative to these extents
	const float* imgExts = getCurrentTwoDImageExtents();
	if (!imgExts) return false;
	//Set up proj.4 to convert from image space to VDC coords
	projPJ dst_proj;
	projPJ src_proj; 
	
	src_proj = pj_init_plus(getImageProjectionString().c_str());
	dst_proj = pj_init_plus(DataStatus::getProjectionString().c_str());
	bool doProj = (src_proj != 0 && dst_proj != 0);
	if (!doProj) return false;

	//If a projection string is latlon, the coordinates are in Radians!
	bool radSrc = pj_is_latlong(src_proj)||(string::npos != getImageProjectionString().find("ob_tran"));
	bool radDst = pj_is_latlong(dst_proj)||(string::npos != DataStatus::getProjectionString().find("ob_tran"));

	static const double RAD2DEG = 180./M_PI;
	static const double DEG2RAD = M_PI/180.0;
	
	//map to (0,1), then use convex combination: 
	pt[0] = (1.+pt[0])*0.5;
	pt[1] = (1.+pt[1])*0.5;
	pt[0] = imgExts[0]*(1.-pt[0]) + pt[0]*imgExts[2];
	pt[1] = imgExts[1]*(1.-pt[1]) + pt[1]*imgExts[3];

	if (radSrc){ //need to convert degrees to radians, image exts are in degrees
		for (int i = 0; i<2; i++) pt[i] *= DEG2RAD;
	}
	
	//apply proj4 to transform the point (in place):
	int rc = pj_transform(src_proj,dst_proj,1,2, pt,pt+1, 0);

	if (rc){
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, "Error in coordinate projection: \n%s",
			pj_strerrno(rc));
		return false;
	}
	if (radDst)  {//results are in radians, convert to degrees, then to meters:
		for (int i = 0; i<2; i++) pt[i] *= (RAD2DEG*111177.0);
	}
	
	return true;
	
}
bool TwoDImageParams::
usingVariable(const string& varname){
	return ((varname == GetHeightVariableName()) && isMappedToTerrain());
}
