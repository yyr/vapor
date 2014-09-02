
#ifndef ISOLINEPARAMS_H
#define ISOLINEPARAMS_H

#include "vapor/ParamNode.h"
#include "params.h"
#include "datastatus.h"
#include "mapperfunction.h"
#include "transferfunction.h"

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
		if (VariablesAre3D()){
			ParamNode* pNode = GetRootNode()->GetNode(IsolineParams::_3DBoxTag);
			if (pNode) {
				return (Box*)pNode->GetNode(Box::_boxTag)->GetParamsBase();
			}
			else assert(0);
			return 0;
		} else {
			ParamNode* pNode = GetRootNode()->GetNode(IsolineParams::_2DBoxTag);
			if (pNode) {
				return (Box*)pNode->GetNode(Box::_boxTag)->GetParamsBase();
			}
			else assert(0);
			return 0;
		}
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
	float GetPanelLineThickness(){
		const vector<double> one(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_panelLineThicknessTag,one)[0]);
	}
	void SetPanelLineThickness(double val){
		GetRootNode()->SetElementDouble(_panelLineThicknessTag, val);
	}
	float GetTextSize(){
		const vector<double> one(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_textSizeTag,one)[0]);
	}
	void SetTextSize(double val){
		GetRootNode()->SetElementDouble(_textSizeTag, val);
	}
	float GetTextDensity(){
		const vector<double> one(1,1.);
		return ((float)GetRootNode()->GetElementDouble(_textDensityTag,one)[0]);
	}
	void SetTextDensity(double val){
		GetRootNode()->SetElementDouble(_textDensityTag, val);
	}
	bool textEnabled() {
		return (GetRootNode()->GetElementLong(_textEnabledTag)[0] != 0);
	}
	void SetTextEnabled(bool val){
		GetRootNode()->SetElementLong(_textEnabledTag,(long)val);
	}
	bool UseSingleColor() {
		return (GetRootNode()->GetElementLong(_useSingleColorTag)[0] != 0);
	}
	void SetUseSingleColor(bool val) {
		GetRootNode()->SetElementLong(_useSingleColorTag, (long)val);
	}
	void SetSingleColor(double clr[3]){
		vector<double> dcolors;
		for (int i = 0; i<3; i++) dcolors.push_back(clr[i]);
		GetRootNode()->SetElementDouble(_singleColorTag,dcolors);
	}
	const vector<double> GetSingleColor(){
		return GetRootNode()->GetElementDouble(_singleColorTag);
	}
	void SetPanelBackgroundColor(const float rgb[3]);

	const vector<double>& GetPanelBackgroundColor();

	void getLineColor(int isoNum, float lineColor[3]);
	
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
	int getNumIsovalues(){
		return GetIsovalues().size();
	}
	int GetNumVariables3D();
	int GetNumVariables2D();
	IsoControl* GetIsoControl(){
		vector<string>path;
		if (VariablesAre3D()) path.push_back(_Variables3DTag);
		else path.push_back(_Variables2DTag);
		path.push_back(GetVariableName());
		path.push_back(_IsoControlTag);
		if (!GetRootNode()->GetNode(path)) return 0;
		return (IsoControl*)(GetRootNode()->GetNode(path)->GetParamsBase());
	}
	IsoControl* GetIsoControl(string varname, bool is3D){
		vector<string>path;
		if (is3D) path.push_back(_Variables3DTag);
		else path.push_back(_Variables2DTag);
		path.push_back(varname);
		path.push_back(_IsoControlTag);
		if (!GetRootNode()->GetNode(path)) return 0;
		return (IsoControl*)(GetRootNode()->GetNode(path)->GetParamsBase());
	}
	int SetIsoControl(const string varname, IsoControl* iControl, bool is3D);
		
	virtual MapperFunction* GetMapperFunc();
	virtual bool UsesMapperFunction() {return true;}
	const vector<double>& GetIsovalues(){
		return (GetIsoControl()->getIsoValues());
	}
	void SetIsovalues(const vector<double>& values){
		GetIsoControl()->setIsoValues(values);
	}
	const vector<double>& GetCursorCoords(){
		return (GetRootNode()->GetElementDouble(_cursorCoordsTag));
	}
	void SetCursorCoords(const vector<double>& coords){
		GetRootNode()->SetElementDouble(_cursorCoordsTag,coords);
	}
	//The color map and the iso control share the same edit bounds.
	virtual float getMinColorEditBound(int) {
		return (float)getMinEditBound();
	}
	virtual float getMaxColorEditBound(int ) {
		return (float)getMaxEditBound();
	}
	virtual void setMinColorEditBound(float val, int) {
		setMinEditBound((double)val);
	}
	virtual void setMaxColorEditBound(float val, int ) {
		setMaxEditBound((double)val);
	}
	double getMinEditBound(){
		return GetRootNode()->GetElementDouble(_editBoundsTag)[0];
	}
	double getMaxEditBound(){
		return GetRootNode()->GetElementDouble(_editBoundsTag)[1];
	}
	void setMinEditBound(double val){
		vector<double>vals = GetRootNode()->GetElementDouble(_editBoundsTag);
		if (vals.size()<1) vals.push_back(val);
		vals[0]=val;
		GetRootNode()->SetElementDouble(_editBoundsTag,vals);
	}
	void setMaxEditBound(double val){
		vector<double>vals = GetRootNode()->GetElementDouble(_editBoundsTag);
		while (vals.size()<2) vals.push_back(val);
		vals[1]=val;
		GetRootNode()->SetElementDouble(_editBoundsTag,vals);
	}
	void SetNumDigits(int val){
		GetRootNode()->SetElementLong(_numDigitsTag,val);
	}
	int GetNumDigits(){
		return GetRootNode()->GetElementLong(_numDigitsTag)[0];
	}
	void SetHistoStretch(float factor){
		GetRootNode()->SetElementDouble(_histoScaleTag,(double)factor);
	}
	float GetHistoStretch(){
		return GetRootNode()->GetElementDouble(_histoScaleTag)[0];
	}
	void SetHistoBounds(const float bnds[2]){
		IsoControl* isoContr = GetIsoControl();
		if(!isoContr) return;
		if(isoContr->getMinHistoValue() == bnds[0] &&
			isoContr->getMaxHistoValue() == bnds[1]) return;
		isoContr->setMinHistoValue(bnds[0]);
		isoContr->setMaxHistoValue(bnds[1]);
		isoContr->setMinColorMapValue(bnds[0]);
		isoContr->setMaxColorMapValue(bnds[1]);
		setAllBypass(false);
	}
	void GetHistoBounds(float bounds[2]){
		 if (!GetIsoControl()){
			 bounds[0] = 0.f;
			 bounds[1] = 1.f;
		 } else {
			bounds[0]=GetIsoControl()->getMinHistoValue();
			bounds[1]=GetIsoControl()->getMaxHistoValue();
		 }
	 }
	//Following required of render params classes that have a histogram: 
	 virtual const float* getCurrentDatarange(){
		 GetHistoBounds(_histoBounds);
		 return _histoBounds;
	 }
	virtual int getSessionVarNum();
	virtual void hookupTF(TransferFunction* , int );
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
	
	virtual bool isPlanar() {return true;}
	float getRealImageWidth() {
		const vector<double>& exts = GetBox()->GetLocalExtents();
		return (float)(exts[3]-exts[0]);
	}
	float getRealImageHeight() {
		const vector<double>& exts = GetBox()->GetLocalExtents();
		return (float)(exts[4]-exts[1]);
	}
	//Space the isovalues to specified interval
	void spaceIsovals(float minval, float maxval);
	
	static const string _isolineParamsTag;

protected:
	static const string _shortName;
	static const string _numDigitsTag;
	static const string _panelBackgroundColorTag;
	static const string _isolineExtentsTag;
	static const string _lineThicknessTag;
	static const string _panelLineThicknessTag;
	static const string _textSizeTag;
	static const string _textDensityTag;
	static const string _variableDimensionTag;
	static const string _cursorCoordsTag;
	static const string _2DBoxTag;
	static const string _3DBoxTag;
	static const string _editBoundsTag;
	static const string _histoScaleTag;
	static const string _histoBoundsTag;
	static const string _IsoControlTag;
	static const string _textEnabledTag;
	static const string _useSingleColorTag;
	static const string _singleColorTag;

	float selectPoint[3];
	float _histoBounds[2];
	
}; //End of Class IsolineParams
};
#endif //ISOLINEPARAMS_H

