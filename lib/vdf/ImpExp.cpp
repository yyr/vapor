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
//	File:		ImpExp.cpp
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Mar  3 14:33:57 MST 2005
//
//	Description:	Implements the ImpExp class
//


#include <sys/types.h>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include <vapor/XmlNode.h>
#include <vapor/ImpExp.h>

using namespace VAPoR;
using namespace VetsUtil;

// Static member initialization
//
const string ImpExp::_rootTag = "VDFImpExp";
const string ImpExp::_pathNameTag = "VDFFilePath";
const string ImpExp::_timeStepTag = "TimeStep";
const string ImpExp::_varNameTag = "VariableName";
const string ImpExp::_regionTag = "Region";
const string ImpExp::_timeSegmentTag = "TimeSegment";

const string ImpExp::_typeAttr = "Type";

const string ImpExp::_stringType = "String";
const string ImpExp::_longType = "Long";
const string ImpExp::_doubleType = "Double";


ImpExp::ImpExp() {
	SetDiagMsg("ImpExp::ImpExp()");

	SetClassName("ImpExp");

	_rootnode = NULL;
}

ImpExp::~ImpExp() {
	SetDiagMsg("ImpExp::~ImpExp()");

	_rootnode = NULL;
}

int ImpExp::Export(
    const string &path, size_t ts, const string &varname,
    const size_t min[3], const size_t max[3], const size_t timeseg[2]
) {
	SetDiagMsg(
		"ImpExp::Export(%s, %d, %s, [%d,%d,%d], [%d,%d,%d])",
		path.c_str(), ts, varname.c_str(),
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	map <const string, string> attrs;
	attrs.clear();

	_rootnode = new XmlNode(_rootTag, attrs);

	_rootnode->SetElementString(_pathNameTag, path);

	vector <long> tsvec(1, ts);
	_rootnode->SetElementLong(_timeStepTag, tsvec);

	_rootnode->SetElementString(_varNameTag, varname);

	vector <long> regionvec(6);
	for(int i=0; i<3; i++) {
		regionvec[i] = min[i];
		regionvec[i+3] = max[i];
	}
	_rootnode->SetElementLong(_regionTag, regionvec);

	vector <long> tsegvec(2);
	tsegvec[0] = timeseg[0];
	tsegvec[1] = timeseg[1];
	_rootnode->SetElementLong(_timeSegmentTag, tsegvec);


    ofstream fileout;
	string xmlpath = _getpath();
    fileout.open(xmlpath.c_str());
    if (! fileout) {
        SetErrMsg("Can't open file \"%s\" for writing", xmlpath.c_str());
        return(-1);
    }
    fileout << "<?xml version=\"1.0\" encoding=\"ISO-8859-1\" standalone=\"yes\"?>" << endl;
    fileout << *_rootnode;

	delete _rootnode;

	return(0);
}


int ImpExp::Import(
    string *path, size_t *ts, string *varname,
    size_t min[3], size_t max[3], size_t timeseg[2]
) {
	SetDiagMsg("ImpExp::Import()");

	ifstream is;
	char line[1024];

	string xmlpath = _getpath();
	is.open(xmlpath.c_str());
	if (! is) {
		SetErrMsg("Can't open file \"%s\" for reading", xmlpath.c_str());
		return(-1);
	}

	map <const string, string> attrs;
	attrs.clear();

	_rootnode = new XmlNode(_rootTag, attrs);

	
	// Create an Expat XML parser to parse the XML formatted metadata file
	// specified by 'path'
	//
	_expatParser = XML_ParserCreate(NULL);
	XML_SetElementHandler(
		_expatParser, impExpStartElementHandler, impExpEndElementHandler
	);
	XML_SetCharacterDataHandler(_expatParser, impExpCharDataHandler);

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
					xmlpath.c_str(), XML_GetCurrentLineNumber(_expatParser),
					XML_ErrorString(XML_GetErrorCode(_expatParser))
				);
				return(-1);
			}
        }
    }

    if (is.bad()) {
		SetErrMsg("Error reading file \"%s\"", xmlpath.c_str());
		return(-1);
    }
    is.close();

	XML_ParserFree(_expatParser);

    if (GetErrCode()) {
        return(-1);
    }


	*path = _rootnode->GetElementString(_pathNameTag);
	*ts = _rootnode->GetElementLong(_timeStepTag)[0];
	*varname = _rootnode->GetElementString(_varNameTag);
	const vector<long> regionvec = _rootnode->GetElementLong(_regionTag);
	for(int i = 0; i<3; i++) {
		min[i] = regionvec[i];
		max[i] = regionvec[i+3];
	}

	const vector<long> timesegvec = _rootnode->GetElementLong(_timeSegmentTag);
	timeseg[0] = timesegvec[0];
	timeseg[1] = timesegvec[1];
	
	delete _rootnode;

	return(0);
}



void	ImpExp::_startElementHandler(
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
	_expatStateStack.push(state);
	
	// Invoke the appropriate start element handler depending on 
	// the XML tree depth
	//
	switch  (level) {
	case 0:
		ImpExp::_startElementHandler0(tagstr, attrs);
	break;
	case 1:
		ImpExp::_startElementHandler1(tagstr, attrs);
	break;
	default:
		_parseError("Invalid tag : %s", tagstr.c_str());
	break;
	}

}

void	ImpExp::_endElementHandler(const  XML_Char *tag)
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
		ImpExp::_endElementHandler0(tagstr);
		break;

	case 1:
		ImpExp::_endElementHandler1(tagstr);
		break;

	default:
		_parseError("Invalid state");
	break;
	}

	_expatStateStack.pop();
	delete state;

}

void	ImpExp::_charDataHandler(const XML_Char *s, int len) {

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
void	ImpExp::_startElementHandler0(
	const string &tag, const char **attrs
) {

	_expatStackElement *state = _expatStateStack.top();
	state->has_data = 0;


	// Verify valid level 0 element
	//
	if (StrCmpNoCase(tag, _rootTag) != 0) {
		_parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

}

void	ImpExp::_startElementHandler1(
	const string &tag, const char **attrs
) {
	_expatStackElement *state = _expatStateStack.top();;
	state->has_data = 0;

	string type;

	// 
	//
	if (! *attrs) {
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
	if (StrCmpNoCase(tag, _pathNameTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _timeStepTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _varNameTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _regionTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _timeSegmentTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			_parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		_parseError("Invalid tag : %s", tag.c_str());
		return;
	}
}


void	ImpExp::_endElementHandler0(
	const string &tag
) {
	_expatStackElement *state = _expatStateStack.top();;

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		_parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	ImpExp::_endElementHandler1(
	const string &tag
) {
	_expatStackElement *state = _expatStateStack.top();
	int	rc;


	if (StrCmpNoCase(state->data_type, _stringType) == 0) {
		rc = _rootnode->SetElementString(tag, _expatStringData);
	}
	else if (StrCmpNoCase(state->data_type, _longType) == 0) {
		rc = _rootnode->SetElementLong(tag, _expatLongData);
	}
	else {
		rc = _rootnode->SetElementDouble(tag, _expatDoubleData);
	}
	if (rc < 0) {
		string s(GetErrMsg()); _parseError("%s", s.c_str());
		return;
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
void	ImpExp::_parseError(
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
string ImpExp::_getpath() {

	char	buf[128];
	uid_t	uid = getuid();

	sprintf (buf, "/tmp/vapor.%6.6d.xml", uid);
	string path(buf);
	return(path);
}
