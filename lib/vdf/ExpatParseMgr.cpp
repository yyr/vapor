//************************************************************************
//																		*
//		     Copyright (C)  2005										*
//     University Corporation for Atmospheric Research					*
//		     All Rights Reserved										*
//																		*
//************************************************************************/
//
//	File:		ExpatParseMgr.cpp
//
//	Author:		John Clyne, with modifications by Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		May 10, 2005
//
//	Description:	Implements the ExpatParseMgr class.  This class centralizes various
//  parsing capabilities used by Metadata, ImpExp, Session, and TransferFunction classes.
//	Classes that are to be parsed must be derived from the ParsedXML class.
//  That (pure virtual) class requires that the subclasses implement methods
//  "elementStartHandler" and "elementEndHandler" that are called at the start and end
//  of an XML tag parsing.  These return false on failure.
//  The actual parsing begins when parse(ifstream&) is called.
//	If user code identifies an error, it can call parseError() on "this", 
//  which will terminate parsing and post an error message
//
//	One way to use this class is to reconstruct multiple classes from one parsed file.
//  To do this, the elementStartHandler method must reset the parsedXML callbacks when a new class
//  is discovered during parsing.  When the elementEndHandler is reached, the callbacks
//  must be "popped" back to previous settings


#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdarg>
#include <cstring>
#include <expat.h>
#include <cassert>
#include <algorithm>
#include "vapor/ExpatParseMgr.h"
#include "vapor/XmlNode.h"


using namespace VAPoR;
using namespace VetsUtil;

const string ExpatParseMgr::_stringType = "String";
const string ExpatParseMgr::_longType = "Long";
const string ExpatParseMgr::_doubleType = "Double";
const string ParsedXml::_stringType = "String";
const string ParsedXml::_longType = "Long";
const string ParsedXml::_doubleType = "Double";
const string ParsedXml::_typeAttr = "Type";

ExpatParseMgr::ExpatParseMgr(ParsedXml* pc){
	SetDiagMsg("ExpatParseMgr::ExpatParseMgr()");
	currentParsedClass = pc;
	// Create an Expat XML parser to parse the XML formatted metadata file
	// specified by 'path'
	//
	_expatParser = XML_ParserCreate(NULL);
	XML_SetElementHandler(
		_expatParser, _StartElementHandler, _EndElementHandler
	);
	XML_SetCharacterDataHandler(_expatParser, _CharDataHandler);

	XML_SetUserData(_expatParser, (void *) this);
}


void ExpatParseMgr::parse(ifstream &is) {
	
	SetDiagMsg("ExpatParseMgr::parse()");

	char line[1024];

	if (!is) {
		SetErrMsg("ExpatParseMgr: Invalid input stream");
		return;
	}

	

	// Parse the file until we run out of elements or a parsing error occurs
	//
    while(is.good()) {
		int	rc;

        is.read(line, sizeof(line)-1);
        if ((rc = is.gcount()) > 0) {
			if (XML_Parse(_expatParser, line, rc, 0) == XML_STATUS_ERROR) {
				SetErrMsg(
					"Error parsing xml file at line %d : %s",
					XML_GetCurrentLineNumber(_expatParser),
					XML_ErrorString(XML_GetErrorCode(_expatParser))
				);
				XML_ParserFree(_expatParser);
				return;
			}
        }
    }
	XML_ParserFree(_expatParser);

    if (is.bad()) {
		SetErrMsg("Error reading XML file");
        return;
    }
    is.close();

	int level = (int) _expatStateStack.size() - 1;  // XML tree depth
	if (level != -1) {
		SetErrMsg("Error reading XML file");
        return;
    }

}

ExpatParseMgr::~ExpatParseMgr() {
	SetDiagMsg("ExpatParseMgr::~ExpatParseMgr()");
}

void	ExpatParseMgr::_startElementHandler(
	const XML_Char *tag, const char **attrs
) {
	string tagstr(tag);
	int level = (int) _expatStateStack.size(); // xml tree depth

	_expatStringData.clear();	// XML char data gets stored here

	// Create a new state element associated with the current 
	// tree level
	//
	ExpatStackElement *state = new ExpatStackElement();
	state->tag = tagstr;
	state->has_data = 0;
	state->user_defined = 0;
	_expatStateStack.push(state);
	
	// Invoke the user-supplied element handler depending on 
	// the XML tree depth
	//
	if (currentParsedClass){
		bool ok = currentParsedClass->elementStartHandler(this, level, tagstr, attrs);
		if (!ok) parseError("Unable to parse start tag %s", tagstr.c_str());
	}

}

void	ExpatParseMgr::_endElementHandler(const  XML_Char *tag)
{
	string tagstr(tag);

	ExpatStackElement *state = _expatStateStack.top();

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
	
	if (currentParsedClass){
		bool ok = currentParsedClass->elementEndHandler(this, level , tagstr);
		if (!ok) parseError("Unable to parse end tag %s", tagstr.c_str());
	}

	_expatStateStack.pop();
	delete state;

}

void	ExpatParseMgr::_charDataHandler(const XML_Char *s, int len) {

	ExpatStackElement *state = _expatStateStack.top();;

	// Make sure this element is allowed to contain data. Note even
	// elements with no real data seem to have a newline, triggering
	// the invocation of this handler.  
	//
	if (state->has_data == 0 && len > 1) {
		parseError(
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

	if (currentParsedClass){
		bool ok = currentParsedClass->charHandler(this, s, len);
		if (!ok) parseError("Unable to parse class-specific character data for tag %s", state->tag.c_str());
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
void	ExpatParseMgr::parseError(
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
			"XML parsing terminated at line %d : %s", 
			XML_GetCurrentLineNumber(_expatParser), msg
		);
	}
	else {
		SetErrMsg(
			"XML parsing terminated at line %d (%s) : %s", 
			XML_GetCurrentLineNumber(_expatParser), 
			XML_ErrorString(XML_GetErrorCode(_expatParser)), msg
		);
	}

}
