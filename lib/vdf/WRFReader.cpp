//
// $Id$
//

#include <iostream>
#include <sstream>
#include <sys/stat.h>
//#include <netcdf.h>
#include <vapor/WRFReader.h>
#ifdef WIN32
#include <windows.h>
#endif

using namespace std;
using namespace VAPoR;

int WRFReader::_WRFReader() {
	_current_file.clear();
	_current_var.clear();
	_current_wrf_ts = 0;
	_wrf = NULL;
	_wrf_fh = NULL;
	_wrf_fh2 = NULL;
	_slice_buf = NULL;

	_grav = 9.81;

	vector <pair<string, double> >  atts = GetGlobalAttributes();

	vector<pair<string, double> >::iterator itr;
	for(itr= atts.begin();itr< atts.end();itr++) {
		pair<string, double> attr = *itr;
		if (attr.first.compare("G") == 0) {
			_grav = attr.second;
			break;
		}
	}

	return(0);
}

WRFReader::WRFReader(
	const MetadataWRF &metadata
) : MetadataWRF(metadata) {

	if (MetadataWRF::GetErrCode()) return;

	if (_WRFReader() < 0) return;
}

WRFReader::WRFReader(
	const vector<string> &infiles
) : MetadataWRF(infiles) {

	if (MetadataWRF::GetErrCode()) return;

	if (_WRFReader() < 0) return;
}

WRFReader::WRFReader(
	const vector<string> &infiles,
	const map <string,string> &atypnames
) : MetadataWRF(infiles, atypnames) {

	if (MetadataWRF::GetErrCode()) return;

	if (_WRFReader() < 0) return;
}

WRFReader::~WRFReader() {
	CloseVariable();
	if (_wrf) delete _wrf;
	if (_slice_buf) delete [] _slice_buf;
}


int WRFReader::OpenVariableRead(
	size_t timestep, const char *varname
) {

	VarType_T vtype = GetVarType(varname);

	size_t dim[3];
	GetDim(dim, -1);
	if (vtype == VAR2D_XY) {
		dim[2] = 1;
	}
	else if (vtype == VAR3D) {
	}
	else {
		SetErrMsg("Unsupported variable type : %d\n", vtype);
		return(-1);
	}
	_nx = dim[0];
	_ny = dim[1];
	_nz = dim[2];


	size_t wrf_ts;
	string wrf_fname;
	int rc = MapVDCTimestep(timestep, wrf_fname, wrf_ts);
	if (rc<0) {
		SetErrMsg("Invalid time step : %d", timestep);
		return(-1);
	}

	if (
		(_current_file.compare(wrf_fname) != 0) || 
		(_current_var.compare(varname) != 0)
	) {

		(void) CloseVariable();
	}

	if (! _wrf || (_current_file.compare(wrf_fname) != 0)) {
		if (_wrf) delete _wrf;

		_wrf = new WRF(wrf_fname, GetAtypNames());
		if (WRF::GetErrCode() != 0) return(-1);
		_current_file = wrf_fname;
		_current_var.clear();
	} 

	if (_current_var.compare(varname) != 0) {
		string elevation = "ELEVATION";
		if (elevation.compare(varname) != 0) {
			if (_wrf_fh) _wrf->Close(_wrf_fh);
			_wrf_fh = _wrf->Open(varname);
			if (! _wrf_fh) return(-1);
			_current_var.assign(varname);
		}
		//
		// Elevation is a derived variable that we calculate from
		// PH and PHB
		//
		else {
			if (_wrf_fh) _wrf->Close(_wrf_fh);
			_wrf_fh = _wrf->Open("PH");
			if (! _wrf_fh) return(-1);
			_current_var.assign("PH");

			if (_wrf_fh2) _wrf->Close(_wrf_fh2);
			_wrf_fh2 = _wrf->Open("PHB");
			if (! _wrf_fh2) return(-1);

			if (! _slice_buf) {
				_slice_buf = new float[_nx*_ny];
			}
		}
	
	}

	_current_wrf_ts = wrf_ts;

	return(0);
}

int WRFReader::CloseVariable() 
{
	if (_wrf && _wrf_fh) {
		_wrf->Close(_wrf_fh);
		if (_wrf_fh2) _wrf->Close(_wrf_fh2);
		SetErrCode(0);
	}
	_wrf_fh = NULL;
	_wrf_fh2 = NULL;
	_current_var.clear();
	_current_wrf_ts = 0;
	return(0);
}

int WRFReader::ReadVariable(
	float *region 
) {
	if (! _wrf_fh || ! _wrf) {
		SetErrMsg("Not open for reading");
		return(-1);
	}

	size_t slice_sz = _nx * _ny;
	float *ptr = region;

	for (size_t z=0; z<_nz; z++) {

		int rc = ReadSlice(z, ptr);
		if (rc<0) return(rc);
		ptr += slice_sz;

	}

	return(0);
}

int WRFReader::ReadSlice(
	int z,
	float *slice
) {
	if (! _wrf_fh || ! _wrf) {
		SetErrMsg("Not open for reading");
		return(-1);
	}

	if (! _wrf_fh2) {
		return(_wrf->GetZSlice(_wrf_fh, _current_wrf_ts, z, slice));
	}

	// If _wrf_fh2 is not NULL, we need to calculate the ELEVATION
	// variable from PH and PHB
	//
	int rc = _wrf->GetZSlice(_wrf_fh, _current_wrf_ts, z, slice);
	if (rc<0) return(rc);

	rc = _wrf->GetZSlice(_wrf_fh2, _current_wrf_ts, z, _slice_buf);
	if (rc<0) return(rc);

	for (int i=0; i<_nx*_ny; i++ ){
		slice[i] = (slice[i] + _slice_buf[i]) / _grav;
	}

	return(0);
}

int    WRFReader::VariableExists(
    size_t ts,
    const char *varname
) const {
	if (ts > GetNumTimeSteps()) return(0);

	vector <string> varnames = GetVariableNames();
	for (int i=0; i<varnames.size(); i++) {
		if (varnames[i].compare(varname) == 0) return(1);
	}
	return(0);
}
