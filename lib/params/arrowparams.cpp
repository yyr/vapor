
#include "arrowparams.h"
#include "datastatus.h"
#include "vapor/Version.h"

#include <string>

using namespace VetsUtil;
using namespace VAPoR;
const string ArrowParams::_shortName = "Barbs";
const string ArrowParams::_arrowParamsTag = "ArrowParams";
const string ArrowParams::_constantColorTag = "ConstantColor";
const string ArrowParams::_rakeExtentsTag = "RakeExtents";
const string ArrowParams::_rakeGridTag = "GridDimensions";
const string ArrowParams::_lineThicknessTag = "LineThickness";
const string ArrowParams::_vectorScaleTag = "VectorScale";
const string ArrowParams::_terrainMapTag = "TerrainMap";
const string ArrowParams::_alignGridTag = "GridAlignedToData";
const string ArrowParams::_alignGridStridesTag = "GridAlignedStrides";
const string ArrowParams::_variableDimensionTag = "VariableDimension";
const string ArrowParams::_heightVariableNameTag = "HeightVariable";

namespace {
	const string ArrowName = "ArrowParams";
};

ArrowParams::ArrowParams(
	XmlNode *parent, int winnum
) : RenderParams(parent, ArrowParams::_arrowParamsTag, winnum) {

	restart();
}


ArrowParams::~ArrowParams() {
}

//Initialize for new metadata.  Keep old transfer functions
//
// For each new variable in the metadata create a variable child node, and build the
// associated isoControl and transfer function nodes.
bool ArrowParams::
reinit(bool doOverride){

	DataStatus* ds = DataStatus::getInstance();
	
	int totNumVariables = ds->getNumSessionVariables()+ ds->getNumSessionVariables2D();
	if (totNumVariables <= 0) return false;
	bool is3D = VariablesAre3D();
	int numVariables;
	if (is3D) numVariables = ds->getNumSessionVariables();
	else numVariables = ds->getNumSessionVariables2D();
	
	//Set up the numRefinements. 
	int maxNumRefinements = ds->getNumTransforms();
	int numrefs = GetRefinementLevel();
	if (doOverride) { 
		numrefs = 0;
		SetFidelityLevel(0.5f);
		SetIgnoreFidelity(false);
	} else {  //Try to use existing values
		const std::string& sessionVersion = ds->getSessionVersion();
		int gt = Version::Compare(sessionVersion, "2.2.4");
		if(gt <= 0) {
			SetFidelityLevel(0.5f);
			SetIgnoreFidelity(true);
		}
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
	}
	SetRefinementLevel(numrefs);
	//Set up the compression level.  Whether or not override is true, make sure
	//That the compression level is valid.  If override is true set it to 0;
	if (doOverride) SetCompressionLevel(0);
	else {
		int numCompressions = 0;
	
		if (ds->getDataMgr()) {
			numCompressions = ds->getDataMgr()->GetCRatios().size();
		}
		
		if (GetCompressionLevel() >= numCompressions){
			SetCompressionLevel(numCompressions-1);
		}
	}
	//Set up the variables. If doOverride is true, just make the first 3 variables the first 3 variables in the VDC.
	//Otherwise try to use the current variables 
	//In either case, if they don't exist replace them with 0.
	
	if (doOverride){
		for (int i = 0; i<3; i++){
			string varname;
			if (i>=numVariables) {
				varname = "0";
			}
			else {
				if (is3D) {
					varname = ds->getVariableName3D(i);
				}
				else {
					varname = ds->getVariableName2D(i);
				}
			}
			SetFieldVariableName(i,varname);
		}
	} else {
		for (int i = 0; i<3; i++){
			string varname = GetFieldVariableName(i);
			int indx;
			if (is3D){
				indx = ds->getActiveVarNum3D(varname);
			}
			else {
				indx = ds->getActiveVarNum2D(varname);
			}
			if (indx < 0) {
				SetFieldVariableName(i,"0");
			}
		}
	}
	//Use HGT as the height variable name, if it's there. If not just use the first 2d variable.
	if (doOverride){
		string varname = "HGT";
		int indx = ds->getActiveVarNum2D(varname);
		if (indx < 0 && ds->getNumActiveVariables2D()>0) {
			varname = ds->getVariableName2D(0);
		}
		SetHeightVariableName(varname);
	} else {
		string varname = GetHeightVariableName();
		int indx = ds->getActiveVarNum2D(varname);
		if (indx < 0 && ds->getNumActiveVariables2D()>0) {
			varname = ds->getVariableName2D(0);
			SetHeightVariableName(varname);
		}
	}
	if (ds->getNumActiveVariables2D() <= 0) SetTerrainMapped(false);
	//Set the vector length so that the max arrow is 10% of the larger of the x or y size of scene
	//Check the rake extents.  If doOverride is true, set the extents to the bottom of the data domain. If not, 
	//shrink the extents to fit inside the domain.
	const float* extents = ds->getLocalExtents();
	vector<double>newExtents(3,0.);
	if (doOverride) {
		for (int i = 0; i<3; i++){
			newExtents.push_back((double)( extents[i+3]-extents[i]));
		}
		
	} else {
		double newExts[6];
		GetRakeLocalExtents(newExts);
		if (DataStatus::pre22Session()){
			//In old session files, rake extents were not 0-based
			float * offset = DataStatus::getPre22Offset();
			for (int i = 0; i<3; i++){
				newExts[i] -= offset[i];
				newExts[i+3] -= offset[i];
			}
		}
		for (int i = 0; i<3; i++){
			if (i != 2){
				newExts[i] = Max(newExts[i], 0.);
				newExts[i+3] = Min(newExts[i+3], (double)(extents[i+3]-extents[i]));
			}
			if (newExts[i] > newExts[i+3]) newExts[i+3] = newExts[i];
		}
		newExtents.clear();
		for (int i = 0; i<6; i++) newExtents.push_back(newExts[i]);
	}
	SetRakeLocalExtents(newExtents);

	//If grid is mapped to data, 
	//Make the grid size default to 10x10x1.  If doOverride is false, make sure the grid dims
	//are at least 1, and no bigger than 10**5.
	int gridSize[3] = {10,10,1};
	vector<long> strides;
	strides.push_back(10);strides.push_back(10);
	if (doOverride) {
		SetRakeGrid(gridSize);
		AlignGridToData(false);
		SetGridAlignStrides(strides);
	} else {
		//Make sure grid is valid...
		const vector<long>& rakeGrid = GetRakeGrid();
		for (int i = 0; i<3; i++){
			gridSize[i] = rakeGrid[i];
			if (gridSize[i] < 1) gridSize[i] = 1;
			if (gridSize[i] > 100000) gridSize[i] = 100000;
		}
		SetRakeGrid(gridSize);
		if (!IsAlignedToData()){
			//set to default
			SetGridAlignStrides(strides);
		}

	}
	
	if (doOverride) {
		const float col[3] = {1.f, 0.f, 0.f};
		SetConstantColor(col);
		SetTerrainMapped(false);
		SetVectorScale(calcDefaultScale());
	}
	initializeBypassFlags();
	return true;
}
//Set everything to default values
void ArrowParams::restart() {
	
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	SetFidelityLevel(0.5f);
	SetIgnoreFidelity(false);
	SetVisualizerNum(vizNum);
	SetFieldVariableName(0, "xvar");
	SetFieldVariableName(1, "yvar");
	SetFieldVariableName(2, "0");
	SetVariables3D(true);
	SetHeightVariableName("HGT");
	
	setEnabled(false);
	const float default_color[3] = {1.0, 0.0, 0.0};
	SetConstantColor(default_color);
	SetVectorScale(1.0);
	SetTerrainMapped(false);
	SetLineThickness(1.0);
	
	int gridsize[3];
	vector<double> exts;
	exts.push_back(-1.);
	exts.push_back(-1.);
	exts.push_back(-1.);
	
	for (int i = 0; i<3; i++){
		exts.push_back(1.);
		gridsize[i] = 10;
	}
	if (!GetRootNode()->HasChild(Box::_boxTag)){
		Box* myBox = new Box();
		ParamNode* boxNode = myBox->GetRootNode();
		GetRootNode()->AddRegisteredNode(Box::_boxTag,boxNode,myBox);
	}
	
	//Don't set the Box values until after it has been registered:
	SetRakeLocalExtents(exts);
	gridsize[2] = 1;
	SetRakeGrid(gridsize);
	vector<long> alignStrides;
	alignStrides.push_back(0);
	alignStrides.push_back(0);
	SetGridAlignStrides(alignStrides);
	AlignGridToData(false);
	
}
//calculate rake extents when aligned to data.
//Also determine the dimensions (size) of the grid-aligned rake
void ArrowParams::calcDataAlignment(double rakeExts[6], int rakeGrid[3],size_t timestep){
	//Find the first data point that fits in the rake extents:
	//Take the rake corner, convert it to voxels
	size_t corner[3],farCorner[3];
	double rakeExtents[6];
	double tempExtents[3];
	DataMgr* dataMgr = DataStatus::getInstance()->getDataMgr();
	if(!dataMgr) {
		return;
	}
	GetRakeLocalExtents(rakeExtents);
	const vector<double>& usrExts = dataMgr->GetExtents(timestep);
	const vector<long> rGrid = GetRakeGrid();
	for (int i = 0; i<3; i++){
		rakeExts[i]=rakeExtents[i]+usrExts[i];
		rakeExts[i+3]=rakeExtents[i+3]+usrExts[i];
		rakeGrid[i] = rGrid[i];
	}
	size_t voxExts[6];
	mapBoxToVox(dataMgr, -1,GetCompressionLevel(),timestep, voxExts);
	for (int i = 0; i<3; i++) {
		corner[i] = voxExts[i];
		farCorner[i] = voxExts[i+3];
	}
	
	//Is the corner actually inside the rake?
	dataMgr->MapVoxToUser(timestep,corner, tempExtents, -1, GetCompressionLevel());
	if (tempExtents[0]<rakeExts[0]) corner[0]++;
	if (tempExtents[1]<rakeExts[1]) corner[1]++;
	//Now get the local (0-based) coords of the corner
	dataMgr->MapVoxToUser(timestep, corner, tempExtents,-1,GetCompressionLevel());
	rakeExts[0] = tempExtents[0];
	rakeExts[1] = tempExtents[1];
	
	//Now find how many voxels are in rake:
	
	vector<long>strides = GetGridAlignStrides();
	if (strides[0] <= 0) rakeGrid[0] = 1; 
	else rakeGrid[0] = 1+(farCorner[0]-corner[0])/strides[0];
	if (strides[1] <= 0) rakeGrid[1] = 1; 
	else rakeGrid[1] = 1+(farCorner[1]-corner[1])/strides[1];

	//Now adjust the rake extents to match the actual far corners:
	if(strides[0]>0)farCorner[0] = corner[0]+(rakeGrid[0]-1)*strides[0];
	else farCorner[0] = corner[0];
	if(strides[1]>0)farCorner[1] = corner[1]+(rakeGrid[1]-1)*strides[1];
	else farCorner[1] = corner[1];

	dataMgr->MapVoxToUser(timestep,farCorner, tempExtents, -1,GetCompressionLevel());
	rakeExts[3]=tempExtents[0];
	rakeExts[4]=tempExtents[1];
}



float ArrowParams::getCameraDistance(ViewpointParams* vpp, RegionParams* , int ){
	//Determine the box that contains the arrows:
	
	double dbexts[6];
	GetRakeLocalExtents(dbexts);
	return RenderParams::getCameraDistance(vpp,dbexts);
}


void ArrowParams::SetConstantColor(const float rgb[3]) {
	vector <double> valvec(3,0);
	for (int i=0; i<3; i++) {
		valvec[i] = rgb[i];
	}
	GetRootNode()->SetElementDouble(_constantColorTag, valvec);
}

const float *ArrowParams::GetConstantColor() {
	const vector<double> white (3,1.);
	vector <double> valvec = GetRootNode()->GetElementDouble(_constantColorTag, white);
	for (int i=0; i<3; i++) _constcolorbuf[i] = valvec[i];
	return(_constcolorbuf);
}

int ArrowParams::GetCompressionLevel(){
	const vector<long> defaultLevel(1,2);
	vector<long> valvec = GetRootNode()->GetElementLong(_CompressionLevelTag,defaultLevel);
	return (int)valvec[0];
 }
void ArrowParams::SetCompressionLevel(int level){
	 vector<long> valvec(1,(long)level);
	 GetRootNode()->SetElementLong(_CompressionLevelTag,valvec);
	 setAllBypass(false);
 }
float ArrowParams::GetFidelityLevel(){
	vector<double> valvec = GetRootNode()->GetElementDouble(_FidelityLevelTag);
	return (float)valvec[0];
 }
void ArrowParams::SetFidelityLevel(float level){
	 vector<double> valvec(1,(double)level);
	 GetRootNode()->SetElementDouble(_FidelityLevelTag,valvec);
 }
bool ArrowParams::GetIgnoreFidelity(){
	vector<long> valvec = GetRootNode()->GetElementLong(_IgnoreFidelityTag);
	return (bool)valvec[0];
 }
void ArrowParams::SetIgnoreFidelity(bool val){
	 vector<long> valvec(1,(long)val);
	 GetRootNode()->SetElementLong(_IgnoreFidelityTag,valvec);
 }
 void ArrowParams::SetVisualizerNum(int viznum){
	vector<long> valvec(1,(long)viznum);
	GetRootNode()->SetElementLong(_VisualizerNumTag,valvec);
	vizNum = viznum;
 }
 int ArrowParams::GetVisualizerNum(){
	const vector<long> visnum(1,0);
	vector<long> valvec = GetRootNode()->GetElementLong(_VisualizerNumTag,visnum);
	return (int)valvec[0];
 }
void ArrowParams::SetFieldVariableName(int i, const string& varName){
	vector <string> svec;
	vector <string> defaultName(1,"0");
	GetRootNode()->GetElementStringVec(_VariableNamesTag, svec,defaultName);
	if(svec.size() <= i) 
		for (int j = svec.size(); j<=i; j++) svec.push_back("0");
	svec[i] = varName;
	GetRootNode()->SetElementStringVec(_VariableNamesTag, svec);
	setAllBypass(false);
}
void ArrowParams::SetHeightVariableName(const string& varName){
	GetRootNode()->SetElementString(_heightVariableNameTag, varName);
	setAllBypass(false);
}
const string& ArrowParams::GetHeightVariableName(){
	return GetRootNode()->GetElementString(_heightVariableNameTag, "HGT");
}
const string& ArrowParams::GetFieldVariableName(int i){
	vector <string> defaultName(3,"0");
	static string retval;
	vector <string> svec;
	GetRootNode()->GetElementStringVec(_VariableNamesTag, svec, defaultName);
	retval=svec[i];
	return retval;
}
double ArrowParams::calcDefaultScale(){
	string varname;
	int sesvarnum;
	double maxvarvals[3];
	bool is3D = VariablesAre3D();
	DataStatus* ds = DataStatus::getInstance();
	const float* stretch = ds->getStretchFactors();
	for (int i = 0; i<3; i++){
		varname = GetFieldVariableName(i);
		if (varname == "0") maxvarvals[i] = 0.;
		else {
			if (is3D){
				sesvarnum = ds->getSessionVariableNum3D(varname);
				maxvarvals[i] = Max(abs(ds->getDefaultDataMax3D(sesvarnum)),abs(ds->getDefaultDataMin3D(sesvarnum)));
			} else {
				sesvarnum = ds->getSessionVariableNum2D(varname);
				maxvarvals[i] = Max(abs(ds->getDefaultDataMax2D(sesvarnum)),abs(ds->getDefaultDataMin2D(sesvarnum)));
			}
		}
	}
	for (int i = 0; i<3; i++) maxvarvals[i] *= stretch[i];
	const float* extents = DataStatus::getInstance()->getLocalExtents();
	double maxVecLength = (double)Max(extents[3]-extents[0],extents[4]-extents[1])*0.1;
	double maxVecVal = Max(maxvarvals[0],Max(maxvarvals[1],maxvarvals[2]));
	if (maxVecVal == 0.) return(maxVecLength);
	else return(maxVecLength/maxVecVal);
}
