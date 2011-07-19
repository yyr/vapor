
#ifndef ARROWPARAMS_H
#define ARROWPARAMS_H

#include "vapor/ParamNode.h"
#include "params.h"

namespace VAPoR {

class PARAMS_API ArrowParams : public RenderParams {
public:
	ArrowParams(XmlNode *parent, int winnum);
	static ParamsBase* CreateDefaultInstance() {
		return new ArrowParams(0, -1);
	}
	const std::string& getShortName() {return _shortName;}
	virtual ~ArrowParams();
	virtual void restart();
	
	virtual Box* GetBox() {return (Box*)GetRootNode()->GetNode(Box::_boxTag)->GetParamsBase();}

	//! Obtain the current compression level.
	//!
	//! Pure virtual method required of render params
	//! \retval level index into the set of available compression ratios
	virtual int GetCompressionLevel();
	//! Set the current compression level.
	//!
	//! Pure virtual method required of render params
	//
	virtual void SetCompressionLevel(int val);
	//! Reinitialize the object for a new dataset.
	//!
	//! Pure virtual method required of Params
	//!
	virtual bool reinit(bool override);

	//! Virtual method used in in sorting geometry
	//! Only needed if geometry is not opaque.
	//! Override default, just find distance to rake extents.
	virtual float getCameraDistance(ViewpointParams* vpp, RegionParams* rp , int timestep);
	
	virtual bool IsOpaque() {return true;}
	//!
	//! Determine if the specified variable is being used
	//!
	virtual bool usingVariable(const std::string& varname){
		for (int i = 0; i<3; i++){
			if (GetFieldVariableName(i) == varname) return true;
		}
		if (IsTerrainMapped() && (varname == "HGT")) return true;
		return false;
	}


	//Get/Set methods based on XML rep:
	const vector<double>& GetRakeExtents(){
		return (GetBox()->GetExtents());
	}
	void GetRakeExtents(double exts[6]){
		GetBox()->GetExtents(exts);
	}
	
	void SetRakeExtents(const vector<double>&exts){
		GetBox()->SetExtents(exts);
		setAllBypass(false);
	}
	const vector<long>& GetRakeGrid(){
		return (GetRootNode()->GetElementLong(_rakeGridTag));
	}
	void SetRakeGrid(const int grid[3]){
		vector<long> griddims;
		for (int i = 0; i<3; i++) griddims.push_back((long)grid[i]);
		GetRootNode()->SetElementLong(_rakeGridTag,griddims);
	}
	float GetLineThickness(){
		return ((float)GetRootNode()->GetElementDouble(_lineThicknessTag)[0]);
	}
	void SetLineThickness(double val){
		GetRootNode()->SetElementDouble(_lineThicknessTag, val);
	}

	//Specify a scale factor for vector length.  (1 is scene diameter)/100
	float GetVectorScale(){
		return ((float)GetRootNode()->GetElementDouble(_vectorScaleTag)[0]);
	}
	void SetVectorScale(double val){
		GetRootNode()->SetElementDouble(_vectorScaleTag, val);
	}
	void SetRefinementLevel(int level){
		GetRootNode()->SetElementLong(_RefinementLevelTag, level);
		setAllBypass(false);
	}
	int GetRefinementLevel(){
		return (GetRootNode()->GetElementLong(_RefinementLevelTag)[0]);
	}
	bool IsTerrainMapped(){
		return (GetRootNode()->GetElementLong(_terrainMapTag)[0]);
	}
	void SetTerrainMapped(bool val) {
		GetRootNode()->SetElementLong(_terrainMapTag, (val ? 1:0));
		setAllBypass(false);
	}
	void SetConstantColor(const float rgb[3]);
	const float *GetConstantColor();
	void SetVisualizerNum(int viznum);
	int GetVisualizerNum();
	void SetFieldVariableName(int i, const string& varName);
	const string& GetFieldVariableName(int i);

	void SetVariables3D(bool val) {
		GetRootNode()->SetElementLong(_variableDimensionTag,(val ? 3:2));
		setAllBypass(false);
	}
	bool VariablesAre3D() {
		return (GetRootNode()->GetElementLong(_variableDimensionTag)[0] == 3);
	}
	void AlignGridToData(bool val) {
		GetRootNode()->SetElementLong(_alignGridTag,(val ? 1:0));
		setAllBypass(false);
	}
	bool IsAlignedToData() {
		if (GetRootNode()->HasElementLong(_alignGridTag))
			return (GetRootNode()->GetElementLong(_alignGridTag)[0]);
		else return false;
	}
	const vector<long> GetGridAlignStrides(){
		return GetRootNode()->GetElementLong(_alignGridStridesTag);
	}
	void SetGridAlignStrides(const vector<long>& strides){
		GetRootNode()->SetElementLong(_alignGridStridesTag, strides);
	}
	//Utility function to find rake when it is aligned to data:
	void calcDataAlignment(double rakeExts[6], int rakeGrid[3]);
	//Utility function to recalculate vector scale factor
	void recalcVectorScale();
	
	static const string _arrowParamsTag;

protected:
static const string _shortName;
static const string _constantColorTag;
static const string _rakeExtentsTag;
static const string _rakeGridTag;
static const string _lineThicknessTag;
static const string _vectorScaleTag;
static const string _terrainMapTag;
static const string _variableDimensionTag;
static const string _alignGridTag;
static const string _alignGridStridesTag;
float _constcolorbuf[4];


}; //End of Class ArrowParams
};
#endif //ARROWPARAMS_H

