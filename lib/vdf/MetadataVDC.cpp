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
//	File:		MetadataVDC.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Sep 30 12:13:03 MDT 2004
//
//	Description:	Implements the MetadataVDC class
//


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include <vapor/MetadataVDC.h>
#include <vapor/XmlNode.h>
#include <vapor/CFuncs.h>

using namespace VAPoR;
using namespace VetsUtil;

// Static member initialization
//
const string MetadataVDC::_childrenTag = "Children";
const string MetadataVDC::_commentTag = "Comment";
const string MetadataVDC::_coordSystemTypeTag = "CoordinateSystem";
const string MetadataVDC::_dataRangeTag = "DataRange";
const string MetadataVDC::_extentsTag = "Extents";
const string MetadataVDC::_gridTypeTag = "GridType";
const string MetadataVDC::_numTimeStepsTag = "NumTimeSteps";
const string MetadataVDC::_basePathTag = "BasePath";
const string MetadataVDC::_auxBasePathTag = "AuxBasePath";
const string MetadataVDC::_rootTag = "Metadata";
const string MetadataVDC::_userTimeTag = "UserTime";
const string MetadataVDC::_userTimeStampTag = "UserTimeStampString";
const string MetadataVDC::_timeStepTag = "TimeStep";
const string MetadataVDC::_varNamesTag = "VariableNames";	// pre ver 4
const string MetadataVDC::_vars3DTag = "Variables3D";
const string MetadataVDC::_vars2DXYTag = "Variables2DXY";
const string MetadataVDC::_vars2DXZTag = "Variables2DXZ";
const string MetadataVDC::_vars2DYZTag = "Variables2DYZ";
const string MetadataVDC::_xCoordsTag = "XCoords";
const string MetadataVDC::_yCoordsTag = "YCoords";
const string MetadataVDC::_zCoordsTag = "ZCoords";
const string MetadataVDC::_periodicBoundaryTag = "PeriodicBoundary";
const string MetadataVDC::_gridPermutationTag = "GridPermutation";
const string MetadataVDC::_mapProjectionTag = "MapProjection";


const string MetadataVDC::_blockSizeAttr = "BlockSize";
const string MetadataVDC::_dimensionLengthAttr = "DimensionLength";
const string MetadataVDC::_numTransformsAttr = "NumTransforms";
const string MetadataVDC::_filterCoefficientsAttr = "FilterCoefficients";
const string MetadataVDC::_liftingCoefficientsAttr = "LiftingCoefficients";
const string MetadataVDC::_msbFirstAttr = "MSBFirst";
const string MetadataVDC::_vdfVersionAttr = "VDFVersion";
const string MetadataVDC::_numChildrenAttr = "NumChildren";
const string MetadataVDC::_waveletNameAttr = "WaveletName";
const string MetadataVDC::_waveletBoundaryModeAttr = "WaveletBoundaryMode";
const string MetadataVDC::_vdcTypeAttr = "VDCType";
const string MetadataVDC::_cRatiosAttr = "CompressionRatios";

namespace {

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

};

int MetadataVDC::SetDefaults() {

	vector <long> periodic_boundary(3,0);
	SetPeriodicBoundary(periodic_boundary);

	vector <long> grid_permutation;
	grid_permutation.push_back(0);
	grid_permutation.push_back(1);
	grid_permutation.push_back(2);
	SetGridPermutation(grid_permutation);

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
int MetadataVDC::_init() {

	_emptyDoubleVec.clear();
	_emptyLongVec.clear();
	_emptyString.clear();

	if (SetNumTimeSteps(1) < 0) return(-1);

	vector<string> varNamesVec(1,"var1");
	if (SetVariables3D(varNamesVec) < 0) return(-1);

	string comment = "";
	_rootnode->SetElementString(_commentTag, comment);

	return(SetDefaults());
}

int MetadataVDC::_init1(
		const size_t dim[3], size_t numTransforms, const size_t bs[3],
		int nFilterCoef, int nLiftingCoef, int msbFirst, int vdfVersion
) {
	map <string, string> attrs;
	ostringstream oss;
	string empty;

	if (! (bs[0] == bs[1] && bs[1] == bs[2])) {
		SetErrMsg("Block dimensions not symmetric");
		return(-1);
	}

	for(int i=0; i<3; i++) {
		_bs[i] = bs[i];
		_dim[i] = dim[i];
	}
	_numTransforms = (int)numTransforms;
	_nFilterCoef = nFilterCoef;
	_nLiftingCoef = nLiftingCoef;
	_msbFirst = msbFirst;
	_vdfVersion = vdfVersion;
	_vdcType = 1;
	_wname = "";
	_wmode = "";
	_cratios.clear();

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

	//
	// For now maintain compatability with pre version 4 
	//
	//oss.str(empty);
	//oss << _vdcType;
	//attrs[_vdcTypeAttr] = oss.str();


	// Create the root node of an xml tree. The node is named (tagged)
	// by 'roottag', and will have XML attributes as specified by 'attrs'
	//
	_rootnode = new XmlNode(_rootTag, attrs);
	if (XmlNode::GetErrCode() != 0) return(-1);
	_rootnode->ErrOnMissing() = false;

	return(_init());
}

// Initialize the class object
//
int MetadataVDC::_init2(
	const size_t dim[3], const size_t bs[3], const vector <size_t> &cratios, 
	string wname, string wmode, int vdfVersion
) {
	map <string, string> attrs;
	ostringstream oss;
	string empty;

	for (int i=0; i<cratios.size()-1; i++) {
		if (cratios[i] == cratios[i+1]) {
			SetErrMsg("Invalid compression ratio - non unique entries");
			return(-1);
		}
	}

	_numTransforms = 0;
	_nFilterCoef = 0;
	_nLiftingCoef = 0;
	_msbFirst = 0;
	_vdfVersion = vdfVersion;
	_wname = wname;
	_wmode = wmode;
	_vdcType = 2;
	_cratios = cratios;


	for(int i=0; i<3; i++) {
		_bs[i] = bs[i];
		_dim[i] = dim[i];
	}

	oss.str(empty);
	oss << (unsigned int)_bs[0] << " " << (unsigned int)_bs[1] << " " << (unsigned int)_bs[2];
	attrs[_blockSizeAttr] = oss.str();

	oss.str(empty);
	oss << (unsigned int)_dim[0] << " " << (unsigned int)_dim[1] << " " << (unsigned int)_dim[2];
	attrs[_dimensionLengthAttr] = oss.str();

	attrs[_waveletNameAttr] = wname;
	attrs[_waveletBoundaryModeAttr] = wmode;

	oss.str(empty);
	oss << _vdfVersion;
	attrs[_vdfVersionAttr] = oss.str();

	oss.str(empty);
	oss << _vdcType;
	attrs[_vdcTypeAttr] = oss.str();

	oss.str(empty);
	for (int i=0; i<cratios.size(); i++) {
		oss << cratios[i];
		if (i != cratios.size()-1) oss << " ";
	}
	attrs[_cRatiosAttr] = oss.str();
		


	// Create the root node of an xml tree. The node is named (tagged)
	// by 'roottag', and will have XML attributes as specified by 'attrs'
	//
	_rootnode = new XmlNode(_rootTag, attrs);
	if (XmlNode::GetErrCode() != 0) return(-1);
	_rootnode->ErrOnMissing() = false;

	return(_init());
}

MetadataVDC::MetadataVDC(
		const size_t dim[3], size_t numTransforms, const size_t bs[3], 
		int nFilterCoef, int nLiftingCoef, int msbFirst, int vdfVersion
) {
	SetDiagMsg(
		"MetadataVDC::MetadataVDC([%d,%d,%d], %d, [%d,%d%d] %d, %d, %d, %d)", 
		dim[0], dim[1], dim[2], numTransforms, bs[0],bs[1],bs[2], 
		nFilterCoef, nLiftingCoef, msbFirst, vdfVersion
	);

	_rootnode = NULL;
	_metafileDirName.clear();
	_metafileName.clear();
	_dataDirName.clear();

	if (_init1(dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst, vdfVersion) < 0){
		return;
	}
}

MetadataVDC::MetadataVDC(
	const size_t dim[3], const size_t bs[3], const vector <size_t> &cratios,
	string wname, string wmode
) {
	SetDiagMsg(
		"MetadataVDC::MetadataVDC([%d,%d,%d], [%d,%d%d] %s, %s)", 
		dim[0], dim[1], dim[2], bs[0],bs[1],bs[2], wname.c_str(), wmode.c_str()
	);

	_rootnode = NULL;
	_metafileDirName.clear();
	_metafileName.clear();
	_dataDirName.clear();

	if (_init2(dim, bs, cratios, wname, wmode, VDF_VERSION) < 0){
		return;
	}
}



MetadataVDC::MetadataVDC(const string &metafile) {

	SetDiagMsg("MetadataVDC::MetadataVDC(%s)", metafile.c_str());

	ifstream is;
	_rootnode = NULL;

	is.open(metafile.c_str());
	if (! is) {
		SetErrMsg("Can't open file \"%s\" for reading", metafile.c_str());
		return;
	}

	// Get directory path of metafile
	char *s = new char[metafile.length() +1];
	(void) Dirname(metafile.c_str(), s);
	_metafileDirName.assign(s);
	delete [] s;
	_metafileName.assign(Basename(metafile.c_str()));

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
	
}

MetadataVDC::MetadataVDC(const MetadataVDC &node)
{
	*this = node;
	this->_rootnode = new XmlNode(*(node._rootnode));
	this->_rootnode->ErrOnMissing() = false;
}

MetadataVDC::~MetadataVDC() {
	SetDiagMsg("MetadataVDC::~MetadataVDC()");

	if (_rootnode) delete _rootnode;
}


int MetadataVDC::Merge(const MetadataVDC &metadata, size_t ts_start) {

	const size_t *bs = this->GetBlockSize();
	const size_t *mbs = metadata.GetBlockSize();

	for(int i=0; i<3; i++) {
		if (bs[i] != mbs[i]) {
			SetErrMsg("Merge failed: block size mismatch");
			return(-1);
		}
	}

	size_t dim[3];
	this->GetDim(dim, -1);

	size_t mdim[3];
	metadata.GetDim(mdim, -1);

	for(int i=0; i<3; i++) {
		if (dim[i] != mdim[i]) {
			SetErrMsg("Merge failed: dimension mismatch");
			return(-1);
		}
	}

	if (this->GetFilterCoef() != metadata.GetFilterCoef()) {
		SetErrMsg("Merge failed: filter coefficient mismatch");
		return(-1);
	}

	if (this->GetLiftingCoef() != metadata.GetLiftingCoef()) {
		SetErrMsg("Merge failed: lifting coefficient mismatch");
		return(-1);
	}

	if (this->GetNumTransforms() != metadata.GetNumTransforms()) {
		SetErrMsg("Merge failed: num transforms mismatch");
		return(-1);
	}

	if (this->GetWaveName().compare(metadata.GetWaveName()) != 0) {
		SetErrMsg("Merge failed: Wavelet names mismatch");
		return(-1);
	}

	if (this->GetBoundaryMode().compare(metadata.GetBoundaryMode()) != 0) {
		SetErrMsg("Merge failed: Wavelet boundary handling mismatch");
		return(-1);
	}


	//
	// Now merge the variable names
	//
	const vector <string> &varnames = this->GetVariableNames();
	const vector <string> &mvarnames = metadata.GetVariableNames();

	const vector <string> &vars3d0 = this->GetVariables3D();
	const vector <string> &vars3d1 = metadata.GetVariables3D();
	vector <string> vars3d;
	for (int i=0; i<vars3d0.size(); i++) vars3d.push_back(vars3d0[i]);
	for (int i=0; i<vars3d1.size(); i++) vars3d.push_back(vars3d1[i]);
	vector_unique(vars3d);

	const vector <string> &vars2dxy0 = this->GetVariables2DXY();
	const vector <string> &vars2dxy1 = metadata.GetVariables2DXY();
	vector <string> vars2dxy;
	for (int i=0; i<vars2dxy0.size(); i++) vars2dxy.push_back(vars2dxy0[i]);
	for (int i=0; i<vars2dxy1.size(); i++) vars2dxy.push_back(vars2dxy1[i]);
	vector_unique(vars2dxy);

	const vector <string> &vars2dxz0 = this->GetVariables2DXZ();
	const vector <string> &vars2dxz1 = metadata.GetVariables2DXZ();
	vector <string> vars2dxz;
	for (int i=0; i<vars2dxz0.size(); i++) vars2dxz.push_back(vars2dxz0[i]);
	for (int i=0; i<vars2dxz1.size(); i++) vars2dxz.push_back(vars2dxz1[i]);
	vector_unique(vars2dxz);

	const vector <string> &vars2dyz0 = this->GetVariables2DYZ();
	const vector <string> &vars2dyz1 = metadata.GetVariables2DYZ();
	vector <string> vars2dyz;
	for (int i=0; i<vars2dyz0.size(); i++) vars2dyz.push_back(vars2dyz0[i]);
	for (int i=0; i<vars2dyz1.size(); i++) vars2dyz.push_back(vars2dyz1[i]);
	vector_unique(vars2dyz);

	long ts = this->GetNumTimeSteps();

	// Need to make a copy of the base names. Later, when we call
	// SetVariable*() all of the base names will be regenerated.
	// This is fine if the base names are all default values, but if they're
	// not - if the MetadataVDC object resulted from a previous merge, for
	// example - they will get clobbered by SetVariable*. 
	//
	map <string, vector<string> > basecopy;
	for (int i = 0; i<varnames.size(); i++) {
	for (long t = 0; t<ts; t++) {

		vector <string> &strvec = basecopy[varnames[i]];
		strvec.push_back(GetVBasePath(t, varnames[i]));
	}
	}

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
	long mts = metadata.GetNumTimeSteps();

	if (mts + ts_start > ts) {
		int rc = this->SetNumTimeSteps(mts+ts_start);
		if (rc < 0) return(-1);
	}

	// Push the variables next, replacing the current path names with 
	// **absolute** paths from the new MetadataVDC object.
	//
	for (size_t ts = 0; ts < mts; ts++) {

		for (int i=0; i<mvarnames.size(); i++) {

			string base;

			if (metadata.ConstructFullVBase(ts, mvarnames[i], &base) < 0) {
				return (-1);
			}

			int rc = this->SetVBasePath(ts+ts_start, mvarnames[i], base);
			if (rc < 0) return(-1);
		}
	}

	return(0);
}

int MetadataVDC::Merge(const string &path, size_t ts_start) {
	MetadataVDC *metadata;

	metadata = new MetadataVDC(path);

	if (MetadataVDC::GetErrCode()) return(-1);

	int rc = Merge(*metadata, ts_start);

	delete metadata;

	return(rc);
} 

int MetadataVDC::ConstructFullVBase(
	size_t ts, const string &var, string *path
) const {

	path->clear();

    const string &bp = GetVBasePath(ts, var);
    if (GetErrCode() != 0 || bp.length() == 0) {
		fprintf(stderr, "GetVBasepath error, ts = %d var = %s basepath = %s\n",
			(int) ts, var.c_str(), (*path).c_str());
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

int MetadataVDC::ConstructFullAuxBase(
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

int MetadataVDC::Write(const string &path, int relative_path) {

	SetDiagMsg("MetadataVDC::Write(%s)", path.c_str());


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

int MetadataVDC::SetGridType(const string &value) {

	if (!IsValidGridType(value)) {
		SetErrMsg("Invalid GridType specification : \"%s\"", value.c_str());
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetGridType(%s)", value.c_str());

	_rootnode->SetElementString(_gridTypeTag, value);
	return(0);
}

int MetadataVDC::SetCoordSystemType(const string &value) {


	if (!IsValidCoordSystemType(value)) {
		SetErrMsg("Invalid CoordinateSystem specification : \"%s\"", value.c_str());
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetCoordSystemType(%s)", value.c_str());

	_rootnode->SetElementString(_coordSystemTypeTag, value);
	return(0);
}

int MetadataVDC::SetExtents(const vector<double> &value) {
	if (!IsValidExtents(value)) {
		SetErrMsg("Invalid Extents specification");
		return(-1);
	}

	SetDiagMsg(
		"MetadataVDC::SetExtents([%f %f %f %f %f %f])",
		value[0], value[1], value[2], value[3], value[4], value[5]
	);

	_rootnode->SetElementDouble(_extentsTag, value);
	return(0);
}

int MetadataVDC::IsValidExtents(const vector<double> &value) const {
	if (value.size() != 6) return (0);
	if (value[0]>=value[3]) return(0);
	if (value[1]>=value[4]) return(0);
	if (value[2]>=value[5]) return(0);
	return(1);
}

int MetadataVDC::_SetNumTimeSteps(long value) {
	size_t newN = (size_t) value;
	size_t oldN = _rootnode->GetNumChildren();
	map <string, string> attrs;

	const vector <string> &var_names = GetVariableNames();

	// Add children
	//
	if (newN > oldN) {
		ostringstream oss;
		string empty;
		empty.clear();

		for (size_t ts = oldN; ts < newN; ts++) {
			XmlNode *child;
			child = _rootnode->NewChild(_timeStepTag, attrs, var_names.size());
			_SetVariables(child, (long)ts);

			vector <double> valvec(1, (double) ts);
			SetTSUserTime(ts, valvec);
			
			oss.str(empty);
			oss << (double) ts;
			SetTSUserTimeStamp(ts, oss.str());


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

int MetadataVDC::_SetVariables(XmlNode *node, long ts) {

	
	const vector <string> &var_names = GetVariableNames();

	map <string, string> attrs; // empty map
	int oldN = node->GetNumChildren();
	int newN = var_names.size();

	if (newN > oldN) {

		// Create new children if needed (if not renaming old children);
		//
		for (int i = oldN; i < newN; i++) {
			node->NewChild(var_names[(size_t) i], attrs, 0);
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
		var->Tag() = var_names[i]; // superfluous if a new variable
		oss.str(empty);

		oss << var_names[i].c_str() << "/" << var_names[i].c_str();
		oss << ".";
		oss.width(4);
		oss.fill('0');
		oss << ts;
		oss.width(0);
		var->SetElementString(_basePathTag, oss.str());
	}

	return(0);
}

int MetadataVDC::SetNumTimeSteps(long value) {
	vector <long> valvec(1,value);


	if (!IsValidTimeStep(value)) {
		SetErrMsg("Invalid NumTimeSteps specification");
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetNumTimeSteps([%d])", value);

	if (_SetNumTimeSteps(value) < 0) return(-1);

	_rootnode->SetElementLong(_numTimeStepsTag, valvec); 
	return(0);
}

long MetadataVDC::GetNumTimeSteps() const {
	const vector <long> &rvec = _rootnode->GetElementLong(_numTimeStepsTag);

	if (rvec.size()) return(rvec[0]);
	else return(0);
};


int MetadataVDC::_SetVariableNames(
	string set_tag,
	const vector <string> &delete_tags,
	const vector <string> &value
) {
	size_t numTS = _rootnode->GetNumChildren();

	// remove duplicate entries
	vector <string> value_unique = value;
	vector_unique(value_unique);

	//
	// If a var name is already in use by a different type of variable,
	// delete it.
	//
	for (int i=0; i<delete_tags.size(); i++) {

		vector <string> del_var_names;
		_rootnode->GetElementStringVec(delete_tags[i], del_var_names);

		for (int j=0; j<value_unique.size(); j++) {
			// vector_delete is a no-op if value_unique[j] is not in
			// del_var_names
			//
			vector_delete(del_var_names, value_unique[j]);
		}
		_rootnode->SetElementString(delete_tags[i], del_var_names);
	}

	// Finally set the new list of variable names
	//
	_rootnode->SetElementString(set_tag, value_unique);

	// For each time step we need to create a list of variable nodes
	//
	for(size_t i = 0; i<numTS; i++) {
		if (_SetVariables(_rootnode->GetChild(i), (long)i) < 0) return(-1);
	}

	//
	// Maintain compatability with pre-version 4 translators 
	//
	vector <string> v = GetVariableNames();
	_rootnode->SetElementString(_varNamesTag,v);

	return(0);
}
	
int MetadataVDC::SetVariables3D(const vector <string> &value) {

	SetDiagMsg("MetadataVDC::SetVariables3D()");

	vector <string> delete_tags;
	delete_tags.push_back(_vars2DXYTag);
	delete_tags.push_back(_vars2DXZTag);
	delete_tags.push_back(_vars2DYZTag);

	return(_SetVariableNames(_vars3DTag, delete_tags, value));
}

int MetadataVDC::SetVariables2DXY(const vector <string> &value) {

	SetDiagMsg("MetadataVDC::SetVariables2DXY()");

	vector <string> delete_tags;
	delete_tags.push_back(_vars3DTag);
	delete_tags.push_back(_vars2DXZTag);
	delete_tags.push_back(_vars2DYZTag);

	return(_SetVariableNames(_vars2DXYTag, delete_tags, value));
}

int MetadataVDC::SetVariables2DXZ(const vector <string> &value) {

	SetDiagMsg("MetadataVDC::SetVariables2DXZ()");

	vector <string> delete_tags;
	delete_tags.push_back(_vars3DTag);
	delete_tags.push_back(_vars2DXYTag);
	delete_tags.push_back(_vars2DYZTag);

	return(_SetVariableNames(_vars2DXZTag, delete_tags, value));
}

int MetadataVDC::SetVariables2DYZ(const vector <string> &value) {

	SetDiagMsg("MetadataVDC::SetVariables2DYZ()");

	vector <string> delete_tags;
	delete_tags.push_back(_vars3DTag);
	delete_tags.push_back(_vars2DXYTag);
	delete_tags.push_back(_vars2DXZTag);

	return(_SetVariableNames(_vars2DYZTag, delete_tags, value));
}

int MetadataVDC::SetTSExtents(size_t ts, const vector<double> &value) {
	if (!IsValidExtents(value)) {
		SetErrMsg("Invalid Extents specification");
		return(-1);
	}

	SetDiagMsg(
		"MetadataVDC::SetExtents(%d, [%f %f %f %f %f %f])",
		ts, value[0], value[1], value[2], value[3], value[4], value[5]
	);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_extentsTag, value);

	return(0);
}

int MetadataVDC::SetTSUserTime(size_t ts, const vector<double> &value) {


	if (! IsValidUserTime(value)) {
		SetErrMsg("Invalid user time specification");
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetTSUserTime(%d, [%d,...])", ts, value[0]);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_userTimeTag, value);
	return(0);
}

int MetadataVDC::SetTSUserTimeStamp(size_t ts, const string &value) {

	SetDiagMsg("MetadataVDC::SetTSUserTimeStamp(%d, [%d,...])", ts, value[0]);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementString(_userTimeStampTag, value);
	return(0);
}

int MetadataVDC::SetTSXCoords(size_t ts, const vector<double> &value) {
	if (! IsValidXCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetTSXCoords(%d)", ts);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_xCoordsTag, value);
	return(0);
}

int MetadataVDC::SetTSYCoords(size_t ts, const vector<double> &value) {
	if (! IsValidYCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetTSYCoords(%d)", ts);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_yCoordsTag, value);
	return(0);
}

int MetadataVDC::SetTSZCoords(size_t ts, const vector<double> &value) {
	if (! IsValidZCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("MetadataVDC::SetTSZCoords(%d)", ts);

	CHK_TS_REQ(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_zCoordsTag, value);
	return(0);
}

int MetadataVDC::SetTSComment(
	size_t ts, const string &value
) {
	XmlNode	*timenode;

	SetDiagMsg("MetadataVDC::SetTSComment(%d, %s)", ts, value.c_str());

	CHK_TS_REQ(ts, -1);
	if (! (timenode = _rootnode->GetChild(ts))) return(-1);

	timenode->SetElementString(_commentTag, value);
	return(0);
}

int MetadataVDC::SetVComment(
	size_t ts, const string &var, const string &value
) {
	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg(
		"MetadataVDC::SetVComment(%d, %s, %s)",
		ts, var.c_str(), value.c_str()
	);

	CHK_VAR_REQ(ts, var, -1);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	varnode->SetElementString(_commentTag, value);
	return(0);
}

string MetadataVDC::GetVBasePath(
	size_t ts, const string &var
) const {

	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg("MetadataVDC::GetVBasePath(%d, %s)", ts, var.c_str());

	CHK_VAR_REQ(ts, var, _emptyString);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	return(varnode->GetElementString(_basePathTag));
}

int MetadataVDC::SetVBasePath(
	size_t ts, const string &var, const string &value
) {

	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg(
		"MetadataVDC::SetVBasePath(%d, %s, %s)", ts, var.c_str(), value.c_str()
	);

	CHK_VAR_REQ(ts, var, -1);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	varnode->SetElementString(_basePathTag, value);
	return(0);
}

int MetadataVDC::SetVDataRange(
	size_t ts, const string &var, const vector<double> &value
) {

	SetDiagMsg(
		"MetadataVDC::SetVDataRange(%d, %s, [%f, %f])", ts, var.c_str(), value[0], value[1]
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

int	MetadataVDC::_RecordUserDataTags(vector<string> &keys, const string &tag) {

	// See if key has already been defined
	//
	for(int i=0; i<(int)keys.size(); i++) {
		if (StrCmpNoCase(keys[i], tag) == 0) return(1);
	} 

	keys.push_back(tag);
	return(0);
}

bool	MetadataVDC::elementStartHandler(ExpatParseMgr* pm, int level , std::string& tagstr, const char **attrs){
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

bool	MetadataVDC::elementEndHandler(ExpatParseMgr* pm, int level , std::string& tagstr)
 
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
void	MetadataVDC::_startElementHandler0(ExpatParseMgr* pm,
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
	string wname = "";
	string wmode = "";
	int vdcType = 1;
	vector <size_t> cratios;


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
		else if (StrCmpNoCase(attr, _waveletNameAttr) == 0) {
			ist >> wname;
		}
		else if (StrCmpNoCase(attr, _waveletBoundaryModeAttr) == 0) {
			ist >> wmode;
		}
		else if (StrCmpNoCase(attr, _vdcTypeAttr) == 0) {
			ist >> vdcType;
		}
		else if (StrCmpNoCase(attr, _cRatiosAttr) == 0) {
			while (! ist.eof()) {
				size_t c;
				ist >> c;
				cratios.push_back(c);
			}
		}
		else {
			pm->parseError("Invalid tag attribute : \"%s\"", attr.c_str());
		}
	}

	if (vdfVersion < 2) {
		bs[1] = bs[2] = bs[0];
	}

	if (vdcType == 1) {
		_init1(
			dim, numTransforms, bs, nFilterCoef, nLiftingCoef, 
			msbFirst, VDF_VERSION
		);
	}
	else if (vdcType == 2) {
		_init2(dim, bs, cratios, wname, wmode, VDF_VERSION);
	}
	else {
		pm->parseError("Invalid VDC type: %d", vdcType);
		return;
	}

	if (GetErrCode()) {
		string s(GetErrMsg()); pm->parseError("%s", s.c_str());
		return;
	}
}

void	MetadataVDC::_startElementHandler1(ExpatParseMgr* pm,
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

void	MetadataVDC::_startElementHandler2(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;
	state->user_defined = 0;

	string type;


	// Its either a variable element (with no attributes) or a data element
	//
	if (! *attrs) {
		const vector <string> &varnames = this->GetVariableNames();

		// Only variable name tags permitted.
		//
		for(int i = 0; i<(int)varnames.size(); i++) {
			if ((StrCmpNoCase(tag, varnames[i]) == 0)) {
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
	if (StrCmpNoCase(tag, _userTimeStampTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
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


void	MetadataVDC::_startElementHandler3(ExpatParseMgr* pm,
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

void	MetadataVDC::_endElementHandler0(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	MetadataVDC::_endElementHandler1(ExpatParseMgr* pm,
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
		istringstream ist(pm->getStringData());
		string v;
		vector <string> vec;

		while (ist >> v) {
			vec.push_back(v);
		}
		if (SetVariables3D(vec) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars2DXYTag) == 0) {
		istringstream ist(pm->getStringData());
		string v;
		vector <string> vec;

		while (ist >> v) {
			vec.push_back(v);
		}
		if (SetVariables2DXY(vec) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars2DXZTag) == 0) {
		istringstream ist(pm->getStringData());
		string v;
		vector <string> vec;

		while (ist >> v) {
			vec.push_back(v);
		}
		if (SetVariables2DXZ(vec) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _vars2DYZTag) == 0) {
		istringstream ist(pm->getStringData());
		string v;
		vector <string> vec;

		while (ist >> v) {
			vec.push_back(v);
		}
		if (SetVariables2DYZ(vec) < 0) {
			string s(GetErrMsg()); pm->parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _periodicBoundaryTag) == 0) {
		if (SetPeriodicBoundary(pm->getLongData()) < 0) {
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

void	MetadataVDC::_endElementHandler2(ExpatParseMgr* pm,
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


void	MetadataVDC::_endElementHandler3(ExpatParseMgr* pm,
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
