//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		flowparams.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		July 2005
//
//	Description:  Definition of FlowParams class 
//		supports all the parameters needed for an Flow renderer
//

#ifndef FLOWPARAMS_H
#define FLOWPARAMS_H

#include <qwidget.h>
#include "params.h"

class FlowTab;

namespace VAPoR {
class MapperFunction;
class FlowMapEditor;
class VaporFlow;
class ExpatParseMgr;
class MainForm;
class FlowParams: public Params {
	
public: 
	FlowParams(int winnum);
	
	~FlowParams();

	
	void setTab(FlowTab* tab); 
	virtual Params* deepCopy();
	virtual void makeCurrent(Params* p, bool newWin);
	//implement change of enablement, or change of local/global
	//
	virtual void updateRenderer(bool prevEnabled,  bool wasLocal, bool newWindow);
	virtual void updateMapBounds();
	virtual void guiStartChangeMapFcn(char* );
	virtual void guiEndChangeMapFcn();
	
	
	//Update the dialog with values from this:
	//
	virtual void updateDialog();
	//And vice-versa:
	//
	virtual void updatePanelState();

	// Reinitialize due to new Session:
	void reinit(bool doOverride);
	//Override default set-enabled to create/destroy FlowLib
	virtual void setEnabled(bool value);
	
	//Methods that record changes in the history:
	//
	virtual void guiSetEnabled(bool);

	//Save, restore stuff:
	XmlNode* buildNode(); 
	virtual bool elementStartHandler(ExpatParseMgr*, int  depth , std::string& tag, const char ** attribs);
	virtual bool elementEndHandler(ExpatParseMgr*, int depth , std::string& tag);
	
	//set dirty-flag, and force rebuilding geometry
	void setDirty();
	
	int getNumGenerators(int dimNum) { return generatorCount[dimNum];}
	int getTotalNumGenerators() { return allGeneratorCount;}
	VaporFlow* getFlowLib(){return myFlowLib;}
	float* regenerateFlowData(float** speeds);
	
	int getMinFrame() {return minFrame;}
	int getMaxFrame() {return maxFrame;}
	int getMaxPoints() {return maxPoints;}
	int getNumSeedPoints() { return numSeedPoints;}
	int getNumInjections() { return numInjections;}
	int getFirstDisplayFrame() {return firstDisplayFrame;}
	int getLastDisplayFrame() {return lastDisplayFrame;}
	int getStartFrame() {return seedTimeStart;}
	int getLastSeeding() {return seedTimeEnd;}
	int getSeedingIncrement() {return seedTimeIncrement;}
	float getShapeDiameter() {return shapeDiameter;}
	
	bool flowIsSteady() {return (flowType == 0);} // 0= steady, 1 = unsteady
	int getShapeType() {return geometryType;} //0 = tube, 1 = point, 2 = arrow
	float getObjectsPerTimestep() {return objectsPerTimestep;}
	void guiSetEditMode(bool val); //edit versus navigate mode
	void guiSetAligned();

	
	void setEditMode(bool mode) {editMode = mode;}
	bool getEditMode() {return editMode;}
	virtual MapperFunction* getMapperFunc() {return mapperFunction;}
	//Methods called from vizwinmgr due to settings in gui:
	void guiSetFlowType(int typenum);
	void guiSetNumTrans(int numtrans);
	void guiSetXVarNum(int varnum);
	void guiSetYVarNum(int varnum);
	void guiSetZVarNum(int varnum);
	void guiRecalc();
	void guiSetRandom(bool rand);
	void guiSetGeneratorDimension(int dimNum);
	void guiSetXCenter(int sliderval);
	void guiSetYCenter(int sliderval);
	void guiSetZCenter(int sliderval);
	void guiSetXSize(int sliderval);
	void guiSetYSize(int sliderval);
	void guiSetZSize(int sliderval);
	void guiSetFlowGeometry(int geomNum);
	void guiSetColorMapEntity( int entityNum);
	void guiSetOpacMapEntity( int entityNum);


protected:
	//Tags for attributes in session save
	//Top level labels
	static const string _mappedVariablesAttr;
	static const string _steadyFlowAttr;
	static const string _instanceAttr;
	static const string _numTransformsAttr;
	static const string _integrationAccuracyAttr;
	static const string _userTimeStepMultAttr;
	static const string _timeSamplingIntervalAttr;

	//flow seeding tags and attributss
	static const string _seedingTag;
	static const string _seedRegionMinAttr;
	static const string _seedRegionMaxAttr;
	static const string _randomGenAttr;
	static const string _randomSeedAttr;
	static const string _generatorCountsAttr;
	static const string _totalGeneratorCountAttr;
	static const string _seedTimesAttr;

	//Geometry attributes
	static const string _geometryTag;
	static const string _geometryTypeAttr;
	static const string _objectsPerTimestepAttr;
	static const string _displayIntervalAttr;
	static const string _shapeDiameterAttr;
	static const string _colorMappedEntityAttr;
	static const string _colorMappingBoundsAttr;
	static const string _hsvAttr;
	static const string _positionAttr;
	static const string _colorControlPointTag;
	static const string _numControlPointsAttr;

	void setFlowType(int typenum){flowType = typenum; setDirty();}
	void setNumTrans(int numtrans){numTransforms = numtrans; setDirty();}
	void setMaxNumTrans(int maxNT) {maxNumTrans = maxNT;}
	void setMinNumTrans(int minNT) {minNumTrans = minNT;}
	void setXVarNum(int varnum){varNum[0] = varnum; setDirty();}
	void setYVarNum(int varnum){varNum[1] = varnum; setDirty();}
	void setZVarNum(int varnum){varNum[2] = varnum; setDirty();}
	void setRandom(bool rand){randomGen = rand; setDirty();}
	void setXCenter(int sliderval);
	void setYCenter(int sliderval);
	void setZCenter(int sliderval);
	void setXSize(int sliderval);
	void setYSize(int sliderval);
	void setZSize(int sliderval);
	void setFlowGeometry(int geomNum){geometryType = geomNum;}
	void setColorMapEntity( int entityNum){colorMapEntityIndex = entityNum;}
	void setOpacMapEntity( int entityNum){opacMapEntityIndex = entityNum;}
	int getColorMapEntityIndex() {return colorMapEntityIndex;}
	int getOpacMapEntityIndex() {return opacMapEntityIndex;}
	void setCurrentDimension(int dimNum) {currentDimension = dimNum;}
	virtual void connectMapperFunction(MapperFunction* tf, MapEditor* tfe);
	//Methods to make sliders and text consistent for seed region:
	void textToSlider(int coord, float center, float size);
	void sliderToText(int coord, int center, int size);
	FlowMapEditor* getFlowMapEditor() {return flowMapEditor;}
	FlowTab* myFlowTab;
	int flowType; //steady = 0, unsteady = 1;
	int instance;
	int numTransforms, maxNumTrans, minNumTrans;
	int numVariables;
	std::vector<std::string> variableNames;
	int varNum[3]; //field variable num's in x, y, and z.
	float integrationAccuracy;
	float userTimeStepMultiplier;
	int timeSamplingInterval;
	bool editMode;
	
	bool randomGen;
	
	
	unsigned int randomSeed;
	float seedBoxMin[3], seedBoxMax[3];
	size_t generatorCount[3];
	size_t allGeneratorCount;
	int seedTimeStart, seedTimeEnd, seedTimeIncrement;
	int currentDimension;//Which dimension is showing in generator count

	int geometryType;  //0= tube, 1=point, 2 = arrow
	float objectsPerTimestep;
	int firstDisplayFrame, lastDisplayFrame;
	
	float shapeDiameter;
	int colorMapEntityIndex; //0 = constant, 1=age, 2 = speed, 3+varnum = variable
	int opacMapEntityIndex;
	
	PanelCommand* savedCommand;
	MapperFunction* mapperFunction;
	FlowMapEditor* flowMapEditor;

	int maxPoints;  //largest length of any flow
	
	
	std::vector<string> colorMapEntity;
	std::vector<string> opacMapEntity;
	int numControlPoints;

	float regionMin[3], regionMax[3];
	
	VaporFlow* myFlowLib;

	//Arrays to hold all the points returned from flow lib
	float* flowData;
	
	//Parameters controlling flowDataAccess.  These are established each time
	//The flow data is regenerated:
	
	int numSeedPoints;
	int numInjections;
	
	//Keep track of min, max frames in available data:
	int maxFrame, minFrame;

};
};
#endif //FLOWPARAMS_H 
