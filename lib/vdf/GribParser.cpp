//************************************************************************
//                                                                       *
//           Copyright (C)  2004                                         *
//     University Corporation for Atmospheric Research                   *
//           All Rights Reserved                                         *
//                                                                       *
//************************************************************************/
//
//  File:        GribParser.cpp
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

#include "vapor/GribParser.h"
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

Variable::Variable() {
	_messages.clear();
	_unitTimes.clear();
	_varTimes.clear();
}

Variable::~Variable() {
}

bool Variable::_Exists(double time) const {
	//if (_unitTimes.find(time) == _unitTimes.end()) return 0;
	if (std::find(_unitTimes.begin(), _unitTimes.end(), time)==_unitTimes.end()) return 0;
	return 1;
}

int Variable::GetOffset(double time, float level) {
	int off = _indices[time][level].offset;
	return off;
}

string Variable::GetFileName(double time, float level) {
    string fname = _indices[time][level].fileName;
    return fname; 
}

void Variable::PrintTimes() {
	cout << "Times: [";
	for (size_t i=0; i< _unitTimes.size(); i++) {
		cout << fixed << _unitTimes[i] << " ";
	}
	cout << "]" << endl;
}

void Variable::PrintIndex(double time, float level){
	cout << fixed<< time << " " << level << " ";
	cout << _indices[time][level].fileName;
	cout << " " << _indices[time][level].offset << endl;
}

void Variable::_AddIndex(double time, float level, string file, int offset) {
	MessageLocation location;
	location.fileName = file;
	location.offset = offset;
	_indices[time][level] = location;
}

DCReaderGRIB::DCReaderGRIB() {
	_Ni = NULL;
	_Nj = NULL;
	_levels.clear();
	_cartesianExtents.clear();
	_gribTimes.clear();
	_vars2d.clear();
	_vars3d.clear();
	_udunit = NULL;
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
    return 0;
}

int DCReaderGRIB::ReadSlice(float *_values){

	Variable* targetVar = Get3dVariable(_openVar);
	double usertime = GetTSUserTime(_openTS);
	float level = targetVar->GetLevel(_sliceNum);
	int offset = targetVar->GetOffset(usertime,level);
	string filename = targetVar->GetFileName(usertime,level);	
 
    FILE* in = fopen(filename.c_str(),"rb");
    if(!in) {
        printf("ERROR: unable to open file %s\n",filename.c_str());
        return -1;
    }

    fseek(in,offset,SEEK_SET);
    
	/* create new handle from a message in a file*/
	int err;
    grib_handle* h = grib_handle_new_from_file(0,in,&err);
    if (h == NULL) {
        printf("Error: unable to create handle from file %s\n",filename.c_str());
    }   

    /* get the size of the _values array*/
	size_t _values_len;
    GRIB_CHECK(grib_get_size(h,"values",&_values_len),0);
    double* _dvalues = new double[_values_len];
 
    /* get data _values*/
    GRIB_CHECK(grib_get_double_array(h,"values",_dvalues,&_values_len),0);

	// convert doubles to floats
    for(size_t i = 0; i < _values_len; i++)
		_values[i] = (float) _dvalues[i];

	//cout << _values[0] << " " << _dvalues[0] << endl;
 
    delete [] _dvalues;
 
 
    //GRIB_CHECK(grib_get_double(_h,"max",&_max),0);
    //GRIB_CHECK(grib_get_double(_h,"min",&_min),0);
    //GRIB_CHECK(grib_get_double(_h,"average",&_average),0);
 
    //printf("%d _values found in %s\n",(int)_values_len,filename.c_str());
    //printf("max=%.10e min=%.10e average=%.10e\n",_max,_min,_average);
 
    //grib_handle_delete(_h);
 
    fclose(in);
	_sliceNum++;
    return 1;
}

string DCReaderGRIB::GetMapProjection() const {

    double lon_0 = (_minLon + _maxLon) / 2.0;
    double lat_0 = (_minLat + _maxLat) / 2.0;
    ostringstream oss;
    oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
    string projstring = "+proj=eqc +ellps=WGS84" + oss.str();

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

int DCReaderGRIB::_InitCartographicExtents(string mapProj,
                                           const std::vector<double> vertCoordinates,
                                           std::vector<double> &extents) const {

    Proj4API proj4API;

    int rc = proj4API.Initialize("", mapProj);
    if (rc<0) {
        SetErrMsg("Invalid map projection : %s", mapProj.c_str());
        return(-1);
    }   

    double x[] = {_minLon, _maxLon};
    double y[] = {_minLat, _maxLat};

    rc = proj4API.Transform(x,y,2,1);
    if (rc < 0) {
        SetErrMsg("Invalid map projection : %s", mapProj.c_str());
        return(-1);
    }  
	
	double top = BarometricFormula(vertCoordinates[vertCoordinates.size()-1]);
	double bottom = BarometricFormula(vertCoordinates[0]);

    extents.push_back(x[0]);
    extents.push_back(y[0]);
    extents.push_back(bottom);
    extents.push_back(x[1]);
    extents.push_back(y[1]);
    extents.push_back(top);

	cout << x[0] << " " << y[0] << " " << bottom << " " << x[1] << " " << y[1] << " " << top << endl;

    return 0;
}

int DCReaderGRIB::_Initialize(std::vector<std::map<std::string, std::string> > records) {

	_udunit = new UDUnits();

	_sliceNum = 0;
    int numRecords = records.size();

	_Ni = atoi(records[0]["Ni"].c_str());
	_Nj = atoi(records[0]["Nj"].c_str());
	_minLat = atof(records[0]["latitudeOfFirstGridPointInDegrees"].c_str());
	_minLon = atof(records[0]["longitudeOfFirstGridPointInDegrees"].c_str());
	_maxLat = atof(records[0]["latitudeOfLastGridPointInDegrees"].c_str());
	_maxLon = atof(records[0]["longitudeOfLastGridPointInDegrees"].c_str());

	if (_maxLon < 0) _maxLon += 360;

    for (int i=0; i<numRecords; i++) {
		std::map<std::string, std::string> record = records[i];
		string levelType = record["typeOfLevel"];
		float level = atof(record["level"].c_str());
		string name  = record["shortName"];
		string file = record["file"];
		int offset = atoi(record["offset"].c_str());
		int year   = atoi(record["yearOfCentury"].c_str());
		int month  = atoi(record["month"].c_str());
		int day    = atoi(record["day"].c_str());
		int hour   = atoi(record["hour"].c_str());
		int minute = atoi(record["minute"].c_str());
		int second = atoi(record["second"].c_str());
		
		double time = _udunit->EncodeTime(year, month, day, hour, minute, second); 
		if (std::find(_gribTimes.begin(), _gribTimes.end(), time) == _gribTimes.end())
			_gribTimes.push_back(time);

		if (std::find(_levels.begin(), _levels.end(), level) == _levels.end())
			if (level != 0.f) _levels.push_back(level);

		int isobaric = strcmp(levelType.c_str(),"isobaricInhPa");
		if (isobaric == 0) {								    // if we have a 3d var...
			if (_vars3d.find(name) == _vars3d.end()) {			// if we have a new 3d var...
				_vars3d[name] = new Variable();
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
			//_vars3d[name]->PrintIndex(time,level);
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

	}
	
	typedef std::map<std::string, Variable*>::iterator it_type;
	for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
    	_vars3d[iterator->first]->_SortLevels();           // Sort the levels that apply to each individual variable
	}
	for (it_type iterator=_vars2d.begin(); iterator!=_vars2d.end(); iterator++){
        _vars2d[iterator->first]->_SortLevels();			// Sort the levels that apply to each individual variable
    }
	sort(_levels.begin(), _levels.end());	// Sort the levels that apply to the entire dataset

	int rc = _InitCartographicExtents(GetMapProjection(),
			  						 _levels,
									 _cartesianExtents);
	if (rc<0) return -1;			 

	return 0;
}

//int DCReaderGRIB::_dump(int ts, string var) {
//}

void DCReaderGRIB::Print3dVars() {
    typedef std::map<std::string, Variable*>::iterator it_type;
    for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
        cout << iterator->first << endl;
        iterator->second->PrintTimes();
    } 
}

void DCReaderGRIB::GetGridDim(size_t dim[3]) const {
	dim[0]=_Ni; dim[1]=_Nj; dim[2]=_levels.size();
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
	_levels.clear();
	_gribTimes.clear();
	_vars2d.clear();
	_vars3d.clear();
}

GribParser::GribParser() {
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
    _consistentKeys.push_back("latitudeOfFirstGridPoinInDegrees");
    _consistentKeys.push_back("latitudeOfLastGridPointInDegrees");
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

int GribParser::_LoadRecord(string file, size_t index) {
    _filename = file;
    _in = fopen(_filename.c_str(),"rb");
    if(!_in) {
        printf("ERROR: unable to open file %s\n",_filename.c_str());
        return -1;
    }   

	size_t offset = atoi(_recordKeys[index]["offset"].c_str());
	fseek(_in,offset,SEEK_SET);

    _recordKeysVerified = 0;
    std::map<std::string, std::string> keyMap;
    	if ((_h = grib_handle_new_from_file(0,_in,&_err)) != NULL) {

        const void* msg;
        size_t size;
        grib_get_message(_h,&msg,&size);
        //cout << _grib_count << " " << size << endl;

        grib_keys_iterator* kiter=NULL;
        _grib_count++;
        //printf("-- Loading GRIB N. %d --\n",_grib_count);
        if(!_h) {
            printf("ERROR: Unable to create grib handle\n");
            return 1;
        }

        kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,_name_space);
        if (!kiter) {
            printf("ERROR: Unable to create keys iterator\n");
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

    //std::map<std::string, std::string>::iterator it = keyMap.begin(); 
    //for (it; it != keyMap.end(); ++it) {
        //cout << "  " << it->first << " " << it->second << endl;
    //}   

        grib_keys_iterator_delete(kiter);
        grib_handle_delete(_h);
    }


    return 0;
}

int GribParser::_LoadRecordKeys(string file) {
	_filename = file;
    _in = fopen(_filename.c_str(),"rb");
    if(!_in) {
        printf("ERROR: unable to open file %s\n",_filename.c_str());
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
		offset+=size;
		ss << offset;		
		keyMap["offset"] = ss.str();
		ss.str(std::string());
		ss.clear();
		keyMap["file"] = file;

        grib_keys_iterator* kiter=NULL;
        _grib_count++;
        //printf("-- Loading GRIB N. %d --\n",_grib_count);
        if(!_h) {
            printf("ERROR: Unable to create grib handle\n");
            return 1;
        }   
            
        kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,_name_space);
        if (!kiter) {
            printf("ERROR: Unable to create keys iterator\n");
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

		//cout << keyMap["shortName"] << " " << keyMap["level"] << " " << keyMap["file"] << " " << keyMap["offset"] << endl;
		_recordKeys.push_back(keyMap);
		keyMap.clear();
        grib_keys_iterator_delete(kiter);
        grib_handle_delete(_h);
    }
	_grib_count=0;   
    return 0;
}

int GribParser::_LoadAllRecordKeys(string file) {
    _filename = file;
    _in = fopen(_filename.c_str(),"rb");
    if(!_in) {
        printf("ERROR: unable to open file %s\n",_filename.c_str());
        return -1;
    } 
	
	_recordKeysVerified = 0;
	std::map<std::string, std::string> keyMap;
	while((_h = grib_handle_new_from_file(0,_in,&_err)) != NULL) {
		grib_keys_iterator* kiter=NULL;
		_grib_count++;
		//printf("-- Loading GRIB N. %d --\n",_grib_count);
		if(!_h) {
			printf("ERROR: Unable to create grib handle\n");
			return 1;
		}
		
		kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,_name_space);
		if (!kiter) {
			printf("ERROR: Unable to create keys iterator\n");
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
	return 0;
}

int GribParser::_VerifyKeys() {
	int numRecords = _recordKeys.size();
  
	for (int i=0; i<numRecords; i++) {
		//cout << "Analyzing GRIB Record " << i << endl;
		
		//cout << "Verifying grid types among all records" << endl;
		string grid;
        grid = _recordKeys[i]["gridType"];
        if ((strcmp(grid.c_str(),"regular_ll")!=0) && (strcmp(grid.c_str(),"regular_gg")!=0)){
            cout << "Error: Invalid grid specification ('" << grid << "') for Record No. " << i << endl;
            return -1;
        } 

		for (size_t k=0; k<_consistentKeys.size(); k++) {
			//cout << "	Verifying consistency of " << _consistentKeys[k] << endl;
			// Check for inconsistent key across multiple records
			if (_recordKeys[i][_consistentKeys[k]].compare(_recordKeys[0][_consistentKeys[k]]) != 0) {
				cout << "Error: Inconsistent key found in Record No. " << i << ", Key: " << _consistentKeys[k] << endl;
				return -1;
			}
		}
	}
	_recordKeysVerified = 1; 
	return 0;
}

int GribParser::_DataDump() {
	/* create new handle from a message in a file*/
	_h = grib_handle_new_from_file(0,_in,&_err);
	if (_h == NULL) {
		printf("Error: unable to create handle from file %s\n",_filename.c_str());
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

int GribParser::_InitializeDCReaderGRIB() {
	if (!_recordKeysVerified) {				// Make sure that our set of keys
		if (_VerifyKeys()) return 1;			// conforms to our requirements
	}

	_metadata = new DCReaderGRIB();
	_metadata->_Initialize(_recordKeys);
	
	return 0;
}

GribParser::~GribParser() {
	if (_value) delete _value;
}
