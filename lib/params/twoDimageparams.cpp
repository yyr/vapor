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
#include <vapor/GeoUtil.h>
#include <vapor/Proj4API.h>
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

	for (int i=0; i<6; i++) {
		_boxExtents[i] = 0.0;
	}
	_textureSizes[0] = _textureSizes[1] = 0;
	_texBuf = NULL;
	_projDefinitionString.clear();

	_imageNums.clear();
	maxTimestep = 1;
	linearInterp = false;
	restart();
	
}

TwoDImageParams::~TwoDImageParams(){
	
	
	if (_texBuf) delete [] _texBuf;
}


//Deepcopy requires cloning tf 
Params* TwoDImageParams::
deepCopy(ParamNode*){
	TwoDImageParams* newParams = new TwoDImageParams(*this);
	ParamNode* pNode = new ParamNode(*(myBox->GetRootNode()));
	newParams->myBox = (Box*)myBox->deepCopy(pNode);
	//TwoD texture must be recreated when needed
	newParams->_timestep = -1;
	for (int i=0; i<4; i++) {
		newParams->_imageExtents[i] = 0.0;
	}
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
		SetFidelityLevel(0);
		SetIgnoreFidelity(false);
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
	
	//Make sure fidelity is valid:
	int fidelity = GetFidelityLevel();
	DataMgr* dataMgr = ds->getDataMgr();
	if (dataMgr && fidelity > maxNumRefinements+dataMgr->GetCRatios().size()-1)
		SetFidelityLevel(maxNumRefinements+dataMgr->GetCRatios().size()-1);
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
	_imageNums.clear();
	
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
	_imageNums.clear();

	resampRate = 1.f;
	opacityMultiplier = 1.f;
	
	useGeoreferencing = true;
	cropImage = false;
	imageFileName = "";

	cursorCoords[0] = cursorCoords[1] = 0.0f;
	//Initialize the mapping bounds to [0,1] until data is read
	
	setEnabled(false);
	SetIgnoreFidelity(false);
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
		SetFidelityLevel(0);
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
				int fid;
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
	oss << (int)GetFidelityLevel();
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
	_imageNums.clear();
	setElevGridDirty(true);
	setAllBypass(false);
	singleImage = false;
	transparentAlpha = false;
	_timestep = -1;
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
		texWidth = _textureSizes[0];
	
	//we need to read the current timestep
	//of image into the texture for this timestep.  
	//If a map projection is undefined, invalid imageExts are returned
	// (i.e. imgExts[2]<imgExts[0])
	
	if (_texBuf) delete [] _texBuf;
	_texBuf = _readTextureImage(
		ts, _textureSizes[0], _textureSizes[1], _imageExtents
	);
	_timestep = ts;
	texWidth = _textureSizes[0];
	texHeight = _textureSizes[1];
	GetBox()->GetUserExtents(_boxExtents, ts);
	return _texBuf;
}

//Get texture from image file, set it in the cache

const unsigned char* TwoDImageParams::_readTextureImage(
	size_t timestep, int &width, int &height, float pcsImgExts[4]
) {
	
	_projDefinitionString = "";

	// Check for a valid file name (this avoids Linux crash):
	//
	struct STAT64 statbuf;
	if (STAT64(imageFileName.c_str(), &statbuf) < 0) {
		MyBase::SetErrMsg(
			VAPOR_ERROR_TWO_D, "Invalid tiff file: %s\n", imageFileName.c_str()
		);
		return 0;
	}

	// Not using memory-mapped IO (m) is reputed to help plug 
	// leaks (but doesn't do any good on windows for me)
	//
    TIFF* tif = XTIFFOpen(imageFileName.c_str(), "rm");
	if (!tif) {
		MyBase::SetErrMsg(
			VAPOR_ERROR_TWO_D, 
			"Unable to open tiff file: %s\n", imageFileName.c_str()
		);
		return 0;
	}

	//
	// Read the image texture from a file
	//
	uint32* texture = _readTiffImage(tif, timestep, width, height);
	if (! texture) return (NULL);

	// get a proj4 definition string if it exists, using geoTiff lib
	//
	string proj4String  = _getProjectionFromGTIF(tif);
	
	// Deal with georeferencing: Data must be geo-referenced, image must
	// be georeferenced, and geo-referencing must be 
	// enabled (isGeoreferenced()=true)
	//
	if (
		DataStatus::getProjectionString().size() && 
		isGeoreferenced() && proj4String.size()
	) {


		double pcsExtents[4];
		double geoCorners[8];

		//
		// Get extents of image in both Projected Coordinates (PCS)
		// and lat-long.
		// N.B. if the proj4String defined a lat-long 
		// projection (proj=latlong), it's modified to be cylindrical 
		// equidistant (proj=eqc)
		//
		int rc = _getGTIFExtents(
			tif, proj4String, width, height, 
			pcsExtents, geoCorners, proj4String
		);
		if (rc<0) return(NULL);



		//
		// Get lat-lon extents for current entire data domain
		//
		double lonlatexts[4];
		if (! _getLonLatExts((size_t)timestep, lonlatexts)) {
			return(NULL);
		}

		//
		// Attempt to crop the texture to the smallest rectangle
		// that covers the data space. Only possible if image is
		// a global, cylindrical-equidistant projection. _extractSubtexture()
		// modifies with, height, and pcsExtents if successful, otherwise
		// they are unchanged.
		//
		uint32 *subtexture = _extractSubtexture(
			texture, proj4String, width, height, 
			lonlatexts, pcsExtents, geoCorners,
			proj4String, width, height, pcsExtents
		);
		_projDefinitionString = proj4String;

		if (subtexture) {
			delete [] texture;
			texture = subtexture;
		}

		pcsImgExts[0] = pcsExtents[0];
		pcsImgExts[1] = pcsExtents[1];
		pcsImgExts[2] = pcsExtents[2];
		pcsImgExts[3] = pcsExtents[3];

	}
	else {
		// set pcsImgExts to the TwoDImage extents
		//
		const vector<double>& boxExts = GetBox()->GetLocalExtents();
		pcsImgExts[0] = boxExts[0];
		pcsImgExts[1] = boxExts[1];
		pcsImgExts[2] = boxExts[3];
		pcsImgExts[3] = boxExts[4];
	}
	XTIFFClose(tif);

	//apply opacity multiplier
	//
	if (opacityMultiplier < 1){
		for (int i = 0; i < width*height; i++){
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

bool TwoDImageParams::twoDIsDirty(int timestep) {
	if (timestep != _timestep) {
		return (true);
	}

	double boxExtents[6];
    GetBox()->GetUserExtents(boxExtents, timestep);
	for (int i=0; i<6; i++) {
		if (boxExtents[i] != _boxExtents[i]) {
			return (true);
		}
	}

	return(false);
}
string TwoDImageParams::_getProjectionFromGTIF(TIFF *tif) const {
	GTIF* gtifHandle = GTIFNew(tif);
	if (! gtifHandle) return("");
	GTIFDefn gtifDef;
	GTIFGetDefn(gtifHandle,&gtifDef);
	GTIFFree(gtifHandle);
	string proj4String = GTIFGetProj4Defn(&gtifDef);

	// If there's no "ellps=" in the string, force it to be spherical,
	// This avoids a bug in the geotiff routines
	//
	if (std::string::npos == proj4String.find("ellps=")){
		proj4String += " +ellps=sphere";
	}
	return (proj4String);
}

bool TwoDImageParams::_getLonLatExts(size_t timestep, double lonlatexts[4]){
	for (int j = 0; j<4; j++) lonlatexts[j] = 0.;

	if (! DataStatus::getProjectionString().size()) return (false);

	Box* box = GetBox();
	double extents[6];
	box->GetUserExtents(extents, timestep);

	// Set up proj.4 to convert from VDC coords to latlon 
	//
	Proj4API proj4API;

	int rc = proj4API.Initialize(DataStatus::getProjectionString(), "");
	if (rc < 0) {
		MyBase::SetErrMsg(
			VAPOR_ERROR_GEOREFERENCE, "Invalid Proj string in data : %s",
			DataStatus::getProjectionString().c_str()
		);
		return false;
	}
	
	const int nrows = 8;
	const int ncols = 8;
	double xsamples[nrows*ncols];
	double ysamples[nrows*ncols];
	//interpolate 8 points on each row of image
	for (int row = 0; row <nrows; row++){
		for (int col = 0; col<ncols; col++){
			double u = ((double)col)/7.;
			double v = ((double)row)/7.;
			xsamples[row*ncols + col] = (1.-u)*extents[0]+u*extents[3];
			ysamples[row*ncols + col] = (1.-v)*extents[1]+v*extents[4];
		}
	}

	//apply proj4 to transform the points(in place):
    rc = proj4API.Transform(xsamples, ysamples, nrows*ncols, 1);
	if (rc<0){
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, "Error in coordinate projection");
		return false;
	}

	GeoUtil::LonExtents(xsamples, ncols, nrows, lonlatexts[0], lonlatexts[2]);
	GeoUtil::LatExtents(ysamples, ncols, nrows, lonlatexts[1], lonlatexts[3]);

	if ((lonlatexts[2] - lonlatexts[0]) >= 360.0) {
		lonlatexts[2] = lonlatexts[0] + 359.9;
	}

	return true;

}

void TwoDImageParams::setupImageNums(TIFF* tif){
	//Initialize to zeroes
	_imageNums.clear();
	//Set up array to hold time differences:
	TIME64_T *timeDifference = new TIME64_T[maxTimestep+1];
	for (int i = 0; i<=maxTimestep; i++) {
		_imageNums.push_back(0);
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
			tiffTimes.push_back((TIME64_T)seconds);

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
			_imageNums[i] = bestpos;
			timeDifference[i] = minTimeDiff;
		}
		//Count the number of different images that are referenced:
		
		for (int i = 0; i< maxTimestep; i++){
			if (_imageNums[i] != _imageNums[i+1]) numDiffImages++;
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
			_imageNums[i] = Min(dircount-1,i);
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

	// Set up proj.4 to convert from PCS image space to PCS VDC coords
	//
	Proj4API proj4API;

	int rc = proj4API.Initialize(
		_projDefinitionString, DataStatus::getProjectionString()
	);
	if (rc < 0) {
		MyBase::SetErrMsg(
			VAPOR_ERROR_GEOREFERENCE, "Invalid Proj string in image : %s",
			DataStatus::getProjectionString().c_str()
		);
		return false;
	}
	
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

	//apply proj4 to transform the four corners (in place):
	//
    rc = proj4API.Transform(interpPoints, interpPoints+1, 32, 2);
	if (rc<0){
		MyBase::SetErrMsg(
			VAPOR_ERROR_TWO_D, "Error in coordinate projection: \n"
		);
		return false;
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

//Map a single point into PCS coordinates, user spac3. 
//Input values are using [-1,1] as coordinates in image
bool TwoDImageParams::mapGeorefPoint(int timestep, double pt[2]){

	//
	// Force loading of texture so we have the correct image 
	// extents. This method is called before the image is loaded. Sigh :-(
	//
	int dummy1, dummy2;
	(void) calcTwoDDataTexture(timestep, dummy1, dummy2);

	// obtain the extents of the image in the projected space.
	// the point pt is relative to these extents
	//
	const float* imgExts = getCurrentTwoDImageExtents();
	//Set up proj.4 to convert from image space to VDC coords

	//
	// Set up transform from PCS in image space to PCS in user space
	//
	Proj4API proj4API;
	int rc = proj4API.Initialize(
		_projDefinitionString, DataStatus::getProjectionString()
	);
	if (rc < 0) {
		MyBase::SetErrMsg(
			"Invalid Proj string in data : %s",
			DataStatus::getProjectionString().c_str()
		);
		return false;
	}
	
	//map to (0,1), then use convex combination: 
	pt[0] = (1.+pt[0])*0.5;
	pt[1] = (1.+pt[1])*0.5;
	pt[0] = imgExts[0]*(1.-pt[0]) + pt[0]*imgExts[2];
	pt[1] = imgExts[1]*(1.-pt[1]) + pt[1]*imgExts[3];

	//apply proj4 to transform the point (in place):
    rc = proj4API.Transform(pt, pt+1, 1, 2);
	if (rc<0){
		MyBase::SetErrMsg(VAPOR_ERROR_TWO_D, "Error in coordinate projection");
		return false;
	}

	return true;
}

bool TwoDImageParams::
usingVariable(const string& varname){
	return ((varname == GetHeightVariableName()) && isMappedToTerrain());
}

uint32* TwoDImageParams::_readTiffImage(
	TIFF *tif, size_t timestep, int &width, int &height
) {
	
	char emsg[1000];
	int ok = TIFFRGBAImageOK(tif,emsg);
	if (!ok){
		MyBase::SetErrMsg(
			VAPOR_WARNING_TWO_D, 
			"Unable to process tiff file:\n %s\nError message: %s",
			imageFileName.c_str(),emsg
		);
		return 0;
	}

	//Check compression.  Some compressions, e.g. jpeg, cause crash on Linux
	short compr = 1;
	ok = TIFFGetField(tif, TIFFTAG_COMPRESSION, &compr);
	if (ok){
		if (compr != COMPRESSION_NONE &&
			compr != COMPRESSION_LZW &&
			compr != COMPRESSION_JPEG &&
			compr != COMPRESSION_CCITTRLE){

			MyBase::SetErrMsg(VAPOR_ERROR_TWO_D,"Unsupported Tiff compression");
			return 0;
				
		}
	}

	//Set the tif directory to the one associated with the
	//current frame num.
	if (! _imageNums.size()) setupImageNums(tif);
	int currentDir = _getImageNum(timestep);
	
	TIFFSetDirectory(tif, currentDir);
	
	uint32 w, h;
	TIFFGetField(tif, TIFFTAG_IMAGEWIDTH, &w);
	TIFFGetField(tif, TIFFTAG_IMAGELENGTH, &h);
	
	size_t npixels = w * h;
	width = w;
	height = h;
	uint32* texture = new uint32[npixels];

	// Check if this is a 2-component 8-bit image.  These are read 
	// by scanline since TIFFReadRGBAImage
	// apparently does not know how to get the alpha channel
	//
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

			// Note: following loop (adapted from tiff docs) has not 
			// been tested.  Are there tiff
			// examples with separate alpha channel?
			//
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
				
				width = w;
				height = h;
				//May need to resample here!
			}
			else {
				MyBase::SetErrMsg(
					VAPOR_WARNING_TWO_D, "Error reading tiff file:\n %s\n",
					imageFileName.c_str()
				);
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

	return(texture);
}

int TwoDImageParams::_getGTIFExtents(
	TIFF *tif, string proj4String, int width, int height, 
	double pcsExtents[4], double geoCorners[8], string &proj4StringNew
) const {
	proj4StringNew = proj4String;

	GTIF* gtifHandle = GTIFNew(tif);
	//
	// Get image extents by converting from pixel to 
	// Projection Coordinate Space (PCS). Note, if the
	// proj4 projection is "latlong" the conversion will be from
	// pixels to geographic coords (i.e. lat and long) :-(
	//
	// N.B. GTIF library expects Y origin at top of image, we
	// us bottom for Y origin
	//
	double extents[4] = {0.0, height-1, width-1, 0};

	bool ok = GTIFImageToPCS(gtifHandle, &extents[0],&extents[1]);
	if (! ok) {
		SetErrMsg("GTIFImageToPCS()");
		return(-1);
	}
	ok = GTIFImageToPCS(gtifHandle, &extents[2],&extents[3]);
	if (! ok) {
		SetErrMsg("GTIFImageToPCS()");
		return(-1);
	}
	GTIFFree(gtifHandle);

	//
	// Set up proj4 to convert from PCS, image space to lat-long
	//
	Proj4API proj4API;
	int rc = proj4API.Initialize(proj4StringNew, "");
	if (rc < 0) return(-1);

	//
	// Make sure extents aren't already in lat-long
	//
	if (! proj4API.IsLatLonSrc()) {
		for (int i=0; i<4; i++) {
			pcsExtents[i] = extents[i];
		}

		//
		// Transform from PCS to lat-long
		//
		proj4API.Transform(extents, extents+1, 2, 2);

		//
		// clockwise order starting with lower-left
		//
		geoCorners[0] = extents[0];
		geoCorners[1] = extents[1];
		geoCorners[2] = extents[0];
		geoCorners[3] = extents[3];
		geoCorners[4] = extents[2];
		geoCorners[5] = extents[3];
		geoCorners[6] = extents[2];
		geoCorners[7] = extents[1];
	}
	else {

		// Oops. Projection string was lat-long. This means we have
		// lat long coordinates in 'extents', not PCS. We need to generate
		// a new proj4 string to find geographic (lat-lon) coordinates
		//
		double lon_0 = (extents[0] + extents[2]) / 2.0;
		double lat_0 = (extents[1] + extents[3]) / 2.0;
		ostringstream oss;
		oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
		proj4StringNew = "+proj=eqc +ellps=WGS84" + oss.str();
		int rc = proj4API.Initialize("", proj4StringNew);
		if (rc < 0) return(-1);

		//
		// clockwise order starting with lower-left
		//
		geoCorners[0] = extents[0];
		geoCorners[1] = extents[1];
		geoCorners[2] = extents[0];
		geoCorners[3] = extents[3];
		geoCorners[4] = extents[2];
		geoCorners[5] = extents[3];
		geoCorners[6] = extents[2];
		geoCorners[7] = extents[1];

		//
		// Transform from lat-long to PCS
		//
		proj4API.Transform(extents, extents+1, 2, 2);
		for (int i=0; i<4; i++) {
			pcsExtents[i] = extents[i];
		}
	}

	// Don't allow latlong image to go all the way to the poles
	// Not sure why they can't to to 90. This code may no longer
	// be necessary.
	//
	if (geoCorners[1] < -89.9999) geoCorners[1] = -89.9999;
	if (geoCorners[7] < -89.9999) geoCorners[7] = -89.9999;
	if (geoCorners[3] > 89.9999) geoCorners[3] = 89.9999;
	if (geoCorners[4] > 89.9999) geoCorners[7] = 89.9999;

	return(0);
}

uint32 *TwoDImageParams::_extractSubtexture(
	uint32 *texture, string proj4StringImage, int width, int height, 
	const double geoExtentsData[4],
	const double pcsExtentsImage[4], const double geoCornersImage[8],
	string &subProj4StringImage, int &subWidth, int &subHeight, 
	double subPCSExtentsImage[4]
) const {

	//
	// Initialize output params to inputs in case of failure
	//
	subProj4StringImage = proj4StringImage;
	subWidth = width;
	subHeight = height;
	for (int i=0; i<4; i++) {
		subPCSExtentsImage[i] = pcsExtentsImage[i];
	}
	
	//
	// Can only extract sub-textures from georeferenced images
	// with cylindrical equi-distant projection that cover entire globe
	//
	if (std::string::npos == subProj4StringImage.find("proj=eqc")) {
		return(NULL);
	}
	if (! (geoCornersImage[0] < -179.0 && geoCornersImage[4] > 179.0 &&
		geoCornersImage[1] < -89 && geoCornersImage[5] > 89)) {

		return (NULL);
	}
	
	//
	// Construct a GeoTile. It supports subregion extraction and handles
	// wraparound
	//
	GeoTileEquirectangular geotile(width, height, 4);
	geotile.Insert("", (unsigned char *) texture);
	size_t pixelSW[2];
	size_t pixelNE[2];
	size_t nx,ny;

	//
	// Get GeoTile's pixel coordinates of subregion. 
	//
	geotile.LatLongToPixelXY(
		geoExtentsData[0], geoExtentsData[1], 0, pixelSW[0], pixelSW[1]
	);

	geotile.LatLongToPixelXY(
		geoExtentsData[2], geoExtentsData[3], 0, pixelNE[0], pixelNE[1]
	);
	int rc = geotile.MapSize(
		pixelSW[0],pixelSW[1],pixelNE[0],pixelNE[1],0,nx, ny
	);
	if (rc != 0) return 0;

	//
	// Extract the image
	//
	unsigned char* dstImage = new unsigned char[nx*ny*4];
	rc = geotile.GetMap(
		pixelSW[0],pixelSW[1],pixelNE[0],pixelNE[1],0,dstImage
	);
	if (rc != 0) return 0;

	//
	// If data crosses -180 or 180 we need to generate a new
	// proj4 string with the correct centering
	//
	if (geoExtentsData[0] < -180 || geoExtentsData[2] > 180.0) {
		double lon_0 = (geoExtentsData[0] + geoExtentsData[2]) / 2.0;
		ostringstream oss;
		oss << " +lon_0=" << lon_0;
		subProj4StringImage = "+proj=eqc +ellps=WGS84" + oss.str();
	}

	//
	// Finally, update the extents of the new image in PCS coordinates 
	// by mapping geographic coordinates of corners to PCS.
	// Since the projection is eqc we only need south-west and north-east 
	// points. 
	//
	Proj4API proj4API;
	proj4API.Initialize("",subProj4StringImage);

	subPCSExtentsImage[0] = geoExtentsData[0];
	subPCSExtentsImage[1] = geoExtentsData[1];
	subPCSExtentsImage[2] = geoExtentsData[2];
	subPCSExtentsImage[3] = geoExtentsData[3];

	proj4API.Transform(subPCSExtentsImage, subPCSExtentsImage+1, 2, 2);
	subWidth = (int) nx;
	subHeight = (int) ny;

	return (uint32 *) dstImage;
}
