
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
	
	virtual Box* GetBox() {
		ParamNode* pNode = GetRootNode()->GetNode(Box::_boxTag);
		if (pNode) return (Box*)pNode->GetParamsBase();
		Box* box = new Box();
		GetRootNode()->AddNode(Box::_boxTag, box->GetRootNode());
		return box;
	}

	
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
		if (IsTerrainMapped() && (varname == GetHeightVariableName())) return true;
		return false;
	}


	//Get/Set methods based on XML rep:
	const vector<double>& GetRakeLocalExtents(){
		return (GetBox()->GetLocalExtents());
	}
	void GetRakeLocalExtents(double exts[6]){
		GetBox()->GetLocalExtents(exts);
	}
	
	void SetRakeLocalExtents(const vector<double>&exts){
		GetBox()->SetLocalExtents(exts);
		setAllBypass(false);
	}
	const vector<long>& GetRakeGrid(){
		const vector<long> defaultGrid(3,1);
		return (GetRootNode()->GetElementLong(_rakeGridTag,defaultGrid));
	}
	void SetRakeGrid(const int grid[3]){
		vector<long> griddims;
		for (int i = 0; i<3; i++) griddims.push_back((long)grid[i]);
		GetRootNode()->SetElementLong(_rakeGridTag,griddims);
	}
	float GetLineThickness(){
		const vector<double> one(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_lineThicknessTag,one)[0]);
	}
	void SetLineThickness(double val){
		GetRootNode()->SetElementDouble(_lineThicknessTag, val);
	}

	//Specify a scale factor for vector length.  (1 is scene diameter)/100
	float GetVectorScale(){
		const vector<double>defaultScale(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_vectorScaleTag,defaultScale)[0]);
	}
	void SetVectorScale(double val){
		GetRootNode()->SetElementDouble(_vectorScaleTag, val);
	}
	
	bool IsTerrainMapped(){
		const vector<long>off(1,0);
		return (GetRootNode()->GetElementLong(_terrainMapTag,off)[0]);
	}
	void SetTerrainMapped(bool val) {
		GetRootNode()->SetElementLong(_terrainMapTag, (val ? 1:0));
		setAllBypass(false);
	}
	void SetConstantColor(const float rgb[3]);
	const float *GetConstantColor();
	
	void SetFieldVariableName(int i, const string& varName);
	const string& GetFieldVariableName(int i);
	void SetHeightVariableName(const string& varName);
	const string& GetHeightVariableName();

	void SetVariables3D(bool val) {
		GetRootNode()->SetElementLong(_variableDimensionTag,(val ? 3:2));
		setAllBypass(false);
	}
	bool VariablesAre3D() {
		const vector<long>three(1,3);
		return (GetRootNode()->GetElementLong(_variableDimensionTag,three)[0] == 3);
	}
	void AlignGridToData(bool val) {
		GetRootNode()->SetElementLong(_alignGridTag,(val ? 1:0));
		setAllBypass(false);
	}
	bool IsAlignedToData() {
		const vector<long> notAligned(1,0);
		return (GetRootNode()->GetElementLong(_alignGridTag,notAligned)[0]);
	}
	const vector<long> GetGridAlignStrides(){
		const vector<long> defaultStrides(3,10);
		return GetRootNode()->GetElementLong(_alignGridStridesTag,defaultStrides);
	}
	void SetGridAlignStrides(const vector<long>& strides){
		GetRootNode()->SetElementLong(_alignGridStridesTag, strides);
	}
	//Utility function to find rake when it is aligned to data:
	void calcDataAlignment(double rakeExts[6], int rakeGrid[3], size_t timestep);
	//Utility function to recalculate vector scale factor
	double calcDefaultScale();
	
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
static const string _heightVariableNameTag;
float _constcolorbuf[4];


}; //End of Class ArrowParams
};
#endif //ARROWPARAMS_H

