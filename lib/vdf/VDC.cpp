#include <cassert>
#include <sstream>
#include <cfloat>
#include "vapor/VDC.h"

using namespace VAPoR;


namespace {

// Product of elements in a vector
//
size_t vproduct(vector <size_t> a) {
	size_t ntotal = 1;

	for (int i=0; i<a.size(); i++) ntotal *= a[i];
	return(ntotal);
}

void _compute_bs(
	const vector <DC::Dimension> &dimensions, 
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

void _compute_periodic(
	const vector <DC::Dimension> &dimensions, 
	const vector <bool> &default_periodic,
	vector <bool> &periodic
) {
	// If the default periodicity exists for a dimension use it.
	// Otherwise set the periodicity to false. No periodicity for
	// time dimensions
	//
	for (int i=0; i<dimensions.size(); i++) {
		if (dimensions[i].GetAxis() == 3) break;
		if (i<default_periodic.size()) {
			periodic.push_back(default_periodic[i]);
		}
		else {
			periodic.push_back(false);	
		}
	}
}

int _axis_block_length(
	const vector <DC::Dimension> &dimensions, 
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

int VDC::Initialize(const vector <string> &paths, AccessMode mode)
{
	if (! paths.size()) {
		SetErrMsg("Invalid argument");
		return(-1);
	}
	_master_path = paths[0];
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

	if (! bs.size()) {
		for (int i=0; i<3; i++) bs.push_back(1);
		wname.clear();
		cratios.clear();
		cratios.push_back(1);
	}
	
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
	vector <string> dimnames(1,name);
	int status = DefineCoordVarUniform(name, dimnames, "", axis, FLOAT, false);
	if (status < 0) {
		_dimsMap.erase(name);	// remove dimension definition
	}
	return(status);
	
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

	vector <bool> periodic;
	_compute_periodic(dimensions, _periodic, periodic);
		
	vector <size_t> cratios(1,1);
	string wname;
	if (compressed) {
		cratios = _cratios;
		wname = _wname;
	}

	// _coordVars contains a table of all the coordinate variables
	//
	_coordVars[varname] = CoordVar (
		varname, dimensions, units, type, bs, wname, 
		cratios, periodic, axis, false
	);

	return(0);
}

int VDC::DefineCoordVarUniform(
	string varname, vector <string> dimnames,
	string units, int axis, XType type, bool compressed
) {
	int rc = VDC::DefineCoordVar(
		varname, dimnames, units, axis, type, compressed
	);
	if (rc<0) return(-1);

	assert(_coordVars.find(varname) != _coordVars.end());

	_coordVars[varname].SetUniform(true);

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

	vector <DC::Dimension> dimensions = itr->second.GetDimensions();
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

bool VDC::GetCoordVarInfo(string varname, DC::CoordVar &cvar) const {

	map <string, CoordVar>::const_iterator itr = _coordVars.find(varname);
	if (itr == _coordVars.end()) return(false);

	cvar = itr->second;
	return(true);
}

bool VDC::GetBaseVarInfo(string varname, DC::BaseVar &var) const {

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


int VDC::_DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
	string units, XType type, bool compressed, double mv, string maskvar
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
		varname,dimnames,coordvars, units, type,compressed, maskvar)) {

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

	vector <bool> periodic;
	_compute_periodic(dimensions, _periodic, periodic);

	vector <size_t> cratios(1,1);
	string wname;
	if (compressed) {
		cratios = _cratios;
		wname = _wname;
	}

	if (! maskvar.empty()) {
		DataVar datavar(
			varname, dimensions, units, type, bs, wname, 
			cratios, periodic, coordvars, 
			mv, maskvar
		);

		_dataVars[varname] = datavar;
	}
	else {
		DataVar datavar(
			varname, dimensions, units, type, bs, wname, 
			cratios, periodic, coordvars
		);
		_dataVars[varname] = datavar;
	}

	return(0);
}

int VDC::DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
	string units, XType type, bool compressed
) {
	return(
		VDC::_DefineDataVar(varname, dimnames, coordvars,
		units, type, compressed, 0.0, "")
	);
}

int VDC::DefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
    string units, XType type, double mv, string maskvar

) {
	return(
		VDC::_DefineDataVar(varname, dimnames, coordvars,
		units, type, true, mv, maskvar)
	);
}
	

bool VDC::GetDataVarInfo(
	string varname, vector <string> &dimnames, vector <string> &coordvars,
	string &units, XType &type, bool &compressed,
	string &maskvar
) const {
	dimnames.clear();
	units.erase();

	map <string, DataVar>::const_iterator itr = _dataVars.find(varname);

	if (itr == _dataVars.end()) return(false);

	vector <DC::Dimension> dimensions = itr->second.GetDimensions();
	for (int i=0; i<dimensions.size(); i++) {
		dimnames.push_back(dimensions[i].GetName());
	}
	coordvars = itr->second.GetCoordvars();
	units = itr->second.GetUnits();
	type = itr->second.GetXType();
	compressed = itr->second.IsCompressed();
	maskvar = itr->second.GetMaskvar();
	return(true);
}

bool VDC::GetDataVarInfo(string varname, DC::DataVar &datavar) const {

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


vector <string> VDC::GetCoordVarNames() const {
	vector <string> names;

	map <string, CoordVar>::const_iterator itr;
	for (itr = _coordVars.begin(); itr != _coordVars.end(); ++itr) {
		names.push_back(itr->first);
	}
	return(names);
}


int VDC::GetNumRefLevels(string varname) const {

	DC::BaseVar var;
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

DC::XType VDC::GetAttType(
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

int VDC::GetMapProjection(
	string lonname, string latname, string &projstring
) const {

	string attname = "MapProj_";
	attname += lonname;
	attname += "_";
	attname += latname;

	return(VDC::GetAtt("",attname, projstring));
}
	
int VDC::SetMapProjection(string lonname, string latname, string projstring) {
	string attname = "MapProj_";
	attname += lonname;
	attname += "_";
	attname += latname;

	return(VDC::GetAtt("",attname, projstring));

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


	// For any 1D Uniform coordinate variables that 
	// were defined go ahead
	// and give them default values
	//
	for (int i=0; i<_newUniformVars.size(); i++) {
		DC::CoordVar cvar;
		VDC::GetCoordVarInfo(_newUniformVars[i], cvar);

		vector <DC::Dimension> dims = cvar.GetDimensions();
		if (dims.size() != 1) continue;

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

	// No compression if no blocking
	//
	if (vproduct(bs) == 1) {
		if (! wname.empty()) return(false);
		if (! (cratios.size() == 1 && cratios[0] == 1)) return(false);
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
	const vector <DC::Dimension> &dimensions,
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

		// Must have same blocking or no blocking
		//
		if (bs0 != bs1 && bs1 != 1) return(false);
	}
	return(true);
}
	
bool VDC::_valid_mask_var(
	string varname, vector <DC::Dimension> dimensions,
	vector <size_t> bs, bool compressed, string maskvar
) const {

	map <string, DataVar>::const_iterator itr = _dataVars.find(maskvar);
	if (itr == _dataVars.end()) {
		SetErrMsg("Mask var \"%s\" not defined", maskvar.c_str());
		return(false);
	}
	const DataVar &mvar = itr->second;


	// Fastest varying data variable dimensions must match mask
	// variable dimensions
	//
	vector <DC::Dimension> cdimensions = mvar.GetDimensions();
	while (dimensions.size() > cdimensions.size()) dimensions.pop_back();
	while (bs.size() > cdimensions.size()) bs.pop_back();

	if (dimensions.size() != cdimensions.size()) {
		SetErrMsg("Data variable and mask variable dimensions must match");
		return(false);
	}

	// Data and mask variable must have same blocking along
	// each axis
	//
	if (! _valid_blocking(dimensions, mvar.GetCoordvars(), bs)) {
		SetErrMsg("Data and mask variables must have same blocking");
		return(false);
	}

	if (compressed) {
		size_t nlevels, dummy;
		CompressionInfo(bs, _wname, nlevels, dummy);

		size_t nlevels_m;
		CompressionInfo(mvar.GetBS(), mvar.GetWName(), nlevels_m, dummy);

		if (nlevels > nlevels_m) {
			SetErrMsg("Data variable and mask variable depth must match");
			return(false);
		}
	}

	return(true);
}

bool VDC::_ValidDefineDataVar(
	string varname, vector <string> dimnames, vector <string> coordvars,
	string units, XType type, bool compressed, string maskvar
) const {

	if (dimnames.size() > 4) {
		SetErrMsg("Invalid number of dimensions");
		return(false);
	}

	if (dimnames.size() != coordvars.size()) {
		SetErrMsg("Number of dimensions and coordinate vars don't match");
		return(false);
	}

	vector <DC::Dimension> dimensions;
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

	for (int i=0; i<coordvars.size(); i++) {
		map <string, CoordVar>::const_iterator itr = _coordVars.find(coordvars[i]);
		assert(itr != _coordVars.end());	// already checked for existance
		vector <DC::Dimension> cdimensions = itr->second.GetDimensions();
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

	// Validate mask variable if one exists
	//
	if (maskvar.empty()) return(true);

	return(_valid_mask_var(varname, dimensions, bs, compressed, maskvar));

}


namespace VAPoR {

std::ostream &operator<<(
	std::ostream &o, const DC::Dimension &d
) {
	o << "  Dimension" << endl;
	o << "   Name: " << d._name << endl;
	o << "   Length: " << d._length << endl;
	o << "   Axis: " << d._axis << endl;

	return(o);
}

std::ostream &operator<<(std::ostream &o, const DC::Attribute &a) {
	o << "  Attribute:" << endl;
	o << "   Name: " << a._name << endl;
	o << "   Type: " << a._type << endl;

	o << "   Values: ";
	for (int i=0; i<a._values.size(); i++) {
		DC::Attribute::podunion p = a._values[i];
		if (a._type == DC::FLOAT) {
			o << p.f;
		}
		else if (a._type == DC::DOUBLE) {
			o << p.d;
		}
		else if (a._type == DC::INT32) {
			o << p.i;
		}
		else if (a._type == DC::INT64) {
			o << p.l;
		}
		if (a._type == DC::TEXT) {
			o << p.c;
		}
		o << " ";
	}
	o << endl;
	return(o);
}

std::ostream &operator<<(std::ostream &o, const DC::BaseVar &var) {

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

	std::map <string, DC::Attribute>::const_iterator itr;
	for (itr = var._atts.begin(); itr != var._atts.end(); ++itr) {
		o << itr->second;
	}

	return(o);
}

std::ostream &operator<<(std::ostream &o, const DC::CoordVar &var) {

	o << "  CoordVar" << endl;
	o << "   Axis: " << var._axis << endl;
	o << "   Uniform: " << var._uniform << endl;

	o << (DC::BaseVar) var;

	return(o);
}

std::ostream &operator<<(std::ostream &o, const DC::DataVar &var) {

	o << "  DataVar" << endl;
	o << "   CoordVars: ";
	for (int i=0; i<var._coordvars.size(); i++) {
		o << var._coordvars[i] << " ";
	}
	o << endl;
	o << "   MaskVar: " << var._maskvar << endl;

	o << (DC::BaseVar) var;

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
		std::map <string, DC::Attribute>::const_iterator itr;
		for (itr = vdc._atts.begin(); itr != vdc._atts.end(); ++itr) {
			o << itr->second;
		}
		o << endl;
	}

	{
		o << " Dimensions" << endl;
		std::map <string, DC::Dimension>::const_iterator itr;
		for (itr = vdc._dimsMap.begin(); itr != vdc._dimsMap.end(); ++itr) {
			o << itr->second;
		}
		o << endl;
	}

	{
		o << " CoordVars" << endl;
		std::map <string, DC::CoordVar>::const_iterator itr;
		for (itr = vdc._coordVars.begin(); itr != vdc._coordVars.end(); ++itr) {
			o << itr->second;
			o << endl;
		}
		o << endl;
	}

	{
		o << " DataVars" << endl;
		std::map <string, DC::DataVar>::const_iterator itr;
		for (itr = vdc._dataVars.begin(); itr != vdc._dataVars.end(); ++itr) {
			o << itr->second;
			o << endl;
		}
		o << endl;
	}

	return(o);

}
};
