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
	
	virtual bool twoDIsDirty(int timestep) {
		return (!twoDDataTextures || twoDDataTextures[0] == 0 || cachedTimestep != timestep);
	}
	virtual void getTextureSize(int sze[2], int){sze[0] = textureSizes[0]; sze[1] = textureSizes[1];}
	virtual bool usingVariable(const string& varname);
	
	virtual int getSessionVarNum() {return -1;}  //following not used by this params
	virtual void setMinColorMapBound(float ){}
	virtual void setMaxColorMapBound(float ){}
	virtual void setMinOpacMapBound(float ){}
	virtual void setMaxOpacMapBound(float ){}
	virtual MapperFunction* getMapperFunc(){return 0;}
	//we need to check for alpha channel
	virtual bool isOpaque() {return (opacityMultiplier >= 0.99f && !transparentAlpha);}
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
	
	
	void setTwoDTexture(unsigned char* tex, int timestep , int imgSize[2],
		float imExts[4] = 0 ){ 
		unsigned char** textureArray = twoDDataTextures;
		if (!textureArray){
			textureArray = new unsigned char*[1];
			textureSizes = new int[2];
			
				textureArray[0] = 0;
				textureSizes[0] = 0;
				textureSizes[1] = 0;
			
			if (imageExtents) delete [] imageExtents;
			imageExtents = 0;
			if(imExts) 
				imageExtents = new float [4];
			twoDDataTextures = textureArray;	
		}
		if (textureArray[0]) 
			delete textureArray[0];
		textureSizes[0] = imgSize[0];
		textureSizes[1] = imgSize[1];
		textureArray[0] = tex;
		if(imExts) {
			for (int k = 0; k < 4; k++)
				imageExtents[k] = imExts[k];
		}
		cachedTimestep = timestep;
		 
	}
	
	unsigned char* calcTwoDDataTexture(int timestep, int wid, int ht);

	//Read texture image from tif (or kml).
	unsigned char* readTextureImage(int timestep, int* wid, int* ht,
		float imgExtents[4]);

	//Whenever the 2D image filename changes or the session changes,
	//we need to reread the file and reset the image extents.
	void setImagesDirty();
	
	//General method that obtains a list of variables (containing the twoD) from the dataMgr
	//Also establishes values of blkMin, blkMax, coordMin, coordMax and actualRefLevel to be used
	//for addressing into the volumes.  Replaces first half of calcTwoDDataTexture.
	float** getTwoDVariables(int ts, int numVars, int* sesVarNums,
				  size_t blkMin[3], size_t blkMax[3], size_t coordMin[3], size_t coordMax[3],
				  int* actualRefLevel);

	virtual unsigned char* getCurrentTwoDTexture(int ) {
		if (!twoDDataTextures) return 0;
		return twoDDataTextures[0];
		
	}
	float * getCurrentTwoDImageExtents(int ){
		if (!imageExtents) return 0;
		return imageExtents;
	}
	
	std::string& getImageProjectionString() {return projDefinitionString;}
	void setImageProjectionString(const char* str){projDefinitionString = str;}
	//Determine the corners of the image in local coordinates
	//Only available when the renderer is enabled.
	bool getImageCorners(int timestep, double cors[8]);
	int getImagePlacement(){return imagePlacement;}
	void setImagePlacement(int val){ imagePlacement = val;}

	//Map a geo-ref point from the image into user coordinates
	//Return false if the image and the scene are not both georeferenced.
	bool mapGeorefPoint(int timestep, double pt[2]);
	bool isSingleImage() {return singleImage;}
	bool hasTransparentAlpha() {return transparentAlpha;}
	
	
protected:
	static const string _shortName;
	static const string _cropImageAttr;
	static const string _georeferencedAttr;
	static const string _resampleRateAttr;
	static const string _opacityMultAttr;
	static const string _imageFileNameAttr;
	static const string _imagePlacementAttr;

	
	
	int getImageNum(int timestep){
		return imageNums[timestep];
	}
	void setupImageNums(TIFF* tif);

	//Cache of twoD textures, one per timestep.
	//Also cache image positions, because each image has 
	//different LL and UR coordinates (meters in projection space)
	//for each time step.
	//The elev grid is cached in the 
	//renderer class
	
	float * imageExtents; //(4 floats for each time step), only for georeferenced images

	int* imageNums;
	
	bool useGeoreferencing;
	bool cropImage;
	float resampRate;
	float opacityMultiplier;
	string imageFileName;
	int imagePlacement;
	std::string projDefinitionString;
	int cachedTimestep;
	bool singleImage;  //indicates there is only one image for all timesteps
	bool transparentAlpha;
	
};
};
#endif //TWODIMAGEPARAMS_H 
