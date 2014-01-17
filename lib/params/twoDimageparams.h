//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		twoDimageparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		March 2009
//
//	Description:	Definition of the TwoDImageParams class
//		This contains all the parameters required to support the
//		TwoD image renderer.  
//
#ifndef TWODIMAGEPARAMS_H
#define TWODIMAGEPARAMS_H


#include <qwidget.h>
#include "params.h"
#include "twoDparams.h"
#include "xtiffio.h"
#include <vector>
#include <string>
#include "datastatus.h"

namespace VAPoR{
class ExpatParseMgr;
class MainForm;
class TransferFunction;
class PanelCommand;
class XmlNode;
class ParamNode;
class FlowParams;
class Histo;

class PARAMS_API TwoDImageParams : public TwoDParams {
	
public: 
	TwoDImageParams(int winnum);

	~TwoDImageParams();
	static ParamsBase* CreateDefaultInstance() {return new TwoDImageParams(-1);}
	const std::string& getShortName() {return _shortName;}
	
	virtual Params* deepCopy(ParamNode* =0); 
	
	

	virtual void getTextureSize(int sze[2]){
		sze[0] = _textureSizes[0];
		sze[1] = _textureSizes[1];
	}
	virtual bool usingVariable(const string& varname);
	
	virtual int getSessionVarNum() {return -1;}  //following not used by this params
	virtual void setMinColorMapBound(float ){}
	virtual void setMaxColorMapBound(float ){}
	virtual void setMinOpacMapBound(float ){}
	virtual void setMaxOpacMapBound(float ){}
	virtual MapperFunction* GetMapperFunc(){return 0;}
	//we need to check for alpha channel
	virtual bool IsOpaque() {return (opacityMultiplier >= 0.99f && !transparentAlpha);}
	//Variables specific to images:
	bool isGeoreferenced() {return useGeoreferencing;}
	void setGeoreferenced(bool val){useGeoreferencing = val;}
	bool imageCrop() {return cropImage;}
	void setImageCrop(bool val){cropImage = val;}
	float getResampRate(){return resampRate;}
	void setResampRate(float val){ resampRate = val;}
	float getOpacMult() {return opacityMultiplier;}
	void setOpacMult(float val){opacityMultiplier = val;}
	string& getImageFileName(){return imageFileName;}
	
	
	void setImageFileName(const char* fname) {imageFileName = std::string(fname);}

		
	//Set all the cached twoD textures dirty, as well as elev grid
	void setTwoDDirty();
	
	
	//Implement virtual function to deal with new session:
	bool reinit(bool doOverride);
	virtual void restart();
	static void setDefaultPrefs();
	ParamNode* buildNode(); 
	bool elementStartHandler(ExpatParseMgr*, int /* depth*/ , std::string& /*tag*/, const char ** /*attribs*/);
	bool elementEndHandler(ExpatParseMgr*, int /*depth*/ , std::string& /*tag*/);
	
	
	const unsigned char* calcTwoDDataTexture(int timestep, int &wid, int &ht);


	//Whenever the 2D image filename changes or the session changes,
	//we need to reread the file and reset the image extents.
	void setImagesDirty();
	
	
	//Override default, allow manip to go outside of data:
	virtual bool isDomainConstrained() {return false;}

	// Return the image extents in the current displayed image
	// coordinate system: PCS, *image* space
	//
	float * getCurrentTwoDImageExtents(){
		return _imageExtents;
	}
	std::string getImageProjectionString() {return _projDefinitionString;}

	
	// Determine the corners of the image in PCS coordinates, user space
	// Only available when the renderer is enabled.
	//
	bool getImageCorners(double cors[8]);
	int getImagePlacement(){return imagePlacement;}
	void setImagePlacement(int val){ imagePlacement = val;}

	//Map a geo-ref point from the image into user coordinates
	//Return false if the image and the scene are not both georeferenced.
	bool mapGeorefPoint(int timestep, double pt[2]);
	bool isSingleImage() {return singleImage;}
	bool hasTransparentAlpha() {return transparentAlpha;}
		
	virtual bool GetIgnoreFidelity() {return ignoreFidelity;}
	virtual void SetIgnoreFidelity(bool val){ignoreFidelity = val;}
	virtual int GetFidelityLevel() {return fidelityLevel;}
	virtual void SetFidelityLevel(int lev){fidelityLevel = lev;}
	
	
protected:
	static const string _shortName;
	static const string _cropImageAttr;
	static const string _georeferencedAttr;
	static const string _resampleRateAttr;
	static const string _opacityMultAttr;
	static const string _imageFileNameAttr;
	static const string _imagePlacementAttr;

	
	int fidelityLevel;
	bool ignoreFidelity;
	void setupImageNums(TIFF* tif);

	//Cache of twoD textures, one per timestep.
	//Also cache image positions, because each image has 
	//different LL and UR coordinates (meters in projection space)
	//for each time step.
	//The elev grid is cached in the 
	//renderer class
	
	float _imageExtents[4]; //(4 floats for each time step), only for georeferenced images

	std::vector <int> _imageNums;
	
	bool useGeoreferencing;
	bool cropImage;
	float resampRate;
	float opacityMultiplier;
	string imageFileName;
	int imagePlacement;
	bool singleImage;  //indicates there is only one image for all timesteps
	bool transparentAlpha;

private:
	int _timestep;
	double _lonlatexts[4];
	double _boxExtents[6];
	const unsigned char *_texBuf;
	std::string _projDefinitionString;

	bool twoDIsDirty(int timestep) ;

	string _getProjectionFromGTIF(TIFF *tif) const;

	int _getImageNum(size_t timestep) {
		if (timestep >= _imageNums.size()) return(0); 
		return _imageNums[timestep];
	}
	
	//Read texture image from tif (or kml).
	// 
	const unsigned char* _readTextureImage(
		size_t timestep, int &width, int &height, float imgExtents[4]
	);

	//
	// Helper function for _readTextureImage. Reads a TIFF or GEOTIFF 
	// image
	//
	uint32* _readTiffImage(
		TIFF *tif, size_t timestep, int &width, int &height
	);

	// Attempt to crop the texture to the smallest rectangle
	// that covers the data space. Only possible if image is
	// a global, cylindrical-equidistant projection. _extractSubtexture()
	// modifies with, height, and pcsExtents if successful, otherwise
	// they are unchanged.
	//
	// pcsExtentsImage and subPCSExtentsImage are in PCS coordinate, image
	// space. 
	//
	uint32 *_extractSubtexture(
		uint32 *texture, string proj4StringImage, int width, int height,
		const double geoExtentsData[4], const double pcsExtentsImage[4], 
		const double geoCornersImage[8], 
		string &subProj4StringImage, int &subWidth, int &subHeight, 
		double subPCSExtentsImage[4]
	) const; 

	//
	// Get the extents of a GEOTiff image. 
	// pcsExtents are in PCS coordinates, image space
	// geoCorners are in geographic (lat-long) coordinates
	//
	int _getGTIFExtents(
		TIFF *tif, string proj4String, int width, int height,
		double pcsExtents[4], double geoCorners[8], string &proj4StringNew
	) const;

	// Compute estimate of extents of data domain in lat-lon coordinates
	//
	bool _getLonLatExts(size_t timestep, double lonlatexts[4]) ;
};
};
#endif //TWODIMAGEPARAMS_H 
