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
#include <vapor/GetAppPath.h>
#include <vapor/NetCDFCFCollection.h>

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

	_udunit = NULL;

}
NetCDFCFCollection::~NetCDFCFCollection() {
	if (_udunit) delete _udunit;

	std::map <string,DerivedVar *>::iterator itr;
	for (itr = _derivedVarsMap.begin(); itr != _derivedVarsMap.end(); ++itr) {
		delete itr->second;
	}
	 _derivedVarsMap.clear();
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
	vector <string> vars = NetCDFCollection::GetVariableNames(1, false);
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
	vector <string> v = NetCDFCollection::GetVariableNames(1, false);
	vars.insert(vars.end(), v.begin(), v.end());

	v = NetCDFCollection::GetVariableNames(2, false);
	vars.insert(vars.end(), v.begin(), v.end());

	v = NetCDFCollection::GetVariableNames(3, false);
	vars.insert(vars.end(), v.begin(), v.end());

	v = NetCDFCollection::GetVariableNames(4, false);
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

vector <string> NetCDFCFCollection::GetDataVariableNames(
	int ndim, bool spatial
) const {

	vector <string> tmp = NetCDFCollection::GetVariableNames(ndim, spatial);

	vector <string> varnames;
	for (int i=0; i<tmp.size(); i++) {
		//
		// Strip off any coordinate variables
		//
		if (IsCoordVarCF(tmp[i]) || IsAuxCoordVarCF(tmp[i])) continue;

		vector <string> cvars;
		NetCDFCFCollection::GetVarCoordVarNames(tmp[i], cvars);

		int myndim = cvars.size();
		if (spatial && IsTimeVarying(tmp[i])) {
			myndim--;	// don't count time dimension
		}
		if (myndim != ndim) continue;

		varnames.push_back(tmp[i]);
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
	vector <string> tmpcvars;
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
			tmpcvars.push_back(auxcvars[i]);
		}
		if (NetCDFCFCollection::IsLatCoordVar(auxcvars[i]) && !hasLatCoord) { 
			hasLatCoord = true;
			tmpcvars.push_back(auxcvars[i]);
		}
		if (NetCDFCFCollection::IsLonCoordVar(auxcvars[i]) && !hasLonCoord) { 
			hasLonCoord = true;
			tmpcvars.push_back(auxcvars[i]);
		}
		if (NetCDFCFCollection::IsVertCoordVar(auxcvars[i]) && !hasVertCoord) { 
			hasVertCoord = true;
			tmpcvars.push_back(auxcvars[i]);
		}
	}


	//
	// Now see if any "coordinate variables" for which we haven't
	// already identified a coord var exist.
	//
	if (tmpcvars.size() != dimnames.size())  {
		for (int i=0; i<dimnames.size(); i++) {
			if (NetCDFCFCollection::IsCoordVarCF(dimnames[i])) {	// is a CF "coordinate variable"?
				if (NetCDFCFCollection::IsTimeCoordVar(dimnames[i]) && !hasTimeCoord) { 
					hasTimeCoord = true;
					tmpcvars.push_back(dimnames[i]);
				}
				if (NetCDFCFCollection::IsLatCoordVar(dimnames[i]) && !hasLatCoord) { 
					hasLatCoord = true;
					tmpcvars.push_back(dimnames[i]);
				}
				if (NetCDFCFCollection::IsLonCoordVar(dimnames[i]) && !hasLonCoord) { 
					hasLonCoord = true;
					tmpcvars.push_back(dimnames[i]);
				}
				if (NetCDFCFCollection::IsVertCoordVar(dimnames[i]) && !hasVertCoord) { 
					hasVertCoord = true;
					tmpcvars.push_back(dimnames[i]);
				}
			}
		}
	}


	//
	// If we still don't have lat and lon coordinate (or auxiliary)
	// variables for 'var' then we look for coordinate variables whose
	// dim names match the dim names of 'var'. Don't think this is
	// part of the CF 1.6 spec, but it seems necessary for ROMS data sets
	//
	//
	// If "coordinate variables" are specified for each dimension we're don
	//
	if (tmpcvars.size() != dimnames.size()) {
		if (! hasLatCoord) {
			vector <string> latcvs = NetCDFCFCollection::GetLatCoordVars();
			for (int i=0; i<latcvs.size(); i++) {
				NetCDFSimple::Variable varinfo;
				NetCDFCollection::GetVariableInfo(latcvs[i], varinfo);
				vector <string> dns = varinfo.GetDimNames();

				if (dimnames.size() >= dns.size()) {
					vector <string> tmp(dimnames.end()-dns.size(), dimnames.end());
					if (tmp == dns) {
						tmpcvars.push_back(latcvs[i]);
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
						tmpcvars.push_back(loncvs[i]);
						break;
					}
				}
			}
		}
	}
	assert(tmpcvars.size() <= dimnames.size());

	//
	// Finally, order the coordinate variables from slowest to fastest
	// varying dimension
	//
	for (int i=0; i<tmpcvars.size(); i++) {
        if (NetCDFCFCollection::IsTimeCoordVar(tmpcvars[i])) {
			cvars.push_back(tmpcvars[i]); break;
        }
	}
	for (int i=0; i<tmpcvars.size(); i++) {
        if (NetCDFCFCollection::IsVertCoordVar(tmpcvars[i])) {
			cvars.push_back(tmpcvars[i]); break;
        }
	}
	for (int i=0; i<tmpcvars.size(); i++) {
        if (NetCDFCFCollection::IsLatCoordVar(tmpcvars[i])) {
			cvars.push_back(tmpcvars[i]); break;
        }
	}
	for (int i=0; i<tmpcvars.size(); i++) {
        if (NetCDFCFCollection::IsLonCoordVar(tmpcvars[i])) {
			cvars.push_back(tmpcvars[i]); break;
        }
	}
	assert(cvars.size() == tmpcvars.size());
	

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

    if (NetCDFCollection::IsDerivedVar(varname)) return(false);

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

	if (_GetMissingValue(varname, mvattname, mv)) { 
		NetCDFCFCollection::SetMissingValueAttName(mvattname);
	}
	int fd = NetCDFCollection::OpenRead(ts, varname);

	NetCDFCFCollection::SetMissingValueAttName("");
	return(fd);
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
	
	
	NetCDFCollection::DerivedVar *derived_var;
	if (standard_name.compare("ocean_s_coordinate_g1") == 0) {
		derived_var = new DerivedVar_ocean_s_coordinate_g1(
			this, terms_map
		);
	}
	else if (standard_name.compare("ocean_s_coordinate_g2") == 0) {
		derived_var = new DerivedVar_ocean_s_coordinate_g2(
			this, terms_map
		);
	}
	else {
		SetErrMsg("Standard formula \"%s\" not supported",standard_name.c_str());
		return(-1);
	}

	// Uninstall any previous instance
	//
	NetCDFCFCollection::UninstallStandardVerticalConverter(newvar);

	NetCDFCollection::InstallDerivedVar(newvar, derived_var);
	

	return(0);
}

void NetCDFCFCollection::UninstallStandardVerticalConverter(string cvar)  {

	if (! NetCDFCollection::IsDerivedVar(cvar)) return;

	std::map <string,DerivedVar *>::iterator itr = _derivedVarsMap.find(cvar);
	if (itr != _derivedVarsMap.end()) {
		NetCDFCollection::RemoveDerivedVar(cvar);
		delete itr->second;
	}

	NetCDFCollection::RemoveDerivedVar(cvar);
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
		vector <string> vars = ncdfcfc.GetDataVariableNames(dim, true);
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

	if (NetCDFCollection::IsDerivedVar(varname)) {
		return(NetCDFCollection::GetMissingValue(varname, mv));
	}

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
	for (int d=1; d<5; d++) {
		vector <string> vars = NetCDFCFCollection::GetDataVariableNames(d,false);

		for (int i=0; i<vars.size(); i++) {
			string attname;
			double mv;
			if (_GetMissingValue(vars[i], attname, mv)) {
				missingValueMap[vars[i]] = mv;
			}
		}
	}
}

NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::DerivedVar_ocean_s_coordinate_g1(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map
) : DerivedVar(ncdfcf) {

	_dims.resize(3);
	_dimnames.resize(3);
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
    itr = formula_map.find("s");
    if (itr != formula_map.end()) _svar = itr->second;

    itr = formula_map.find("C");
    if (itr != formula_map.end()) _Cvar = itr->second;

    itr = formula_map.find("eta");
    if (itr != formula_map.end()) _etavar = itr->second;

    itr = formula_map.find("depth");
    if (itr != formula_map.end()) _depthvar = itr->second;

    itr = formula_map.find("depth_c");
    if (itr != formula_map.end()) _depth_cvar = itr->second;

	vector <size_t> dims_tmp;
	vector <string> dimnames_tmp;
	if (_ncdfc->VariableExists(_svar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_svar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_svar);
	}
	else if (_ncdfc->VariableExists(_Cvar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_Cvar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_Cvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[0] = dims_tmp[0];
	_dimnames[0] = dimnames_tmp[0];

	if (_ncdfc->VariableExists(_etavar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_etavar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_etavar);
	}
	else if (_ncdfc->VariableExists(_depthvar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_depthvar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_depthvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[1] = dims_tmp[0];
	_dims[2] = dims_tmp[1];
	_dimnames[1] = dimnames_tmp[0];
	_dimnames[2] = dimnames_tmp[1];

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

	int fd = _ncdfc->OpenRead(0, _svar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_s, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_svar, mv)) {	// zero out any mv
		for (int i=0; i<nz; i++) {
			if (_s[i] == mv) _s[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _Cvar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_C, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_Cvar, mv)) {
		for (int i=0; i<nz; i++) {
			if (_C[i] == mv) _C[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _etavar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_eta, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_etavar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_eta[i] == mv) _eta[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _depthvar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_depth,fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_depthvar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_depth[i] == mv) _depth[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _depth_cvar); if (fd<0) return(-1);
	rc = _ncdfc->Read(&_depth_c, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);

	_is_open = true;
	return(0);
}

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::Read(
	float *buf, int
) {
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
	float *slice, int 
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

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g1::SeekSlice(
	int offset, int whence, int 
) {
	size_t nz = _dims[0];

    int slice;
    if (whence == 0) {
        slice = offset;
    }
    else if (whence == 1) {
        slice = _slice_num + offset;
    }
    else if (whence == 2) {
        slice = offset + nz - 1;
    }
    if (slice<0) slice = 0;
    if (slice>nz-1) slice = nz-1;

	_slice_num = slice;

    return(0);
}


NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::DerivedVar_ocean_s_coordinate_g2(
	NetCDFCFCollection *ncdfcf, 
	const std::map <string, string> &formula_map
) : DerivedVar(ncdfcf) {

	_dims.resize(3);
	_dimnames.resize(3);
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
    itr = formula_map.find("s");
    if (itr != formula_map.end()) _svar = itr->second;

    itr = formula_map.find("C");
    if (itr != formula_map.end()) _Cvar = itr->second;

    itr = formula_map.find("eta");
    if (itr != formula_map.end()) _etavar = itr->second;

    itr = formula_map.find("depth");
    if (itr != formula_map.end()) _depthvar = itr->second;

    itr = formula_map.find("depth_c");
    if (itr != formula_map.end()) _depth_cvar = itr->second;

	vector <size_t> dims_tmp;
	vector <string> dimnames_tmp;
	if (_ncdfc->VariableExists(_svar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_svar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_svar);
	}
	else if (_ncdfc->VariableExists(_Cvar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_Cvar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_Cvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[0] = dims_tmp[0];
	_dimnames[0] = dimnames_tmp[0];

	if (_ncdfc->VariableExists(_etavar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_etavar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_etavar);
	}
	else if (_ncdfc->VariableExists(_depthvar)) {
		dims_tmp = _ncdfc->GetSpatialDims(_depthvar);
		dimnames_tmp = _ncdfc->GetSpatialDimNames(_depthvar);
	}
	else {
		_ok = false;
		return;
	}
	_dims[1] = dims_tmp[0];
	_dims[2] = dims_tmp[1];
	_dimnames[1] = dimnames_tmp[0];
	_dimnames[2] = dimnames_tmp[1];

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

	int fd = _ncdfc->OpenRead(0, _svar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_s, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_svar, mv)) {	// zero out any mv
		for (int i=0; i<nz; i++) {
			if (_s[i] == mv) _s[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _Cvar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_C, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_Cvar, mv)) {
		for (int i=0; i<nz; i++) {
			if (_C[i] == mv) _C[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _etavar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_eta, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_etavar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_eta[i] == mv) _eta[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _depthvar); if (fd<0) return(-1);
	rc = _ncdfc->Read(_depth, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);
	if (_ncdfc->GetMissingValue(_depthvar, mv)) {
		for (int i=0; i<nx*ny; i++) {
			if (_depth[i] == mv) _depth[i] = 0.0;
		}
	} 

	fd = _ncdfc->OpenRead(0, _depth_cvar); if (fd<0) return(-1);
	rc = _ncdfc->Read(&_depth_c, fd); if (rc<0) return(-1);
	rc = _ncdfc->Close(fd); if (rc<0) return(-1);

	_is_open = true;
	return(0);
}

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::Read(
	float *buf, int
) {
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
	float *slice, int fd
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

int NetCDFCFCollection::DerivedVar_ocean_s_coordinate_g2::SeekSlice(    int offset, int whence, int 
) {
    size_t nz = _dims[0];

    int slice;
    if (whence == 0) {
        slice = offset;
    }
    else if (whence == 1) {
        slice = _slice_num + offset;
    }
    else if (whence == 2) {
        slice = offset + nz - 1;
    }
    if (slice<0) slice = 0;
    if (slice>nz-1) slice = nz-1;

    _slice_num = slice;
    
    return(0);
}
