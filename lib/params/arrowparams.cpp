
#include "arrowparams.h"
#include "datastatus.h"
#include "vapor/ParamsBase.h"
#include "params.h"
#include "renderparams.h"

#include <string>

using namespace VetsUtil;
using namespace VAPoR;
const string ArrowParams::_shortName = "Barbs";
const string ArrowParams::_arrowParamsTag = "ArrowParams";
const string ArrowParams::_constantColorTag = "ConstantColor";
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
	Command::blockCapture();
	restart();
	Command::unblockCapture();
}


ArrowParams::~ArrowParams() {

}

//Initialize for new metadata.  Keep old transfer functions
//
// For each new variable in the metadata create a variable child node, and build the
// associated isoControl and transfer function nodes.
void ArrowParams::
Validate(int type){
	//Command capturing should be disabled
	assert(!Command::isRecording());
	bool doOverride = (type == 0);
	DataStatus* ds = DataStatus::getInstance();
	DataMgr* dataMgr = ds->getDataMgr();
	if (!dataMgr) return;
	int totNumVariables = dataMgr->GetVariables2DXY().size()+dataMgr->GetVariables3D().size();
	if (totNumVariables <= 0) return;
	bool is3D = VariablesAre3D();
	int numVariables;
	if (is3D) numVariables = dataMgr->GetVariables3D().size();
	else numVariables = dataMgr->GetVariables2DXY().size();
	
	//Set up the numRefinements. 
	int maxNumRefinements = ds->getNumTransforms();
	int numrefs = GetRefinementLevel();
	if (doOverride) { 
		numrefs = 0;
	} else {  //Try to use existing values
		if (numrefs > maxNumRefinements) numrefs = maxNumRefinements;
		if (numrefs < 0) numrefs = 0;
	}
	SetRefinementLevel(numrefs);
	//Set up the compression level.  Whether or not override is true, make sure
	//That the compression level is valid.  If override is true set it to 0;
	if (doOverride) SetCompressionLevel(0);
	else {
		int numCompressions = 0;
	
		if (dataMgr) {
			numCompressions = dataMgr->GetCRatios().size();
		}
		
		if (GetCompressionLevel() >= numCompressions){
			SetCompressionLevel(numCompressions-1);
		}
	}
	//Set up the variables. If doOverride is true, just make the first 3 variables the first 3 variables in the VDC.
	//Otherwise try to use the current variables 
	//In either case, if they don't exist replace them with 0.
	const vector<string>& vars = (is3D ? dataMgr->GetVariables3D() : dataMgr->GetVariables2DXY());
	if (doOverride){

		for (int i = 0; i<3; i++){
			string varname;
			if (i>=numVariables) {
				varname = "0";
			}
			else {
				varname = vars[i];
			}
			
			SetFieldVariableName(i,varname);
		}
	} 
	//Use HGT as the height variable name, if it's there. If not just use the first 2d variable.
	if (doOverride){
		string varname = "HGT";
		int indx = ds->getActiveVarNum2D(varname);
		if (indx < 0 && ds->getNumActiveVariables2D()>0) {
			varname = dataMgr->GetVariables2DXY()[0];
		}
		SetHeightVariableName(varname);
	} else {
		string varname = GetHeightVariableName();
		int indx = ds->getActiveVarNum2D(varname);
		if (indx < 0 && ds->getNumActiveVariables2D()>0) {
			varname = dataMgr->GetVariables2DXY()[0];
			SetHeightVariableName(varname);
		}
	}
	if (ds->getNumActiveVariables2D() <= 0) SetTerrainMapped(false);
	//Set the vector length so that the max arrow is 10% of the larger of the x or y size of scene
	//Check the rake extents.  If doOverride is true, set the extents to the bottom of the data domain. If not, 
	//shrink the extents to fit inside the domain.
	const double* extents = ds->getLocalExtents();
	vector<double>newExtents(3,0.);
	if (doOverride) {
		for (int i = 0; i<3; i++){
			newExtents.push_back((double)( extents[i+3]-extents[i]));
		}
		
	} else {
		double newExts[6];
		GetRakeLocalExtents(newExts);
		
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
		const double col[3] = {1.f, 0.f, 0.f};
		SetConstantColor(col);
		SetTerrainMapped(false);
		SetVectorScale(calcDefaultScale());
	}
	initializeBypassFlags();
	return;
}
//Set everything to default values
void ArrowParams::restart() {
	Command::blockCapture();
	SetVizNum(0);
	SetRefinementLevel(0);
	SetCompressionLevel(0);
	
	SetFieldVariableName(0, "xvar");
	SetFieldVariableName(1, "yvar");
	SetFieldVariableName(2, "0");
	SetVariables3D(true);
	SetHeightVariableName("HGT");
	
	SetEnabled(false);
	
	const double default_color[3] = {1.0, 0.0, 0.0};
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
	Box* myBox = new Box();
	vector<string>path;
	path.push_back(Box::_boxTag);
	SetParamsBase(path,myBox);
	
	
	//Don't set the Box values until after it has been registered:
	SetRakeLocalExtents(exts);
	gridsize[2] = 1;
	SetRakeGrid(gridsize);
	vector<long> alignStrides;
	alignStrides.push_back(0);
	alignStrides.push_back(0);
	SetGridAlignStrides(alignStrides);
	AlignGridToData(false);
	Command::unblockCapture();
}
//calculate rake extents when aligned to data.
//Also determine the dimensions (size) of the grid-aligned rake
void ArrowParams::getDataAlignment(double rakeExts[6], int rakeGrid[3],size_t timestep){
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
	DataStatus::getInstance()->mapBoxToVox(GetBox(), -1,GetCompressionLevel(),timestep, voxExts);
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




int ArrowParams::SetConstantColor(const double rgb[3]) {
	vector <double> valvec;
	for (int i=0; i<3; i++) {
		valvec.push_back(rgb[i]);
	}
	return SetValueDouble(_constantColorTag,"Change barb color",valvec);
}

const vector<double> ArrowParams::GetConstantColor() {
	const vector<double> white (3,1.);
	const vector <double> valvec = GetValueDoubleVec(_constantColorTag, white);
	
	return(valvec);
}

int ArrowParams::SetFieldVariableName(int i, const string& varName){
	vector <string> svec;
	vector <string> defaultName(1,"0");
	GetValueStringVec(_VariableNamesTag, svec,defaultName);
	if(svec.size() < 3) 
		for (int j = svec.size(); j<3; j++) svec.push_back("0");  //fill extra spaces with "0"
	svec[i] = varName;
	//Capture the change to variable name and to scale
	Command* cmd = Command::CaptureStart(this,"set barb field name");
	int rc = SetValueStringVec(_VariableNamesTag,"",svec);
	if (!rc) rc = SetValueDouble(_vectorScaleTag,"",calcDefaultScale());
	Command::CaptureEnd(cmd,this);
	if (rc) return rc;
	setAllBypass(false);
	return 0;
}
int ArrowParams::SetHeightVariableName(const string& varName){
	return SetValueString(_heightVariableNameTag,"Set barb rake extents",varName);
}
const string ArrowParams::GetHeightVariableName(){
	return GetValueString(_heightVariableNameTag, "HGT");
}
const string ArrowParams::GetFieldVariableName(int i){
	vector <string> defaultName(3,"0");
	static string retval;
	vector <string> svec;
	GetValueStringVec(_VariableNamesTag, svec, defaultName);
	retval=svec[i];
	return retval;
}
double ArrowParams::calcDefaultScale(){
	string varname;
	double maxvarvals[3];
	DataStatus* ds = DataStatus::getInstance();
	const double* stretch = ds->getStretchFactors();
	for (int i = 0; i<3; i++){
		varname = GetFieldVariableName(i);
		if (varname == "0" || !ds->getDataMgr()) maxvarvals[i] = 0.;
		else {
			maxvarvals[i] = Max(abs(ds->getDefaultDataMax(varname)),abs(ds->getDefaultDataMin(varname)));
		}
	}
	for (int i = 0; i<3; i++) maxvarvals[i] *= stretch[i];
	const double* extents = DataStatus::getInstance()->getLocalExtents();
	double maxVecLength = (double)Max(extents[3]-extents[0],extents[4]-extents[1])*0.1;
	double maxVecVal = Max(maxvarvals[0],Max(maxvarvals[1],maxvarvals[2]));
	if (maxVecVal == 0.) return(maxVecLength);
	else return(maxVecLength/maxVecVal);
}
int ArrowParams::SetRakeLocalExtents(const vector<double>&exts){
	return GetBox()->SetValueDouble(Box::_extentsTag,"Set Barb Rake extents", exts, this);
	
}

int ArrowParams::SetRakeGrid(const int grid[3]){
	vector<long> griddims;
	for (int i = 0; i<3; i++){
		griddims.push_back((long)grid[i]);
	}
	return SetValueLong(_rakeGridTag,"Set barb grid", griddims);
}
int ArrowParams::SetVectorScale(double val){
	int rc = 0;
	return SetValueDouble(_vectorScaleTag, "set barb scale", val);
}
int ArrowParams::SetGridAlignStrides(const vector<long>& strides){
	return SetValueLong(_alignGridStridesTag, "Set barb grid strides", strides);
}
