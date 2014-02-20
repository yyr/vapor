
#ifndef ARROWPARAMS_H
#define ARROWPARAMS_H

#include "vapor/ParamNode.h"
#include "params.h"
#include "command.h"
#include "datastatus.h"

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

	virtual void Validate(bool useDefault){ reinit(useDefault);}
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
	
	int SetRakeLocalExtents(const vector<double>&exts){
		int rc = 0;
		vector<double> xexts;
		for (int i = 0; i<6; i++){
			xexts.push_back(exts[i]);
			if (xexts[i] < 0.){ xexts[i] = 0.; rc = -1;}
			if (xexts[i] > 10000.){ xexts[i] = 10000.; rc = -1;}
			if (i>=3 && xexts[i-3] >= xexts[i]){ xexts[i-3] = xexts[i]; rc = -1;}
		}
	
		GetBox()->CaptureSetDouble(Box::_extentsTag,"Set Barb Rake extents", xexts, this);
		return rc;
	}
	const vector<long>& GetRakeGrid(){
		const vector<long> defaultGrid(3,1);
		return (GetRootNode()->GetElementLong(_rakeGridTag,defaultGrid));
	}
	int SetRakeGrid(const int grid[3]){
		int rc = 0;
		vector<long> griddims;
		for (int i = 0; i<3; i++){
			griddims.push_back((long)grid[i]);
			if (griddims[i] < 1) {griddims[i] = 1; rc= -1;}
			if (griddims[i] > 10000) {griddims[i] = 10000; rc= -1;}
		}
		CaptureSetLong(_rakeGridTag,"Set barb grid", griddims);
		return rc;
	}
	float GetLineThickness(){
		const vector<double> one(1,1.);
		
		return ((float)GetRootNode()->GetElementDouble(_lineThicknessTag,one)[0]);
	}
	int SetLineThickness(double val){
		int rc = 0;
		if (val <= 0. || val > 100.) {val = 1.; rc = -1;}
		CaptureSetDouble(_lineThicknessTag,"Set barb thickness",val);
		return rc;
	}

	//Specify a scale factor for vector length.  (1 is scene diameter)/100
	float GetVectorScale(){
		const vector<double>defaultScale(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_vectorScaleTag,defaultScale)[0]);
	}
	int SetVectorScale(double val){
		int rc = 0;
		if (val <= 0. || val > 1.e30){
			val = 0.01; 
			rc = -1;
		}
		CaptureSetDouble(_vectorScaleTag, "set barb scale", val);
		return rc;
	}
	
	bool IsTerrainMapped(){
		const vector<long>off(1,0);
		return ((bool)GetRootNode()->GetElementLong(_terrainMapTag,off)[0]);
	}
	int SetTerrainMapped(bool val) {
		int rc = CaptureSetLong(_terrainMapTag, "Set barb terrain-mapping", (long)val);
		setAllBypass(false);
		return rc;
	}
	int SetConstantColor(const float rgb[3]);
	const float *GetConstantColor();
	
	int SetFieldVariableName(int i, const string& varName);
	const string& GetFieldVariableName(int i);
	int SetHeightVariableName(const string& varName);
	const string& GetHeightVariableName();

	int SetVariables3D(bool val) {
		if (val == VariablesAre3D()) return 0;
		CaptureSetLong(_variableDimensionTag,"Set barb var dimensions",(long)(val ? 3:2));
		setAllBypass(false);
		return 0;
	}
	bool VariablesAre3D() {
		const vector<long>three(1,3);
		return (GetRootNode()->GetElementLong(_variableDimensionTag,three)[0] == 3);
	}
	int AlignGridToData(bool val) {
		if (IsAlignedToData() == val) return 0;
		CaptureSetLong(_alignGridTag, "Set barb grid alignment",(val ? 1:0));
		return 0;
	}
	bool IsAlignedToData() {
		const vector<long> notAligned(1,0);
		return ((bool)GetRootNode()->GetElementLong(_alignGridTag,notAligned)[0]);
	}
	const vector<long> GetGridAlignStrides(){
		
		const vector<long> defaultStrides(3,10);
		return GetRootNode()->GetElementLong(_alignGridStridesTag,defaultStrides);
	}
	int SetGridAlignStrides(const vector<long>& strides){
		int rc = 0;
		vector<long> xstrides;
		const vector<long>& pstrides = GetGridAlignStrides();
		bool changed = false;
		for (int i = 0; i<2; i++){
			xstrides.push_back(strides[i]);
			if (xstrides[i] <= 0){
				xstrides[i] = 1;
				rc = -1;
			}
			if (xstrides[i] != pstrides[i]) changed = true;
		}
		if (!changed) return rc;
		CaptureSetLong(_alignGridStridesTag, "Set barb grid strides", xstrides);
		return rc;
	}
	//Utility function to find rake when it is aligned to data:
	void calcDataAlignment(double rakeExts[6], int rakeGrid[3], size_t timestep);
	//Utility function to recalculate vector scale factor
	double calcDefaultScale();
	
	static const string _arrowParamsTag;

protected:
static const string _shortName;
static const string _constantColorTag;
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

