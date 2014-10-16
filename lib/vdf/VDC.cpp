#include <cassert>
#include <sstream>
#include "vapor/VDC.h"

using namespace VAPoR;


namespace {

void _compute_bs(
	const vector <VDC::Dimension> &dimensions, 
	const vector <size_t> &default_bs,
	vector <size_t> &bs
) {
	bs.clear();

	// Hack for 1D time varying where blocking makes no sense
	//
	if (dimensions.size() == 1 && dimensions[0].GetAxis() == 3) {
		bs.push_back(1);
		return;
	}

	// If the default block size exists for a dimension use it.
	// Otherwise set the bs to 1
	//
	for (int i=0; i<dimensions.size(); i++) {
		if (i<default_bs.size() && dimensions[i].GetAxis() != 3) {
			bs.push_back(default_bs[i]);
		}
		else {
			bs.push_back(1);	// slowest varying block size can be one
		}
	}

	assert(dimensions.size() == bs.size());
}

int _axis_block_length(
	const vector <VDC::Dimension> &dimensions, 
	const vector <size_t> &bs, int axis
) {
	assert (dimensions.size() == bs.size());

	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() == axis) return(bs[i]);
	}
	return(0);
}


};

VDC::VDC() {

	_master_path.clear();
	_mode = R;
	_defineMode = false;

	_bs.clear();
	for (int i=0; i<3; i++) _bs.push_back(64);

	_wname = "bior3.3";

	_cratios.clear();
	_cratios.push_back(500);
	_cratios.push_back(100);
	_cratios.push_back(10);
	_cratios.push_back(1);

	_periodic.clear();
	for (int i=0; i<3; i++) _periodic.push_back(false);

	_coordVars.clear();
	_dataVars.clear();
	_dimsMap.clear();
	_atts.clear();
	_newUniformVars.clear();
}

int VDC::Initialize(string path, AccessMode mode)
{
	_master_path = path;
	if (mode == R) {
		_defineMode = false;
	}
	else {
		_defineMode = true;
	}
	_mode = mode;

	int rc = _udunits.Initialize();
	if (rc<0) {
		SetErrMsg(
			"Failed to initialize udunits2 library : %s", 
			_udunits.GetErrMsg().c_str()
		);
		return(-1);
	}

	if (mode == R || mode == A) {
		return(_ReadMasterMeta());
	}
	return(0);
}

int VDC::SetCompressionBlock(
    vector <size_t> bs, string wname,
    vector <size_t> cratios
) {
	if (! cratios.size()) cratios.push_back(1);
	
	if (! _ValidCompressionBlock(bs, wname, cratios)) {
		SetErrMsg("Invalid compression settings");
		return(-1);
	}

	_bs = bs;
	_wname = wname;
	_cratios = cratios;

	return(0);
}

void VDC::GetCompressionBlock(
    vector <size_t> &bs, string &wname,
    vector <size_t> &cratios
) const {
	bs = _bs;
	wname = _wname;
	cratios = _cratios;
}

int VDC::DefineDimension(string name, size_t length, int axis) {
	if (! _defineMode) {
		SetErrMsg("Not in define mode");
		return(-1);
	}

	// Can't redefine existing dimension names in append mode
	//
	if (_mode == A) {
		if (_dimsMap.find(name) != _dimsMap.end()) {
			SetErrMsg("Dimension name %s already defined", name.c_str());
			return(-1);
		}
	}

	if (!_ValidDefineDimension(name, length, axis)) {
		return(-1);
	}

	Dimension dimension(name, length, axis);

	//
	// _dimsMap contains all defined dimensions for the VDC
	//
	_dimsMap[name] = dimension;

	//
	// A 1D coordinate variable is implicitly defined for each dimension
	//
	int status = DefineCoordVarUniform(name, name, "", axis, FLOAT, false);
	if (status < 0) {
		_dimsMap.erase(name);	// remove dimension definition
	}
	return(status);
	
}

bool VDC::GetDimension(
	string name, size_t &length, int &axis
) const {
	length = 0;
	axis = 0;

	map <string, Dimension>::const_iterator itr = _dimsMap.find(name);
	if (itr == _dimsMap.end()) return(false);

	length = itr->second.GetLength();
	axis = itr->second.GetAxis();
	return(true);
}

bool VDC::GetDimension(string name, Dimension &dimension) const {
	dimension.Clear();

	map <string, Dimension>::const_iterator itr = _dimsMap.find(name);
	if (itr == _dimsMap.end()) return(false);

	dimension = itr->second;
	return(true);
}

vector <string> VDC::GetDimensionNames() const {
	vector <string> dimnames;
	std::map <string, Dimension>::const_iterator itr = _dimsMap.begin();
	for (;itr!=_dimsMap.end(); ++itr) {
		dimnames.push_back(itr->first);
	}
	return(dimnames);
}


int VDC::DefineCoordVar(
	string varname, vector <string> dimnames,
	string units, int axis, XType type, bool compressed
) {
	if (! _defineMode) {
		SetErrMsg("Not in define mode");
		return(-1);
	}

	if (_mode == A) {
		if (_coordVars.find(varname) != _coordVars.end()) {
			SetErrMsg("Variable \"%s\" already defined", varname.c_str());
			return(-1);
		} 
	}

	if (axis == 3 && units.empty()) units = "seconds"; 

	if (! _ValidDefineCoordVar(varname,dimnames,units,axis,type,compressed)) {
		return(-1);
	}

	//
	// Make sure dimensions were previously defined
	//
	vector <Dimension> dimensions;
	for (int i=0; i<dimnames.size(); i++) {
		Dimension dimension;
		VDC::GetDimension(dimnames[i], dimension);
		assert(! dimension.GetName().empty()); 
		dimensions.push_back(dimension);
	}

	// Determine block size
	//
	vector <size_t> bs;
	_compute_bs(dimensions, _bs, bs);
		
	vector <size_t> cratios(1,1);
	string wname;
	if (compressed) {
		cratios = _cratios;
		wname = _wname;
	}
	CoordVar coordvar(
		varname, dimensions, units, type, bs, wname, 
		cratios, _periodic, axis, false
	);

	// _coordVars contains a table of all the coordinate variables
	//
	_coordVars[varname] = coordvar;

	return(0);

}

int VDC::DefineCoordVarUniform(
	string varname, string dimname,
	string units, int axis, XType type, bool compressed
) {
	if (! _defineMode) {
		SetErrMsg("Not in define mode");
		return(-1);
	}

	if (_mode == A) {
		if (_coordVars.find(varname) != _coordVars.end()) {
			SetErrMsg("Variable \"%s\" already defined", varname.c_str());
			return(-1);
		} 
	}

	if (axis == 3 && units.empty()) units = "seconds"; 

	vector <string> dimnames;
	dimnames.push_back(dimname);
	if (! _ValidDefineCoordVar(varname,dimnames,units,axis,type,compressed)) {
		return(-1);
	}

    vector <Dimension> dimensions;
	Dimension dimension;
	VDC::GetDimension(dimname, dimension);
	assert(! dimension.GetName().empty());
	dimensions.push_back(dimension);

	// Determine block size
	//
	vector <size_t> bs;
	_compute_bs(dimensions, _bs, bs);

	vector <size_t> cratios(1,1);
	string wname;
	if (compressed) {
		cratios = _cratios;
		wname = _wname;
	}
	CoordVar coordvar(
		varname, dimensions, units, type, bs, wname, 
		cratios, _periodic, axis, true
	);

	_coordVars[varname] = coordvar;

	// Keep track of any uniform variables that get defined
	//
	_newUniformVars.push_back(varname);

	return(0);
}

bool VDC::GetCoordVarInfo(
    string varname, vector <string> &dimnames,
    string &units, int &axis, XType &type, bool &compressed, bool &uniform
) const {
	dimnames.clear();
	units.erase();

	map <string, CoordVar>::const_iterator itr = _coordVars.find(varname);

	if (itr == _coordVars.end()) return(false);

	vector <VDC::Dimension> dimensions = itr->second.GetDimensions();
	for (int i=0; i<dimensions.size(); i++) {
		dimnames.push_back(dimensions[i].GetName());
	}
	units = itr->second.GetUnits();
	axis = itr->second.GetAxis();
	type = itr->second.GetXType();
	compressed = itr->second.IsCompressed();
	uniform = itr->second.GetUniform();
	return(true);
}

bool VDC::GetCoordVarInfo(string varname, VDC::CoordVar &cvar) const {

	map <string, CoordVar>::const_iterator itr = _coordVars.find(varname);
	if (itr == _coordVars.end()) return(false);

	cvar = itr->second;
	return(true);
}

bool VDC::GetBaseVarInfo(string varname, VDC::BaseVar &var) const {

	map <string, CoordVar>::const_iterator itr1 = _coordVars.find(varname);
	if (itr1 != _coordVars.end()) {
		var = itr1->second;
		return(true);
	}

	map <string, DataVar>::const_iterator itr2 = _dataVars.find(varname);
	if (itr2 != _dataVars.end()) { 
		var = itr2->second;
		return(true);
	}
	return(false);
}

int VDC::DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
	string units, XType type, bool compressed
) {
	if (! _defineMode) {
		SetErrMsg("Not in define mode");
		return(-1);
	}

	if (_mode == A) {
		if (_dataVars.find(varname) != _dataVars.end()) {
			SetErrMsg("Variable \"%s\" already defined", varname.c_str());
			return(-1);
		} 
	}

	if (! _ValidDefineDataVar(
		varname,dimnames,coordvars, units, type,compressed)) {

		return(-1);
	}

	//
	// Dimensions must have previosly been defined
	//
	vector <Dimension> dimensions;
	for (int i=0; i<dimnames.size(); i++) {
		Dimension dimension;
		VDC::GetDimension(dimnames[i], dimension);
		assert(! dimension.GetName().empty());
		dimensions.push_back(dimension);
	}

	// Determine block size
	//
	vector <size_t> bs;
	_compute_bs(dimensions, _bs, bs);

	vector <size_t> cratios(1,1);
	string wname;
	if (compressed) {
		cratios = _cratios;
		wname = _wname;
	}
	DataVar datavar(
		varname, dimensions, units, type, bs, wname, 
		cratios, _periodic, coordvars
	);

	_dataVars[varname] = datavar;

	return(0);

}

int VDC::DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
	string units, XType type, bool compressed, double missing_value
) {
	if (! _defineMode) {
		SetErrMsg("Not in define mode");
		return(-1);
	}

	if (_mode == A) {
		if (_dataVars.find(varname) != _dataVars.end()) {
			SetErrMsg("Variable \"%s\" already defined", varname.c_str());
			return(-1);
		} 
	}

	if (! _ValidDefineDataVar(
		varname,dimnames,coordvars, units, type,compressed)) {

		return(-1);
	}

	//
	// Dimensions must have previosly been defined
	//
	vector <Dimension> dimensions;
	for (int i=0; i<dimnames.size(); i++) {
		Dimension dimension;
		VDC::GetDimension(dimnames[i], dimension);
		assert(! dimension.GetName().empty());
		dimensions.push_back(dimension);
	}

	// Determine block size
	//
	vector <size_t> bs;
	_compute_bs(dimensions, _bs, bs);

	vector <size_t> cratios(1,1);
	string wname;
	if (compressed) {
		cratios = _cratios;
		wname = _wname;
	}
	DataVar datavar(
		varname, dimensions, units, type, bs, wname, 
		cratios, _periodic, coordvars, 
		missing_value
	);

	_dataVars[varname] = datavar;

	return(0);

}

bool VDC::GetDataVarInfo(
	string varname, vector <string> &dimnames, vector <string> &coordvars,
	string &units, XType &type, bool &compressed,
	bool &has_missing, double &missing_value
) const {
	dimnames.clear();
	units.erase();

	map <string, DataVar>::const_iterator itr = _dataVars.find(varname);

	if (itr == _dataVars.end()) return(false);

	vector <VDC::Dimension> dimensions = itr->second.GetDimensions();
	for (int i=0; i<dimensions.size(); i++) {
		dimnames.push_back(dimensions[i].GetName());
	}
	coordvars = itr->second.GetCoordvars();
	units = itr->second.GetUnits();
	type = itr->second.GetXType();
	compressed = itr->second.IsCompressed();
	has_missing = itr->second.GetHasMissing();
	missing_value = itr->second.GetMissingValue();
	return(true);
}

bool VDC::GetDataVarInfo(string varname, VDC::DataVar &datavar) const {

	map <string, DataVar>::const_iterator itr = _dataVars.find(varname);

	if (itr == _dataVars.end()) return(false);

	datavar = itr->second;
	return(true);
}

vector <string> VDC::GetDataVarNames() const {
	vector <string> names;

	map <string, DataVar>::const_iterator itr;
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {
		names.push_back(itr->first);
	}
	return(names);
}

vector <string> VDC::GetDataVarNames(int ndim, bool spatial) const {
	vector <string> names;

	map <string, DataVar>::const_iterator itr;
	for (itr = _dataVars.begin(); itr != _dataVars.end(); ++itr) {

		// if spatial is true we ignore time dimensions
		//
		int myndim = itr->second.GetDimensions().size();
		if (spatial && VDC::IsTimeVarying(itr->first)) {
			myndim--;
		}
		if (myndim == ndim) {
			names.push_back(itr->first);
		}
	}
				
	return(names);
}

vector <string> VDC::GetCoordVarNames() const {
	vector <string> names;

	map <string, CoordVar>::const_iterator itr;
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		names.push_back(itr->first);
	}
	return(names);
}

vector <string> VDC::GetCoordVarNames(int ndim, bool spatial) const {
	vector <string> names;

	map <string, CoordVar>::const_iterator itr;
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {

		// if spatial is true we ignore time dimensions
		//
		if (spatial) {
			if (VDC::IsTimeVarying(itr->first)) {
				if (itr->second.GetDimensions().size() == ndim - 1) {
					names.push_back(itr->first);
				}
			}
			else {
				if (itr->second.GetDimensions().size() == ndim) {
					names.push_back(itr->first);
				}
			}
		}
		else {
			if (itr->second.GetDimensions().size() == ndim) {
				names.push_back(itr->first);
			}
		}
	}
				
	return(names);
}

bool VDC::IsTimeVarying(string name) const {

	const BaseVar *vptr = NULL;

	// First the coordinate variable. Could be a Coordinate or Data variable, or
	// may not exist
	//
	map <string, CoordVar>::const_iterator itr1;
	itr1 = _coordVars.find(name);
	if (itr1 != _coordVars.end()) vptr = &(itr1->second);

	if (! vptr) {	// not a coordinate variable
		map <string, DataVar>::const_iterator itr2;
		itr2 = _dataVars.find(name);
		if (itr2 != _dataVars.end()) vptr = &(itr2->second);
	}

	if (! vptr) return (false);	// variable doesn't exist

	// 
	// Now look for a time dimension. Theoretically we only need to 
	// check the last dimension name as they are supposed to be ordered
	//
	vector <VDC::Dimension> dimensions = vptr->GetDimensions();
	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() == 3) return(true);
	}
	return(false);
}

bool VDC::IsCompressed(string varname) const {

	if (VDC::IsDataVar(varname)) {
		VDC::DataVar var;
		bool ok = VDC::GetDataVarInfo(varname, var);
		if (ok) return(var.IsCompressed());
	}
	else if (VDC::IsCoordVar(varname)) {
		VDC::CoordVar cvar;
		bool ok = VDC::GetCoordVarInfo(varname, cvar);
		if (ok) return(cvar.IsCompressed());
	}
	return(false);	// not found
}

int VDC::GetNumTimeSteps(string varname) const {
	vector <VDC::Dimension> dimensions;

	// Verify variable exists first and return error if not
	//
	VDC::BaseVar var;
	bool status = VDC::GetBaseVarInfo(varname, var);
	if (! status) {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	 dimensions = var.GetDimensions();

	if (! VDC::IsTimeVarying(varname)) return(1); 

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

int VDC::GetNumRefLevels(string varname) const {

	VDC::BaseVar var;
	bool status = VDC::GetBaseVarInfo(varname, var);
	if (! status) {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}

	if (! var.IsCompressed()) return(1);

	size_t nlevels, maxcratio;
	CompressionInfo(var.GetBS(), var.GetWName(), nlevels, maxcratio);

	return(nlevels);
}

int VDC::GetCRatios(string varname, vector <size_t> &cratios) const {

	VDC::BaseVar var;
	bool status = VDC::GetBaseVarInfo(varname, var);
	if (! status) {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}

	cratios = var.GetCRatios();
	return(0);
}


int VDC::PutAtt(
    string varname, string attname, XType type, const vector <double> &values
) {
	Attribute attr(attname, type, values);

	if (varname.empty()) {
		_atts[attname] = attr;
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::iterator itr = _dataVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		atts[attname] = attr;
		itr->second.SetAttributes(atts);
	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::iterator itr = _coordVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		atts[attname] = attr;
		itr->second.SetAttributes(atts);
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	return(0);
}

int VDC::PutAtt(
    string varname, string attname, XType type, const vector <long> &values
) {
	Attribute attr(attname, type, values);

	if (varname.empty()) {
		_atts[attname] = attr;
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::iterator itr = _dataVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		atts[attname] = attr;
		itr->second.SetAttributes(atts);
	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::iterator itr = _coordVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		atts[attname] = attr;
		itr->second.SetAttributes(atts);
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	return(0);
}

int VDC::PutAtt(
    string varname, string attname, XType type, const string &values
) {
	Attribute attr(attname, type, values);

	if (varname.empty()) {
		_atts[attname] = attr;
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::iterator itr = _dataVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		atts[attname] = attr;
		itr->second.SetAttributes(atts);
	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::iterator itr = _coordVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		atts[attname] = attr;
		itr->second.SetAttributes(atts);
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	return(0);
}

int VDC::GetAtt(
    string varname, string attname, vector <double> &values
) const {
	values.clear();

	if (varname.empty() && (_atts.find(attname) != _atts.end())) {
		map <string, Attribute>::const_iterator itr = _atts.find(attname);
		assert(itr != _atts.end());
		const Attribute &attr = itr->second;
		attr.GetValues(values);
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::const_iterator itr = _dataVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		map <string, Attribute>::const_iterator itr1 = atts.find(attname);
		if (itr1 == atts.end()) {
			SetErrMsg("Undefined variable attribute name : %s",attname.c_str());
			return(-1);
		}
		const Attribute &attr = itr1->second;
		attr.GetValues(values);

	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::const_iterator itr = _coordVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		map <string, Attribute>::const_iterator itr1 = atts.find(attname);
		if (itr1 == atts.end()) {
			SetErrMsg("Undefined variable attribute name : %s",attname.c_str());
			return(-1);
		}
		const Attribute &attr = itr1->second;
		attr.GetValues(values);
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	return(0);
}

int VDC::GetAtt(
    string varname, string attname, vector <long> &values
) const {
	values.clear();

	if (varname.empty() && (_atts.find(attname) != _atts.end())) {
		map <string, Attribute>::const_iterator itr = _atts.find(attname);
		assert(itr != _atts.end());
		const Attribute &attr = itr->second;
		attr.GetValues(values);
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::const_iterator itr = _dataVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		map <string, Attribute>::const_iterator itr1 = atts.find(attname);
		if (itr1 == atts.end()) {
			SetErrMsg("Undefined variable attribute name : %s",attname.c_str());
			return(-1);
		}
		const Attribute &attr = itr1->second;
		attr.GetValues(values);

	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::const_iterator itr = _coordVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		map <string, Attribute>::const_iterator itr1 = atts.find(attname);
		if (itr1 == atts.end()) {
			SetErrMsg("Undefined variable attribute name : %s",attname.c_str());
			return(-1);
		}
		const Attribute &attr = itr1->second;
		attr.GetValues(values);
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	return(0);
}

int VDC::GetAtt(
    string varname, string attname, string &values
) const {
	values.clear();

	if (varname.empty() && (_atts.find(attname) != _atts.end())) {
		map <string, Attribute>::const_iterator itr = _atts.find(attname);
		assert(itr != _atts.end());
		const Attribute &attr = itr->second;
		attr.GetValues(values);
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::const_iterator itr = _dataVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		map <string, Attribute>::const_iterator itr1 = atts.find(attname);
		if (itr1 == atts.end()) {
			SetErrMsg("Undefined variable attribute name : %s",attname.c_str());
			return(-1);
		}
		const Attribute &attr = itr1->second;
		attr.GetValues(values);

	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::const_iterator itr = _coordVars.find(varname);
		map <string, Attribute> atts = itr->second.GetAttributes();
		map <string, Attribute>::const_iterator itr1 = atts.find(attname);
		if (itr1 == atts.end()) {
			SetErrMsg("Undefined variable attribute name : %s",attname.c_str());
			return(-1);
		}
		const Attribute &attr = itr1->second;
		attr.GetValues(values);
	}
	else {
		SetErrMsg("Undefined variable name : %s", varname.c_str());
		return(-1);
	}
	return(0);
}

vector <string> VDC::GetAttNames(
    string varname
) const {
	vector <string> attnames;

	std::map <string, Attribute>::const_iterator itr;

	if (varname.empty()) {	// global attributes
		for (itr = _atts.begin(); itr != _atts.end(); ++itr) {
			attnames.push_back(itr->first);
		}
	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::const_iterator itr1 = _dataVars.find(varname);
		map <string, Attribute> atts = itr1->second.GetAttributes();
		for (itr = atts.begin(); itr != atts.end(); ++itr) {
			attnames.push_back(itr->first);
		}
	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::const_iterator itr1 = _coordVars.find(varname);
		map <string, Attribute> atts = itr1->second.GetAttributes();
		for (itr = atts.begin(); itr != atts.end(); ++itr) {
			attnames.push_back(itr->first);
		}
	} 
	return(attnames);
}

VDC::XType VDC::GetAttType(
    string varname,
	string attname
) const {

	std::map <string, Attribute>::const_iterator itr;

	if (varname.empty()) {	// global attributes
		itr = _atts.find(attname);
		if (itr == _atts.end()) return(INVALID);

		return(itr->second.GetXType());

	} else if (_dataVars.find(varname) != _dataVars.end()) {
		map <string, DataVar>::const_iterator itr1 = _dataVars.find(varname);
		map <string, Attribute> atts = itr1->second.GetAttributes();
		itr = atts.find(attname);
		if (itr == _atts.end()) return(INVALID);

		return(itr->second.GetXType());

	} else if (_coordVars.find(varname) != _coordVars.end()) {
		map <string, CoordVar>::const_iterator itr1 = _coordVars.find(varname);
		map <string, Attribute> atts = itr1->second.GetAttributes();
		itr = atts.find(attname);
		if (itr == _atts.end()) return(INVALID);

		return(itr->second.GetXType());
	} 
	return(INVALID);
}

bool VDC::ParseDimensions(
    const vector <VDC::Dimension> &dimensions,
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

	vector <VDC::Dimension> sdimensions = dimensions;
	if (sdimensions[sdimensions.size()-1].GetAxis() == 3) { // time varying
		numts = sdimensions[sdimensions.size()-1].GetLength();
		sdimensions.pop_back();	// remove time varying dimension
	}

	for (int i=0; i<sdimensions.size(); i++) {
		sdims.push_back(sdimensions[i].GetLength());
	}
	return(true);
}

int VDC::EndDefine() {
	if (! _defineMode) return(0); 


	if (_mode == R) return(0);

	if (_master_path.empty()) {
		// Initialized() not called
		//
		SetErrMsg("Master file not specified");
		return(-1);
	} 

	if (_WriteMasterMeta() < 0)  return(-1);

	_defineMode = false;


	// For any Uniform coordinate variables that were defined go ahead
	// and give them default values
	//
	for (int i=0; i<_newUniformVars.size(); i++) {
		VDC::CoordVar cvar;
		VDC::GetCoordVarInfo(_newUniformVars[i], cvar);

		vector <VDC::Dimension> dims = cvar.GetDimensions();
		assert(dims.size() == 1);

		size_t l = dims[0].GetLength();

		float *buf = new float[l];
		for (size_t j=0; j<l; j++) buf[j] = (float) j;

		int rc = PutVar(_newUniformVars[i], -1, buf);
		if (rc<0) {
			delete [] buf;
			return(rc);
		}
		delete [] buf;

	}

	return(0);
}


VDC::Attribute::Attribute(
	string name, XType type, const vector <float> &values
) {
	_name = name;
	_type = type;
	_values.clear();
	Attribute::SetValues(values);
}

void VDC::Attribute::SetValues(const vector <float> &values) 
{
	_values.clear();
	vector <double> dvec;
    for (int i=0; i<values.size(); i++) {
		dvec.push_back((double) values[i]);
	}
	VDC::Attribute::SetValues(dvec);
}

VDC::Attribute::Attribute(
	string name, XType type, const vector <double> &values
) {
	_name = name;
	_type = type;
	_values.clear();
	Attribute::SetValues(values);
}

void VDC::Attribute::SetValues(const vector <double> &values) 
{
	_values.clear();
    for (int i=0; i<values.size(); i++) {
        podunion pod;
        if (_type == FLOAT) {
            pod.f = (float) values[i];
        }
        else if (_type == DOUBLE) {
            pod.d = (double) values[i];
        }
        else if (_type == INT32) {
            pod.i = (int) values[i];
        }
        else if (_type == INT64) {
            pod.l = (int) values[i];
        }
        else if (_type == TEXT) {
            pod.c = (char) values[i];
        }
        _values.push_back(pod);
    }
}

VDC::Attribute::Attribute(
	string name, XType type, const vector <int> &values
) {
	_name = name;
	_type = type;
	_values.clear();
	Attribute::SetValues(values);
}

void VDC::Attribute::SetValues(const vector <int> &values) 
{
	_values.clear();
	vector <long> lvec;
    for (int i=0; i<values.size(); i++) {
		lvec.push_back((long) values[i]);
	}
	VDC::Attribute::SetValues(lvec);
}

VDC::Attribute::Attribute(
	string name, XType type, const vector <long> &values
) {
	_name = name;
	_type = type;
	_values.clear();
	Attribute::SetValues(values);
}

void VDC::Attribute::SetValues(const vector <long> &values) 
{
	_values.clear();

    for (int i=0; i<values.size(); i++) {
        podunion pod;
        if (_type == FLOAT) {
            pod.f = (float) values[i];
        }
        else if (_type == DOUBLE) {
            pod.d = (double) values[i];
        }
        else if (_type == INT32) {
            pod.i = (int) values[i];
        }
        else if (_type == INT64) {
            pod.l = (long) values[i];
        }
        else if (_type == TEXT) {
            pod.c = (char) values[i];
        }
        _values.push_back(pod);
    }
}

VDC::Attribute::Attribute(
	string name, XType type, const string &values
) {
	_name = name;
	_type = type;
	_values.clear();
	Attribute::SetValues(values);
}

void VDC::Attribute::SetValues(const string &values) 
{
	_values.clear();
    for (int i=0; i<values.size(); i++) {
        podunion pod;
        if (_type == FLOAT) {
            pod.f = (float) values[i];
        }
        else if (_type == DOUBLE) {
            pod.d = (double) values[i];
        }
        else if (_type == INT32) {
            pod.i = (int) values[i];
        }
        else if (_type == INT64) {
            pod.l = (long) values[i];
        }
        else if (_type == TEXT) {
            pod.c = (char) values[i];
        }
        _values.push_back(pod);
    }
}

void VDC::Attribute::GetValues(
	vector <float> &values
) const {
	values.clear();

	vector <double> dvec;
	VDC::Attribute::GetValues(dvec);
    for (int i=0; i<dvec.size(); i++) {
		values.push_back((float) dvec[i]);
	}
}

void VDC::Attribute::GetValues(
	vector <double> &values
) const {
	values.clear();

    for (int i=0; i<_values.size(); i++) {
        podunion pod = _values[i];
        if (_type == FLOAT) {
			values.push_back((double) pod.f);
        }
        else if (_type == DOUBLE) {
			values.push_back((double) pod.d);
        }
        else if (_type == INT32) {
			values.push_back((double) pod.i);
        }
        else if (_type == INT64) {
			values.push_back((double) pod.l);
        }
        else if (_type == TEXT) {
			values.push_back((double) pod.c);
        }
    }
}

void VDC::Attribute::GetValues(
	vector <int> &values
) const {
	values.clear();

	vector <long> lvec;
	VDC::Attribute::GetValues(lvec);
    for (int i=0; i<lvec.size(); i++) {
		values.push_back((int) lvec[i]);
	}
}

void VDC::Attribute::GetValues(
	vector <long> &values
) const {
	values.clear();

    for (int i=0; i<_values.size(); i++) {
        podunion pod = _values[i];
        if (_type == FLOAT) {
			values.push_back((long) pod.f);
        }
        else if (_type == DOUBLE) {
			values.push_back((long) pod.d);
        }
        else if (_type == INT32) {
			values.push_back((long) pod.i);
        }
        else if (_type == INT64) {
			values.push_back((long) pod.l);
        }
        else if (_type == TEXT) {
			values.push_back((long) pod.c);
        }
    }
}

void VDC::Attribute::GetValues(
	string &values
) const {
	values.clear();

    for (int i=0; i<_values.size(); i++) {
        podunion pod = _values[i];
        if (_type == FLOAT) {
			values += (char) pod.f;
        }
        else if (_type == DOUBLE) {
			values += (char) pod.d;
        }
        else if (_type == INT32) {
			values += (char) pod.i;
        }
        else if (_type == INT64) {
			values += (char) pod.l;
        }
        else if (_type == TEXT) {
			values += (char) pod.c;
        }
    }
}


bool VDC::_ValidDefineDimension(string name, size_t length, int axis) const {

	if (length < 1) {
		SetErrMsg("Dimension must be of length 1 or more");
		return(false);
	}

	if (axis<0 || axis>3) {
		SetErrMsg("Invalid axis specification : %d", axis);
		return(false);
	}

	return(true);
}

bool VDC::_ValidDefineCoordVar(
	string varname, vector <string> dimnames,
	string units, int axis, XType type, bool compressed
) const {

	if (dimnames.size() > 4) {
		SetErrMsg("Invalid number of dimensions");
		return(false);
	}

	if (axis == 3 && dimnames.size() != 1) {
		SetErrMsg("Time coordinate variables must have exactly one dimension");
		return(false);
	}

	for (int i=0; i<dimnames.size(); i++) {
		if (_dimsMap.find(dimnames[i]) == _dimsMap.end()) {
			SetErrMsg("Dimension \"%s\" not defined", dimnames[i].c_str());
			return(false);
		}
	}

	if (axis<0 || axis>3) {
		SetErrMsg("Invalid axis specification : %d", axis);
		return(false);
	}

	if (! units.empty() && ! _udunits.ValidUnit(units)) {
		SetErrMsg("Unrecognized units specification : %s ", units.c_str());
		return(false);
	}

	if (compressed && type != FLOAT) {
		SetErrMsg("Only FLOAT data supported with compressed variables");
		return(false);
	}

	//
	// IF this is a dimension coordinate variable (a variable with
	// the same name as any dimension) then the dimensions, axis, 
	// and compression cannot be changed.
	//
	map <string, Dimension>::const_iterator itr = _dimsMap.find(varname);
	if (itr != _dimsMap.end()) { 
		if (dimnames.size() != 1 || varname.compare(dimnames[0]) != 0) {
			SetErrMsg("Invalid dimension coordinate variable definition");
			return(false);
		}
		if (itr->second.GetAxis() != axis) {
			SetErrMsg("Invalid dimension coordinate variable definition");
			return(false);
		}
		if (compressed) {
			SetErrMsg("Invalid dimension coordinate variable definition");
			return(false);
		}
	}

	// 
	// If multidimensional the dimensions must be ordered X, Y, Z, T
	//
	if (dimnames.size() > 1) {
		int axis = -1;
		for (int i=0; i<dimnames.size(); i++) {
			itr = _dimsMap.find(dimnames[i]);	
			assert(itr != _dimsMap.end());	// already checked for existance
			if (itr->second.GetAxis() <= axis) {
				SetErrMsg("Dimensions must be ordered X, Y, Z, T");
				return(false);
			}
			axis = itr->second.GetAxis();
		}
	}
		
	return(true);
}

bool VDC::_ValidCompressionBlock(
    vector <size_t> bs, string wname, 
    vector <size_t> cratios
) const {
	for (int i=0; i<cratios.size(); i++) {
		if (cratios[i] < 1) return(false);
	}

	size_t nlevels, maxcratio;
	bool status = CompressionInfo(
		bs, wname, nlevels, maxcratio
	);
	if (! status) return(false);
	
	if (! wname.empty()) { 
		for (int i=0; i<cratios.size(); i++) {
			if (cratios[i] > maxcratio) return(false);
		}
	}
	return(true);
}

bool VDC::_valid_blocking(
	const vector <VDC::Dimension> &dimensions,
	const vector <string> &coordvars,
	const vector <size_t> &bs
) const {
	assert(dimensions.size() == coordvars.size());

	for (int i=0; i<dimensions.size(); i++) {
		int axis = dimensions[i].GetAxis();
		string cvarname = coordvars[i];

		map <string, CoordVar>::const_iterator itr = _coordVars.find(cvarname);	
		assert(itr != _coordVars.end());

		const CoordVar &cvar = itr->second;

		size_t bs0 = _axis_block_length(dimensions, bs, axis);

		size_t bs1 = _axis_block_length(
			cvar.GetDimensions(), cvar.GetBS(), axis
		);
		if (bs1 == 0) continue;	// dimension not defined for this axis

		if (bs0 != bs1) return(false);
	}
	return(true);
}
	

bool VDC::_ValidDefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
	string units, XType type, bool compressed
) const {

	if (dimnames.size() > 4) {
		SetErrMsg("Invalid number of dimensions");
		return(false);
	}

	if (dimnames.size() != coordvars.size()) {
		SetErrMsg("Number of dimensions and coordinate vars don't match");
		return(false);
	}

	vector <VDC::Dimension> dimensions;
	for (int i=0; i<dimnames.size(); i++) {
		std::map <string, Dimension>::const_iterator itr;
		if ((itr = _dimsMap.find(dimnames[i])) == _dimsMap.end()) {
			SetErrMsg("Dimension \"%s\" not defined", dimnames[i].c_str());
			return(false);
		}
		dimensions.push_back(itr->second);
	}

	for (int i=0; i<coordvars.size(); i++) {
		if (_coordVars.find(coordvars[i]) == _coordVars.end()) {
			SetErrMsg("Coordinate var \"%s\" not defined", coordvars[i].c_str());
			return(false);
		}
	}

	if (! units.empty() && ! _udunits.ValidUnit(units)) {
		SetErrMsg("Unrecognized units specification : %s", units.c_str());
		return(false);
	}

	if (compressed && type != FLOAT) {
		SetErrMsg("Only FLOAT data supported with compressed variables");
		return(false);
	}

	// 
	// If multidimensional the dimensions and coord names must be 
	// ordered X, Y, Z, T
	//
	if (dimnames.size() > 1) {
		int axis = -1;
		for (int i=0; i<dimensions.size(); i++) {
			if (dimensions[i].GetAxis() <= axis) {
				SetErrMsg("Dimensions must be ordered X, Y, Z, T");
				return(false);
			}
			axis = dimensions[i].GetAxis();
		}
	}

	if (coordvars.size() > 1) {
		map <string, CoordVar>::const_iterator itr;
		int axis = -1;
		for (int i=0; i<coordvars.size(); i++) {
			itr = _coordVars.find(coordvars[i]);	
			assert(itr != _coordVars.end());	// already checked for existance
			if (itr->second.GetAxis() <= axis) {
				SetErrMsg("Dimensions must be ordered X, Y, Z, T");
				return(false);
			}
			axis = itr->second.GetAxis();
		}
	}

	// Determine block size
	//
	vector <size_t> bs;
	_compute_bs(dimensions, _bs, bs);

	// Data and coordinate variables must have same blocking along
	// each axis
	//
	if (! _valid_blocking(dimensions, coordvars, bs)) {
		SetErrMsg("Coordinate and data variables must have same blocking");
		return(false);
	}

#ifdef	DEAD
	//
	// Lastly, the dimesion names of each coordinate variable must be a 
	// subset of the variable's dimension names
	//
	for (int i=0; i<coordvars.size(); i++) {
		map <string, CoordVar>::const_iterator itr = _coordVars.find(coordvars[i]);
		assert(itr != _coordVars.end());	// already checked for existance

		for (int j=0; j<itr->second.GetDimensions().size(); j++) {
			string dimname = itr->second.GetDimensions()[j].GetName();
			if (find(dimnames.begin(),dimnames.end(),dimname)==dimnames.end()) {
				SetErrMsg(
					"Coordinate variable dims must be a subset of data variable"
				);
				return(false);
			}
		}
	}
#else
	for (int i=0; i<coordvars.size(); i++) {
		map <string, CoordVar>::const_iterator itr = _coordVars.find(coordvars[i]);
		assert(itr != _coordVars.end());	// already checked for existance
		vector <VDC::Dimension> cdimensions = itr->second.GetDimensions();
		bool match = false;
		for (int j=0; j<cdimensions.size(); j++) {
			for (int k=0; k<dimensions.size(); k++) {
				if (cdimensions[j].GetLength() == dimensions[k].GetLength() &&
					cdimensions[j].GetAxis() == dimensions[k].GetAxis()) {

					match = true;
					break;
				}
			}
		}
		if (! match) {
			SetErrMsg(
				"Coordinate variable dims must be a subset of data variable"
			);
			return(false);
		}
	}



#endif
		
	return(true);
}


namespace VAPoR {

std::ostream &operator<<(
	std::ostream &o, const VDC::Dimension &d
) {
	o << "  Dimension" << endl;
	o << "   Name: " << d._name << endl;
	o << "   Length: " << d._length << endl;
	o << "   Axis: " << d._axis << endl;

	return(o);
}

std::ostream &operator<<(std::ostream &o, const VDC::Attribute &a) {
	o << "  Attribute:" << endl;
	o << "   Name: " << a._name << endl;
	o << "   Type: " << a._type << endl;

	o << "   Values: ";
	for (int i=0; i<a._values.size(); i++) {
		VDC::Attribute::podunion p = a._values[i];
		if (a._type == VDC::FLOAT) {
			o << p.f;
		}
		else if (a._type == VDC::DOUBLE) {
			o << p.d;
		}
		else if (a._type == VDC::INT32) {
			o << p.i;
		}
		else if (a._type == VDC::INT64) {
			o << p.l;
		}
		if (a._type == VDC::TEXT) {
			o << p.c;
		}
		o << " ";
	}
	o << endl;
	return(o);
}

std::ostream &operator<<(std::ostream &o, const VDC::BaseVar &var) {

	o << "  BaseVar" << endl;
	o << "   Name: " << var._name << endl;
	o << "   Dimensions: " << endl;
	for (int i=0; i<var._dimensions.size(); i++) {
		o << var._dimensions[i] << endl;
	}
	o << "   Units: " << var._units << endl;
	o << "   XType: " << var._type << endl;
	o << "   Compressed: " << (var._cratios.size() > 0) << endl;
	o << "   WName: " << var._wname << endl;
	o << "   CRatios: ";
	for (int i=0; i<var._cratios.size(); i++) {
		o << var._cratios[i] << " ";
	}
	o << endl;
	o << "   Block Size: ";
	for (int i=0; i<var._bs.size(); i++) {
		o << var._bs[i] << " ";
	}
	o << endl;
	o << "   Periodic: ";
	for (int i=0; i<var._periodic.size(); i++) {
		o << var._periodic[i] << " ";
	}
	o << endl;

	std::map <string, VDC::Attribute>::const_iterator itr;
	for (itr = var._atts.begin(); itr != var._atts.end(); ++itr) {
		o << itr->second;
	}

	return(o);
}

std::ostream &operator<<(std::ostream &o, const VDC::CoordVar &var) {

	o << "  CoordVar" << endl;
	o << "   Axis: " << var._axis << endl;
	o << "   Uniform: " << var._uniform << endl;

	o << (VDC::BaseVar) var;

	return(o);
}

std::ostream &operator<<(std::ostream &o, const VDC::DataVar &var) {

	o << "  DataVar" << endl;
	o << "   CoordVars: ";
	for (int i=0; i<var._coordvars.size(); i++) {
		o << var._coordvars[i] << " ";
	}
	o << endl;
	o << "   HasMissing: " << var._has_missing << endl;
	o << "   MissingValue: " << var._missing_value << endl;

	o << (VDC::BaseVar) var;

	return(o);
}


std::ostream &operator<<(std::ostream &o, const VDC &vdc) {

	o << "VDC" << endl;
	o << " MasterPath: " << vdc._master_path << endl;
	o << " AccessMode: " << vdc._mode << endl;
	o << " DefineMode: " << vdc._defineMode << endl;
	o << " Block Size: ";
	for (int i=0; i<vdc._bs.size(); i++) {
		o << vdc._bs[i] << " ";
	}
	o << endl;
	o << " WName: " << vdc._wname << endl;
	o << " CRatios: ";
	for (int i=0; i<vdc._cratios.size(); i++) {
		o << vdc._cratios[i] << " ";
	}
	o << endl;
	o << " Periodic: ";
	for (int i=0; i<vdc._periodic.size(); i++) {
		o << vdc._periodic[i] << " ";
	}
	o << endl;

	{
		o << " Attributes" << endl;
		std::map <string, VDC::Attribute>::const_iterator itr;
		for (itr = vdc._atts.begin(); itr != vdc._atts.end(); ++itr) {
			o << itr->second;
		}
		o << endl;
	}

	{
		o << " Dimensions" << endl;
		std::map <string, VDC::Dimension>::const_iterator itr;
		for (itr = vdc._dimsMap.begin(); itr != vdc._dimsMap.end(); ++itr) {
			o << itr->second;
		}
		o << endl;
	}

	{
		o << " CoordVars" << endl;
		std::map <string, VDC::CoordVar>::const_iterator itr;
		for (itr = vdc._coordVars.begin(); itr != vdc._coordVars.end(); ++itr) {
			o << itr->second;
			o << endl;
		}
		o << endl;
	}

	{
		o << " DataVars" << endl;
		std::map <string, VDC::DataVar>::const_iterator itr;
		for (itr = vdc._dataVars.begin(); itr != vdc._dataVars.end(); ++itr) {
			o << itr->second;
			o << endl;
		}
		o << endl;
	}

	return(o);

}
};
