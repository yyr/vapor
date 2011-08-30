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
#ifndef WIN32
#include <unistd.h>
//Silence an annoying and unnecessary compiler warning
#else
#pragma warning(disable : 4251 4267 4996)
#include "windows.h"
#include "Winnetwk.h"
#endif
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

ImpExp::ImpExp() {
	_objInitialized = 0;

	SetDiagMsg("ImpExp::ImpExp()");

	SetClassName("ImpExp");

	_rootnode = NULL;
	_objInitialized = 1;
}

ImpExp::~ImpExp() {
	SetDiagMsg("ImpExp::~ImpExp()");
	if (! _objInitialized) return;

	_rootnode = NULL;
	_objInitialized = 0;
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

	map <string, string> attrs;
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
	string xmlpath = GetPath();
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

	string xmlpath = GetPath();
	is.open(xmlpath.c_str());
	if (! is) {
		SetErrMsg("Can't open file \"%s\" for reading", xmlpath.c_str());
		return(-1);
	}

	map <string, string> attrs;
	attrs.clear();

	_rootnode = new XmlNode(_rootTag, attrs);

	
	ExpatParseMgr* parseMgr = new ExpatParseMgr(this);
	parseMgr->parse(is);
	delete parseMgr;
	

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


bool ImpExp::
elementStartHandler(ExpatParseMgr* pm, int level , std::string& tagstr, const char **attrs){

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
	default:
		//pm->parseError("Invalid tag : %s", tagstr.c_str());
		pm->skipElement(tagstr, level);
		return true;
	}
	return true;
}
bool ImpExp::
elementEndHandler(ExpatParseMgr* pm, int level , std::string& tagstr)
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

	default:
		pm->parseError("Invalid state");
		return false;
	}
	return true;
}


// Level 0 start element handler. The only element tag permitted at this
// level is the '_rootTag' tag
//
void	ImpExp::_startElementHandler0(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {

	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;


	// Verify valid level 0 element
	//
	if (StrCmpNoCase(tag, _rootTag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}

}

void	ImpExp::_startElementHandler1(ExpatParseMgr* pm,
	const string &tag, const char **attrs
) {
	ExpatStackElement *state = pm->getStateStackTop();
	state->has_data = 0;

	string type;

	// 
	//
	if (! *attrs) {
		//pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		pm->skipElement(tag, 1);
		return;
	}

	// Must be a data element. Make sure it has a type attribute.
	//


	string attr = *attrs;
	attrs++;
	string value = *attrs;
	attrs++;

	if (*attrs) {
		pm->skipElement(tag, 1);
		//pm->parseError("Too many attributes");
		return;
	}
	istringstream ist(value);


	state->has_data = 1;
	if (StrCmpNoCase(attr, _typeAttr) != 0) {
		pm->skipElement(tag, 1);
		//pm->parseError("Invalid attribute : %s", attr.c_str());
		return;
	}


	ist >> type;
	state->data_type = type;
	if (StrCmpNoCase(tag, _pathNameTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _timeStepTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _varNameTag) == 0) {
		if (StrCmpNoCase(type, _stringType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _regionTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else if (StrCmpNoCase(tag, _timeSegmentTag) == 0) {
		if (StrCmpNoCase(type, _longType) != 0) {
			pm->parseError("Invalid attribute type : \"%s\"", type.c_str());
			return;
		}
	}
	else {
		pm->skipElement(tag, 1);
		//pm->parseError("Invalid tag : %s", tag.c_str());
		return;
	}
}


void	ImpExp::_endElementHandler0(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();

	// this test is probably superfluous
	if (StrCmpNoCase(tag, state->tag) != 0) {
		pm->parseError("Invalid tag : \"%s\"", tag.c_str());
		return;
	}
}

void	ImpExp::_endElementHandler1(ExpatParseMgr* pm,
	const string &tag
) {
	ExpatStackElement *state = pm->getStateStackTop();


	if (StrCmpNoCase(state->data_type, _stringType) == 0) {
		_rootnode->SetElementString(tag, pm->getStringData());
	}
	else if (StrCmpNoCase(state->data_type, _longType) == 0) {
		_rootnode->SetElementLong(tag, pm->getLongData());
	}
	else {
		_rootnode->SetElementDouble(tag, pm->getDoubleData());
	}
}

string ImpExp::GetPath() {

	char	buf[128];
#ifdef WIN32
	char buf2[50];
	DWORD size = 50;
	
	WNetGetUser(0,buf2,&size);
	sprintf (buf, "C:/TEMP/vapor.%s.xml", buf2);
#else
	uid_t	uid = getuid();
	sprintf (buf, "/tmp/vapor.%6.6d.xml", uid);
#endif
	string path(buf);

	return(path);
}
