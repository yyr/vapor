
#include "isolineparams.h"
#include "datastatus.h"
#include "vapor/Version.h"
#include "glutil.h"
#include <string>
#include "transferfunction.h"
#include "vapor/MapperFunctionBase.h"
#include "command.h"

using namespace VetsUtil;
using namespace VAPoR;
const string IsolineParams::_shortName = "Contours";
const string IsolineParams::_isolineParamsTag = "IsolineParams";
const string IsolineParams::_panelBackgroundColorTag= "PanelBackgroundColor";
const string IsolineParams::_isolineExtentsTag = "IsolineExtents";
const string IsolineParams::_lineThicknessTag = "LineThickness";
const string IsolineParams::_panelLineThicknessTag = "PanelLineThickness";
const string IsolineParams::_textSizeTag = "TextSize";
const string IsolineParams::_textDensityTag = "TextDensity";
const string IsolineParams::_variableDimensionTag = "VariableDimension";
const string IsolineParams::_cursorCoordsTag = "CursorCoords";
const string IsolineParams::_2DBoxTag = "Box2D";
const string IsolineParams::_3DBoxTag = "Box3D";

const string IsolineParams::_numDigitsTag = "NumDigits";
const string IsolineParams::_textEnabledTag = "TextEnabled";
const string IsolineParams::_useSingleColorTag = "UseSingleColor";
const string IsolineParams::_singleColorTag = "SingleColor";

namespace {
	const string IsolineName = "IsolineParams";
};

IsolineParams::IsolineParams(
	XmlNode *parent, int winnum
) : RenderParams(parent, IsolineParams::_isolineParamsTag, winnum) {

	restart();
}


IsolineParams::~IsolineParams() {
}

//Initialize for new metadata.  Keep old transfer functions
//
void IsolineParams::
Validate(int type){

	DataStatus* ds = DataStatus::getInstance();
	DataMgrV3_0* dataMgr = ds->getDataMgr();
	bool doOverride = (type == 0);
	int numVariables2D = dataMgr->GetDataVarNames(2,true).size();
	int numVariables3D = dataMgr->GetDataVarNames(3,true).size();
	int totNumVariables = numVariables2D+numVariables3D;
	if (totNumVariables <= 0) return;
	if (doOverride){//make it 3D if there are any 3D variables
		if (numVariables3D>0) SetVariables3D(true);
		else SetVariables3D(false);
	}
	bool is3D = VariablesAre3D();
	int numVariables;
	if (is3D) numVariables = numVariables3D;
	else numVariables = numVariables2D;
	//Set up the current variable. If doOverride is true, just make the current variable the first variable in the VDC.
	//Otherwise try to use the current variable in the state
	string varname;
	if (doOverride){
		
		if (is3D) {
			varname = dataMgr->GetDataVarNames(3,true)[0];
		}
		else {
			varname = dataMgr->GetDataVarNames(2,true)[0];
		}
			
		SetVariableName(varname);
	} else {
		
		string varname = GetVariableName();
		bool found = false;
		if (is3D){
			vector<string> varnames = dataMgr->GetDataVarNames(3,true);
			for (int i = 0; i<varnames.size(); i++)
				if (varname == varnames[i]){
					found = true;
					break;
				}
			if (!found) varname = dataMgr->GetDataVarNames(3,true)[0];
		}
		else {
			vector<string> varnames = dataMgr->GetDataVarNames(2,true);
			for (int i = 0; i<varnames.size(); i++)
				if (varname == varnames[i]){
					found = true;
					break;
				}
			if (!found) varname = dataMgr->GetDataVarNames(2,true)[0];
		}
		
	}
	//Set up the numRefinements. 
	int maxRefinement = dataMgr->GetNumRefLevels(varname)-1;
	int numrefs = GetRefinementLevel();
	if (doOverride) { 
		numrefs = 0;
		SetFidelityLevel(0);
		SetIgnoreFidelity(false);
		SetNumDigits(5);
	} else {  //Try to use existing values
		if (numrefs > maxRefinement) numrefs = maxRefinement;
		if (numrefs < 0) numrefs = 0;
		if (GetNumDigits() < 2) SetNumDigits(2);
		if (GetNumDigits() > 12) SetNumDigits(12);
	}
	SetRefinementLevel(numrefs);
	//Make sure fidelity is valid:
	int fidelity = GetFidelityLevel();
	
	vector<size_t> cratios;
	dataMgr->GetCRatios(varname, cratios);
	if (dataMgr && fidelity > maxRefinement+cratios.size()-1)
		SetFidelityLevel(maxRefinement+cratios.size()-1);
	//Set up the compression level.  Whether or not override is true, make sure
	//That the compression level is valid.  If override is true set it to 0;
	if (doOverride) SetCompressionLevel(0);
	else {
		int numCompressions = 0;
	
		if (ds->getDataMgr()) {
			numCompressions = cratios.size();
		}
		
		if (GetCompressionLevel() >= numCompressions){
			SetCompressionLevel(numCompressions-1);
		}
	}
	//Create arrays of pointers to IsoControls
	assert(totNumVariables > 0);
	IsoControl** new2DIsoControls = new IsoControl*[numVariables2D];
	IsoControl** new3DIsoControls = new IsoControl*[numVariables3D];
	//If we are overriding previous values, delete the existing isocontrols, replace with new ones.
	//Set the histo bounds to the actual bounds in the data
	if (doOverride){
		for (int i = 0; i<numVariables3D; i++){
			string varname = dataMgr->GetDataVarNames(3,true)[i];
			//will need to set the iso value:
			float dataMin = ds->getDefaultDataMin(varname);
			float dataMax = ds->getDefaultDataMax(varname);
			if (dataMin == dataMax){
				dataMin -= 0.5f; dataMax += 0.5;
			}
			
			new3DIsoControls[i] = new IsoControl(this, 8);
			new3DIsoControls[i]->setVarNum(i);
			new3DIsoControls[i]->setMinHistoValue(dataMin);
			new3DIsoControls[i]->setMaxHistoValue(dataMax);
			vector<double> isovals(1,0.5*(dataMin+dataMax)); 
			new3DIsoControls[i]->setIsoValues(isovals);
			
		}
		for (int i = 0; i<numVariables2D; i++){
			string varname = dataMgr->GetDataVarNames(2,true)[i];
			//will need to set the iso value:
			float dataMin = ds->getDefaultDataMin(varname);
			float dataMax = ds->getDefaultDataMax(varname);
			if (dataMin == dataMax){
				dataMin -= 0.5f; dataMax += 0.5;
			}
			
			new2DIsoControls[i] = new IsoControl(this, 8);
			new2DIsoControls[i]->setVarNum(i);
			new2DIsoControls[i]->setMinHistoValue(dataMin);
			new2DIsoControls[i]->setMaxHistoValue(dataMax);
			vector<double> isovals(1,0.5*(dataMin+dataMax)); 
			new2DIsoControls[i]->setIsoValues(isovals);
		}
	} else if (type == 1) {
		//attempt to make use of existing isocontrols.
		//delete any that are no longer referenced
		//Unnecessary with type=2.
		for (int i = 0; i<numVariables3D; i++){
			string varname = dataMgr->GetDataVarNames(3,true)[i];
			if(i<GetNumVariables3D()){ //make copy of existing ones, don't set their root nodes yet
				float dataMin = ds->getDefaultDataMin(varname);
				float dataMax = ds->getDefaultDataMax(varname);
				
				if (GetIsoControl(varname,true)){	
					new3DIsoControls[i] = (IsoControl*)GetIsoControl(varname,true)->deepCopy(0);
				} else {
					new3DIsoControls[i] = new IsoControl(this, 8);
					new3DIsoControls[i]->setVarNum(i);
					new3DIsoControls[i]->setMinHistoValue(dataMin);
					new3DIsoControls[i]->setMaxHistoValue(dataMax);
					vector<double> isovals(1,0.5*(dataMin+dataMax)); 
					new3DIsoControls[i]->setIsoValues(isovals);
				}
				new3DIsoControls[i]->setParams(this);
				
			} else { //create new  isocontrols
				
				new3DIsoControls[i] = new IsoControl(this, 8);
				new3DIsoControls[i]->setMinHistoValue(ds->getDefaultDataMin(varname));
				new3DIsoControls[i]->setMaxHistoValue(ds->getDefaultDataMax(varname));
				vector<double> isovals(1,0.5*(ds->getDefaultDataMin(varname)+ds->getDefaultDataMax(varname))); 
				new3DIsoControls[i]->setIsoValues(isovals);
				new3DIsoControls[i]->setVarNum(i);
				new3DIsoControls[i]->setParams(this);
				
			}
			
		}
		for (int i = 0; i<numVariables2D; i++){
			string varname = dataMgr->GetDataVarNames(2,true)[i];
			if(i<GetNumVariables2D()){ //make copy of existing ones, don't set their root nodes yet
				float dataMin = ds->getDefaultDataMin(varname);
				float dataMax = ds->getDefaultDataMax(varname);
				
				if (GetIsoControl(varname,false)){	
					new2DIsoControls[i] = (IsoControl*)GetIsoControl(varname,false)->deepCopy(0);
				} else {
					new2DIsoControls[i] = new IsoControl(this, 8);
					new2DIsoControls[i]->setVarNum(i);
					new2DIsoControls[i]->setMinHistoValue(dataMin);
					new2DIsoControls[i]->setMaxHistoValue(dataMax);
					vector<double> isovals(1,0.5*(dataMin+dataMax)); 
					new2DIsoControls[i]->setIsoValues(isovals);
				}
				new2DIsoControls[i]->setParams(this);
				
			} else { //create new  isocontrols
				
				new2DIsoControls[i] = new IsoControl(this, 8);
				new2DIsoControls[i]->setMinHistoValue(ds->getDefaultDataMin(varname));
				new2DIsoControls[i]->setMaxHistoValue(ds->getDefaultDataMax(varname));
				vector<double> isovals(1,0.5*(ds->getDefaultDataMin(varname)+ds->getDefaultDataMax(varname))); 
				new2DIsoControls[i]->setIsoValues(isovals);
				new2DIsoControls[i]->setVarNum(i);
				new2DIsoControls[i]->setParams(this);
				
			}
			
		}
	} //end if(doOverride or type == 1)

	if (type != 2){
		//Delete all existing variable nodes
		GetRootNode()->DeleteNode(_Variables3DTag);
		GetRootNode()->DeleteNode(_Variables2DTag);
	
		vector<string> path;
		vector<double>ccs;
		for (int i = 0; i<numVariables3D; i++){
			path.clear();
			path.push_back(_Variables3DTag);
			std::string varname = dataMgr->GetDataVarNames(3,true)[i];
			path.push_back(varname);
			path.push_back(_IsoControlTag);
			SetParamsBase(path, new3DIsoControls[i]);
			ccs = GetIsoControl(varname, true)->GetColorMap()->GetControlPoints();
		}
	
		for (int i = 0; i<numVariables2D; i++){
			path.clear();
			path.push_back(_Variables2DTag);
			std::string varname = dataMgr->GetDataVarNames(2,true)[i];
			path.push_back(varname);
			path.push_back(_IsoControlTag);
			SetParamsBase(path, new2DIsoControls[i]);
		}
		if (numVariables2D > 0)assert(GetRootNode()->GetNode(_Variables2DTag)->GetNumChildren() == numVariables2D);
		if (numVariables3D > 0)assert(GetRootNode()->GetNode(_Variables3DTag)->GetNumChildren() == numVariables3D);
	}
	
	delete [] new2DIsoControls;
	delete [] new3DIsoControls;
	
	if (doOverride) {
		SetHistoStretch(1.0);
	}
	

	
	
	const double* extents = ds->getLocalExtents();
	//Must set (or correct) extents for both 2D and 3D boxes
	for (int dim = 2; dim<4; dim++){
		if (dim == 2) SetVariables3D(false);
		else SetVariables3D(true);
		vector<double>newExtents(3,0.);
	
		if (doOverride) {
			for (int i = 0; i<3; i++){
				newExtents.push_back((double)( extents[i+3]-extents[i]));
			}
			newExtents[2] = newExtents[5] = extents[5]*0.5;
		
		} else {
			double newExts[6];
			GetLocalExtents(newExts);
		
			for (int i = 0; i<3; i++){
				if (i != 2){
					newExts[i] = Max(newExts[i], 0.);
					newExts[i+3] = Min(newExts[i+3], (double)(extents[i+3]-extents[i]));
				}
				if (newExts[i] > newExts[i+3]) newExts[i+3] = newExts[i];
			}
			newExtents.clear();
			for (int i = 0; i<6; i++) newExtents.push_back(newExts[i]);
			newExtents[5] = newExtents[2];
		}
		SetLocalExtents(newExtents);
	}
	//Set dimensionality back to what it was before...
	SetVariables3D(is3D);

	if (doOverride) { //set default colors
		const float black_color[3] = {.0, .0, .0};
		SetPanelBackgroundColor(black_color);
	}
	initializeBypassFlags();
	
	if (doOverride){ SetLineThickness(1.0); SetPanelLineThickness(1.0);}
	else {
		if (GetLineThickness() < 1.0 || GetLineThickness() > 100.) SetLineThickness(1.0);
		if (GetPanelLineThickness() < 1.0 || GetPanelLineThickness() > 100.) SetPanelLineThickness(1.0);
	}
	initializeBypassFlags();
	IsoControl* ictl = GetIsoControl();
	return;
}
//Set everything to default values
void IsolineParams::restart() {
	Command::blockCapture();
	//Delete any child nodes
	GetRootNode()->DeleteAll();
	
	SetNumDigits(5);
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetFidelityLevel(0);
	SetIgnoreFidelity(false);
	SetVizNum(0);
	
	SetVariables3D(true);
	double clr[3] = {1.,1.,1.};
	SetSingleColor(clr);
	SetUseSingleColor(true);
	
	//Create the isocontrol.  Just one placeholder, but it contains the histo bounds and isovalues and colors
	//The transferFunction and Isocontrol share the map/histo bounds and the edit bounds.
	vector<string>ctlPath;
	ctlPath.push_back(_Variables2DTag);
	ctlPath.push_back("isovar2d");
	ctlPath.push_back(_IsoControlTag);
	IsoControl* iControl = (IsoControl*)IsoControl::CreateDefaultInstance();
	iControl->setMinMapValue(0.);
	iControl->setMaxMapValue(1.);
	SetIsoControl("isovar2d",iControl,false);
	ctlPath.clear();
	ctlPath.push_back(_Variables3DTag);
	ctlPath.push_back("isovar3d");
	ctlPath.push_back(_IsoControlTag);
	iControl = (IsoControl*)IsoControl::CreateDefaultInstance();
	iControl->setMinMapValue(0.);
	iControl->setMaxMapValue(1.);
	iControl->setParams(this);
	SetIsoControl("isovar3d",iControl,true);
	SetVariableName("isovar3d");
	selectPoint[0]=selectPoint[1]=selectPoint[2]=0.f;
	vector<double>zeros;
	zeros.push_back(0.);
	SetIsovalues(zeros);
	zeros.push_back(0.);
	SetCursorCoords(zeros);
	float bnds[2] = {0.,1.};
	SetHistoBounds(bnds);
	SetHistoStretch(1.);
	zeros.push_back(0.);

	vector<double> bounds;
	bounds.push_back(0.);
	bounds.push_back(1.);
	SetEditBounds(bounds);
	SetEnabled(false);
	const float black_color[3] = {.0, .0, .0};
	
	SetPanelBackgroundColor(black_color);

	SetLineThickness(1.0);
	
	vector<double> exts;
	exts.push_back(-1.);
	exts.push_back(-1.);
	exts.push_back(-1.);
	
	for (int i = 0; i<3; i++){
		exts.push_back(1.);
	}
	//Create box nodes if they don't already exist
	if (!GetRootNode()->HasChild(IsolineParams::_2DBoxTag)){
		Box* myBox2 = new Box();
		vector<string> path;
		path.push_back(IsolineParams::_2DBoxTag);
		path.push_back(Box::_boxTag);

		SetParamsBase(path,myBox2);
	}
	if (!GetRootNode()->HasChild(IsolineParams::_3DBoxTag)){
		Box* myBox3 = new Box();
		vector<string> path;
		path.push_back(IsolineParams::_3DBoxTag);
		path.push_back(Box::_boxTag);

		SetParamsBase(path,myBox3);
	}
	
	//Don't set the Box values until after it has been registered:
	SetLocalExtents(exts);
	SetVariables3D(true);
	SetLocalExtents(exts);
	SetTextSize(10.);
	SetTextDensity(0.25);
	SetTextEnabled(false);
	GetBox()->SetAngles(zeros, this);
	Command::unblockCapture();
	
}

int IsolineParams::GetNumVariables2D() {
	if (!GetRootNode()->HasChild(_Variables2DTag)) return 0;
	return(GetRootNode()->GetNode(_Variables2DTag)->GetNumChildren());
}
int IsolineParams::GetNumVariables3D() {
	if (!GetRootNode()->HasChild(_Variables3DTag)) return 0;
	return(GetRootNode()->GetNode(_Variables3DTag)->GetNumChildren());
}
void IsolineParams::SetPanelBackgroundColor(const float rgb[3]){
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	SetValueDouble(_panelBackgroundColorTag, "Set background color", valvec);
}

const vector<double> IsolineParams::GetPanelBackgroundColor() {
	return GetValueDoubleVec(_panelBackgroundColorTag);
}

void IsolineParams::spaceIsovals(float minval, float interval){
	vector<double>newIsos;
	const vector<double>& isovals = GetIsovalues();
	if (isovals.size() == 1) newIsos.push_back(minval);
	else {
		for (int i = 0; i<isovals.size(); i++){
			double ival = minval + i*interval;
			newIsos.push_back(ival);
		}
		SetIsovalues(newIsos);
	}
}
//Hook up the colors from a transfer function 
//
void IsolineParams::
hookupTF(TransferFunction* tf, int ){
	//We want to keep the existing IsoControl,
	//but replace its color control points.
	IsoControl *isoctl = GetIsoControl();
	ColorMapBase* icmap = isoctl->GetColorMap();
	ColorMapBase* vcm = tf->GetColorMap();
	icmap->clear();
	
	//Determine normalized values
	float minval = vcm->minValue();
	float maxval = vcm->maxValue();
	for (int i = 0; i< vcm->numControlPoints(); i++){
		float normval = (vcm->controlPointValue(i) - minval)/(maxval - minval);
		icmap->addNormControlPoint(normval,vcm->controlPointColor(i));
	}
}

TransferFunction* IsolineParams::GetTransferFunction(){
	//Convert it to a transfer function so it can be saved as such.
	//Start with a default TransferFunction, replace its colormap with
	//The one from the 
	TransferFunction* tf = new TransferFunction(this,8);
	
	//Set it opaque
	tf->setOpaque();
	IsoControl *isoctl = GetIsoControl();
	ColorMapBase* icmap = isoctl->GetColorMap();
	//Remove the colors from the TF
	ColorMapBase* tfcm = tf->GetColorMap();
	tfcm->clear();
	
	tf->setMinMapValue(isoctl->getMinHistoValue());
	tf->setMaxMapValue(isoctl->getMaxHistoValue());
	//Determine normalized values
	float minval = icmap->minValue();
	float maxval = icmap->maxValue();
	//Add the control points from the IsoControl
	for (int i = 0; i< icmap->numControlPoints(); i++){
		float normval = (icmap->controlPointValue(i) - minval)/(maxval - minval);
		tfcm->addNormControlPoint(normval,icmap->controlPointColor(i));
	}
	return  tf;
}
void IsolineParams::getLineColor(int isoNum, float lineColor[3]){
	if (UseSingleColor()){
		const vector<double>& clr = GetSingleColor();
		for (int i = 0; i<3; i++) lineColor[i] = clr[i];
		return;
	}
	float isoval = (float)GetIsovalues()[isoNum];
	ColorMapBase* cmap = GetIsoControl()->GetColorMap();
	ColorMapBase::Color clr = cmap->color(isoval);
	clr.toRGB(lineColor);
}
int IsolineParams::
SetIsoControl(const string varname, IsoControl* ictl, bool is3D){
	vector<string>path;
	if (is3D)path.push_back(_Variables3DTag);
	else path.push_back(_Variables2DTag);
	path.push_back(varname);
	path.push_back(_IsoControlTag);
	return SetParamsBase(path,ictl);
	
}
	
