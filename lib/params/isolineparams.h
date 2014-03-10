
#ifndef ISOLINEPARAMS_H
#define ISOLINEPARAMS_H

#include "vapor/ParamNode.h"
#include "params.h"
#include "datastatus.h"

namespace VAPoR {

class PARAMS_API IsolineParams : public RenderParams {
public:
	IsolineParams(XmlNode *parent, int winnum);
	static ParamsBase* CreateDefaultInstance() {
		return new IsolineParams(0, -1);
	}
	const std::string& getShortName() {return _shortName;}
	virtual ~IsolineParams();
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
			if (GetVariableName() == varname) return true;
		}
		return false;
	}
	void setSelectedPointLocal(const float point[3]){
		for (int i = 0; i<3; i++) selectPoint[i] = point[i];
	}
	virtual const float* getSelectedPointLocal() {
		return selectPoint;
	}

	//Get/Set methods based on XML rep:
	const vector<double>& GetIsolineLocalExtents(){
		return (GetBox()->GetLocalExtents());
	}
	void GetLocalExtents(double exts[6]){
		GetBox()->GetLocalExtents(exts);
	}
	
	void SetLocalExtents(const vector<double>&exts){
		GetBox()->SetLocalExtents(exts);
		setAllBypass(false);
	}
	
	float GetLineThickness(){
		const vector<double> one(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_lineThicknessTag,one)[0]);
	}
	void SetLineThickness(double val){
		GetRootNode()->SetElementDouble(_lineThicknessTag, val);
	}

	void SetConstantColor(const float rgb[3]);
	const vector<double>& GetConstantColor();
	
	void SetVariableName(const string& varName);
	const string& GetVariableName();

	void SetVariables3D(bool val) {
		GetRootNode()->SetElementLong(_variableDimensionTag,(val ? 3:2));
		setAllBypass(false);
	}
	bool VariablesAre3D() {
		const vector<long>three(1,3);
		return (GetRootNode()->GetElementLong(_variableDimensionTag,three)[0] == 3);
	}
	const vector<double>& GetIsovalues(){
		return (GetRootNode()->GetElementDouble(_isovaluesTag));
	}
	void SetIsovalues(const vector<double>& values){
		GetRootNode()->SetElementDouble(_isovaluesTag,values);
	}
	const vector<double>& GetCursorCoords(){
		return (GetRootNode()->GetElementDouble(_cursorCoordsTag));
	}
	void SetCursorCoords(const vector<double>& coords){
		GetRootNode()->SetElementDouble(_cursorCoordsTag,coords);
	}
	
	//Override default, allow probe manip to go outside of data:
	virtual bool isDomainConstrained() {return false;}
	float getLocalBoxMin(int i) {return (float)GetBox()->GetLocalExtents()[i];}
	float getLocalBoxMax(int i) {return (float)GetBox()->GetLocalExtents()[i+3];}
	void setLocalBoxMin(int i, float val){
		double exts[6];
		GetBox()->GetLocalExtents(exts);
		exts[i] = val;
		GetBox()->SetLocalExtents(exts);
	}
	void setLocalBoxMax(int i, float val){
		double exts[6];
		GetBox()->GetLocalExtents(exts);
		exts[i+3] = val;
		GetBox()->SetLocalExtents(exts);
	}
	
	float getRealImageWidth() {
		const vector<double>& exts = GetBox()->GetLocalExtents();
		return (float)(exts[3]-exts[0]);
	}
	float getRealImageHeight() {
		const vector<double>& exts = GetBox()->GetLocalExtents();
		return (float)(exts[4]-exts[1]);
	}
	
	static const string _isolineParamsTag;

protected:
	static const string _shortName;
	static const string _constantColorTag;
	static const string _isolineExtentsTag;
	static const string _lineThicknessTag;
	static const string _variableDimensionTag;
	static const string _cursorCoordsTag;
	static const string _isovaluesTag;

	float selectPoint[3];
	
}; //End of Class IsolineParams
};
#endif //ISOLINEPARAMS_H

