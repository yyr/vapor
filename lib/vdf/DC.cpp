#include <cassert>
#include <sstream>
#include "vapor/DC.h"

using namespace VAPoR;

DC::BaseVar::BaseVar(
	string name, std::vector <DC::Dimension> dimensions,
    string units, XType type, std::vector <bool> periodic
)  :
	_name(name),
	_dimensions(dimensions),
	_units(units),
	_type(type),
	_periodic(periodic) 
{
	_wname = "";
	_cratios.push_back(1);
	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() == 4) {
			_bs.push_back(1);
		}
		else {
			_bs.push_back(dimensions[i].GetLength());
		}
	}
}

bool DC::GetDimension(
    string name, size_t &length, int &axis
) const {
    length = 0;
    axis = 0;

	DC::Dimension dim;
	bool ok = GetDimension(name, dim);
	if (! ok) return(false);

	length = dim.GetLength();
	axis = dim.GetAxis();

    return(true);
}

vector <string> DC::GetDataVarNames(int ndim, bool spatial) const {
	vector <string> names, allnames;

	allnames = GetDataVarNames();

	for (int i=0; i<allnames.size(); i++) {
		DataVar dvar;
		bool ok = GetDataVarInfo(allnames[i], dvar);
		if (! ok) continue;

		vector <Dimension> dims = dvar.GetDimensions();

		// if spatial is true we ignore time dimensions
		//
		int myndim = dims.size();
		if (spatial && IsTimeVarying(allnames[i])) {
			myndim--;
		}
		if (myndim == ndim) {
			names.push_back(allnames[i]);
		}
	}
	return(names);
}

vector <string> DC::GetCoordVarNames(int ndim, bool spatial) const {
	vector <string> names, allnames;

	allnames = GetCoordVarNames();

	for (int i=0; i<allnames.size(); i++) {
		CoordVar cvar;
		bool ok = GetCoordVarInfo(allnames[i], cvar);
		if (! ok) continue;

		vector <Dimension> dims = cvar.GetDimensions();

		// if spatial is true we ignore time dimensions
		//
		int myndim = dims.size();
		if (spatial && IsTimeVarying(allnames[i])) {
			myndim--;
		}
		if (myndim == ndim) {
			names.push_back(allnames[i]);
		}
	}
	return(names);
}


bool DC::IsTimeVarying(string varname) const {

	BaseVar var;

	bool ok = GetBaseVarInfo(varname, var);
	if (! ok) return(false);

	// 
	// Now look for a time dimension. Theoretically we only need to 
	// check the last dimension name as they are supposed to be ordered
	//
	vector <DC::Dimension> dimensions = var.GetDimensions();
	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() == 3) return(true);
	}
	return(false);
}

bool DC::IsCompressed(string varname) const {

	BaseVar var;

	bool ok = GetBaseVarInfo(varname, var);
	if (! ok) return(false);

	return(var.IsCompressed());
}

int DC::GetNumTimeSteps(string varname) const {
	vector <DC::Dimension> dimensions;

	// Verify variable exists first and return error if not
	//
	DC::BaseVar var;
	bool status = GetBaseVarInfo(varname, var);
	if (! status) {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	 dimensions = var.GetDimensions();

	if (! IsTimeVarying(varname)) return(1); 

	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() == 3) {
			return((int) dimensions[i].GetLength());
		}
	}

	// Shouldn't ever get here
	//
	SetErrMsg("Internal error");
	return(-1);
}

int DC::GetCRatios(string varname, vector <size_t> &cratios) const {

	DC::BaseVar var;
	bool status = GetBaseVarInfo(varname, var);
	if (! status) {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}

	cratios = var.GetCRatios();
	return(0);
}

int DC::GetMapProjection(string varname, string &projstring) const {

	DataVar dvar;
	bool ok = GetDataVarInfo(varname, dvar);
	if (! ok) return(false);

	vector <string> cvarnames = dvar.GetCoordvars();

	string lonname, latname;
	for (int i=0; i<cvarnames.size(); i++) {
		CoordVar cvar;
		bool ok = GetCoordVarInfo(cvarnames[i], cvar);
		if (! ok) continue;

		if (cvar.GetAxis() == 0) lonname = cvarnames[i];
		if (cvar.GetAxis() == 1) latname = cvarnames[i];
	}

	return(GetMapProjection(lonname, latname, projstring));
}


bool DC::ParseDimensions(
    const vector <DC::Dimension> &dimensions,
    vector <size_t> &sdims, size_t &numts
) {
	sdims.clear();
	numts = 1;

	//
	// Make sure dimensions vector is valid
	//
	if (! (dimensions.size() >= 1 && dimensions.size() <= 4)) return(false);

	int axis = -1;
	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() <= axis) {
			return(false);
		}
		axis = dimensions[i].GetAxis();
	}

	vector <DC::Dimension> sdimensions = dimensions;
	if (sdimensions[sdimensions.size()-1].GetAxis() == 3) { // time varying
		numts = sdimensions[sdimensions.size()-1].GetLength();
		sdimensions.pop_back();	// remove time varying dimension
	}

	for (int i=0; i<sdimensions.size(); i++) {
		sdims.push_back(sdimensions[i].GetLength());
	}
	return(true);
}
