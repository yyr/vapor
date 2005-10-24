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
const string Metadata::_xCoordsTag = "XCoords";
const string Metadata::_yCoordsTag = "YCoords";
const string Metadata::_zCoordsTag = "ZCoords";

const string Metadata::_blockSizeAttr = "BlockSize";
const string Metadata::_dimensionLengthAttr = "DimensionLength";
const string Metadata::_numTransformsAttr = "NumTransforms";
const string Metadata::_filterCoefficientsAttr = "FilterCoefficients";
const string Metadata::_liftingCoefficientsAttr = "LiftingCoefficients";
const string Metadata::_msbFirstAttr = "MSBFirst";
const string Metadata::_vdfVersionAttr = "VDFVersion";
const string Metadata::_numChildrenAttr = "NumChildren";


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
	

	// Set some default metadata attributes. 
	//
	string gridTypeStr = "regular";
	_rootnode->SetElementString(_gridTypeTag, gridTypeStr);

	string coordSystemType = "cartesian";
	_rootnode->SetElementString(_coordSystemTypeTag, coordSystemType);

	double maxdim = max(dim[0], max(dim[1],dim[2]));
	double extents[] = {
		0.0, 0.0, 0.0, 
		(double) dim[0]/maxdim, (double) dim[1]/maxdim, (double) dim[2]/maxdim
	};

	vector<double> extentsVec(extents, &extents[sizeof(extents)/sizeof(extents[0])]);
	SetExtents(extentsVec);

	if (SetNumTimeSteps(1) < 0) return(-1);

	vector<string> varNamesVec(1,"var1");
	if (SetVariableNames(varNamesVec) < 0) return(-1);

	string comment = "";
	_rootnode->SetElementString(_commentTag, comment);

	return(0);
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
	_metafileDirName = NULL;
	_metafileName = NULL;

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

	_metafileDirName = new char[path.length()+1];
	_metafileName = new char[path.length()+1];

	// Get directory path of metafile
	_metafileDirName = Dirname(path.c_str(), _metafileDirName);
	strcpy(_metafileName, Basename(path.c_str()));

	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	parseMgr->parse(is);
	delete parseMgr;
	
	_objInitialized = 1;
}

Metadata::~Metadata() {
	SetDiagMsg("Metadata::~Metadata()");
	if (! _objInitialized) return;


	if (_rootnode) delete _rootnode;
    if (_metafileDirName) delete [] _metafileDirName;
    if (_metafileName) delete [] _metafileName;

	_objInitialized = 0;
}


int Metadata::Write(const string &path) const {

	SetDiagMsg("Metadata::Write(%s)", path.c_str());

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

	SetDiagMsg("Metadata::SetVariableNames([%s,...])", value[0].c_str());

	ostringstream oss;

	//	Encode variable name vector as a white-space delimited string
	//
	_varNames.clear();
	for(size_t i=0; i<value.size(); i++) {
		oss << value[i] << " ";
		_varNames.push_back(value[i]);
	}


	// For each time step we need to create a list of variable nodes
	//
	for(size_t i = 0; i<numTS; i++) {
		if (_SetVariableNames(_rootnode->GetChild(i), (long)i) < 0) return(-1);
	}

	_rootnode->SetElementString(_varNamesTag, oss.str());

	return(0);
}

int Metadata::SetTSUserTime(size_t ts, const vector<double> &value) {


	if (! IsValidUserTime(value)) {
		SetErrMsg("Invalid user time specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSUserTime(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_userTimeTag, value);
	return(0);
}

int Metadata::SetTSXCoords(size_t ts, const vector<double> &value) {
	if (! IsValidXCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSXCoords(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_xCoordsTag, value);
	return(0);
}

int Metadata::SetTSYCoords(size_t ts, const vector<double> &value) {
	if (! IsValidYCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSYCoords(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_yCoordsTag, value);
	return(0);
}

int Metadata::SetTSZCoords(size_t ts, const vector<double> &value) {
	if (! IsValidZCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSZCoords(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	_rootnode->GetChild(ts)->SetElementDouble(_zCoordsTag, value);
	return(0);
}

int Metadata::SetTSComment(
	size_t ts, const string &value
) {
	XmlNode	*timenode;

	SetDiagMsg("Metadata::SetTSComment(%d, %s)", ts, value.c_str());

	CHK_TS(ts, -1);
	if (! (timenode = _rootnode->GetChild(ts))) return(-1);

	timenode->SetElementString(_commentTag, value);
	return(0);
}

const string &Metadata::GetTSComment(
	size_t ts
) const {

	XmlNode	*timenode;

	SetDiagMsg("Metadata::GetTSComment(%d)", ts);

	CHK_TS(ts, _emptyString);
	if (! (timenode = _rootnode->GetChild(ts))) return(_emptyString);

	return(timenode->GetElementString(_commentTag));
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

	CHK_VAR(ts, var, -1);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	varnode->SetElementString(_commentTag, value);
	return(0);
}

const string &Metadata::GetVComment(
	size_t ts, const string &var
) const {

	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg("Metadata::GetVComment(%d, %s)", ts, var.c_str());

	CHK_VAR(ts, var, _emptyString);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	return(varnode->GetElementString(_commentTag));
}

const string &Metadata::GetVBasePath(
	size_t ts, const string &var
) const {

	XmlNode	*timenode;
	XmlNode	*varnode;

	SetDiagMsg("Metadata::GetVBasePath(%d, %s)", ts, var.c_str());

	CHK_VAR(ts, var, _emptyString);
	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);

	return(varnode->GetElementString(_basePathTag));
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
	CHK_VAR(ts, var, -1);

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
	int vdfVersion = VDF_VERSION;
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
	else {
		// must be user data
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
	else if (StrCmpNoCase(tag, _xCoordsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _yCoordsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _zCoordsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user data
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

	if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	} else if (StrCmpNoCase(tag, _basePathTag) == 0) {
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
		// must be user data
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
	else if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (SetComment(pm->getStringData()) < 0) {
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
	else {
		int	rc;
		// must be user data

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
		// must be user data

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
		// must be user data

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
