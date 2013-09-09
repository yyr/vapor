#include <iostream>
#include <sstream>
#include <iterator>
#include <algorithm>
#include <cassert>
#ifdef _WINDOWS
#include "vapor/udunits2.h"
#else
#include <udunits2.h>
#endif
#include <vapor/NetCDFCFCollection.h>
#include <vapor/GetAppPath.h>

using namespace VAPoR;
using namespace VetsUtil;
using namespace std;

NetCDFCFCollection::NetCDFCFCollection() : NetCDFCollection() {

	_coordinateVars.clear();
	_auxCoordinateVars.clear();
	_lonCoordVars.clear();
	_latCoordVars.clear();
	_vertCoordVars.clear();
	_timeCoordVars.clear();
	_missingValueMap.clear();
	_derivedVarsMap.clear();
	_derivedVar = NULL;

	_udunit = NULL;

}
NetCDFCFCollection::~NetCDFCFCollection() {
	if (_udunit) delete _udunit;

	map <string, NetCDFCFCollection::DerivedVar *>::iterator itr;
	for (itr = _derivedVarsMap.begin(); itr != _derivedVarsMap.end(); ++itr) {
		if (itr->second) delete itr->second;
	}
}

int NetCDFCFCollection::Initialize(
	const vector <string> &files
) {

	_coordinateVars.clear();
	_auxCoordinateVars.clear();
	_lonCoordVars.clear();
	_latCoordVars.clear();
	_vertCoordVars.clear();
	_timeCoordVars.clear();
	_missingValueMap.clear();

	if (_udunit) delete _udunit;
	_udunit = new UDUnits();
	int rc = _udunit->Initialize();
	if (rc<0) {
		SetErrMsg(
			"Failed to initialized udunits2 library : %s",
			_udunit->GetErrMsg().c_str()
		);
		return(0);
	}


	vector <string> emptyvec;
	rc = NetCDFCollection::Initialize(files, emptyvec, emptyvec);
	if (rc<0) return(-1);

	
	//
	// Identify all of the coordinate variables and 
	// auxiliary coordinate variables. Not sure if we need to 
	// make a distinction between "coordinate variable" and 
	// "auxiliary coordinate variable".
	//
	// First look for "coordinate variables", which are 1D and have
	// the same name as their one dimension
	//
	vector <string> vars = NetCDFCollection::GetVariableNames(1);
	for (int i=0; i<vars.size(); i++) {

		NetCDFSimple::Variable varinfo;
		(void) NetCDFCollection::GetVariableInfo(vars[i], varinfo);

		if (_IsCoordinateVar(varinfo)) {
			_coordinateVars.push_back(vars[i]);
		} 
	}

	//
	// Get all the 1D, 2D 3D, and 4D variables
	vars.clear();
	vector <string> v = NetCDFCollection::GetVariableNames(1);
	vars.insert(vars.end(), v.begin(), v.end());

	v = NetCDFCollection::GetVariableNames(2);
	vars.insert(vars.end(), v.begin(), v.end());

	v = NetCDFCollection::GetVariableNames(3);
	vars.insert(vars.end(), v.begin(), v.end());

	v = NetCDFCollection::GetVariableNames(4);
	vars.insert(vars.end(), v.begin(), v.end());

	for (int i=0; i<vars.size(); i++) {

		NetCDFSimple::Variable varinfo;
		(void) NetCDFCollection::GetVariableInfo(vars[i], varinfo);

		vector <string> coordattr = _GetCoordAttrs(varinfo);
		if (! coordattr.size()) continue;

		for (int j=0; j<coordattr.size(); j++) {

			//
			// Make sure the auxiliary coordinate variable 
			// actually exists in the data collection and has not
			// already been identified as a 1D "coordinate variable".
			//
			if (NetCDFCollection::VariableExists(coordattr[j]) &&
				(find(
					_coordinateVars.begin(), _coordinateVars.end(),
					coordattr[j]) == _coordinateVars.end())) {

				_auxCoordinateVars.push_back(coordattr[j]);
			}
		}
	}

    //
    // sort and remove duplicate auxiliary coordinate variables
    //
    sort(_auxCoordinateVars.begin(), _auxCoordinateVars.end());
    vector <string>::iterator lasts;
    lasts = unique(_auxCoordinateVars.begin(), _auxCoordinateVars.end());
    _auxCoordinateVars.erase(lasts, _auxCoordinateVars.end());

	//
	// Lastly, determine the type of coordinate variable (lat, lon,
	// vertical, or time)
	//
	vector <string> cvars = _coordinateVars;
	cvars.insert(
		cvars.end(), _auxCoordinateVars.begin(), _auxCoordinateVars.end()
	);

	for (int i=0; i<cvars.size(); i++) {
		NetCDFSimple::Variable varinfo;
		(void) NetCDFCollection::GetVariableInfo(cvars[i], varinfo);

		if (_IsLonCoordVar(varinfo)) {
			_lonCoordVars.push_back(cvars[i]);
		}
		else if (_IsLatCoordVar(varinfo)) {
			_latCoordVars.push_back(cvars[i]);
		}
		else if (_IsVertCoordVar(varinfo)) {
			_vertCoordVars.push_back(cvars[i]);
		}
		else if (_IsTimeCoordVar(varinfo)) {
			_timeCoordVars.push_back(cvars[i]);
		}
	}

	// 
	// Reinitialize the base class now that we know the time coordinate
	// variable. We're assuming that the time coordinate variable and
	// the time dimension have the same name. Thus we're not handling
	// the case where time variable could be a CF "auxilliary 
	// coordinate variable".
	//
	vector <string> tv;
	for (int i=0; i<_timeCoordVars.size(); i++) {
		if (IsCoordVarCF(_timeCoordVars[i])) {
			tv.push_back(_timeCoordVars[i]);
		}
	}


	rc = NetCDFCollection::Initialize(files, tv, tv);
	if (rc<0) return(-1);
	
	_GetMissingValueMap(_missingValueMap);
	return(0);
}

vector <string> NetCDFCFCollection::GetDataVariableNames(int ndim) const 
{
	vector <string> tmp = NetCDFCollection::GetVariableNames(ndim);

	vector <string> varnames;
	for (int i=0; i<tmp.size(); i++) {
		//
		// Strip off any coordinate variables
		//
		if (IsCoordVarCF(tmp[i]) || IsAuxCoordVarCF(tmp[i])) continue;

		vector <string> cvars;
		NetCDFCFCollection::GetVarCoordVarNames(tmp[i], cvars);

		if (NetCDFCFCollection::IsTimeVarying(tmp[i])) {
			if (cvars.size() != ndim+1) continue;
		}
		else {
			if (cvars.size() != ndim) continue;
		}
		varnames.push_back(tmp[i]);
	}

	return(varnames);
}

vector <size_t>  NetCDFCFCollection::GetDims(string varname) const {
	if (NetCDFCFCollection::IsDerivedVar(varname)) {
		NetCDFCFCollection::DerivedVar *derivedVar = _derivedVarsMap.find(varname)->second;
		return(derivedVar->GetDims());
	}
	return(NetCDFCollection::GetDims(varname));
}


vector <string> NetCDFCFCollection::GetVariableNames(int ndim) const {

	vector <string> varnames = NetCDFCollection::GetVariableNames(ndim);

	//
	// Add any derived variables
	//
	map <string, DerivedVar *>::const_iterator itr;
	for (itr = _derivedVarsMap.begin(); itr != _derivedVarsMap.end(); ++itr) {
		DerivedVar *derivedVar = itr->second;
		if (derivedVar->GetDims().size() == ndim) {
			varnames.push_back(itr->first);
		}
	}
	return(varnames);
}

int NetCDFCFCollection::GetVarCoordVarNames(
	string var, vector <string> &cvars
) const {
	cvars.clear();

	NetCDFSimple::Variable varinfo;

	int rc = NetCDFCollection::GetVariableInfo(var, varinfo);
	if (rc<0) return(rc);

	vector <string> dimnames = varinfo.GetDimNames();

	//
	// First look for auxiliary coordinate variables 
	//
	bool hasTimeCoord = false;
	bool hasLatCoord = false;
	bool hasLonCoord = false;
	bool hasVertCoord = false;
	vector <string> auxcvars = _GetCoordAttrs(varinfo);
	for (int i=0; i<auxcvars.size(); i++) {

		//
		// Make sure variable specified by "coordinate" attribute is in 
		// fact an auxiliary coordinate variable (any coordinate variables
		// specified by the "coordinate" attribute should have already
		// been picked up above
		//
		if (NetCDFCFCollection::IsTimeCoordVar(auxcvars[i]) && !hasTimeCoord) { 
			hasTimeCoord = true;
			cvars.push_back(auxcvars[i]);
		}
		if (NetCDFCFCollection::IsLatCoordVar(auxcvars[i]) && !hasLatCoord) { 
			hasLatCoord = true;
			cvars.push_back(auxcvars[i]);
		}
		if (NetCDFCFCollection::IsLonCoordVar(auxcvars[i]) && !hasLonCoord) { 
			hasLonCoord = true;
			cvars.push_back(auxcvars[i]);
		}
		if (NetCDFCFCollection::IsVertCoordVar(auxcvars[i]) && !hasVertCoord) { 
			hasVertCoord = true;
			cvars.push_back(auxcvars[i]);
		}
	}
	if (cvars.size() == dimnames.size()) return(0);


	//
	// Now see if any "coordinate variables" for which we haven't
	// already identified a coord var exist.
	//
	for (int i=0; i<dimnames.size(); i++) {
		if (NetCDFCFCollection::IsCoordVarCF(dimnames[i])) {	// is a CF "coordinate variable"?
			if (NetCDFCFCollection::IsTimeCoordVar(dimnames[i]) && !hasTimeCoord) { 
				hasTimeCoord = true;
				cvars.push_back(dimnames[i]);
			}
			if (NetCDFCFCollection::IsLatCoordVar(dimnames[i]) && !hasLatCoord) { 
				hasLatCoord = true;
				cvars.push_back(dimnames[i]);
			}
			if (NetCDFCFCollection::IsLonCoordVar(dimnames[i]) && !hasLonCoord) { 
				hasLonCoord = true;
				cvars.push_back(dimnames[i]);
			}
			if (NetCDFCFCollection::IsVertCoordVar(dimnames[i]) && !hasVertCoord) { 
				hasVertCoord = true;
				cvars.push_back(dimnames[i]);
			}
		}
	}

	//
	// If "coordinate variables" are specified for each dimension we're don
	//
	if (cvars.size() == dimnames.size()) return(0);

	//
	// If we still don't have lat and lon coordinate (or auxiliary)
	// variables for 'var' then we look for coordinate variables whose
	// dim names match the dim names of 'var'. Don't think this is
	// part of the CF 1.6 spec, but it seems necessary for ROMS data sets
	//
	if (! hasLatCoord) {
		vector <string> latcvs = NetCDFCFCollection::GetLatCoordVars();
		for (int i=0; i<latcvs.size(); i++) {
			NetCDFSimple::Variable varinfo;
			NetCDFCollection::GetVariableInfo(latcvs[i], varinfo);
			vector <string> dns = varinfo.GetDimNames();

			if (dimnames.size() >= dns.size()) {
				vector <string> tmp(dimnames.end()-dns.size(), dimnames.end());
				if (tmp == dns) {
					cvars.push_back(latcvs[i]);
					break;
				}
			}
		}
	}

	if (! hasLonCoord) {
		vector <string> loncvs = NetCDFCFCollection::GetLonCoordVars();
		for (int i=0; i<loncvs.size(); i++) {
			NetCDFSimple::Variable varinfo;
			NetCDFCollection::GetVariableInfo(loncvs[i], varinfo);
			vector <string> dns = varinfo.GetDimNames();

			if (dimnames.size() >= dns.size()) {
				vector <string> tmp(dimnames.end()-dns.size(), dimnames.end());
				if (tmp == dns) {
					cvars.push_back(loncvs[i]);
					break;
				}
			}
		}
	}
		

	assert(cvars.size() <= dimnames.size());

	return(0);
}

bool NetCDFCFCollection::IsVertCoordVarUp(string cvar) const {
	NetCDFSimple::Variable varinfo;

	int rc = NetCDFCollection::GetVariableInfo(cvar, varinfo);
	if (rc<0) {
		SetErrCode(0);
		return(false);
	}

	string s;
	varinfo.GetAtt("positive", s);

	if (StrCmpNoCase(s, "up") == 0) return(true);
	else return(false);
}

int NetCDFCFCollection::GetVarUnits(string var, string &units) const {
	units.clear();

	NetCDFSimple::Variable varinfo;

	int rc = NetCDFCollection::GetVariableInfo(var, varinfo);
	if (rc<0) return(rc);

	varinfo.GetAtt("units", units);
	return(0);
}

int NetCDFCFCollection::Convert(
    const string from, const string to, const float *src, float *dst, size_t n
) const {
	bool status = _udunit->Convert(from, to, src, dst, n);

	if (! status) {
		SetErrMsg(
			"NetCDFCFCollection::Convert(%s , %s,,) : failed",
			from.c_str(), to.c_str()
		);
		return(-1);
	}
	return(0);
}

bool NetCDFCFCollection::GetMissingValue(string varname, double &mv) const {
	map <string, double>::const_iterator itr;

	if (NetCDFCFCollection::IsDerivedVar(varname)) return(false);

	itr = _missingValueMap.find(varname);
	if (itr != _missingValueMap.end()) {
		mv = itr->second;
		return(true);
	}
	return(false);
}

int NetCDFCFCollection::OpenRead(size_t ts, string varname) {
	double mv;
	string mvattname;

	if (_derivedVar) {
		_derivedVar->Close();
		_derivedVar = NULL;
	}

	if (NetCDFCFCollection::IsDerivedVar(varname)) {
		DerivedVar *derivedVar = _derivedVarsMap[varname];
		int rc = derivedVar->Open(ts);
		if (rc<0) return(rc);
		_derivedVar = derivedVar;	// Can't set _derivedVar until after Open()
		return(0);
	}

	if (_GetMissingValue(varname, mvattname, mv)) { 
		NetCDFCFCollection::SetMissingValueAttName(mvattname);
	}
	int rc = NetCDFCollection::OpenRead(ts, varname);

	NetCDFCFCollection::SetMissingValueAttName("");
	return(rc);
}

int NetCDFCFCollection::ReadSlice(float *data) {

	if (_derivedVar) {
		return(_derivedVar->ReadSlice(data));
	}
	return(NetCDFCollection::ReadSlice(data));
}

int NetCDFCFCollection::Read(float *data) {

	if (_derivedVar) {
		return(_derivedVar->Read(data));
	}
	return(NetCDFCollection::Read(data));
}

int NetCDFCFCollection::Close() {

    if (_derivedVar) {
        _derivedVar->Close();
		_derivedVar = NULL;
		return(0);
    }

    return(NetCDFCollection::Close());
}


bool NetCDFCFCollection::IsVertDimensionless(string cvar) const {

	//
	// Return false if variable isn't a vertical coordinate variable at all
	//
	if (! IsVertCoordVar(cvar)) return (false);

	//
	// If we get to here the cvar is a valid coordinate variable
	// so GetVariableInfo() should return successfully
	//
	NetCDFSimple::Variable varinfo;
	(void) NetCDFCollection::GetVariableInfo(cvar, varinfo);

	string unit;
	varinfo.GetAtt("units", unit);
	if (unit.empty()) return(false);	// No coordinates attribute found

	return(! (_udunit->IsPressureUnit(unit) || _udunit->IsLengthUnit(unit)));
}


int NetCDFCFCollection::_parse_formula(
	string formula_terms, map <string, string> &parsed_terms
) const {

	// Remove ":" to ease parsing. It's superflous
	//
	replace(formula_terms.begin(), formula_terms.end(), ':', ' ');

    string buf; // Have a buffer string
    stringstream ss(formula_terms); // Insert the string into a stream

    vector<string> tokens; // Create vector to hold our words

    while (ss >> buf) {
        tokens.push_back(buf);
	}

	if (tokens.size() % 2) {
		SetErrMsg("Invalid formula_terms string : %s", formula_terms.c_str());
		return(-1);
	} 

	for (int i=0; i<tokens.size(); i+=2) {
		parsed_terms[tokens[i]] = tokens[i+1];
	}
	return(0);
}

int NetCDFCFCollection::InstallStandardVerticalConverter(
	string cvar, string newvar, string units
) {
	if (! NetCDFCFCollection::IsVertCoordVar(cvar)) {
		SetErrMsg("Variable %s not a vertical coordinate variable",cvar.c_str());
		return(-1);
	}

    NetCDFSimple::Variable varinfo;
    (void) NetCDFCollection::GetVariableInfo(cvar, varinfo);

    string standard_name;
    varinfo.GetAtt("standard_name", standard_name);
    if (standard_name.empty()) return(-1);

    string formula_terms;
    varinfo.GetAtt("formula_terms", formula_terms);
    if (formula_terms.empty()) return(-1);

	map <string, string> terms_map;
	int rc = _parse_formula(formula_terms, terms_map);
	if (rc<0) return(-1);
	
	
	NetCDFCFCollection::DerivedVar *derived_var;
	if (standard_name.compare("ocean_s_coordinate_g1") == 0) {
		derived_var = new DerivedVar_ocean_s_coordinate_g1(
			this, terms_map, units
		);
	}
	else if (standard_name.compare("ocean_s_coordinate_g2") == 0) {
		derived_var = new DerivedVar_ocean_s_coordinate_g2(
			this, terms_map, units
		);
	}
	else {
		SetErrMsg("Standard formula \"%s\" not supported",standard_name.c_str());
		return(-1);
	}

	_derivedVarsMap[newvar] = derived_var;
	return(0);
}

void NetCDFCFCollection::UninstallStandardVerticalConverter(string cvar)  {

	if (! NetCDFCFCollection::IsDerivedVar(cvar)) return;

	NetCDFCFCollection::DerivedVar *derivedVar = _derivedVarsMap.find(cvar)->second;

	if (_derivedVar == derivedVar) _derivedVar = NULL;

	delete derivedVar;

	_derivedVarsMap.erase(cvar);
}


void NetCDFCFCollection::FormatTimeStr(double seconds, string &str) const {

	int year, month, day, hour, minute, second;
	_udunit->DecodeTime(seconds, &year, &month, &day, &hour, &minute, &second);

	ostringstream oss;
	oss.fill('0');
	oss.width(4); oss << year; oss << "-";
	oss.width(2); oss << month; oss << "-";
	oss.width(2); oss << day; oss << " ";
	oss.width(2); oss << hour; oss << ":";
	oss.width(2); oss << minute; oss << ":";
	oss.width(2); oss << second; oss << " ";

	str = oss.str();
}



namespace VAPoR {
std::ostream &operator<<(
    std::ostream &o, const NetCDFCFCollection &ncdfcfc
) {
    o << "NetCDFCFCollection" << endl;
    o << " _coordinateVars : ";
    for (int i=0; i<ncdfcfc._coordinateVars.size(); i++) {
        o << ncdfcfc._coordinateVars[i] << " ";
    }
    o << endl;

    o << " _auxCoordinateVars : ";
    for (int i=0; i<ncdfcfc._auxCoordinateVars.size(); i++) {
        o << ncdfcfc._auxCoordinateVars[i] << " ";
    }
    o << endl;

    o << " _lonCoordVars : ";
    for (int i=0; i<ncdfcfc._lonCoordVars.size(); i++) {
        o << ncdfcfc._lonCoordVars[i] << " ";
    }
    o << endl;

    o << " _latCoordVars : ";
    for (int i=0; i<ncdfcfc._latCoordVars.size(); i++) {
        o << ncdfcfc._latCoordVars[i] << " ";
    }
    o << endl;

    o << " _vertCoordVars : ";
    for (int i=0; i<ncdfcfc._vertCoordVars.size(); i++) {
        o << ncdfcfc._vertCoordVars[i] << " ";
    }
    o << endl;

    o << " _timeCoordVars : ";
    for (int i=0; i<ncdfcfc._timeCoordVars.size(); i++) {
        o << ncdfcfc._timeCoordVars[i] << " ";
    }
    o << endl;

	o << " _missingValueMap : ";
	std::map <string, double>::const_iterator itr;
	for (itr = ncdfcfc._missingValueMap.begin(); itr != ncdfcfc._missingValueMap.end(); ++itr) {
		o << itr->first << " " << itr->second << ", ";
	}



	o << " Data Variables and coordinates :" << endl;
	for (int dim = 1; dim<4; dim++) {
		vector <string> vars = ncdfcfc.GetDataVariableNames(dim);
		for (int i=0; i<vars.size(); i++) {
			o << "  " << vars[i]  << " : ";
			vector <string> cvars;
			int rc = ncdfcfc.GetVarCoordVarNames(vars[i], cvars);
			if (rc<0) continue;
			for (int j=0; j<cvars.size(); j++) {
				o << cvars[j] << " ";
			}
			o << endl;
		}
	}

    o << endl;
    o << endl;

	o << (NetCDFCollection) ncdfcfc;

    return(o);
}
};

bool NetCDFCFCollection::_IsCoordinateVar(
	const NetCDFSimple::Variable &varinfo
) const {
	string varname = varinfo.GetName();
	vector <string> dimnames = varinfo.GetDimNames();

	if (dimnames.size() != 1) return(false);

	if (varname.compare(dimnames[0]) != 0) return(false);

	return(true);
}

vector <string> NetCDFCFCollection::_GetCoordAttrs(
	const NetCDFSimple::Variable &varinfo
) const {
	vector <string> coordattrs;

	string s;
	varinfo.GetAtt("coordinates", s);
	if (s.empty()) return(coordattrs);	// No coordinates attribute found

	//
	// split the string using white space as the delimiter
	//
	stringstream ss(s);
	istream_iterator<std::string> begin(ss);
	istream_iterator<std::string> end;
	coordattrs.insert(coordattrs.begin(), begin, end);

	return(coordattrs);
}

bool NetCDFCFCollection::_IsLonCoordVar(
	const NetCDFSimple::Variable &varinfo
) const {

	string s;
	varinfo.GetAtt("axis", s);
	if (StrCmpNoCase(s, "X") == 0) return(true);

	s.clear();
	varinfo.GetAtt("standard_name", s);
	if (StrCmpNoCase(s, "longitude") == 0) return(true);
	
	string unit;
	varinfo.GetAtt("units", unit);
	if (unit.empty()) return(false);	// No coordinates attribute found

	return(_udunit->IsLonUnit(unit));
}

bool NetCDFCFCollection::_IsLatCoordVar(
	const NetCDFSimple::Variable &varinfo
) const {
	string s;
	varinfo.GetAtt("axis", s);
	if (StrCmpNoCase(s, "Y") == 0) return(true);

	s.clear();
	varinfo.GetAtt("standard_name", s);
	if (StrCmpNoCase(s, "latitude") == 0) return(true);
	
	string unit;
	varinfo.GetAtt("units", unit);
	if (unit.empty()) return(false);	// No coordinates attribute found

	return(_udunit->IsLatUnit(unit));
}

bool NetCDFCFCollection::_IsVertCoordVar(
	const NetCDFSimple::Variable &varinfo
) const {
	string s;
	varinfo.GetAtt("axis", s);
	if (StrCmpNoCase(s, "Z") == 0) return(true);

	s.clear();
	varinfo.GetAtt("standard_name", s);

	if (StrCmpNoCase(s,"atmosphere_ln_pressure_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"atmosphere_sigma_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"atmosphere_hybrid_sigma_pressure_coordinate") == 0) return(true);
	if (StrCmpNoCase(s,"atmosphere_hybrid_height_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"atmosphere_sleve_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"ocean_sigma_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"ocean_s_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"ocean_double_sigma_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"ocean_double_sigma_coordinate")==0) return(true);
	if (StrCmpNoCase(s,"ocean_s_coordinate_g1")==0) return(true);
	if (StrCmpNoCase(s,"ocean_s_coordinate_g2")==0) return(true);

	string unit;
	varinfo.GetAtt("units", unit);
	if (unit.empty()) return(false);	// No coordinates attribute found

	return(_udunit->IsPressureUnit(unit) || _udunit->IsLengthUnit(unit));
}

bool NetCDFCFCollection::_IsTimeCoordVar(
	const NetCDFSimple::Variable &varinfo
) const {
	string s;
	varinfo.GetAtt("axis", s);
	if (StrCmpNoCase(s, "T") == 0) return(true);

	s.clear();
	varinfo.GetAtt("standard_name", s);
	if (StrCmpNoCase(s, "time") == 0) return(true);
	
	string unit;
	varinfo.GetAtt("units", unit);
	if (unit.empty()) return(false);	// No coordinates attribute found

	return(_udunit->IsTimeUnit(unit));
}

bool NetCDFCFCollection::_GetMissingValue(
	string varname,
	string &attname,
	double &mv
) const {
	attname.clear();
	mv = 0.0;

	NetCDFSimple::Variable varinfo;
	(void) NetCDFCollection::GetVariableInfo(varname, varinfo);

	vector <double> dvec;

	attname = "_FillValue";
	varinfo.GetAtt(attname, dvec);
	if (dvec.size()) {
		mv = dvec[0];
		return(true);
	}
	else {
		//
		// Use of "missing_value" is deprecated, but still
		// supported
		//
		attname = "missing_value";
		varinfo.GetAtt(attname, dvec);
		if (dvec.size()) {
			mv = dvec[0];
			return(true);
		}
	}
	return(false);
}

void NetCDFCFCollection::_GetMissingValueMap(
	map <string, double> &missingValueMap
) const {
	missingValueMap.clear();

	//
	// Generate a map from all data variables with missing value
	// attributes to missing value values. 
	//
	for (int d=1; d<4; d++) {
		vector <string> vars = NetCDFCFCollection::GetDataVariableNames(d);

		for (int i=0; i<vars.size(); i++) {
			string attname;
			double mv;
			if (_GetMissingValue(vars[i], attname, mv)) {
				missingValueMap[vars[i]] = mv;
			}
		}
	}
}

NetCDFCFCollection::UDUnits::UDUnits() {

    _statmsg[UT_SUCCESS] = "Success";
    _statmsg[UT_BAD_ARG] = "An argument violates the function's contract";
    _statmsg[UT_EXISTS] = "Unit, prefix, or identifier already exists";
    _statmsg[UT_NO_UNIT] = "No such unit exists";
    _statmsg[UT_OS] = "Operating-system error.";
    _statmsg[UT_NOT_SAME_SYSTEM] = "The units belong to different unit-systems";
    _statmsg[UT_MEANINGLESS] = "The operation on the unit(s) is meaningless";
    _statmsg[UT_NO_SECOND] = "The unit-system doesn't have a unit named \"second\"";
    _statmsg[UT_VISIT_ERROR] = "An error occurred while visiting a unit";
    _statmsg[UT_CANT_FORMAT] = "A unit can't be formatted in the desired manner";
    _statmsg[UT_SYNTAX] = "string unit representation contains syntax error";
    _statmsg[UT_UNKNOWN] = "string unit representation contains unknown word";
    _statmsg[UT_OPEN_ARG] = "Can't open argument-specified unit database";
    _statmsg[UT_OPEN_ENV] = "Can't open environment-specified unit database";
    _statmsg[UT_OPEN_DEFAULT] = "Can't open installed, default, unit database";
    _statmsg[UT_PARSE] = "Error parsing unit specification";

	_pressureUnit = NULL;
	_timeUnit = NULL;
	_latUnit = NULL;
	_lonUnit = NULL;
	_lengthUnit = NULL;
	_status = (int) UT_SUCCESS;
	_unitSystem = NULL;
}

int NetCDFCFCollection::UDUnits::Initialize() {

	//
	// Need to turn off error messages, which go to stderr by default
	//
	ut_set_error_message_handler(ut_ignore);

	vector <string> paths;
	paths.push_back("udunits");
	paths.push_back("udunits2.xml");
	string path =  GetAppPath("VAPOR", "share", paths).c_str();
	if (! path.empty()) {
		_unitSystem = ut_read_xml(path.c_str());
	}
	else {
		_unitSystem = ut_read_xml(NULL);
	}
	if (! _unitSystem) {
		_status = (int) ut_get_status();
		return(-1);
	};

	string unitstr;

	//
	// We need to be able to determine if a given unit is of a
	// particular type (e.g. time, pressure, mass, etc). The udunit2
	// API doesn't support this directly. So we create a 'unit' 
	// of a particular known type, and then later we can query udunit2
	// to see if it is possible to convert between the known unit type
	// and a unit of unknown type
	//

	unitstr = "Pa";	// Pascal units of Pressure
	_pressureUnit = ut_parse(_unitSystem, unitstr.c_str(), UT_ASCII);
	if (! _pressureUnit) {
		_status = (int) ut_get_status();
		return(-1);
	}

	unitstr = "seconds";
	_timeUnit = ut_parse(_unitSystem, unitstr.c_str(), UT_ASCII);
	if (! _timeUnit) {
		_status = (int) ut_get_status();
		return(-1);
	}

	unitstr = "degrees_north";
	_latUnit = ut_parse(_unitSystem, unitstr.c_str(), UT_ASCII);
	if (! _latUnit) {
		_status = (int) ut_get_status();
		return(-1);
	}

	unitstr = "degrees_east";
	_lonUnit = ut_parse(_unitSystem, unitstr.c_str(), UT_ASCII);
	if (! _lonUnit) {
		_status = (int) ut_get_status();
		return(-1);
	}

	unitstr = "meter";
	_lengthUnit = ut_parse(_unitSystem, unitstr.c_str(), UT_ASCII);
	if (! _lengthUnit) {
		_status = (int) ut_get_status();
		return(-1);
	}

	return(0);
}

bool NetCDFCFCollection::UDUnits::AreUnitsConvertible(const ut_unit *unit, string unitstr) const {


	ut_unit *myunit = ut_parse(_unitSystem, unitstr.c_str(), UT_ASCII);
	if (! myunit) {
		ut_set_status(UT_SUCCESS);	// clear error message
		return(false);
	}

	bool status = true;
	cv_converter *cv = NULL;
	if (! (cv = ut_get_converter((ut_unit *) unit, myunit))) {
		status = false;
		ut_set_status(UT_SUCCESS);
	}

	if (myunit) ut_free(myunit);
	if (cv) cv_free(cv);
	return(status);

}

bool NetCDFCFCollection::UDUnits::IsPressureUnit(string unitstr) const {
	return(AreUnitsConvertible(_pressureUnit, unitstr));
}

bool NetCDFCFCollection::UDUnits::IsTimeUnit(string unitstr) const {
	return(AreUnitsConvertible(_timeUnit, unitstr));
}

bool NetCDFCFCollection::UDUnits::IsLatUnit(string unitstr) const {
	bool status = AreUnitsConvertible(_latUnit, unitstr);
	if (! status) return(false);

	// udunits2 does not distinguish between longitude and latitude, only
	// whether a unit is a plane angle measure. N.B. the conditional
	// below is probably all that is needed for this method.
	//
	if ( !(
		(unitstr.compare("degrees_north") == 0)  ||
		(unitstr.compare("degree_north") == 0)  ||
		(unitstr.compare("degree_N") == 0)  ||
		(unitstr.compare("degrees_N") == 0)  ||
		(unitstr.compare("degreeN") == 0)  ||
		(unitstr.compare("degreesN") == 0))) { 

		status = false;
	}
	return(status);
}

bool NetCDFCFCollection::UDUnits::IsLonUnit(string unitstr) const {
	bool status = AreUnitsConvertible(_lonUnit, unitstr);
	if (! status) return(false);

	// udunits2 does not distinguish between longitude and latitude, only
	// whether a unit is a plane angle measure. N.B. the conditional
	// below is probably all that is needed for this method.
	//
	if ( !(
		(unitstr.compare("degrees_east") == 0)  ||
		(unitstr.compare("degree_east") == 0)  ||
		(unitstr.compare("degree_E") == 0)  ||
		(unitstr.compare("degrees_E") == 0)  ||
		(unitstr.compare("degreeE") == 0)  ||
		(unitstr.compare("degreesE") == 0))) { 

		status = false;
	}
	return(status);
}

bool NetCDFCFCollection::UDUnits::IsLengthUnit(string unitstr) const {
	return(AreUnitsConvertible(_lengthUnit, unitstr));
}

bool NetCDFCFCollection::UDUnits::Convert(
	const string from,
	const string to,
	const float *src,
	float *dst,
	size_t n
) const {

	ut_unit *fromunit = ut_parse(_unitSystem, from.c_str(), UT_ASCII);
	if (! fromunit) {
		ut_set_status(UT_SUCCESS);	// clear error message
		return(false);
	}

	ut_unit *tounit = ut_parse(_unitSystem, to.c_str(), UT_ASCII);
	if (! tounit) {
		ut_free(fromunit);
		ut_set_status(UT_SUCCESS);	// clear error message
		return(false);
	}

	cv_converter *cv = NULL;
	cv = ut_get_converter((ut_unit *) fromunit, tounit);
	if (! cv) {
		ut_free(tounit);
		ut_free(fromunit);
		ut_set_status(UT_SUCCESS);
		return(false);
	}

	cv_convert_floats(cv, src, n, dst);

	if (fromunit) ut_free(fromunit);
	if (tounit) ut_free(tounit);
	if (cv) cv_free(cv);
	return(true);
}

void NetCDFCFCollection::UDUnits::DecodeTime(
    double seconds, int* year, int* month, int* day, 
	int* hour, int* minute, int* second
) const {

	double dummy;
	double second_d;
	ut_decode_time(seconds, year, month, day, hour, minute, &second_d, &dummy);
	*second = (int) second_d;
}



string NetCDFCFCollection::UDUnits::GetErrMsg() const {
	map <int, string>::const_iterator itr;
	itr = _statmsg.find(_status);
	if (itr == _statmsg.end()) return(string("UNKNOWN"));

	return(itr->second);
}

NetCDFCFCollection::UDUnits::~UDUnits() {
	if (_unitSystem) ut_free_system(_unitSystem);
	if (_pressureUnit) ut_free(_pressureUnit);
	if (_timeUnit) ut_free(_timeUnit);
	if (_latUnit) ut_free(_latUnit);
	if (_lonUnit) ut_free(_lonUnit);
	if (_lengthUnit) ut_free(_lengthUnit);
}

NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::DerivedVar_ocean_s_coordinate_g1(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map, string units
) : DerivedVar(ncdfcf, formula_map, units) {

	_dims.resize(3);
	_s = NULL;
	_C = NULL;
	_eta = NULL;
	_depth = NULL;
	_depth_c = 0;
	_svar.clear();
	_Cvar.clear();
	_etavar.clear();
	_depthvar.clear();
	_depth_cvar.clear();
	_is_open = false;
	_ok = true;

    map <string, string>::const_iterator itr;
    itr = _formula_map.find("s");
    if (itr != _formula_map.end()) _svar = itr->second;

    itr = _formula_map.find("C");
    if (itr != _formula_map.end()) _Cvar = itr->second;

    itr = _formula_map.find("eta");
    if (itr != _formula_map.end()) _etavar = itr->second;

    itr = _formula_map.find("depth");
    if (itr != _formula_map.end()) _depthvar = itr->second;

    itr = _formula_map.find("depth_c");
    if (itr != _formula_map.end()) _depth_cvar = itr->second;

	vector <size_t> dims_tmp;
	if (_ncdfcf->VariableExists(_svar)) {
		dims_tmp = _ncdfcf->GetDims(_svar);
	}
	else if (_ncdfcf->VariableExists(_Cvar)) {
		dims_tmp = _ncdfcf->GetDims(_Cvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[0] = dims_tmp[0];

	if (_ncdfcf->VariableExists(_etavar)) {
		dims_tmp = _ncdfcf->GetDims(_etavar);
	}
	else if (_ncdfcf->VariableExists(_depthvar)) {
		dims_tmp = _ncdfcf->GetDims(_depthvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[1] = dims_tmp[0];
	_dims[2] = dims_tmp[1];

	_s = new float[_dims[0]];
	_C = new float[_dims[0]];
	_eta = new float[_dims[1] * _dims[2]];
	_depth = new float[_dims[1] * _dims[2]];
}

NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::~DerivedVar_ocean_s_coordinate_g1() {

	if (_s) delete [] _s;
	if (_C) delete [] _C;
	if (_eta) delete [] _eta;
	if (_depth) delete [] _depth;
}


int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::Open(size_t) {

	if (_is_open) return(0);	// Only open first time step
	if (! _ok) {
		SetErrMsg("Missing forumla terms");
		return(-1);
	}

	_slice_num = 0;

	size_t nx = _dims[2];
	size_t ny = _dims[1];
	size_t nz = _dims[0];

	int rc;
	double mv;

	rc = _ncdfcf->OpenRead(0, _svar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_s); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_svar, mv)) {	// zero out any mv
		for (int i=0; i<nz; i++) {
			if (_s[i] == mv) _s[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _Cvar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_C); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_Cvar, mv)) {
		for (int i=0; i<nz; i++) {
			if (_C[i] == mv) _C[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _etavar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_eta); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_etavar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_eta[i] == mv) _eta[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _depthvar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_depth); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_depthvar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_depth[i] == mv) _depth[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _depth_cvar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(&_depth_c); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);

	_is_open = true;
	return(0);
}

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::Read(float *buf) {
	size_t nx = _dims[2];
	size_t ny = _dims[1];
	size_t nz = _dims[0];

	for (size_t z=0; z<nz; z++) {
	for (size_t y=0; y<ny; y++) {
	for (size_t x=0; x<nx; x++) {
		buf[z*nx*ny + y*nx + x] = 
			_depth_c*_s[z] + (_depth[y*nx+x] - _depth_c)* _C[z] + _eta[y*nx+x] *
			(1+(_depth_c*_s[z] + (_depth[y*nx+x]-_depth_c)*_C[z])/_depth[y*nx+x]);
	}
	}
	}
	return(0);
}

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::ReadSlice(
	float *slice
) {
	size_t nx = _dims[2];
	size_t ny = _dims[1];
	size_t nz = _dims[0];

	if (_slice_num >= nz) return(0);


	size_t z = _slice_num;

	for (size_t y=0; y<ny; y++) {
	for (size_t x=0; x<nx; x++) {
		slice[y*nx + x] = 
			_depth_c*_s[z] + (_depth[y*nx+x] - _depth_c)* _C[z] + _eta[y*nx+x] *
			(1+(_depth_c*_s[z] + (_depth[y*nx+x]-_depth_c)*_C[z])/_depth[y*nx+x]);
	}
	}

	_slice_num++;
	return(1);
}

NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::DerivedVar_ocean_s_coordinate_g2(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map, string units
) : DerivedVar(ncdfcf, formula_map, units) {

	_dims.resize(3);
	_s = NULL;
	_C = NULL;
	_eta = NULL;
	_depth = NULL;
	_depth_c = 0;
	_svar.clear();
	_Cvar.clear();
	_etavar.clear();
	_depthvar.clear();
	_depth_cvar.clear();
	_is_open = false;
	_ok = true;

    map <string, string>::const_iterator itr;
    itr = _formula_map.find("s");
    if (itr != _formula_map.end()) _svar = itr->second;

    itr = _formula_map.find("C");
    if (itr != _formula_map.end()) _Cvar = itr->second;

    itr = _formula_map.find("eta");
    if (itr != _formula_map.end()) _etavar = itr->second;

    itr = _formula_map.find("depth");
    if (itr != _formula_map.end()) _depthvar = itr->second;

    itr = _formula_map.find("depth_c");
    if (itr != _formula_map.end()) _depth_cvar = itr->second;

	vector <size_t> dims_tmp;
	if (_ncdfcf->VariableExists(_svar)) {
		dims_tmp = _ncdfcf->GetDims(_svar);
	}
	else if (_ncdfcf->VariableExists(_Cvar)) {
		dims_tmp = _ncdfcf->GetDims(_Cvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[0] = dims_tmp[0];

	if (_ncdfcf->VariableExists(_etavar)) {
		dims_tmp = _ncdfcf->GetDims(_etavar);
	}
	else if (_ncdfcf->VariableExists(_depthvar)) {
		dims_tmp = _ncdfcf->GetDims(_depthvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[1] = dims_tmp[0];
	_dims[2] = dims_tmp[1];

	_s = new float[_dims[0]];
	_C = new float[_dims[0]];
	_eta = new float[_dims[1] * _dims[2]];
	_depth = new float[_dims[1] * _dims[2]];
}

NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::~DerivedVar_ocean_s_coordinate_g2() {

	if (_s) delete [] _s;
	if (_C) delete [] _C;
	if (_eta) delete [] _eta;
	if (_depth) delete [] _depth;
}


int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::Open(size_t) {

	if (_is_open) return(0);	// Only open first time step
	if (! _ok) {
		SetErrMsg("Missing forumla terms");
		return(-1);
	}

	_slice_num = 0;

	size_t nx = _dims[2];
	size_t ny = _dims[1];
	size_t nz = _dims[0];

	int rc;
	double mv;

	rc = _ncdfcf->OpenRead(0, _svar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_s); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_svar, mv)) {	// zero out any mv
		for (int i=0; i<nz; i++) {
			if (_s[i] == mv) _s[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _Cvar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_C); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_Cvar, mv)) {
		for (int i=0; i<nz; i++) {
			if (_C[i] == mv) _C[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _etavar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_eta); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_etavar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_eta[i] == mv) _eta[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _depthvar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(_depth); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);
	if (_ncdfcf->GetMissingValue(_depthvar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_depth[i] == mv) _depth[i] = 0.0;
		}
	} 

	rc = _ncdfcf->OpenRead(0, _depth_cvar); if (rc<0) return(-1);
	rc = _ncdfcf->Read(&_depth_c); if (rc<0) return(-1);
	rc = _ncdfcf->Close(); if (rc<0) return(-1);

	_is_open = true;
	return(0);
}

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::Read(float *buf) {
	size_t nx = _dims[2];
	size_t ny = _dims[1];
	size_t nz = _dims[0];

	for (size_t z=0; z<nz; z++) {
	for (size_t y=0; y<ny; y++) {
	for (size_t x=0; x<nx; x++) {
		buf[z*nx*ny + y*nx + x] = 
			_eta[y*nx+x] + (_eta[y*nx+x] + _depth[y*nx+x]) * ((_depth_c*_s[z] + 
			_depth[y*nx+x]*_C[z])/(_depth_c+_depth[y*nx+x]));
	}
	}
	}
	return(0);
}

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::ReadSlice(
	float *slice
) {
	size_t nx = _dims[2];
	size_t ny = _dims[1];
	size_t nz = _dims[0];

	if (_slice_num >= nz) return(0);


	size_t z = _slice_num;

	for (size_t y=0; y<ny; y++) {
	for (size_t x=0; x<nx; x++) {
		slice[y*nx + x] = 
			_eta[y*nx+x] + (_eta[y*nx+x] + _depth[y*nx+x]) * ((_depth_c*_s[z] + 
			_depth[y*nx+x]*_C[z])/(_depth_c+_depth[y*nx+x]));
	}
	}

	_slice_num++;
	return(1);
}
