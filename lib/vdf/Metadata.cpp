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
const string Metadata::_numChildrenAttr = "NumChildren";
const string Metadata::_typeAttr = "Type";

const string Metadata::_stringType = "String";
const string Metadata::_longType = "Long";
const string Metadata::_doubleType = "Double";


// Initialize the class object
//
int Metadata::_init(
		const size_t dim[3], size_t numTransforms, size_t bs,
		int nFilterCoef, int nLiftingCoef, int msbFirst
) {
	map <const string, string> attrs;
	ostringstream oss;
	string empty;

	SetClassName("Metadata");

	if (! IsPowerOfTwo((int)bs)) {
		SetErrMsg("Block dimension is not a power of two: bs=%d", bs);
		return(-1);
	}

	_emptyDoubleVec.clear();
	_emptyLongVec.clear();
	_emptyString.clear();

	_bs = bs;
	_dim[0] = dim[0]; _dim[1] = dim[1]; _dim[2] = dim[2];
	_numTransforms = (int)numTransforms;
	_nFilterCoef = nFilterCoef;
	_nLiftingCoef = nLiftingCoef;
	_msbFirst = msbFirst;

	_varNames.clear();

	oss.str(empty);
	oss << (unsigned int)_bs;
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
		const size_t dim[3], size_t numTransforms, size_t bs, 
		int nFilterCoef, int nLiftingCoef, int msbFirst
) {
	_objInitialized = 0;
	SetDiagMsg(
		"Metadata::Metadata([%d,%d,%d], %d, %d, %d, %d, %d)", 
		dim[0], dim[1], dim[2], numTransforms, bs, nFilterCoef, nLiftingCoef,
		msbFirst
	);

	_rootnode = NULL;
	_metafileDirName = NULL;
	_metafileName = NULL;

	if (_init(dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst) < 0){
		return;
	}
	_objInitialized = 1;
}


Metadata::Metadata(const string &path) {
	_objInitialized = 0;

	SetDiagMsg("Metadata::Metadata(%s)", path.c_str());

	ifstream is;
	char line[1024];

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

	// Create an Expat XML parser to parse the XML formatted metadata file
	// specified by 'path'
	//
	_expatParser = XML_ParserCreate(NULL);
	XML_SetElementHandler(
		_expatParser, metadataStartElementHandler, metadataEndElementHandler
	);
	XML_SetCharacterDataHandler(_expatParser, metadataCharDataHandler);

	XML_SetUserData(_expatParser, (void *) this);

	// Parse the file until we run out of elements or a parsing error occurs
	//
    while(is.good()) {
		int	rc;

        is.read(line, sizeof(line)-1);
        if ((rc=is.gcount()) > 0) {
			if (XML_Parse(_expatParser, line, rc, 0) == XML_STATUS_ERROR) {
				SetErrMsg(
					"Error parsing xml file \"%s\" at line %d : %s",
					path.c_str(), XML_GetCurrentLineNumber(_expatParser),
					XML_ErrorString(XML_GetErrorCode(_expatParser))
				);
				XML_ParserFree(_expatParser);
				return;
			}
        }
    }
	XML_ParserFree(_expatParser);

    if (is.bad()) {
		SetErrMsg("Error reading file \"%s\"", path.c_str());
        return;
    }
    is.close();

	int level = (int) _expatStateStack.size() - 1;  // XML tree depth
	if (level != -1) {
		SetErrMsg("Error reading file \"%s\"", path.c_str());
        return;
    }

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

	return(_rootnode->SetElementString(_gridTypeTag, value));
}

int Metadata::SetCoordSystemType(const string &value) {


	if (!IsValidCoordSystemType(value)) {
		SetErrMsg("Invalid CoordinateSystem specification : \"%s\"", value.c_str());
		return(-1);
	}

	SetDiagMsg("Metadata::SetCoordSystemType(%s)", value.c_str());

	return(_rootnode->SetElementString(_coordSystemTypeTag, value));
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

	return(_rootnode->SetElementDouble(_extentsTag, value));
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
	map <const string, string> attrs;

	// Add children
	//
	if (newN > oldN) {
		ostringstream oss;
		string empty;
		empty.clear();

		for (size_t i = oldN; i < newN; i++) {
			XmlNode *child;
			child = _rootnode->NewChild(_timeStepTag, attrs, _varNames.size());
			_SetVariableNames(child, (long)i);
		}
	}
	// Delete children
	//
	else {
		for (size_t i = newN; i< oldN; i++) {
			_rootnode->DeleteChild(newN);
		}
	}
	return(0);
}

int Metadata::_SetVariableNames(XmlNode *node, long ts) {

	map <const string, string> attrs; // empty map
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

	return(_rootnode->SetElementLong(_numTimeStepsTag, valvec)); 
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

	return(_rootnode->SetElementString(_varNamesTag, oss.str()));

	return(0);
}

int Metadata::SetTSUserTime(size_t ts, const vector<double> &value) {


	if (! IsValidUserTime(value)) {
		SetErrMsg("Invalid user time specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSUserTime(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	return(_rootnode->GetChild(ts)->SetElementDouble(_userTimeTag, value));
}

int Metadata::SetTSXCoords(size_t ts, const vector<double> &value) {
	if (! IsValidXCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSXCoords(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	return(_rootnode->GetChild(ts)->SetElementDouble(_xCoordsTag, value));
}

int Metadata::SetTSYCoords(size_t ts, const vector<double> &value) {
	if (! IsValidYCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSYCoords(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	return(_rootnode->GetChild(ts)->SetElementDouble(_yCoordsTag, value));
}

int Metadata::SetTSZCoords(size_t ts, const vector<double> &value) {
	if (! IsValidZCoords(value)) {
		SetErrMsg("Invalid coordinate array specification");
		return(-1);
	}

	SetDiagMsg("Metadata::SetTSZCoords(%d, [%d,...])", ts, value[0]);

	CHK_TS(ts, -1);
	return(_rootnode->GetChild(ts)->SetElementDouble(_zCoordsTag, value));
}

int Metadata::SetTSComment(
	size_t ts, const string &value
) {
	XmlNode	*timenode;

	SetDiagMsg("Metadata::SetTSComment(%d, %s)", ts, value.c_str());

	CHK_TS(ts, -1);
	if (! (timenode = _rootnode->GetChild(ts))) return(-1);

	return(timenode->SetElementString(_commentTag, value));
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

	return(varnode->SetElementString(_commentTag, value));
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
	XmlNode	*timenode;
	XmlNode	*varnode;

    if (! IsValidVDataRange(value)) {
        SetErrMsg("Invalid data range specification");
        return(-1);
    }

	SetDiagMsg(
		"Metadata::SetVDataRange(%d, %s, [%f, ...])", ts, var.c_str(), value[0]
	);


	timenode = _rootnode->GetChild(ts);
	varnode = timenode->GetChild(var);
	CHK_VAR(ts, var, -1);

	return(varnode->SetElementDouble(_dataRangeTag, value));
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

void	Metadata::_startElementHandler(
	const XML_Char *tag, const char **attrs
) {
	string tagstr(tag);
	int level = (int) _expatStateStack.size(); // xml tree depth

	_expatStringData.clear();	// XML char data gets stored here

	// Create a new state element associated with the current 
	// tree level
	//
	_expatStackElement *state = new _expatStackElement();
	state->tag = tagstr;
	state->has_data = 0;
	state->user_defined = 0;
	_expatStateStack.push(state);
	
	// Invoke the appropriate start element handler depending on 
	// the XML tree depth
	//
	switch  (level) {
	case 0:
		Metadata::_startElementHandler0(tagstr, attrs);
	break;
	case 1:
		Metadata::_startElementHandler1(tagstr, attrs);
	break;
	case 2:
		Metadata::_startElementHandler2(tagstr, attrs);
	break;
	case 3:
		Metadata::_startElementHandler3(tagstr, attrs);
	break;
	default:
		_parseError("Invalid tag : %s", tagstr.c_str());
	break;
	}

}

void	Metadata::_endElementHandler(const  XML_Char *tag)
{
	string tagstr(tag);

	_expatStackElement *state = _expatStateStack.top();

	int level = (int) _expatStateStack.size() - 1; 	// XML tree depth

	// remove leading and trailing white space from the XML char data
	//
	StrRmWhiteSpace(_expatStringData);

	// Parse the element data, storing results in either the 
	// _expatStringData, _expatDoubleData, or _expatLongData vectors
	// as appropriate.
	//
	_expatLongData.clear();
	_expatDoubleData.clear();
	if (state->has_data) {
		if (StrCmpNoCase(state->data_type, _doubleType) == 0) {
			istringstream ist(_expatStringData);
			double v;

			while (ist >> v) {
				_expatDoubleData.push_back(v);
			}
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			istringstream ist(_expatStringData);
			long v;

			while (ist >> v) {
				_expatLongData.push_back(v);
			}
		}
		else {
			// do nothing - data already stored in appropriate form
		}
	}

	// Invoke the appropriate end element handler for an element at
	// XML tree depth, 'level'
	//
	switch  (level) {
	case 0:
		Metadata::_endElementHandler0(tagstr);
	break;
	case 1:
		Metadata::_endElementHandler1(tagstr);
	break;
	case 2:
		Metadata::_endElementHandler2(tagstr);
	break;
	case 3:
		Metadata::_endElementHandler3(tagstr);
	break;
	default:
		_parseError("Invalid state");
	break;
	}

	_expatStateStack.pop();
	delete state;

}

void	Metadata::_charDataHandler(const XML_Char *s, int len) {

	_expatStackElement *state = _expatStateStack.top();;

	// Make sure this element is allowed to contain data. Note even
	// elements with no real data seem to have a newline, triggering
	// the invocation of this handler.  
	//
	if (state->has_data == 0 && len > 1) {
		_parseError(
			"Element \"%s\" not permitted to have data", state->tag.c_str()
		);
		return;
	}

	// Append the char data for this element to the buffer. Note, XML 
	// character for a single element may be split into multiple pieces,
	// triggering this callback more than one time for a given element. Hence
	// we *append* the data.
	//
	_expatStringData.append(s, len);
}

// Level 0 start element handler. The only element tag permitted at this
// level is the '_rootTag' tag
//
void	Metadata::_startElementHandler0(
	const string &tag, const char **attrs
) {

	_expatStackElement *state = _expatStateStack.top();
	state->has_data = 0;
	state->user_defined = 0;

	_expatCurrentTS = 0;

	size_t bs;
	size_t dim[3];
	int nFilterCoef = 1;
	int nLiftingCoef = 1;
	int msbFirst = 1;
	int numTransforms = 0;


	// Verify valid level 0 element
	//
	if (StrCmpNoCase(tag, _rootTag) != 0) {
		_parseError("Invalid tag : \"%s\"", tag.c_str());
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
			ist >> bs;
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
		else {
			_parseError("Invalid tag attribute : \"%s\"", attr.c_str());
		}
	}

	_init(dim, numTransforms, bs, nFilterCoef, nLiftingCoef, msbFirst);
	if (GetErrCode()) {
		string s(GetErrMsg()); _parseError("%s", s.c_str());
		return;
	}


}

void	Metadata::_startElementHandler1(
	const string &tag, const char **attrs
) {
	_expatStackElement *state = _expatStateStack.top();;
	state->has_data = 0;
	state->user_defined = 0;

	string type;

	// Either have a TimeStep element (with no attributes) or a data element
	//
	if (! *attrs) {
		if (StrCmpNoCase(tag, _timeStepTag) == 0) {
			if (*attrs) {
				_parseError("Unexpected element attribute : %s", *attrs);
				return;
			}
		}
		else {
			_parseError("Invalid tag : \"%s\"", tag.c_str());
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
		_parseError("Too many attributes");
		return;
	}
	istringstream ist(value);


	state->has_data = 1;
	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		_parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;
	if (StrCmpNoCase(tag, _numTimeStepsTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _extentsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _coordSystemTypeTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _gridTypeTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _varNamesTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user data
		if (!((StrCmpNoCase(type, _stringType) != 0) ||
			(StrCmpNoCase(type, _longType) != 0) || 
			(StrCmpNoCase(type, _doubleType) != 0))) {

			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
		state->user_defined = 1;
	}
}

void	Metadata::_startElementHandler2(
	const string &tag, const char **attrs
) {
	_expatStackElement *state = _expatStateStack.top();;
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
				_expatCurrentVar = tag;
				return;
			}
		}
		_parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

	// Must be a data element. Make sure it has a type attribute.
	//


	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		_parseError("Too many attributes");
		return;
	}
	istringstream ist(value);



	state->has_data = 1;

	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		_parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;

	if (StrCmpNoCase(tag, _userTimeTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _xCoordsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _yCoordsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _zCoordsTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user data
		if (!((StrCmpNoCase(type, _stringType) != 0) ||
			(StrCmpNoCase(type, _longType) != 0) || 
			(StrCmpNoCase(type, _doubleType) != 0))) {

			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
		state->user_defined = 1;
	}
}


void	Metadata::_startElementHandler3(
	const string &tag, const char **attrs
) {
	_expatStackElement *state = _expatStateStack.top();;
	state->has_data = 0;
	state->user_defined = 0;

	string type;

	if (! *attrs) {
		_parseError("Expected element attribute");
		return;
	}

	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		_parseError("Too many attributes");
		return;
	}
	istringstream ist(value);


	// Must be a data element. Make sure it has a type attribute.
	//

	state->has_data = 1;

	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		_parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;

	if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	} else if (StrCmpNoCase(tag, _basePathTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	} else if (StrCmpNoCase(tag, _dataRangeTag) == 0) {
		if (StrCmpNoCase(type, _doubleType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		// must be user data
		if (!((StrCmpNoCase(type, _stringType) != 0) ||
			(StrCmpNoCase(type, _longType) != 0) || 
			(StrCmpNoCase(type, _doubleType) != 0))) {

			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
		state->user_defined = 1;
	}
}

void	Metadata::_endElementHandler0(
	const string &tag
) {
	_expatStackElement *state = _expatStateStack.top();;

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		_parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	Metadata::_endElementHandler1(
	const string &tag
) {
	_expatStackElement *state = _expatStateStack.top();

	if (! state->has_data) {
		if (StrCmpNoCase(tag, _timeStepTag) == 0) {
			_expatCurrentTS++;
		} 
	}
	else if (StrCmpNoCase(tag, _numTimeStepsTag) == 0) {
		if (SetNumTimeSteps(_expatLongData[0]) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _extentsTag) == 0) {
		if (SetExtents(_expatDoubleData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (SetComment(_expatStringData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _coordSystemTypeTag) == 0) {
		if (SetCoordSystemType(_expatStringData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _gridTypeTag) == 0) {
		if (SetGridType(_expatStringData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _varNamesTag) == 0) {
		istringstream ist(_expatStringData);
		string v;
		vector <string> vec;

		while (ist >> v) {
			vec.push_back(v);
		}
		if (SetVariableNames(vec) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else {
		int	rc;
		// must be user data

		if (StrCmpNoCase(state->data_type, _stringType) == 0) {
			rc = SetUserDataString(tag, _expatStringData);
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			rc = SetUserDataLong(tag, _expatLongData);
		}
		else {
			rc = SetUserDataDouble(tag, _expatDoubleData);
		}
		if (rc < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
}

void	Metadata::_endElementHandler2(
	const string &tag
) {
	_expatStackElement *state = _expatStateStack.top();

	if (! state->has_data) {
		// do nothing

	} else if (StrCmpNoCase(tag, _userTimeTag) == 0) {
		if (SetTSUserTime(_expatCurrentTS, _expatDoubleData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (SetTSComment(_expatCurrentTS, _expatStringData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _xCoordsTag) == 0) {
		if (SetTSXCoords(_expatCurrentTS, _expatDoubleData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _yCoordsTag) == 0) {
		if (SetTSYCoords(_expatCurrentTS, _expatDoubleData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _zCoordsTag) == 0) {
		if (SetTSZCoords(_expatCurrentTS, _expatDoubleData) < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else {
		int	rc;
		// must be user data

		if (StrCmpNoCase(state->data_type, _stringType) == 0) {
			rc = SetTSUserDataString(_expatCurrentTS, tag, _expatStringData);
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			rc = SetTSUserDataLong(_expatCurrentTS, tag, _expatLongData);
		}
		else {
			rc = SetTSUserDataDouble(_expatCurrentTS, tag, _expatDoubleData);
		}
		if (rc < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
}


void	Metadata::_endElementHandler3(
	const string &tag
) {
	_expatStackElement *state = _expatStateStack.top();

	if (StrCmpNoCase(tag, _commentTag) == 0) {
		if (SetVComment(_expatCurrentTS, _expatCurrentVar, _expatStringData)<0){
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _basePathTag) == 0) {
		if (SetVUserDataString(
			_expatCurrentTS, _expatCurrentVar, tag,  _expatStringData) < 0) {

			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _dataRangeTag) == 0) {
		if (SetVDataRange(
			_expatCurrentTS, _expatCurrentVar, _expatDoubleData) < 0) {

			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
	else {
		int	rc;
		// must be user data

		if (StrCmpNoCase(state->data_type, _stringType) == 0) {
			rc = SetVUserDataString(
				_expatCurrentTS, _expatCurrentVar, tag, _expatStringData
			);
		}
		else if (StrCmpNoCase(state->data_type, _longType) == 0) {
			rc = SetVUserDataLong(
				_expatCurrentTS, _expatCurrentVar, tag, _expatLongData
			);
		}
		else {
			rc = SetVUserDataDouble(
				_expatCurrentTS, _expatCurrentVar, tag, _expatDoubleData
			);
		}
		if (rc < 0) {
			string s(GetErrMsg()); _parseError("%s", s.c_str());
			return;
		}
	}
}

namespace {

void	startNoOp(
	void *userData, const XML_Char *tag, const char **attrs
) {
}

void endNoOp(void *userData, const XML_Char *tag) {
}

void	charDataNoOp(
	void *userData, const XML_Char *s, int len
) {
}

};

// Record an XML file parsing error and terminate file parsing
//
void	Metadata::_parseError(
	const char *format, 
	...
) {
	va_list args;
	int	done = 0;
	const int alloc_size = 256;
	int rc;
	char	*msg = NULL;
	size_t	msg_size = 0;

	// Register NoOp handlers with XML parser or we'll continue to 
	// parse the xml file
	//
	XML_SetElementHandler(_expatParser, startNoOp, endNoOp);
	XML_SetCharacterDataHandler(_expatParser, charDataNoOp);

	// Loop until we've successfully buffered the error message, growing
	// the message buffer as needed
	//
	while (! done) {
		va_start(args, format);
#ifdef WIN32
		rc = _vsnprintf(msg, msg_size, format, args);
#else
		rc = vsnprintf(msg, msg_size, format, args);
#endif
		va_end(args);

		if (rc < (int)(msg_size-1)) {
			done = 1;
		} else {
			if (msg) delete [] msg;
			msg = new char[msg_size + alloc_size];
			assert(msg != NULL);
			msg_size += alloc_size;
		}
	}

	
	if (XML_GetErrorCode(_expatParser) == XML_ERROR_NONE) {
		SetErrMsg(
			"Metafile parsing terminated at line %d : %s", 
			XML_GetCurrentLineNumber(_expatParser), msg
		);
	}
	else {
		SetErrMsg(
			"Metafile parsing terminated at line %d (%s) : %s", 
			XML_GetCurrentLineNumber(_expatParser), 
			XML_ErrorString(XML_GetErrorCode(_expatParser)), msg
		);
	}

}
