//************************************************************************
//                                                                       *
//           Copyright (C)  2004                                         *
//     University Corporation for Atmospheric Research                   *
//           All Rights Reserved                                         *
//                                                                       *
//************************************************************************/
//
//  File:        DCReaderGRIB.cpp
//
//  Author:      Scott Pearse
//               National Center for Atmospheric Research
//               PO 3000, Boulder, Colorado
//
//  Date:        June 2014
//
//  Description: TBD 
//               
//

#include <iostream>
#include <sstream>
#include <iterator>
#include <cassert>
#include <string>

#include "vapor/DCReaderGRIB.h"
#include "vapor/Proj4API.h"
#define bzero(b,len) (memset((b), '\0', (len)), (void) 0)
#ifdef WIN32
#pragma warning(disable : 4251)
#endif

using namespace VetsUtil;
using namespace VAPoR;
using namespace std;


#define MAX_KEY_LEN 255
#define MAX_VAL_LEN 1024

int DCReaderGRIB::_sliceNum=0;
int DCReaderGRIB::_openTS=0;

DCReaderGRIB::Variable::Variable() {
	_messages.clear();
	_unitTimes.clear();
	_varTimes.clear();
}

DCReaderGRIB::Variable::~Variable() {
}

bool DCReaderGRIB::Variable::_Exists(double time) const {
	//if (_unitTimes.find(time) == _unitTimes.end()) return 0;
	if (std::find(_unitTimes.begin(), _unitTimes.end(), time)==_unitTimes.end()) return 0;
	return 1;
}

int DCReaderGRIB::Variable::GetOffset(double time, float level) {
	int off = _indices[time][level].offset;
	return off;
}

string DCReaderGRIB::Variable::GetFileName(double time, float level) {
    string fname = _indices[time][level].fileName;
    return fname; 
}

void DCReaderGRIB::Variable::PrintTimes() {
	cout << "Times: [";
	for (size_t i=0; i< _unitTimes.size(); i++) {
		cout << fixed << _unitTimes[i] << " ";
	}
	cout << "]" << endl;
}

void DCReaderGRIB::Variable::PrintIndex(double time, float level){
	cout << fixed << time << " " << level << " ";
	cout << _indices[time][level].fileName;
	cout << " " << _indices[time][level].offset << endl;
}

void DCReaderGRIB::Variable::_AddIndex(double time, float level, string file, int offset) {
	MessageLocation location;
	location.fileName = file;
	location.offset = offset;
	_indices[time][level] = location;
}

DCReaderGRIB::DCReaderGRIB(const vector <string> files) {
	_Ni = NULL;
	_Nj = NULL;
	_pressureLevels.clear();
	_cartesianExtents.clear();
	_gribTimes.clear();
	_vars2d.clear();
	_vars3d.clear();
	_udunit = NULL;

	DCReaderGRIB::_Initialize(files);
}



//int DCReaderGRIB::OpenVariableRead(size_t gribTS, string gribVar) {
int DCReaderGRIB::OpenVariableRead(size_t timestep, string varname,
							 int reflevel, int lod) {

    if (timestep > _gribTimes.size()) return -1;

    if (_vars3d.find(varname) == _vars3d.end()) {
        if (_vars2d.find(varname) == _vars2d.end())
			return -1; 		// variable does not exist
    }

    _openVar = varname;
    _openTS = timestep;
	Variable *targetVar = _vars3d[_openVar];
	double usertime = GetTSUserTime(_openTS);
    float level = targetVar->GetLevel(_sliceNum);
    string filename = targetVar->GetFileName(usertime,level);

    _inFile = fopen(filename.c_str(),"rb");
    if(!_inFile) {
        char err[50];
        sprintf(err,"ERROR: unable to open file %s\n",filename.c_str());
        MyBase::SetErrMsg(err);
        return -1; 
    }

    return 0;
}

int DCReaderGRIB::Read(float *_values) {
	float *ptr = _values;
	
	int rc;
	while ((rc = DCReaderGRIB::ReadSlice(ptr)) > 0) {
		ptr += _Ni * _Nj;
	}
	return rc;
}

int DCReaderGRIB::ReadSlice(float *_values){

	Variable *targetVar = _vars3d[_openVar];
	double usertime = GetTSUserTime(_openTS);
	float level = targetVar->GetLevel(_sliceNum);
	int offset = targetVar->GetOffset(usertime,level);
	string filename = targetVar->GetFileName(usertime,level);	
	
	//cout << level << " " << offset << endl;

    int rc = fseek(_inFile,offset,SEEK_SET);
	if (rc != 0) MyBase::SetErrMsg("fseek error during GRIB ReadSlice");  

	/* create new handle from a message in a file*/
	int err;
    grib_handle* h = grib_handle_new_from_file(0,_inFile,&err);
    if (h == NULL) {
		char erro[50];
        sprintf(erro,"Error: unable to create handle from file %s\n",filename.c_str());
        MyBase::SetErrMsg(erro);
    }   

    /* get the size of the _values array*/
	size_t _values_len;
    GRIB_CHECK(grib_get_size(h,"values",&_values_len),0);
    double* _dvalues = new double[_values_len];
 
    /* get data _values*/
    GRIB_CHECK(grib_get_double_array(h,"values",_dvalues,&_values_len),0);

	// re-order values according to scan direciton and convert doubles to floats
	int vaporIndex, i, j;
    for(size_t gribIndex = 0; gribIndex < _values_len; gribIndex++) {
		//bool _iScanNeg = targetVar->get_iScanNeg();  // 0: i scans positively, 1: negatively
		//bool _jScan = targetVar->get_jScan();  // 1: j scans positively, 0: negatively
		if (_iScanNeg == 1) {
			if (_jScanPos == 1) {   // 1 1
				i = _Ni - gribIndex%_Ni;
				j = gribIndex/_Ni;
				vaporIndex = j*_Ni + i;
			}
			else {              // 1 0
				vaporIndex = _Ni*_Nj - gribIndex - 1;
			}
		}

		else if (_jScanPos == 0) {  // 0 0
            i = gribIndex % _Ni;
            j = ((_Ni * _Nj) - 1 - gribIndex) / _Ni;
            vaporIndex = j * _Ni + i;
		}
		// if the case is 0 1, we don't have to do anything...
		//
		_values[vaporIndex] = (float) _dvalues[gribIndex];
	}

    delete [] _dvalues;

	cout << _values[0] << endl;
 
	//if (_sliceNum == _pressureLevels.size()-1) _sliceNum = 0;
	//else _sliceNum++;
	if (_sliceNum == 0) _sliceNum = _pressureLevels.size()-1;
	else _sliceNum--;
    grib_handle_delete(h);
	return 1;
}

string DCReaderGRIB::GetMapProjection() const {
	double lat_0;
	double lon_0;
	ostringstream oss;
	string projstring;
	if (strcmp(_gridType.c_str(), "regular_ll")){
	    lon_0 = (_minLon + _maxLon) / 2.0;
	    lat_0 = (_minLat + _maxLat) / 2.0;
	    oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
	    projstring = "+proj=eqc +ellps=WGS84" + oss.str();
	}
	else if(strcmp(_gridType.c_str(),"polar_stereographic")){
		if (_minLat < 0.) lat_0 = -90.0;
		else lat_0 = 90.0; 

		float lat_ts = _minLat;
		float lon_0 = _minLon;
		//lat_0 = 	
	}
//	else if(
//
//	}
    return(projstring);
}


double DCReaderGRIB::BarometricFormula(const double pressure) const {
    double Po = 101325;  // (kg/m3)    Mass density of air @ MSL
    double g  = 9.81;    // (m/s2)     gravity
    double M  = .0289;   // (kg/mol)   Molar mass of air
    double R  = 8.31447; // (J/(mol•K) Univ. gas constant
    double To = 288.15;  // (K)        Std. temperature of air @ MSL
	double pressure2 = pressure * 100; // pressure in pascals
	double height;

	height = -1 * R * To * log(pressure2/Po) / (g * M);
	return height;
}

int DCReaderGRIB::_InitCartographicExtents(string mapProj){
                                           //const std::vector<double> vertCoordinates,
                                           //std::vector<double> &extents) const {

    Proj4API proj4API;

    int rc = proj4API.Initialize("", mapProj);
    if (rc<0) {
        SetErrMsg("Invalid map projection : %s", mapProj.c_str());
        return(-1);
    }   

	double x[2];
	double y[2];
	if (strcmp(_gridType.c_str(),"regular_ll")){
	    x[0] = _minLon;
		x[1] = _maxLon;
	    y[0] = _minLat;
		y[1] = _maxLat;

	    rc = proj4API.Transform(x,y,2,1);
	    if (rc < 0) {
	        SetErrMsg("Invalid map projection : %s", mapProj.c_str());
	        return(-1);
	    }
	}
	else if(strcmp(_gridType.c_str(),"polar_stereographic")){
		x[0] = _minLon;
		y[0] = _minLat;

	    rc = proj4API.Transform(x,y,2,1);
    	if (rc < 0) {
        	SetErrMsg("Invalid map projection : %s", mapProj.c_str());
        	return(-1);
    	}

		x[1] = x[0] + _Ni*_DxInMetres;
		y[1] = y[0] + _Nj*_DyInMetres;
	}

    rc = proj4API.Transform(x,y,2,1);
    if (rc < 0) {
        SetErrMsg("Invalid map projection : %s", mapProj.c_str());
        return(-1);
    }  
	
	OpenVariableRead(0,"ELEVATION",0,0);
	float *values = new float[_Ni*_Nj];
	ReadSlice(values);

	float max = values[0];
	for (int i=0; i<_Ni; i++){
		for (int j=0; j<_Nj; j++) {
			float value = values[j*_Ni+i];
			if (value > max) max = value;
		}
	}

	_sliceNum = _pressureLevels.size()-1;
    ReadSlice(values);

    float min = values[0];
    for (int i=0; i<_Ni; i++){
        for (int j=0; j<_Nj; j++) {
            float value = values[j*_Ni+i];
            if (value < min) min = value;
        }   
    }
	
	_sliceNum += 1;
	
	/*
	for (i=0; i<_pressureLevels.size(); i++) {
		double height = BarometricFormula(_pressureLevels[i]);
		_meterLevels.push_back(height);
	}
	
	double bottom = _meterLevels[0];//BarometricFormula(_pressureLevels[_pressureLevels.size()-1]);
	double top = _meterLevels[_meterLevels.size()-1];//BarometricFormula(_pressureLevels[0]);
	*/

    _cartographicExtents.push_back(x[0]);
    _cartographicExtents.push_back(y[0]);
    _cartographicExtents.push_back(min);
    _cartographicExtents.push_back(x[1]);
    _cartographicExtents.push_back(y[1]);
    _cartographicExtents.push_back(max);

	delete values;

    return 0;
}

int DCReaderGRIB::_Initialize(const vector <string> files) {

	parser = new GribParser();
	for (int i=0; i<files.size(); i++){
		parser->_LoadRecordKeys(files[i]);
	}
	int rc = parser->_VerifyKeys();
    if (rc<0) return -1; 

	std::vector<std::map<std::string, std::string> > records = parser->GetRecords();

	_udunit = new UDUnits();

    int numRecords = records.size();

	_Ni = atoi(records[0]["Ni"].c_str());
	_Nj = atoi(records[0]["Nj"].c_str());
	_gridType = records[0]["gridType"];
	_iScanNeg = atoi(records[0]["iScanNegsNegatively"].c_str());
    _jScanPos = atoi(records[0]["jScansPositively"].c_str());
	_DxInMetres = atof(records[0]["_DxInMetres"].c_str());	
	_DyInMetres = atof(records[0]["_DyInMetres"].c_str());

	if (_iScanNeg==0) {		// i scans positively
		_maxLon = atof(records[0]["longitudeOfLastGridPointInDegrees"].c_str());
		_minLon = atof(records[0]["longitudeOfFirstGridPointInDegrees"].c_str());
	}
	else {					// i scans negatively
		_minLon = atof(records[0]["longitudeOfLastGridPointInDegrees"].c_str());
		_maxLon = atof(records[0]["longitudeOfFirstGridPointInDegrees"].c_str());
	}
	if (_jScanPos==0) {		// j scans negatively
		_maxLat = atof(records[0]["latitudeOfFirstGridPointInDegrees"].c_str());
		_minLat = atof(records[0]["latitudeOfLastGridPointInDegrees"].c_str());
	}
	else { 					// j scans positively
        _minLat = atof(records[0]["latitudeOfFirstGridPointInDegrees"].c_str());
        _maxLat = atof(records[0]["latitudeOfLastGridPointInDegrees"].c_str());
	}

	if (_maxLon < 0) _maxLon += 360;

    for (int i=0; i<numRecords; i++) {
		std::map<std::string, std::string> record = records[i];
		string levelType = record["typeOfLevel"];
		float level = atof(record["level"].c_str());
		string name  = record["shortName"];
		string file = record["file"];
		int offset = atoi(record["offset"].c_str());
		int year   = atoi(record["yearOfCentury"].c_str()) + 2000;
		int month  = atoi(record["month"].c_str());
		int day    = atoi(record["day"].c_str());
		int hour   = atoi(record["hour"].c_str());
		int minute = atoi(record["minute"].c_str());
		int second = atoi(record["second"].c_str());
		bool _iScanNeg = atoi(record["_iScanNegsNegatively"].c_str());
		bool _jScan = atoi(record["_jScansPositively"].c_str());
	
		double time = _udunit->EncodeTime(year, month, day, hour, minute, second); 
		if (std::find(_gribTimes.begin(), _gribTimes.end(), time) == _gribTimes.end())
			_gribTimes.push_back(time);

		if (std::find(_pressureLevels.begin(), _pressureLevels.end(), level) == _pressureLevels.end())
			if (level != 0.f) _pressureLevels.push_back(level);

		int isobaric = strcmp(levelType.c_str(),"isobaricInhPa");
		if (isobaric == 0) {								    // if we have a 3d var...
			if (_vars3d.find(name) == _vars3d.end()) {			// if we have a new 3d var...
				_vars3d[name] = new Variable();
				_vars3d[name]->setScanDirection(_iScanNeg,_jScan);
			}
			
			std::vector<double> varTimes = _vars3d[name]->GetTimes();
			if (std::find(varTimes.begin(),varTimes.end(),time) == varTimes.end())	// we only want to record new timestamps
				_vars3d[name]->_AddTime(time);										// for our vars.  (_gribTimes records all times 
			
			// Add level data for current var object
			std::vector<float> varLevels = _vars3d[name]->GetLevels();
			if (std::find(varLevels.begin(),varLevels.end(),time) == varLevels.end())
				_vars3d[name]->_AddLevel(level);
			
			_vars3d[name]->_AddMessage(i);									// in the grib file)
			_vars3d[name]->_AddIndex(time,level,file,offset);
		}

		int surface = strcmp(levelType.c_str(),"surface");
		int meanSea = strcmp(levelType.c_str(),"meanSea");
		if ((surface == 0) || (meanSea == 0)) {	        		// if we have a 2d var...
	        if (_vars2d.find(name) == _vars2d.end()) {			// if we have a new 2d var...
            	_vars2d[name] = new Variable();
			}

			// Add level data to current var object
			// (this should only happen once for a 2D var
            std::vector<float> varLevels = _vars2d[name]->GetLevels();
            if (std::find(varLevels.begin(),varLevels.end(),time) == varLevels.end())
                _vars2d[name]->_AddLevel(level);

			_vars2d[name]->_AddTime(time);
        	_vars2d[name]->_AddMessage(i);	
			_vars2d[name]->_AddIndex(time,level,file,offset);
		}

		int entireAtmos = strcmp(levelType.c_str(),"entireAtmosphere");
        if (entireAtmos == 0) {									// if we have a 1d var...
            if (_vars1d.find(name) == _vars1d.end()) { 			// if we have a new 2d var...
                _vars1d[name] = new Variable();
            }   
            _vars1d[name]->_AddTime(time);
            _vars1d[name]->_AddMessage(i);   
			_vars1d[name]->_AddIndex(time,level,file,offset);
        }

		if (name == "gh") _vars3d["ELEVATION"] = _vars3d["gh"];
	}
	
	typedef std::map<std::string, Variable*>::iterator it_type;
	for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
    	_vars3d[iterator->first]->_SortLevels();           // Sort the levels that apply to each individual variable
	}
	for (it_type iterator=_vars2d.begin(); iterator!=_vars2d.end(); iterator++){
        _vars2d[iterator->first]->_SortLevels();			// Sort the levels that apply to each individual variable
    }
	sort(_pressureLevels.begin(), _pressureLevels.end());	// Sort the levels that apply to the entire dataset
	reverse(_pressureLevels.begin(), _pressureLevels.end());

	rc = _InitCartographicExtents(GetMapProjection());
			  						 //_pressureLevels,
									 //_cartesianExtents);
	
	_sliceNum = _pressureLevels.size()-1;

	if (rc<0) return -1;			 

	return 0;
}

void DCReaderGRIB::Print3dVars() {
    typedef std::map<std::string, Variable*>::iterator it_type;
    for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
        cout << iterator->first << endl;
        iterator->second->PrintTimes();
    } 
}

void DCReaderGRIB::GetGridDim(size_t dim[3]) const {
	dim[0]=_Ni; dim[1]=_Nj; dim[2]=_pressureLevels.size();
}

std::vector<long> DCReaderGRIB::GetPeriodicBoundary() const {
	std::vector<long> dum;
	dum.push_back(0);
	dum.push_back(0);
	dum.push_back(0);
	return dum;
}

std::vector<long> DCReaderGRIB::GetGridPermutation() const {
    std::vector<long> dum; 
    dum.push_back(0);
    dum.push_back(1);
    dum.push_back(2);
    return dum; 
}

void DCReaderGRIB::GetTSUserTimeStamp(size_t ts, std::string &s) const {
	s = "dum";
}

long DCReaderGRIB::GetNumTimeSteps() const {
	return long(_gribTimes.size());
}

std::vector<string> DCReaderGRIB::GetVariables3D() const {
    typedef std::map<std::string, Variable*>::const_iterator it_type;
	std::vector<string> vars;
    for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
        vars.push_back(iterator->first);
    }   
	return vars;
}

std::vector<string> DCReaderGRIB::GetVariables2DXY() const {
    typedef std::map<std::string, Variable*>::const_iterator it_type;
    std::vector<string> vars;
    for (it_type iterator=_vars2d.begin(); iterator!=_vars2d.end(); iterator++){
        vars.push_back(iterator->first);
    }
    return vars;
}

void DCReaderGRIB::Print2dVars() {
    typedef std::map<std::string, Variable*>::iterator it_type;
    for (it_type iterator=_vars2d.begin(); iterator!=_vars2d.end(); iterator++){
        cout << iterator->first << endl;
        iterator->second->PrintTimes();
    } 
}

void DCReaderGRIB::Print1dVars() {
    typedef std::map<std::string, Variable*>::iterator it_type;
    for (it_type iterator=_vars1d.begin(); iterator!=_vars1d.end(); iterator++){
        cout << iterator->first << endl;
        iterator->second->PrintTimes();
    } 
}

bool DCReaderGRIB::VariableExists( size_t ts, string varname,
								   int reflevel, int lod) const {

	double dts = _gribTimes[ts]; //timesteps are stored by udunits double, not index
	if (_vars3d.find(varname) != _vars3d.end()){        // we have a 3d var
		if (_vars3d.find(varname)->second->_Exists(dts)) return true;
	}
	else if (_vars2d.find(varname) != _vars2d.end()){   // we have a 2d var
		if (_vars2d.find(varname)->second->_Exists(dts)) return true;
	}
	return false;
}

void DCReaderGRIB::GetLatLonExtents(size_t ts, double lon_exts[2], double lat_exts[2]) const {
	lon_exts[0] = _minLon;
	lon_exts[1] = _maxLon;
	lat_exts[0] = _minLat;
	lat_exts[1] = _maxLat;
}

std::vector<std::string> DCReaderGRIB::GetVariables2DExcluded() const {
	std::vector <string> vars;
	typedef std::map<std::string, Variable*>::const_iterator it_type;
	for(it_type iterator = _vars3d.begin(); iterator != _vars3d.end(); iterator++) {
    	vars.push_back(iterator->first);
	}
	return vars;
}

std::vector<std::string> DCReaderGRIB::GetVariables3DExcluded() const {
    std::vector <string> vars;
    typedef std::map<std::string, Variable*>::const_iterator it_type;
    for(it_type itr = _vars2d.begin(); itr != _vars2d.end(); itr++) {
        vars.push_back(itr->first);
    }    
    return vars;
}

DCReaderGRIB::~DCReaderGRIB() {
	if (_udunit) delete _udunit;
	_cartesianExtents.clear();
	_pressureLevels.clear();
	_gribTimes.clear();
	_vars2d.clear();
	_vars3d.clear();
}

DCReaderGRIB::GribParser::GribParser() {
	_recordKeys.clear();
	_consistentKeys.clear();
	_varyingKeys.clear();

	// key iteration vars
	_key_iterator_filter_flags     = GRIB_KEYS_ITERATOR_ALL_KEYS;
	_name_space                    = 0; // NULL will return all keys
	_grib_count                    = 0;
	_value                         = new char[MAX_VAL_LEN];
	_vlen                          = MAX_VAL_LEN;	
	_recordKeysVerified		       = 0;
	
	// _consistentKeys must be the same across all records
	_consistentKeys.push_back("gridType");
    _consistentKeys.push_back("longitudeOfFirstGridPointInDegrees");
    _consistentKeys.push_back("longitudeOfLastGridPointInDegrees");
    _consistentKeys.push_back("latitudeOfFirstGridPointInDegrees");
    _consistentKeys.push_back("latitudeOfLastGridPointInDegrees");
	_consistentKeys.push_back("iScansNegatively");
	_consistentKeys.push_back("jScansPositively");
	_consistentKeys.push_back("DxInMetres");
	_consistentKeys.push_back("DyInMetres");
	_consistentKeys.push_back("Ni");
	_consistentKeys.push_back("Nj");

	// _varyingKeys are those that may change over time, but must
	// fit some criteria for a legal data conversion
	_varyingKeys.push_back("shortName");
	_varyingKeys.push_back("units");
	_varyingKeys.push_back("yearOfCentury");
	_varyingKeys.push_back("month");
	_varyingKeys.push_back("day");
	_varyingKeys.push_back("hour");
	_varyingKeys.push_back("minute");
	_varyingKeys.push_back("second");
	_varyingKeys.push_back("typeOfLevel");
	_varyingKeys.push_back("level");	

	// data dump vars
	_values     = NULL;
	_in         = NULL;
	_h          = NULL;
	_values_len = 0;	
    _err        = 0;
}

int DCReaderGRIB::GribParser::_LoadRecord(string file, size_t index) {
    _filename = file;
    _in = fopen(_filename.c_str(),"rb");
    if(!_in) {
        char *error;
        sprintf(error,"ReadSlice ERROR: unable to open file %s\n",_filename.c_str());
        MyBase::SetErrMsg(error);
        return -1;
    }   

	size_t offset = atoi(_recordKeys[index]["offset"].c_str());
	int rc = fseek(_in,offset,SEEK_SET);
	if (rc != 0) MyBase::SetErrMsg("fseek error during GRIB _LoadRecord()");

    _recordKeysVerified = 0;
    std::map<std::string, std::string> keyMap;
    	if ((_h = grib_handle_new_from_file(0,_in,&_err)) != NULL) {

        const void* msg;
        size_t size;
        grib_get_message(_h,&msg,&size);

        grib_keys_iterator* kiter=NULL;
        _grib_count++;
        if(!_h) {
        	MyBase::SetErrMsg("Unable to create grib handle");
			fclose(_in);
			return 1;
        }

        kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,_name_space);
        if (!kiter) {
            MyBase::SetErrMsg("ERROR: Unable to create keys iterator\n");
			fclose(_in);
            return 1;
        }

        while(grib_keys_iterator_next(kiter)) {
            const char* name = grib_keys_iterator_get_name(kiter);
            _vlen=MAX_VAL_LEN;
            bzero(_value,_vlen);
            GRIB_CHECK(grib_get_string(_h,name,_value,&_vlen),name);
            std::string gribKey(name);
            std::string gribValue(_value);
            for (size_t i=0;i<_varyingKeys.size();i++){
                if(strcmp(_varyingKeys[i].c_str(),gribKey.c_str())==0){
                     keyMap[gribKey] = gribValue;
                }
            }
            for (size_t i=0;i<_consistentKeys.size();i++){
                if(strcmp(_consistentKeys[i].c_str(),gribKey.c_str())==0){
                     keyMap[gribKey] = gribValue;
                }
            }
        }

        grib_keys_iterator_delete(kiter);
        grib_handle_delete(_h);
    }

	fclose(_in);
    return 0;
}

int DCReaderGRIB::GribParser::_LoadRecordKeys(string file) {
    _in = fopen(file.c_str(),"rb");
    if(!_in) {
        char *error;
        sprintf(error,"ERROR: unable to open file %s\n",file.c_str());
        MyBase::SetErrMsg(error);
	    return -1;
    } 

	const void* msg;
	size_t size;
	size_t offset=0;
    stringstream ss;
	_recordKeysVerified = 0;
    std::map<std::string, std::string> keyMap;
    while((_h = grib_handle_new_from_file(0,_in,&_err)) != NULL) {

		grib_get_message(_h,&msg,&size);
		ss << offset;		
		keyMap["offset"] = ss.str();
		ss.str(std::string());
		ss.clear();
		offset+=size;
		keyMap["file"] = file;

        grib_keys_iterator* kiter=NULL;
        _grib_count++;
        if(!_h) {
        	MyBase::SetErrMsg("Unable to create grib handle");
			fclose(_in);
            return 1;
        }   
            
        kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,_name_space);
        if (!kiter) {
            MyBase::SetErrMsg("Unable to create keys iterator");
			fclose(_in);
            return 1;
        }   

        while(grib_keys_iterator_next(kiter)) {
            const char* name = grib_keys_iterator_get_name(kiter);
            _vlen=MAX_VAL_LEN;
            bzero(_value,_vlen);
            GRIB_CHECK(grib_get_string(_h,name,_value,&_vlen),name);
            std::string gribKey(name);
            std::string gribValue(_value);
			for (size_t i=0;i<_varyingKeys.size();i++){
				if(strcmp(_varyingKeys[i].c_str(),gribKey.c_str())==0){
					 keyMap[gribKey] = gribValue;
				}
			}
			for (size_t i=0;i<_consistentKeys.size();i++){
				if(strcmp(_consistentKeys[i].c_str(),gribKey.c_str())==0){
                     keyMap[gribKey] = gribValue;
                }
			}
		}   

		_recordKeys.push_back(keyMap);
		keyMap.clear();
        grib_keys_iterator_delete(kiter);
        grib_handle_delete(_h);
    }
	_grib_count=0;  
	fclose(_in); 
    return 0;
}

int DCReaderGRIB::GribParser::_LoadAllRecordKeys(string file) {
    _filename = file;
    _in = fopen(_filename.c_str(),"rb");
    if(!_in) {
        char *error;
        sprintf(error,"ERROR: unable to open file %s\n",_filename.c_str());
        MyBase::SetErrMsg(error);
        return -1;
    } 
	
	_recordKeysVerified = 0;
	std::map<std::string, std::string> keyMap;
	while((_h = grib_handle_new_from_file(0,_in,&_err)) != NULL) {
		grib_keys_iterator* kiter=NULL;
		_grib_count++;
		if(!_h) {
			MyBase::SetErrMsg("Unable to create grib handle");
			fclose(_in);
			return 1;
		}
		
		kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,_name_space);
		if (!kiter) {
			printf("ERROR: Unable to create keys iterator\n");
			fclose(_in);
			return 1;
		}

		while(grib_keys_iterator_next(kiter)) {
			const char* name = grib_keys_iterator_get_name(kiter);
			_vlen=MAX_VAL_LEN;
			bzero(_value,_vlen);
			GRIB_CHECK(grib_get_string(_h,name,_value,&_vlen),name);
			std::string temp1(name);
			std::string temp2(_value);
			keyMap[temp1] = temp2;
		}

		// push each record back into our "keys" vector
		_recordKeys.push_back(keyMap);
		grib_keys_iterator_delete(kiter);
		grib_handle_delete(_h);
	}
	fclose(_in);
	return 0;
}

int DCReaderGRIB::GribParser::_VerifyKeys() {
	int numRecords = _recordKeys.size();
 
	int n; 
	char error[50];
	for (int i=0; i<numRecords; i++) {
		
		string grid;
        grid = _recordKeys[i]["gridType"];
        if ((strcmp(grid.c_str(),"regular_ll")!=0) && (strcmp(grid.c_str(),"regular_gg")!=0)){
			n=sprintf(error,"Error: Invalid grid specification ('%s') for Record No. %d",grid.c_str(),i);
			MyBase::SetErrMsg(error);
            return -1;
        } 

		for (size_t k=0; k<_consistentKeys.size(); k++) {
			// Check for inconsistent key across multiple records
			if (_recordKeys[i][_consistentKeys[k]].compare(_recordKeys[0][_consistentKeys[k]]) != 0) {
				n=sprintf(error,"Error: Inconsistent key found in Record No. %i, Key: %s",i,_consistentKeys[k].c_str());
				MyBase::SetErrMsg(error);
				return -1;
			}
		}
	}
	_recordKeysVerified = 1; 
	return 0;
}

int DCReaderGRIB::GribParser::_DataDump() {
	/* create new handle from a message in a file*/
	_h = grib_handle_new_from_file(0,_in,&_err);
	if (_h == NULL) {
		char *error;
        sprintf(error,"Error: unable to create handle from file %s\n",_filename.c_str());
        MyBase::SetErrMsg(error);
	}

	/* get the size of the _values array*/
	GRIB_CHECK(grib_get_size(_h,"_values",&_values_len),0);
 	_values = new double[_values_len];
 
	/* get data _values*/
	GRIB_CHECK(grib_get_double_array(_h,"_values",_values,&_values_len),0);

	for(size_t i = 0; i < _values_len; i++)
		printf("%lu  %.10e\n",i+1,_values[i]);
 
	delete [] _values;
 
 
	GRIB_CHECK(grib_get_double(_h,"max",&_max),0);
	GRIB_CHECK(grib_get_double(_h,"min",&_min),0);
	GRIB_CHECK(grib_get_double(_h,"average",&_average),0);
 
	printf("%d _values found in %s\n",(int)_values_len,_filename.c_str());
	printf("max=%.10e min=%.10e average=%.10e\n",_max,_min,_average);
 
	grib_handle_delete(_h);
 
	fclose(_in);
	return 0;
}

DCReaderGRIB::GribParser::~GribParser() {
	if (_value) delete _value;
}

//DCReaderGRIB::DerivedVarElevation::DerivedVarElevation(int Ni, int Nj){
	
//}
