//
//      $Id$
//
//************************************************************************
//																		*
//		     Copyright (C)  2004										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		Metadata.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Sep 30 12:13:03 MDT 2004
//
//	Description:	Implements the Metadata class
//


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include <vapor/Metadata.h>
#include <vapor/XmlNode.h>
#include <vapor/CFuncs.h>

using namespace VAPoR;
using namespace VetsUtil;

// Static member initialization
//
const string Metadata::_childrenTag = "Children";
const string Metadata::_commentTag = "Comment";
const string Metadata::_coordSystemTypeTag = "CoordinateSystem";
const string Metadata::_dataRangeTag = "DataRange";
const string Metadata::_extentsTag = "Extents";
const string Metadata::_gridTypeTag = "GridType";
const string Metadata::_numTimeStepsTag = "NumTimeSteps";
const string Metadata::_basePathTag = "BasePath";
const string Metadata::_auxBasePathTag = "AuxBasePath";
const string Metadata::_rootTag = "Metadata";
const string Metadata::_userTimeTag = "UserTime";
const string Metadata::_timeStepTag = "TimeStep";
const string Metadata::_varNamesTag = "VariableNames";
const string Metadata::_vars3DTag = "Variables3D";
const string Metadata::_vars2DXYTag = "Variables2DXY";
const string Metadata::_vars2DXZTag = "Variables2DXZ";
const string Metadata::_vars2DYZTag = "Variables2DYZ";
const string Metadata::_xCoordsTag = "XCoords";
const string Metadata::_yCoordsTag = "YCoords";
const string Metadata::_zCoordsTag = "ZCoords";
const string Metadata::_periodicBoundaryTag = "PeriodicBoundary";
const string Metadata::_gridPermutationTag = "GridPermutation";


const string Metadata::_blockSizeAttr = "BlockSize";
const string Metadata::_dimensionLengthAttr = "DimensionLength";
const string Metadata::_numTransformsAttr = "NumTransforms";
const string Metadata::_filterCoefficientsAttr = "FilterCoefficients";
const string Metadata::_liftingCoefficientsAttr = "LiftingCoefficients";
const string Metadata::_msbFirstAttr = "MSBFirst";
const string Metadata::_vdfVersionAttr = "VDFVersion";
const string Metadata::_numChildrenAttr = "NumChildren";


int Metadata::SetDefaults() {

	vector <long> periodic_boundary(3,0);
	SetPeriodicBoundary(periodic_boundary);

	// Set some default metadata attributes. 
	//
	string gridTypeStr = "regular";
	SetGridType(gridTypeStr);

	string coordSystemType = "cartesian";
	SetCoordSystemType(coordSystemType);

	double maxdim = max(_dim[0], max(_dim[1],_dim[2]));
	double extents[] = {
		0.0, 0.0, 0.0, 
		(double) _dim[0]/maxdim, 
		(double) _dim[1]/maxdim, 
		(double) _dim[2]/maxdim
	};

	vector<double> extentsVec(extents, &extents[sizeof(extents)/sizeof(extents[0])]);
	SetExtents(extentsVec);

	return(0);
}

// Initialize the class object
//
int Metadata::_init(
		const size_t dim[3], size_t numTransforms, size_t bs[3],
		int nFilterCoef, int nLiftingCoef, int msbFirst, int vdfVersion
) {
	map <string, string> attrs;
	ostringstream oss;
	string empty;

	SetClassName("Metadata");

	if (! (bs[0] == bs[1] && bs[1] == bs[2])) {
		SetErrMsg("Block dimensions not symmetric");
		return(-1);
	}

	for(int i=0; i<3; i++) {
		if (! IsPowerOfTwo((int)bs[i])) {
			SetErrMsg("Block dimension is not a power of two: bs=%d", bs[i]);
			return(-1);
		}
	}

	_emptyDoubleVec.clear();
	_emptyLongVec.clear();
	_emptyString.clear();

	for(int i=0; i<3; i++) {
		_bs[i] = bs[i];
		_dim[i] = dim[i];
	}
	_numTransforms = (int)numTransforms;
	_nFilterCoef = nFilterCoef;
	_nLiftingCoef = nLiftingCoef;
	_msbFirst = msbFirst;
	_vdfVersion = vdfVersion;

	_varNames.clear();
	_varNames3D.clear();
	_varNames2DXY.clear();
	_varNames2DXZ.clear();
	_varNames2DYZ.clear();

	oss.str(empty);
	oss << (unsigned int)_bs[0] << " " << (unsigned int)_bs[1] << " " << (unsigned int)_bs[2];
	attrs[_blockSizeAttr] = oss.str();

	oss.str(empty);
	oss << (unsigned int)_dim[0] << " " << (unsigned int)_dim[1] << " " << (unsigned int)_dim[2];
	attrs[_dimensionLengthAttr] = oss.str();

	oss.str(empty);
	oss << _numTransforms;
	attrs[_numTransformsAttr] = oss.str();

	oss.str(empty);
	oss << _nFilterCoef;
	attrs[_filterCoefficientsAttr] = oss.str();

	oss.str(empty);
	oss << _nLiftingCoef;
	attrs[_liftingCoefficientsAttr] = oss.str();

	oss.str(empty);
	oss << _msbFirst;
	attrs[_msbFirstAttr] = oss.str();

	oss.str(empty);
	oss << _vdfVersion;
	attrs[_vdfVersionAttr] = oss.str();


	// Create the root node of an xml tree. The node is named (tagged)
	// by 'roottag', and will have XML attributes as specified by 'attrs'
	//
	_rootnode = new XmlNode(_rootTag, attrs);
	if (XmlNode::GetErrCode() != 0) return(-1);
	
	_periodicBoundaryDefault.assign(3,0);

	_gridPermutationDefault.assign(3,0);	// allocate space
	_gridPermutationDefault[0] = 0;
	_gridPermutationDefault[1] = 1;
	_gridPermutationDefault[2] = 2;

	if (SetNumTimeSteps(1) < 0) return(-1);

	vector<string> varNamesVec(1,"var1");
	if (SetVariableNames(varNamesVec) < 0) return(-1);

	string comment = "";
	_rootnode->SetElementString(_commentTag, comment);

	

	return(SetDefaults());
}

Metadata::Metadata(
		const size_t dim[3], size_t numTransforms, size_t bs[3], 
		int nFilterCoef, int nLiftingCoef, int msbFirst, int vdfVersion
) {
	_objInitialized = 0;
	SetDiagMsg(
		"Metadata::Metadata([%d,%d,%d], %d, [%d,%d%d] %d, %d, %d, %d)", 
		dim[0], dim[1], dim[2], numTransforms, bs[0],bs[1],bs[2], 
		nFilterCoef, nLiftingCoef, msbFirst, vdfVersion
	);

	_rootnode = NULL;
	_metafileDirName.clear();
	_metafileName.clear();
	_dataDirName.clear();

	if (_init(dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst, vdfVersion) < 0){
		return;
	}
	_objInitialized = 1;
}




Metadata::Metadata(const string &path) {
	_objInitialized = 0;

	SetDiagMsg("Metadata::Metadata(%s)", path.c_str());

	ifstream is;
	

	_rootnode = NULL;

	is.open(path.c_str());
	if (! is) {
		SetErrMsg("Can't open file \"%s\" for reading", path.c_str());
		return;
	}

	// Get directory path of metafile
	char *s = new char[path.length() +1];
	(void) Dirname(path.c_str(), s);
	_metafileDirName.assign(s);
	delete [] s;
	_metafileName.assign(Basename(path.c_str()));

	// Create data directory base name
	//
	size_t p = _metafileName.find_first_of(".");
	if (p != string::npos) {
		_dataDirName = _metafileName.substr(0, p);
	}
	else {
		_dataDirName = _metafileName;
	}
	_dataDirName.append("_data");

	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	parseMgr->parse(is);
	delete parseMgr;
	
	_objInitialized = 1;
}

Metadata::~Metadata() {
	SetDiagMsg("Metadata::~Metadata()");
	if (! _objInitialized) return;


	if (_rootnode) delete _rootnode;

	_objInitialized = 0;
}


int Metadata::Merge(Metadata *metadata, size_t ts_start) {

	const size_t *bs = this->GetBlockSize();
	const size_t *mbs = metadata->GetBlockSize();

	for(int i=0; i<3; i++) {
		if (bs[i] != mbs[i]) {
			SetErrMsg("Merge failed: block size mismatch");
			return(-1);
		}
	}

	const size_t *dim = this->GetDimension();
	const size_t *mdim = metadata->GetDimension();

	for(int i=0; i<3; i++) {
		if (dim[i] != mdim[i]) {
			SetErrMsg("Merge failed: dimension mismatch");
			return(-1);
		}
	}

	if (this->GetVDFVersion() < 2 || metadata->GetVDFVersion() < 2) {
		SetErrMsg("Pre version 2 vdf files not supported");
		return(-1);
	}

	//
	// Ensure that both metadata objects have current version numbers
	//
	if (this->MakeCurrent() < 0) {
		SetErrMsg("Could not make Metadata object current");
		return(-1);
	}
	if (metadata->MakeCurrent() < 0) {
		SetErrMsg("Could not make Metadata object current");
		return(-1);
	}
		

	if (this->GetFilterCoef() != metadata->GetFilterCoef()) {
		SetErrMsg("Merge failed: filter coefficient mismatch");
		return(-1);
	}

	if (this->GetLiftingCoef() != metadata->GetLiftingCoef()) {
		SetErrMsg("Merge failed: lifting coefficient mismatch");
		return(-1);
	}

	if (this->GetNumTransforms() != metadata->GetNumTransforms()) {
		SetErrMsg("Merge failed: num transforms mismatch");
		return(-1);
	}

	if (this->GetVDFVersion() != metadata->GetVDFVersion()) {
		SetErrMsg("Merge failed: VDF file version mismatch");
		return(-1);
	}

	//
	// Now merge the variable names
	//
	const vector <string> varnames = this->GetVariableNames();
	const vector <string> mvarnames = metadata->GetVariableNames();

	const vector <string> vars3d0 = this->GetVariables3D();
	const vector <string> vars3d1 = metadata->GetVariables3D();
	vector <string> vars3d;
	for (int i=0; i<vars3d0.size(); i++) vars3d.push_back(vars3d0[i]);
	for (int i=0; i<vars3d1.size(); i++) vars3d.push_back(vars3d1[i]);

	const vector <string> vars2dxy0 = this->GetVariables2DXY();
	const vector <string> vars2dxy1 = metadata->GetVariables2DXY();
	vector <string> vars2dxy;
	for (int i=0; i<vars2dxy0.size(); i++) vars2dxy.push_back(vars2dxy0[i]);
	for (int i=0; i<vars2dxy1.size(); i++) vars2dxy.push_back(vars2dxy1[i]);

	const vector <string> vars2dxz0 = this->GetVariables2DXZ();
	const vector <string> vars2dxz1 = metadata->GetVariables2DXZ();
	vector <string> vars2dxz;
	for (int i=0; i<vars2dxz0.size(); i++) vars2dxz.push_back(vars2dxz0[i]);
	for (int i=0; i<vars2dxz1.size(); i++) vars2dxz.push_back(vars2dxz1[i]);

	const vector <string> vars2dyz0 = this->GetVariables2DYZ();
	const vector <string> vars2dyz1 = metadata->GetVariables2DYZ();
	vector <string> vars2dyz;
	for (int i=0; i<vars2dyz0.size(); i++) vars2dyz.push_back(vars2dyz0[i]);
	for (int i=0; i<vars2dyz1.size(); i++) vars2dyz.push_back(vars2dyz1[i]);

	long ts = this->GetNumTimeSteps();

	// Need to make a copy of the base names. Later, when we call
	// SetVariableNames() all of the base names will be regenerated.
	// This is fine if the base names are all default values, but if they're
	// not - if the Metadata object resulted from a previous merge, for
	// example - they will get clobbered by SetVariableNames. 
	//
	map <string, vector<string> > basecopy;
	for (int i = 0; i<varnames.size(); i++) {
	for (long t = 0; t<ts; t++) {

		vector <string> &strvec = basecopy[varnames[i]];
		strvec.push_back(GetVBasePath(t, varnames[i]));
	}
	}

	vector <string> newnames = varnames;
	for (int i=0; i<mvarnames.size(); i++) {
		int match = 0;
		for (int j=0; j<varnames.size() && ! match; j++) {
			if (mvarnames[i].compare(varnames[j]) == 0) match = 1;
		}
		if (! match) {	// name not found => add it
			newnames.push_back(mvarnames[i]);
		}
	}
	if (newnames.size() > varnames.size()) { 
		int rc = SetVariableNames(newnames);
		if (rc < 0) return(-1);
	}

	// Ugh. SetVariableNames destroys the variable type
	//
	SetVariables3D(vars3d);
	SetVariables2DXY(vars2dxy);
	SetVariables2DXZ(vars2dxz);
	SetVariables2DYZ(vars2dyz);

	// Restore basenames of the old variables to whatever they were before
	// SetVariableNames clobbered them.
	//
	for (int i = 0; i<varnames.size(); i++) {
	for (long t = 0; t<ts; t++) {
		vector <string> &strvec = basecopy[varnames[i]];

		int rc = SetVBasePath(t, varnames[i], strvec[t]);
		if (rc < 0) return(-1);
	}
	}


	//
	// Bump up the number of time steps if needed
	//
	long mts = metadata->GetNumTimeSteps();

	if (mts + ts_start > ts) {
		int rc = this->SetNumTimeSteps(mts+ts_start);
		if (rc < 0) return(-1);
	}

	// Push the variables next, replacing the current path names with 
	// **absolute** paths from the new Metadata object.
	//
	for (size_t ts = 0; ts < mts; ts++) {

		for (int i=0; i<mvarnames.size(); i++) {

			string base;

			if (metadata->ConstructFullVBase(ts, mvarnames[i], &base) < 0) {
				return (-1);
			}

			int rc = this->SetVBasePath(ts+ts_start, mvarnames[i], base);
			if (rc < 0) return(-1);
		}
	}

	return(0);
}

int Metadata::MakeCurrent() {

	if (GetVDFVersion() < 1) {
		SetErrMsg("Can't make a pre-version 1 VDF file current");
		return(-1);
	}
	if (GetVDFVersion() == VDF_VERSION) return(0);

	_vdfVersion = VDF_VERSION;

	map <string, string> &attrs = _rootnode->Attrs();
	ostringstream oss;
    oss << _vdfVersion;
    attrs[_vdfVersionAttr] = oss.str();

	const vector <string> &vnames = GetVariableNames();
	vector <string> myvnames = vnames;	// get a local copy

	return(SetVariableNames(vnames));
}
	

int Metadata::Merge(const string &path, size_t ts_start) {
	Metadata *metadata;

	metadata = new Metadata(path);

	if (Metadata::GetErrCode()) return(-1);

	return(Merge(metadata, ts_start));
} 

int Metadata::ConstructFullVBase(
	size_t ts, const string &var, string *path
) const {

	path->clear();

    const string &bp = GetVBasePath(ts, var);
    if (GetErrCode() != 0 || bp.length() == 0) {
		fprintf(stderr, "GetVBasepath error, ts = %d var = %s basepath = %s\n",
			ts, var.c_str(), (*path).c_str());
        return (-1);
    }
#ifdef WIN32
	if (bp[1] == ':' && bp[2] == '/') {
#else
	if (bp[0] == '/') {
#endif
		path->assign(bp);
		return(0);
	}

	path->assign(GetParentDir());
	path->append("/");

	path->append(GetDataDirName());
	path->append("/");

	path->append(bp);

	return(0);
}

int Metadata::ConstructFullAuxBase(
	size_t ts, string *path
) const {

	path->clear();

    const string &bp = GetTSAuxBasePath(ts);
    if (GetErrCode() != 0 || bp.length() == 0) {
        return (-1);
    }

	if (bp[0] == '/') {
		path->assign(bp);
		return(0);
	}

	path->assign(GetParentDir());
	path->append("/");

	path->append(GetDataDirName());
	path->append("/");

	path->append(bp);

	return(0);
}

int Metadata::Write(const string &path, int relative_path) {

	SetDiagMsg("Metadata::Write(%s)", path.c_str());


	// 
	// This is an ugly hack to handle absolute path names
	//
	if (! relative_path) {
		size_t numTS = _rootnode->GetNumChildren();

		for(size_t ts = 0; ts<numTS; ts++) {
			XmlNode *node = _rootnode->GetChild(ts);
			int n = node->GetNumChildren();

			for (size_t i = 0; i < n; i++) {
				XmlNode *var = node->GetChild(i);

				string base = var->GetElementString(_basePathTag);
				if (base[0] != '/') {
					if (ConstructFullVBase(ts, var->Tag(), &base) < 0) {
						return(-1);
					}
					var->SetElementString(_basePathTag, base);
				}
			}

			string base = node->GetElementString(_auxBasePathTag);
			if (base[0] != '/') {
				if (ConstructFullAuxBase(ts, &base) < 0) {
					return(-1);
				}
				node->SetElementString(_auxBasePathTag, base);
			}

		}
	}

	ofstream fileout;
	fileout.open(path.c_str());
	if (! fileout) {
		SetErrMsg("Can't open file \"%s\" for writing", path.c_str());
		return(-1);
	}
	fileout << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
	fileout << *_rootnode;
	return(0);
}

int Metadata::SetGridType(const string &value) {


	if (!IsValidGridType(value)) {
		SetErrMsg("Invalid GridType specification : \"%s\"", value.c_str());
		return(-1);
	}

	SetDiagMsg("Metadata::SetGridType(%s)", value.c_str());

	_rootnode->SetElementString(_gridTypeTag, value);
	return(0);
}

int Metadata::SetCoordSystemType(const string &value) {


	if (!IsValidCoordSystemType(value)) {
		SetErrMsg("Invalid CoordinateSystem specification : \"%s\"", value.c_str());
		return(-1);
	}

	SetDiagMsg("Metadata::SetCoordSystemType(%s)", value.c_str());

	_rootnode->SetElementString(_coordSystemTypeTag, value);
	return(0);
}

int Metadata::SetExtents(const vector<double> &value) {
	if (!IsValidExtents(value)) {
		SetErrMsg("Invalid Extents specification");
		return(-1);
	}

	SetDiagMsg(
		"Metadata::SetExtents([%f %f %f %f %f %f])",
		value[0], value[1], value[2], value[3], value[4], value[5]
	);

	_rootnode->SetElementDouble(_extentsTag, value);
	return(0);
}

int Metadata::IsValidExtents(const vector<double> &value) const {
	if (value.size() != 6) return (0);
	if (value[0]>=value[3]) return(0);
	if (value[1]>=value[4]) return(0);
	if (value[2]>=value[5]) return(0);
	return(1);
}

int Metadata::_SetNumTimeSteps(long value) {
	size_t newN = (size_t) value;
	size_t oldN = _rootnode->GetNumChildren();
	map <string, string> attrs;

	// Add children
	//
	if (newN > oldN) {
		ostringstream oss;
		string empty;
		empty.clear();

		for (size_t ts = oldN; ts < newN; ts++) {
			XmlNode *child;
			child = _rootnode->NewChild(_timeStepTag, attrs, _varNames.size());
			_SetVariableNames(child, (long)ts);

			vector <double> valvec(1, (double) ts);
			SetTSUserTime(ts, valvec);


			// Set the base path for any auxiliary data.
			//
			oss.str(empty);
			oss << "aux/aux";
			oss << ".";
			oss.width(4);
			oss.fill('0');
			oss << ts;
			oss.width(0);
			child->SetElementString(_auxBasePathTag, oss.str());

		}
	}
	// Delete children
	//
	else {
		for (size_t ts = newN; ts< oldN; ts++) {
			_rootnode->DeleteChild(newN);
		}
	}
	return(0);
}

//
// Delete 'value' from 'vec' if 'value' is contained in 'vec'. Otherwise
// 'vec' is unchanged.
//
void vector_delete (vector <string> &vec, const string &value) {

	vector<string>::iterator itr;
	for (itr = vec.begin(); itr != vec.end(); ) {
		if (itr->compare(value) == 0) {
			vec.erase(itr);
			itr = vec.begin();
		}
		else {
			itr++;
		}
	}
}

//
// Delete any duplicate entries from 'vec'.
//
void vector_unique (vector <string> &vec) {

	vector <string> tmpvec;
	vector<string>::iterator itr1;
	vector<string>::iterator itr2;

	for (itr1 = vec.begin(); itr1 != vec.end(); itr1++) {
		bool match = false;
		for (itr2 = tmpvec.begin(); itr2 != tmpvec.end(); itr2++) {
			if (itr1->compare(*itr2) == 0) {
				match = true;
			}
		}
		if (! match) tmpvec.push_back(*itr1);
	}
	vec = tmpvec;
}

int Metadata::_SetVariableNames(XmlNode *node, long ts) {

	map <string, string> attrs; // empty map
	int oldN = node->GetNumChildren();
	int newN = _varNames.size();

	if (newN > oldN) {

		// Create new children if needed (if not renaming old children);
		//
		for (int i = oldN; i < newN; i++) {
			node->NewChild(_varNames[(size_t) i], attrs, 0);
		}
	}
	else {
		for (int i = oldN-1; i>= newN; i--) {
			node->DeleteChild((size_t) i);
		}
	}


	string empty;
	ostringstream oss;

	// Set the variable name (superflous if the child is new) and
	// set the data file path for each variable child node
	//
	for (size_t i = 0; i < newN; i++) {
		XmlNode *var = node->GetChild(i);
		var->Tag() = _varNames[i]; // superfluous if a new variable
		oss.str(empty);

		oss << _varNames[i].c_str() << "/" << _varNames[i].c_str();
		oss << ".";
		oss.width(4);
		oss.fill('0');
		oss << ts;
		oss.width(0);
		var->SetElementString(_basePathTag, oss.str());
	}

	return(0);
}

int Metadata::SetNumTimeSteps(long value) {
	vector <long> valvec(1,value);


	if (!IsValidTimeStep(value)) {
		SetErrMsg("Invalid NumTimeSteps specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetNumTimeSteps([%d])", value);

	if (_SetNumTimeSteps(value) < 0) return(-1);

	_rootnode->SetElementLong(_numTimeStepsTag, valvec); 
	return(0);
}

long Metadata::GetNumTimeSteps() const {
	const vector <long> &rvec = _rootnode->GetElementLong(_numTimeStepsTag);

	if (rvec.size()) return(rvec[0]);
	else return(-1);
};

int Metadata::SetVariableNames(const vector <string> &value) {
	size_t numTS = _rootnode->GetNumChildren();

	SetDiagMsg("Metadata::SetVariableNames()");

	vector <string> value_unique = value;
	vector_unique(value_unique);

	_varNames = value_unique;

	// For each time step we need to create a list of variable nodes
	//
	for(size_t i = 0; i<numTS; i++) {
		if (_SetVariableNames(_rootnode->GetChild(i), (long)i) < 0) return(-1);
	}

	_rootnode->SetElementStringVec(_varNamesTag, value_unique);

	_varNames3D = value_unique;


    if (this->GetVDFVersion() < 3) return(0);

	// By default all variables are of type 3D
	_rootnode->SetElementStringVec(_vars3DTag, value_unique);


	// Clear all other data types
	string empty;
	_rootnode->SetElementString(_vars2DXYTag, empty); _varNames2DXY.clear();
	_rootnode->SetElementString(_vars2DXZTag, empty); _varNames2DXZ.clear();
	_rootnode->SetElementString(_vars2DYZTag, empty); _varNames2DYZ.clear();

	return(0);
}


int Metadata::_setVariableTypes(
	const string &tag,
	const vector <string> &delete_tags,
	const vector <string> &value
) {
	vector <string> value_unique = value;
	vector_unique(value_unique);
	// 
	// Make sure the variables all exist (i.e. previously defined)
	//
	const vector <string> &vnames = GetVariableNames();
	vector<string>::const_iterator itr1;
	vector<string>::const_iterator itr2;
	for (itr1 = value_unique.begin(); itr1 != value_unique.end(); itr1++) {
		bool match = false;
		for (itr2=vnames.begin(); itr2!=vnames.end(); itr2++) {
			if (itr1->compare(*itr2) == 0) match = true;
		}
		if (! match) {
			SetErrMsg("Variable %s not defined", itr1->c_str());
			return(-1);
		}
	}

	_rootnode->SetElementStringVec(tag, value_unique);

	//
	// Remove variables in 'value' from all other type
	// lists.
	//
	for (itr1 = delete_tags.begin(); itr1 != delete_tags.end(); itr1++) {
		vector <string> vtypes;
		_rootnode->GetElementStringVec(*itr1, vtypes);


		// Delete each var name found in 'value' from the 
		// type list associated with the tag *itr1
		//
		for (itr2 = value_unique.begin(); itr2 != value_unique.end(); itr2++) {
			vector_delete(vtypes, *itr2);
		}
		//
		// Now restore the remaining, undeleted variable names
		//
		_rootnode->SetElementStringVec(*itr1, vtypes);
	}

	// 
	// Update cache 
	//
	_rootnode->GetElementStringVec(_vars3DTag, _varNames3D);
	_rootnode->GetElementStringVec(_vars2DXYTag, _varNames2DXY);
	_rootnode->GetElementStringVec(_vars2DXZTag, _varNames2DXZ);
	_rootnode->GetElementStringVec(_vars2DYZTag, _varNames2DYZ);

	return(0);
}


int Metadata::SetVariables3D(const vector <string> &value) {

	SetDiagMsg("Metadata::SetVariables3D()");

	if (this->GetVDFVersion() < 3) {
		SetErrMsg("Pre version 3 vdf files not supported");
		return(-1);
	}

	vector <string> delete_tags;
	
	delete_tags.push_back(_vars2DXYTag);
	delete_tags.push_back(_vars2DXZTag);
	delete_tags.push_back(_vars2DYZTag);

	if (_setVariableTypes(_vars3DTag, delete_tags, value)<0) return(-1);

	return(0);
}

int Metadata::SetVariables2DXY(const vector <string> &value) {

	SetDiagMsg("Metadata::SetVariables2DXY()");

	if (this->GetVDFVersion() < 3) {
		SetErrMsg("Pre version 3 vdf files not supported");
		return(-1);
	}

	vector <string> delete_tags;
	
	delete_tags.push_back(_vars3DTag);
	delete_tags.push_back(_vars2DXZTag);
	delete_tags.push_back(_vars2DYZTag);

	if (_setVariableTypes(_vars2DXYTag, delete_tags, value)<0) return(-1);
	return(0);
}

int Metadata::SetVariables2DXZ(const vector <string> &value) {

	SetDiagMsg("Metadata::SetVariables2DXZ()");

	if (this->GetVDFVersion() < 3) {
		SetErrMsg("Pre version 3 vdf files not supported");
		return(-1);
	}

	vector <string> delete_tags;
	
	delete_tags.push_back(_vars3DTag); 
	delete_tags.push_back(_vars2DXYTag);
	delete_tags.push_back(_vars2DYZTag);

	if (_setVariableTypes(_vars2DXZTag, delete_tags, value)<0) return(-1);
	return(0);
}

int Metadata::SetVariables2DYZ(const vector <string> &value) {

	SetDiagMsg("Metadata::SetVariables2DYZ()");

	if (this->GetVDFVersion() < 3) {
		SetErrMsg("Pre version 3 vdf files not supported");
		return(-1);
	}

	vector <string> delete_tags;
	
	delete_tags.push_back(_vars3DTag);
	delete_tags.push_back(_vars2DXYTag);
	delete_tags.push_back(_vars2DXZTag);

	if (_setVariableTypes(_vars2DYZTag, delete_tags, value)<0) return(-1);
	return(0);
}

int Metadata::SetTSUserTime(size_t ts, const vector<double> &value) {


	if (! IsValidUserTime(value)) {
		SetErrMsg("Invalid user time specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSUserTime(%d, [%d,...])", ts, value[0]);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_userTimeTag, value);
	return(0);
}

int Metadata::SetTSXCoords(size_t ts, const vector<double> &value) {
	if (! IsValidXCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSXCoords(%d)", ts);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_xCoordsTag, value);
	return(0);
}

int Metadata::SetTSYCoords(size_t ts, const vector<double> &value) {
	if (! IsValidYCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSYCoords(%d)", ts);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_yCoordsTag, value);
	return(0);
}

int Metadata::SetTSZCoords(size_t ts, const vector<double> &value) {
	if (! IsValidZCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSZCoords(%d)", ts);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_zCoordsTag, value);
	return(0);
}

int Metadata::SetTSComment(
	size_t ts, const string &value
) {
	XmlNode	*timenode;

	SetDiagMsg("Metadata::SetTSComment(%d, %s)", ts, value.c_str());

	CHK_TS_REQ(ts, -1);
	if (! (timenode = _rootnode->GetChild(ts))) return(-1);

	timenode->SetElementString(_commentTag, value);
	return(0);
}

int Metadata::SetVComment(
	size_t ts, const string &var, const string &value
) {
	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg(
		"Metadata::SetVComment(%d, %s, %s)",
		ts, var.c_str(), value.c_str()
	);

	CHK_VAR_REQ(ts, var, -1);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	varnode->SetElementString(_commentTag, value);
	return(0);
}

const string &Metadata::GetVBasePath(
	size_t ts, const string &var
) const {

	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg("Metadata::GetVBasePath(%d, %s)", ts, var.c_str());

	CHK_VAR_REQ(ts, var, _emptyString);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	return(varnode->GetElementString(_basePathTag));
}

int Metadata::SetVBasePath(
	size_t ts, const string &var, const string &value
) {

	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg(
		"Metadata::SetVBasePath(%d, %s, %s)", ts, var.c_str(), value.c_str()
	);

	CHK_VAR_REQ(ts, var, -1);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	varnode->SetElementString(_basePathTag, value);
	return(0);
}

int Metadata::SetVDataRange(
	size_t ts, const string &var, const vector<double> &value
) {

	SetDiagMsg(
		"Metadata::SetVDataRange(%d, %s, [%f, %f])", ts, var.c_str(), value[0], value[1]
	);

	XmlNode	*timenode;
	XmlNode	*varnode;

    if (! IsValidVDataRange(value)) {
        SetErrMsg("Invalid data range specification");
        return(-1);
    }

	if (_vdfVersion > 1) {
		SetErrMsg("Operation only permitted on pre-version 2 files");
		return(-1);
	}


	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);
	CHK_VAR_REQ(ts, var, -1);

	varnode->SetElementDouble(_dataRangeTag, value);
	return(0);
}

int	Metadata::_RecordUserDataTags(vector<string> &keys, const string &tag) {

	// See if key has already been defined
	//
	for(int i=0; i<(int)keys.size(); i++) {
		if (StrCmpNoCase(keys[i], tag) == 0) return(1);
	} 

	keys.push_back(tag);
	return(0);
}

bool	Metadata::elementStartHandler(ExpatParseMgr* pm, int level , std::string& tagstr, const char **attrs){
	//const XML_Char *tag, const char **attrs
//) {
	
	// Invoke the appropriate start element handler depending on 
	// the XML tree depth
	//
	switch  (level) {
		case 0:
			_startElementHandler0(pm, tagstr, attrs);
			break;
		case 1:
			_startElementHandler1(pm, tagstr, attrs);
			break;
		case 2:
			_startElementHandler2(pm, tagstr, attrs);
			break;
		case 3:
			_startElementHandler3(pm, tagstr, attrs);
			break;
		default:
			pm->parseError("Invalid tag : %s", tagstr.c_str());
			return false;
	}
	return true;
}

bool	Metadata::elementEndHandler(ExpatParseMgr* pm, int level , std::string& tagstr)
 
{

	

	// Invoke the appropriate end element handler for an element at
	// XML tree depth, 'level'
	//
	switch  (level) {
	case 0:
		_endElementHandler0(pm, tagstr);
		break;
	case 1:
		_endElementHandler1(pm, tagstr);
		break;
	case 2:
		_endElementHandler2(pm, tagstr);
		break;
	case 3:
		_endElementHandler3(pm, tagstr);
		break;
	default:
		pm->parseError("Invalid state");
		return false;
	}
	return true;
}



// Level 0 start element handler. The only element tag permitted at this
// level is the '_rootTag' tag
//
void	Metadata::_startElementHandler0(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {

	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	_currentTS = 0;

	size_t bs[3];
	size_t dim[3];
	int nFilterCoef = 1;
	int nLiftingCoef = 1;
	int msbFirst = 1;
	int vdfVersion = 1;  //If it's not set it should be 1 ?? not VDF_VERSION;?
	int numTransforms = 0;


	// Verify valid level 0 element
	//
	if (StrCmpNoCase(tag, _rootTag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

	// Parse attributes
	//
	while (*attrs) {
		string attr = *attrs;
		attrs++;
		string value = *attrs;
		attrs++;

		istringstream ist(value);
		if (StrCmpNoCase(attr, _blockSizeAttr) == 0) {
			ist >> bs[0]; 
			
			// Pre version 2, block size was a scalar;
			if (! ist.eof()) ist >> bs[1]; 
			if (! ist.eof()) ist >> bs[2];
		}
		else if (StrCmpNoCase(attr, _dimensionLengthAttr) == 0) {
			ist >> dim[0]; ist >> dim[1]; ist >> dim[2];
		}
		else if (StrCmpNoCase(attr, _numTransformsAttr) == 0) {
			ist >> numTransforms;
		}
		else if (StrCmpNoCase(attr, _filterCoefficientsAttr) == 0) {
			ist >> nFilterCoef;
		}
		else if (StrCmpNoCase(attr, _liftingCoefficientsAttr) == 0) {
			ist >> nLiftingCoef;
		}
		else if (StrCmpNoCase(attr, _msbFirstAttr) == 0) {
			ist >> msbFirst;
		}
		else if (StrCmpNoCase(attr, _vdfVersionAttr) == 0) {
			ist >> vdfVersion;
		}
		else {
			pm->parseError("Invalid tag attribute : \"%s\"", attr.c_str());
		}
	}

	if (vdfVersion < 2) {
		bs[1] = bs[2] = bs[0];
	}

	_init(dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst, vdfVersion);
	if (GetErrCode()) {
		string s(GetErrMsg()); pm->parseError("%s", s.c_str());
		return;
	}


}

void	Metadata::_startElementHandler1(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	string type;

	// Either have a TimeStep element (with no attributes) or a data element
	//
	if (! *attrs) {
		if (StrCmpNoCase(tag, _timeStepTag) == 0) {
			if (*attrs) {
				pm->parseError("Unexpected element attribute : %s", *attrs);
				return;
			}
		}
		else {
			pm->parseError("Invalid tag : \"%s\"", tag.c_str());
			return;
		}
		return;
	}

	// Must be a data element. Make sure it has a type attribute.
	//


	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		pm->parseError("Too many attributes");
		return;
	}
	istringstream ist(value);


	state->has_data = 1;
	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		pm->parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;
	if (StrCmpNoCase(tag, _numTimeStepsTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _extentsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _coordSystemTypeTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _gridTypeTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _varNamesTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars3DTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars2DXYTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars2DXZTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars2DYZTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _periodicBoundaryTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _gridPermutationTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user or optional data
		if (!((StrCmpNoCase(type, _stringType) != 0) ||
			(StrCmpNoCase(type, _longType) != 0) || 
			(StrCmpNoCase(type, _doubleType) != 0))) {

			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
		state->user_defined = 1;
	}
}

void	Metadata::_startElementHandler2(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	string type;


	// Its either a variable element (with no attributes) or a data element
	//
	if (! *attrs) {

		// Only variable name tags permitted.
		//
		for(int i = 0; i<(int)_varNames.size(); i++) {
			if ((StrCmpNoCase(tag, _varNames[i]) == 0)) {
				_currentVar = tag;
				return;
			}
		}
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

	// Must be a data element. Make sure it has a type attribute.
	//


	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		pm->parseError("Too many attributes");
		return;
	}
	istringstream ist(value);



	state->has_data = 1;

	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		pm->parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;

	if (StrCmpNoCase(tag, _userTimeTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _auxBasePathTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user or optional data
		if (!((StrCmpNoCase(type, _stringType) != 0) ||
			(StrCmpNoCase(type, _longType) != 0) || 
			(StrCmpNoCase(type, _doubleType) != 0))) {

			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
		state->user_defined = 1;
	}
}


void	Metadata::_startElementHandler3(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	string type;

	if (! *attrs) {
		pm->parseError("Expected element attribute");
		return;
	}

	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		pm->parseError("Too many attributes");
		return;
	}
	istringstream ist(value);


	// Must be a data element. Make sure it has a type attribute.
	//

	state->has_data = 1;

	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		pm->parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;

	if (StrCmpNoCase(tag, _basePathTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	} else if (StrCmpNoCase(tag, _dataRangeTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user or optional data
		if (!((StrCmpNoCase(type, _stringType) != 0) ||
			(StrCmpNoCase(type, _longType) != 0) || 
			(StrCmpNoCase(type, _doubleType) != 0))) {

			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
		state->user_defined = 1;
	}
}

void	Metadata::_endElementHandler0(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	Metadata::_endElementHandler1(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	if (! state->has_data) {
		if (StrCmpNoCase(tag, _timeStepTag) == 0) {
			_currentTS++;
		} 
	}
	else if (StrCmpNoCase(tag, _numTimeStepsTag) == 0) {
		if (SetNumTimeSteps(pm->getLongData()[0]) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _extentsTag) == 0) {
		if (SetExtents(pm->getDoubleData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _coordSystemTypeTag) == 0) {
		if (SetCoordSystemType(pm->getStringData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _gridTypeTag) == 0) {
		if (SetGridType(pm->getStringData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _varNamesTag) == 0) {
		istringstream ist(pm->getStringData());
		string v;
		vector <string> vec;

		while (ist >> v) {
			vec.push_back(v);
		}
		if (SetVariableNames(vec) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars3DTag) == 0) {
		_rootnode->SetElementString(_vars3DTag, pm->getStringData());
		_rootnode->GetElementStringVec(_vars3DTag, _varNames3D);
	}
	else if (StrCmpNoCase(tag, _vars2DXYTag) == 0) {
		_rootnode->SetElementString(_vars2DXYTag, pm->getStringData());
		_rootnode->GetElementStringVec(_vars2DXYTag, _varNames2DXY);
	}
	else if (StrCmpNoCase(tag, _vars2DXZTag) == 0) {
		_rootnode->SetElementString(_vars2DXZTag, pm->getStringData());
		_rootnode->GetElementStringVec(_vars2DXZTag, _varNames2DXZ);
	}
	else if (StrCmpNoCase(tag, _vars2DYZTag) == 0) {
		_rootnode->SetElementString(_vars2DYZTag, pm->getStringData());
		_rootnode->GetElementStringVec(_vars2DYZTag, _varNames2DYZ);
	}
	else if (StrCmpNoCase(tag, _periodicBoundaryTag) == 0) {
		if (SetPeriodicBoundary(pm->getLongData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _gridPermutationTag) == 0) {
		if (SetGridPermutation(pm->getLongData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else {
		int	rc;
		// must be user or optional data

		if (StrCmpNoCase(state->data_type, _stringType) == 0) {
			rc = SetUserDataString(tag, pm->getStringData());
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			rc = SetUserDataLong(tag, pm->getLongData());
		}
		else {
			rc = SetUserDataDouble(tag, pm->getDoubleData());
		}
		if (rc < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
}

void	Metadata::_endElementHandler2(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	if (! state->has_data) {
		// do nothing

	} else if (StrCmpNoCase(tag, _userTimeTag) == 0) {
		if (SetTSUserTime(_currentTS, pm->getDoubleData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _auxBasePathTag) == 0) {
		if (SetTSUserDataString(_currentTS, tag, pm->getStringData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (SetTSComment(_currentTS, pm->getStringData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _xCoordsTag) == 0) {
		if (SetTSXCoords(_currentTS, pm->getDoubleData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _yCoordsTag) == 0) {
		if (SetTSYCoords(_currentTS, pm->getDoubleData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _zCoordsTag) == 0) {
		if (SetTSZCoords(_currentTS, pm->getDoubleData()) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else {
		int	rc;
		// must be user or optional data

		if (StrCmpNoCase(state->data_type, _stringType) == 0) {
			rc = SetTSUserDataString(_currentTS, tag, pm->getStringData());
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			rc = SetTSUserDataLong(_currentTS, tag, pm->getLongData());
		}
		else {
			rc = SetTSUserDataDouble(_currentTS, tag, pm->getDoubleData());
		}
		if (rc < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
}


void	Metadata::_endElementHandler3(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (SetVComment(_currentTS, _currentVar, pm->getStringData())<0){
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _basePathTag) == 0) {
		if (SetVUserDataString(
			_currentTS, _currentVar, tag,  pm->getStringData()) < 0) {

			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _dataRangeTag) == 0) {
		if (SetVDataRange(
			_currentTS, _currentVar, pm->getDoubleData()) < 0) {

			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else {
		int	rc;
		// must be user or optional data

		if (StrCmpNoCase(state->data_type, _stringType) == 0) {
			rc = SetVUserDataString(
				_currentTS, _currentVar, tag, pm->getStringData()
			);
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			rc = SetVUserDataLong(
				_currentTS, _currentVar, tag, pm->getLongData()
			);
		}
		else {
			rc = SetVUserDataDouble(
				_currentTS, _currentVar, tag, pm->getDoubleData()
			);
		}
		if (rc < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
}
