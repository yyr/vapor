//************************************************************************
//																	   *
//		   Copyright (C)  2004										 *
//	 University Corporation for Atmospheric Research				   *
//		   All Rights Reserved										 *
//																	   *
//************************************************************************/
//
//  File:        DCReaderGRIB.cpp
//
//  Author:      Scott Pearse
//               National Center for Atmospheric Research
//               PO 3000, Boulder, Colorado
//
//  Date:	     June 2014
//
//  Description: TBD 
//			   
//

#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>
#include <iterator>
#include <cassert>
#include <string>
#include <cmath>

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
	_pressureLevels.clear();
	_indices.clear();
}

DCReaderGRIB::Variable::~Variable() {
	if (_messages.size()) _messages.clear();
	if (_unitTimes.size()) _unitTimes.clear();
	if (_varTimes.size()) _varTimes.clear();
	if (_pressureLevels.size()) _pressureLevels.clear();
	if (_indices.size()) _indices.clear();
}

bool DCReaderGRIB::Variable::_Exists(double time) const {
	if (std::find(_unitTimes.begin(), _unitTimes.end(), time)==_unitTimes.end()) return 0;
	return 1;
}

int DCReaderGRIB::Variable::GetOffset(double time, float level) const {
	return ((_indices.find(time)->second).find(level)->second).offset;
}

string DCReaderGRIB::Variable::GetFileName(double time, float level) const {
	return ((_indices.find(time)->second).find(level)->second).fileName;
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
	_iValues = NULL;
	_ignoreForecastData = 0;
	_Ni = 0;
	_Nj = 0;
	_pressureLevels.clear();
	_cartesianExtents.clear();
	_gribTimes.clear();
	_vars2d.clear();
	_vars3d.clear();
	_udunit = NULL;

	DCReaderGRIB::_Initialize(files);
}

int DCReaderGRIB::OpenVariableRead(size_t timestep, string varname,
							 int reflevel, int lod) {

	if (timestep > _gribTimes.size()) return -1;

	_openVar = varname;
	_openTS = timestep;

	Variable *targetVar;
	string filename;
	float level;
	double usertime = GetTSUserTime(_openTS);
	if (_vars3d.find(varname) != _vars3d.end()) {	   // we have a 3d var
		targetVar = _vars3d[_openVar];
		_sliceNum = _pressureLevels.size()-1;
		level = targetVar->GetLevel(_sliceNum);
		filename = targetVar->GetFileName(usertime,level);
	}
	else if (_vars2d.find(varname) != _vars2d.end()) {  // we have a 2d var
		targetVar = _vars2d[_openVar];
		level = targetVar->GetLevel(0);
		filename = targetVar->GetFileName(usertime,level);
		_sliceNum = 0;
	}
	else return -1; 		// variable does not exist
	
	_inFile = fopen(filename.c_str(),"rb");
	if(!_inFile) {
		MyBase::SetErrMsg("ERROR: unable to open file %s",filename.c_str());
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

int DCReaderGRIB::ReadSlice(float *values){
	
	if (_sliceNum < 0) return 0;
	int savedSliceNum;
	Variable *targetVar;
	if (_vars3d.find(_openVar) != _vars3d.end()) {	   // we have a 3d var
		targetVar = _vars3d[_openVar];
	}
	else if (_vars2d.find(_openVar) != _vars2d.end()) {  // we have a 2d var
		targetVar = _vars2d[_openVar];
		savedSliceNum = _sliceNum;
		_sliceNum = 0;
	}
	else {
		MyBase::SetErrMsg("Variable %s not found in 2d or 3d variable set",_openVar.c_str());
		return -1;
	}
	double usertime = GetTSUserTime(_openTS);
	float level = targetVar->GetLevel(_sliceNum);
	int offset = targetVar->GetOffset(usertime,level);
	string filename = targetVar->GetFileName(usertime,level);	

	int rc = fseek(_inFile,offset,SEEK_SET);
	if (rc != 0) {
		MyBase::SetErrMsg("fseek error during GRIB ReadSlice");  
		return -1;
	}

	// Check to see if boot.def can be found, otherwise
	// grib_api will catastrophically fail
	string bootDefFile = getenv("GRIB_DEFINITION_PATH");
	std::ifstream infile(bootDefFile);
	if (!infile.good()) {
		MyBase::SetErrMsg("ERROR: unable to access boot.def for grib_api."
		"  Check for [VAPORHOME]/share/grib_api/definitions/boot.def");
		return -1;
	}

	// create new handle from a message in a file
	int err;
	grib_handle* h = grib_handle_new_from_file(0,_inFile,&err);
	if (h == NULL) {
		MyBase::SetErrMsg("Error: unable to create handle from file %s",filename.c_str());
		return -1;
	}   

	// get the size of the _values array
	size_t values_len;
	GRIB_CHECK(grib_get_size(h,"values",&values_len),0);
	double* _dvalues = new double[values_len];
 
	// get data _values
	GRIB_CHECK(grib_get_double_array(h,"values",_dvalues,&values_len),0);

	// re-order values according to scan direciton and convert doubles to floats
	float min,max;
	min = (float) _dvalues[0];
	max = (float) _dvalues[0];
	int vaporIndex, i, j;
	for(size_t gribIndex = 0; gribIndex < values_len; gribIndex++) {
		if (_iScanNeg == 1) {
			if (_jScanPos == 1) {   // 1 1
				i = _Ni - gribIndex%_Ni;
				j = gribIndex/_Ni;
				vaporIndex = j*_Ni + i;
			}
			else {			  // 1 0
				vaporIndex = _Ni*_Nj - gribIndex - 1;
			}
		}

		else {
			if (_jScanPos == 0) {  // 0 0
				i = gribIndex % _Ni;
				j = ((_Ni * _Nj) - 1 - gribIndex) / _Ni;
				vaporIndex = j * _Ni + i;
			}
			else {
				vaporIndex = gribIndex;
			}
		}

		//string oc = targetVar->getOperatingCenter();
		//int id = targetVar->getParamId();
		//If we are looking at geopotential height, divide by g
		//if ((oc == "ecmf") && (id = 126)) { 
		if (_openVar == "z") {		// z is geopotential.  Divide by g to get geopotential height
			values[vaporIndex] = (float) _dvalues[gribIndex] / 9.8;
		}
		else {
			values[vaporIndex] = (float) _dvalues[gribIndex];
		}
		if (values[vaporIndex] < min) min = values[vaporIndex];
		if (values[vaporIndex] > max) max = values[vaporIndex];
	}

	// Apply linear interpolation on _values if we are on a gaussian grid
	//if(!strcmp(_gridType.c_str(),"regular_gg")) _LinearInterpolation(values);

	//cout << _openVar << " " << _openTS << " " <<  usertime << " " << level << " " << _sliceNum << " " << offset << " " << filename << " " << min << " " << max << endl;
	delete [] _dvalues;

	grib_handle_delete(h);

	_sliceNum--;	
	return 1;
}

void DCReaderGRIB::_LinearInterpolation(float *values) {
	if (_iValues) delete [] _iValues;
	_iValues = new float[_Ni*_Nj];
	int lat, gLatIndex, dataIndex1, dataIndex2;
	float weight, point1, point2;

	for (int i=0; i<_Ni*_Nj; i++) {
		lat = i/_Ni;							// find the index of our regular latitude to be interpolated
		weight = _weights[lat];					// the weight that corresponds to our current regular latitude index
		gLatIndex = _latIndices[lat];			// find the gaussian latitude index below our regular latitude
		dataIndex1 = gLatIndex * _Ni + i;
		dataIndex2 = (gLatIndex+1) * _Ni + i;
		point1 = values[dataIndex1];			// data point below our interpolation
		point2 = values[dataIndex2];			// data point above our interpolation
		_iValues[i] = weight * point1 + (1-weight) * point2;
	}
	values = _iValues;
}

string DCReaderGRIB::GetMapProjection() const {
	double lat_0;
	double lon_0;
	double lat_ts;
	ostringstream oss;
	string projstring;
	if (!strcmp(_gridType.c_str(), "regular_ll") || 
		!strcmp(_gridType.c_str(), "regular_gg")){
		lon_0 = (_minLon + _maxLon) / 2.0;
		lat_0 = (_minLat + _maxLat) / 2.0;
		oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0;
		projstring = "+proj=eqc +ellps=WGS84" + oss.str();
	}
	else if(!strcmp(_gridType.c_str(),"polar_stereographic")){
		if (_minLat < 0.){
			lat_0 = -90.0;
			lat_ts = -60.0;
		}
		else {
			lat_0 = 90.0; 
			lat_ts = 60.0;
		}

		string lon_0 = _orientationOfTheGridInDegrees;

		oss << " +lon_0=" << lon_0 << " +lat_0=" << lat_0 << " +lat_ts=" << lat_ts;
		
		projstring = "+proj=stere +ellps=WGS84" + oss.str();	
	}
	else if(!strcmp(_gridType.c_str(),"lambert")) {
		lon_0 = _LoVInDegrees;
		string lat_1 = _Latin1InDegrees;
		string lat_2 = _Latin2InDegrees;

		oss << " +lon_0=" << lon_0 << " +lat_1=" << lat_1 << " +lat_2=" << lat_2;
	
		projstring = "+proj=lcc +ellps=WGS84" + oss.str();
	}
	else if(!strcmp(_gridType.c_str(),"mercator")) {
		lon_0 = (_minLon + _maxLon) / 2;
		lat_ts = _LaDInDegrees;	
		
		oss << " +lon_0=" << lon_0 << " +lat_ts=" << lat_ts;
		projstring = "+proj=merc +ellps=WGS84" + oss.str();
	}
	else projstring = "";
	return(projstring);
}


double DCReaderGRIB::BarometricFormula(const double pressure) const {
	double Po = 101325;  // (kg/m3)	Mass density of air @ MSL
	double g  = 9.81;	// (m/s2)	 gravity
	double M  = .0289;   // (kg/mol)   Molar mass of air
	double R  = 8.31447; // (J/(mol•K) Univ. gas constant
	double To = 288.15;  // (K)		Std. temperature of air @ MSL
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
	if (!strcmp(_gridType.c_str(),"regular_ll") ||
		!strcmp(_gridType.c_str(),"regular_gg") ||
		!strcmp(_gridType.c_str(),"mercator")) {
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
	else if(!strcmp(_gridType.c_str(),"polar_stereographic") || 
			(!strcmp(_gridType.c_str(),"lambert"))) {
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

	float min, max;
	if (!strcmp(_gridType.c_str(),"regular_gg")) {
		min = 0;
		max = 0;
	}
	else if (GetGridType() == "stretched"){
		for (int i=0; i<_pressureLevels.size(); i++) {
			double height = BarometricFormula(_pressureLevels[i]);
			_vertCoordinates.push_back(height);
		}
	
		min = _vertCoordinates.front();
		max = _vertCoordinates.back();
	}
	else {	
		if (OpenVariableRead(0,"ELEVATION",0,0) != 0) return -1;
		else {
			float *values = new float[_Ni*_Nj];
			ReadSlice(values);

			min = values[0];
			for (int i=0; i<_Ni; i++){
				for (int j=0; j<_Nj; j++) {
					float value = values[j*_Ni+i];
					if (value < min) min = value;
				}   
			} 

			_sliceNum = 0;
			ReadSlice(values);

			max = values[0];
			for (int i=0; i<_Ni; i++){
				for (int j=0; j<_Nj; j++) {
					float value = values[j*_Ni+i];
					if (value > max) max = value;
				}   
			}
			if (values) delete [] values;
		}
	}
	
	#ifdef DEAD
	for (i=0; i<_pressureLevels.size(); i++) {
		double height = BarometricFormula(_pressureLevels[i]);
		_vertCoordinates.push_back(height);
	}
	
	double bottom = _vertCoordinates[0];
	double top = _vertCoordinates[_vertCoordinates.size()-1];
	#endif

	_cartographicExtents.push_back(x[0]);
	_cartographicExtents.push_back(y[0]);
	_cartographicExtents.push_back(min);
	_cartographicExtents.push_back(x[1]);
	_cartographicExtents.push_back(y[1]);
	_cartographicExtents.push_back(max);

	return 0;
}

int DCReaderGRIB::_Initialize(const vector <string> files) {
	int rc;
	parser = new GribParser();
	for (int i=0; i<files.size(); i++){
		rc = parser->_LoadRecordKeys(files[i]);
		if (rc !=0) {
			MyBase::SetErrMsg("ERROR: Unable to operate on file %s.  Program aborting.",files[i].c_str());
			return -1;
		}
	}
	rc = parser->_VerifyKeys();
	if (rc<0) return -1; 

	std::vector<std::map<std::string, std::string> > records = parser->GetRecords();

	// If the second record contains 'simulation' data, we will ignore all 
	// other records that contain 'forecast' data
	// Note: we cannot assess this from the first record.  The first timestep
	// in all simulation data will have P2=0, just like with simulation data
	//if (atof(records[1]["P2"].c_str()) == 0.0) _ignoreForecastData = 1;
	//if (records[0]["P2"] == "") _ignoreForecastData = 1;

	_ignoreForecastData = parser->doWeIgnoreForecastTimes();

	_udunit = new UDUnits();

	int numRecords = records.size();

	_gridType = records[0]["gridType"];
	if (_gridType.compare("regular_gg") == 0) {
		MyBase::SetErrMsg("Error: The grid type 'regular_gg' is currently unsupported. Process aborting.");
		return -1;		
	}

	_Ni = atoi(records[0]["Ni"].c_str());
	_Nj = atoi(records[0]["Nj"].c_str());
	_iScanNeg = atoi(records[0]["iScanNegsNegatively"].c_str());
	_jScanPos = atoi(records[0]["jScansPositively"].c_str());
	_DxInMetres = atof(records[0]["DxInMetres"].c_str());	
	_DyInMetres = atof(records[0]["DyInMetres"].c_str());	
	_LaDInDegrees = atof(records[0]["LaDInDegrees"].c_str());
	_LoVInDegrees = atof(records[0]["LoVInDegrees"].c_str());
	_Latin1InDegrees = records[0]["Latin1InDegrees"];
	_Latin2InDegrees = records[0]["Latin2InDegrees"];
	_orientationOfTheGridInDegrees = records[0]["orientationOfTheGridInDegrees"];
	
	int Nx = atoi(records[0]["Nx"].c_str());
	int Ny = atoi(records[0]["Ny"].c_str());
	double DiInMetres = atof(records[0]["DiInMetres"].c_str());	
	double DjInMetres = atof(records[0]["DjInMetres"].c_str());

	if(!strcmp(_gridType.c_str(),"polar_stereographic") ||
		!strcmp(_gridType.c_str(),"lambert")){
		_Ni = Nx;
		_Nj = Ny;
	}

	if(!strcmp(_gridType.c_str(),"mercator")) {
		_DxInMetres = DiInMetres;
		_DyInMetres = DjInMetres;
	}


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
	if (_minLon > _maxLon) _minLon = _minLon - 360;

	if (!strcmp(_gridType.c_str(),"regular_gg")) {
		_generateWeightTable();
	}
	
	int year=0;
	int month=0;
	int day=0;
	int hour=0;
	int minute=0;
	int second=0;

	for (int i=0; i<numRecords; i++) {
		std::map<std::string, std::string> record = records[i];
		string levelType = record["typeOfLevel"];
		float level = atof(record["level"].c_str());
		string name  = record["shortName"];
		string file = record["file"];
		int paramId = atoi(record["paramId"].c_str());
		string operatingCenter = record["centre"];
		int offset = atoi(record["offset"].c_str());		
		float P2 = atof(record["P2"].c_str());
		if (!((P2 > 0.0) && _ignoreForecastData)) {
			
			std::stringstream ss;
			string date = record["dataDate"];
			string time = record["dataTime"];
			ss << date[0] << date[1] << date[2] << date[3];
			year = atoi(ss.str().c_str());
		
			ss.str(std::string());	
			ss.clear();
			ss << date[4] << date[5];
			month = atoi(ss.str().c_str());
				
			ss.str(std::string());
			ss.clear();
			ss << date[6] << date[7];
			day = atoi(ss.str().c_str());
				
			ss.str(std::string());
			ss.clear();
			ss << time[0] << time[1];
			hour = atoi(record["dataTime"].c_str())/100;
			if (P2 > 0.0) hour += int(P2);
			ss.str(std::string());
			ss.clear();

			minute = 0;
			second = 0;

			#ifdef DEAD	
			// Alternative date/time generator			
			year   = atoi(record["yearOfCentury"].c_str()) + 2000;
			month  = atoi(record["month"].c_str());
			day	= atoi(record["day"].c_str());
			hour   = atoi(record["hour"].c_str());
			minute = atoi(record["minute"].c_str());
			second = atoi(record["second"].c_str());
			#endif
		}

		double time = _udunit->EncodeTime(year, month, day, hour, minute, second); 
		bool _iScanNeg = atoi(record["_iScanNegsNegatively"].c_str());
		bool _jScan = atoi(record["_jScansPositively"].c_str());
	
		//if (std::find(_pressureLevels.begin(), _pressureLevels.end(), level) == _pressureLevels.end()) {
			//if (!strcmp(name.c_str(),"gh")) _pressureLevels.push_back(level);
		//	if (level != 0) _pressureLevels.push_back(level);
		//}


		//////
		//	Process all 3D vars!
		//////
		//int isobaric = strcmp(levelType.c_str(),"isobaricInhPa");
		//if (isobaric == 0) {									// if we have a 3d var...
		if (levelType == "isobaricInhPa") {
			if (std::find(_pressureLevels.begin(), _pressureLevels.end(), level) == _pressureLevels.end()) {
				_pressureLevels.push_back(level);
			}

			if (std::find(_gribTimes.begin(), _gribTimes.end(), time) == _gribTimes.end())
				_gribTimes.push_back(time);

			if (_vars3d.find(name) == _vars3d.end()) {			// if we have a new 3d var...
				_vars3d[name] = new Variable();
				_vars3d[name]->setParamId(paramId);
				_vars3d[name]->setOperatingCenter(operatingCenter);
				_vars3d[name]->setScanDirection(_iScanNeg,_jScan);
			}

			std::vector<double> varTimes = _vars3d[name]->GetTimes();
			if (std::find(varTimes.begin(),varTimes.end(),time) == varTimes.end())	// we only want to record new timestamps
				_vars3d[name]->_AddTime(time);										// for our vars.  (_gribTimes records all times 
		
			// Add level data for current var object
			std::vector<float> varLevels = _vars3d[name]->GetLevels();
			if (std::find(varLevels.begin(),varLevels.end(),level) == varLevels.end())
				_vars3d[name]->_AddLevel(level);		
			
			_vars3d[name]->_AddMessage(i);									// in the grib file)
			_vars3d[name]->_AddIndex(time,level,file,offset);
		}


		//////
		//	Process all 2D vars!
		//////
		int surface = strcmp(levelType.c_str(),"surface");
		int meanSea = strcmp(levelType.c_str(),"meanSea");
		if ((surface == 0) || (meanSea == 0)) {					// if we have a 2d var...
				if (_vars2d.find(name) == _vars2d.end()) {			// if we have a new 2d var...
					_vars2d[name] = new Variable();
					_vars2d[name]->setParamId(paramId);
				}

			// Add level data to current var object
			// (this should only happen once for a 2D var)
			std::vector<float> varLevels = _vars2d[name]->GetLevels();
			if (std::find(varLevels.begin(),varLevels.end(),level) == varLevels.end())
				_vars2d[name]->_AddLevel(level);

			_vars2d[name]->_AddTime(time);
			_vars2d[name]->_AddMessage(i);	
			_vars2d[name]->_AddIndex(time,level,file,offset);
		}

		//////
		//	Process all 1D vars!
		//////
		int entireAtmos = strcmp(levelType.c_str(),"entireAtmosphere");
		if (entireAtmos == 0) {									// if we have a 1d var...
			if (_vars1d.find(name) == _vars1d.end()) { 			// if we have a new 2d var...
				_vars1d[name] = new Variable();
				_vars1d[name]->setParamId(paramId);
			}   
			_vars1d[name]->_AddTime(time);
			_vars1d[name]->_AddMessage(i);   
			_vars1d[name]->_AddIndex(time,level,file,offset);
		}
	}
	
	typedef std::map<std::string, Variable*>::iterator it_type;
	for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
		if (iterator->first == "gh") _vars3d["ELEVATION"] = new Variable(*_vars3d["gh"]);	// KISTI's definition of geopotential height
		if (iterator->first == "z") _vars3d["ELEVATION"] = new Variable(*_vars3d["z"]);	// KISTI's definition of geopotential height
		//if ((_vars3d[iterator->first]->getParamId() == 129) &&
		//	(_vars3d[iterator->first]->getOperatingCenter() == "ecmf"))
		//		_vars3d["ELEVATION"] = new Variable(*_vars3d[iterator->first]);
	}

	for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
		_vars3d[iterator->first]->_SortLevels();		   //  Sort the levels that apply to each individual variable
		_vars3d[iterator->first]->_SortTimes();				// Sort udunit times that apply to each individual variable
	}
	for (it_type iterator=_vars2d.begin(); iterator!=_vars2d.end(); iterator++){
		_vars2d[iterator->first]->_SortLevels();			// Sort the levels that apply to each individual variable
		_vars2d[iterator->first]->_SortTimes();			 // Sort udunit times that apply to each individual variable
	}

	sort(_pressureLevels.begin(), _pressureLevels.end());	// Sort the level that apply to the entire dataset
	reverse(_pressureLevels.begin(), _pressureLevels.end());

	sort(_gribTimes.begin(), _gribTimes.end());

	rc = _InitCartographicExtents(GetMapProjection());
	
	_sliceNum = _pressureLevels.size()-1;

	if (parser) delete parser;

	if (rc<0) return -1;			 

	return 0;
}

string DCReaderGRIB::GetGridType() const {
	if (!strcmp("regular_gg",_gridType.c_str())) return "regular";
	else if (_vars3d.find("ELEVATION") != _vars3d.end()) return "layered";
	else return "stretched";
}

void DCReaderGRIB::_generateWeightTable() {
	_gaussianLats = new double[_Nj];
	grib_get_gaussian_latitudes(_Nj/2,_gaussianLats);

	_regularLats = new double[_Nj];
	double increment = (_maxLat - _minLat) / (_Nj-1);
	for (int i=0; i<_Nj; i++) {
		_regularLats[i] = _maxLat - i*increment;
	}

	double regular, gaussianBelow, gaussianAbove, weight;
	for (int regj=0; regj<_Nj; regj++) {

		// Find nearest neighbor below current regular lat
		for (int gausj=0; gausj<_Nj; gausj++) {
			if (_regularLats[regj] >= _gaussianLats[gausj]) { // this gaussian lat is below our regular lat
				regular = _regularLats[regj];
				gaussianAbove = _gaussianLats[gausj];
				gaussianBelow = _gaussianLats[gausj+1];
				break;
			}
		}

		// calculate weights
		weight = 1 - abs(( ((float) regular  - (float)gaussianAbove)) 
					/ ((float)gaussianAbove - (float)gaussianBelow));
		_weights.push_back(weight);				// Store the weight for gaussian point "above" a regular latitude
		_latIndices.push_back(regj);			// Store the guassian point "above" the indexed latitude
	}
}

void DCReaderGRIB::Print3dVars() {
	typedef std::map<std::string, Variable*>::iterator it_type;
	for (it_type iterator=_vars3d.begin(); iterator!=_vars3d.end(); iterator++){
		cout << iterator->first << endl;
		iterator->second->PrintTimes();
	} 
}

void DCReaderGRIB::Print2dVars() {
	typedef std::map<std::string, Variable*>::iterator it_type;
	for (it_type iterator=_vars2d.begin(); iterator!=_vars2d.end(); iterator++){
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
	double seconds = _gribTimes[ts];
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

	s = oss.str();
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
	if (_vars3d.find(varname) != _vars3d.end()){		// we have a 3d var
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
	vector<string> empty;
	return empty;
}

std::vector<std::string> DCReaderGRIB::GetVariables3DExcluded() const {
	vector<string> empty;
	return empty;
}

DCReaderGRIB::~DCReaderGRIB() {
	if (_udunit) delete _udunit;
	_cartesianExtents.clear();
	_cartographicExtents.clear();
	_pressureLevels.clear();
	_vertCoordinates.clear();
	_gribTimes.clear();

	typedef std::map<std::string, Variable*>::iterator it_type;
	for (it_type it = _vars1d.begin(); it != _vars1d.end(); it++) {
		if (it->second) delete it->second;
		it->second = NULL;
		_vars1d[it->first] = NULL;
	}

	for (it_type it = _vars2d.begin(); it != _vars2d.end(); it++) {
		if (it->second) delete it->second;
		_vars2d[it->first] = NULL;
	}

	for (it_type it = _vars3d.begin(); it != _vars3d.end(); it++) {
		if (it->second) delete it->second;
		_vars3d[it->first] = NULL;
	}

	_vars1d.clear();
	_vars2d.clear();
	_vars3d.clear();
	_weights.clear();
	_latIndices.clear();
}

DCReaderGRIB::GribParser::GribParser() {
	_recordKeys.clear();
	_consistentKeys.clear();
	_varyingKeys.clear();

	// key iteration vars
	_key_iterator_filter_flags	 = GRIB_KEYS_ITERATOR_ALL_KEYS;
	_grib_count					= 0;
	_value						 = new char[MAX_VAL_LEN];
	_vlen						  = MAX_VAL_LEN;	
	_recordKeysVerified			   = 0;
	
	// _consistentKeys must be the same across all records
	_consistentKeys.push_back("gridType");
	_consistentKeys.push_back("longitudeOfFirstGridPointInDegrees");
	_consistentKeys.push_back("longitudeOfLastGridPointInDegrees");
	_consistentKeys.push_back("latitudeOfFirstGridPointInDegrees");
	_consistentKeys.push_back("latitudeOfLastGridPointInDegrees");
	_consistentKeys.push_back("Latin1InDegrees");
	_consistentKeys.push_back("Latin2InDegrees");
	_consistentKeys.push_back("LoVInDegrees");
	_consistentKeys.push_back("LaDInDegrees");
	_consistentKeys.push_back("orientationOfTheGridInDegrees");
	_consistentKeys.push_back("iScansNegatively");
	_consistentKeys.push_back("jScansPositively");
	_consistentKeys.push_back("DxInMetres");
	_consistentKeys.push_back("DyInMetres");
	_consistentKeys.push_back("DiInMetres");
	_consistentKeys.push_back("DjInMetres");
	_consistentKeys.push_back("Ni");
	_consistentKeys.push_back("Nj");
	_consistentKeys.push_back("Nx");
	_consistentKeys.push_back("Ny");

	// _varyingKeys are those that may change over time, but must
	// fit some criteria for a legal data conversion
	_varyingKeys.push_back("paramId");
	_varyingKeys.push_back("centre");
	_varyingKeys.push_back("shortName");
	_varyingKeys.push_back("units");
	_varyingKeys.push_back("yearOfCentury");
	_varyingKeys.push_back("month");
	_varyingKeys.push_back("day");
	_varyingKeys.push_back("hour");
	_varyingKeys.push_back("minute");
	_varyingKeys.push_back("second");
	_varyingKeys.push_back("P2");
	_varyingKeys.push_back("typeOfLevel");
	_varyingKeys.push_back("level");	
	_varyingKeys.push_back("dataDate");
	_varyingKeys.push_back("dataTime");

	_values	 = NULL;
	_values_len = 0;	
	_err		= 0;
}

int DCReaderGRIB::GribParser::_LoadRecordKeys(string file) {
	FILE* _in = fopen(file.c_str(),"rb");
	if(!_in) {
		MyBase::SetErrMsg("ERROR: unable to open file %s.",file.c_str());
		return -1;
	} 

	grib_handle *_h=NULL;
	grib_keys_iterator* kiter=NULL;
	const void* msg;
	char* name_space = NULL;
	size_t size;
	size_t offset=0;
	stringstream ss;
	_recordKeysVerified = 0;
	std::map<std::string, std::string> keyMap;

	// Check to see if boot.def can be found, otherwise
	// grib_api will catastrophically fail
	string bootDefFile = getenv("GRIB_DEFINITION_PATH");
	std::ifstream inFile(bootDefFile);
	if (!(inFile.good())) {
		MyBase::SetErrMsg("ERROR: unable to access boot.def for grib_api."
		"  Check for [VAPORHOME]/share/grib_api/definitions/boot.def");
		return -1;
	}

	while((_h = grib_handle_new_from_file(0,_in,&_err))) {

		if(_h==NULL) {
			MyBase::SetErrMsg("Unable to create grib handle");
			fclose(_in);
			return -1;
		}   
		
		for (size_t i=0; i<_consistentKeys.size(); i++) {
			keyMap[_consistentKeys[i]] = "";
		}
		for (size_t i=0; i<_varyingKeys.size(); i++) {
			keyMap[_varyingKeys[i]] = "";
		}

		grib_get_message(_h,&msg,&size);
		ss << offset;		
		keyMap["offset"] = ss.str();
		ss.str(std::string());
		ss.clear();
		offset+=size;
		keyMap["file"] = file;

		_grib_count++;
		if(!_h) {
			MyBase::SetErrMsg("Unable to create grib handle");
			fclose(_in);
			return -1;
		}   
			
		kiter = grib_keys_iterator_new(_h,_key_iterator_filter_flags,name_space);
		if (!kiter) {
			MyBase::SetErrMsg("Unable to create keys iterator");
			fclose(_in);
			return -1;
		}   

		while(grib_keys_iterator_next(kiter)) {
			string name = grib_keys_iterator_get_name(kiter);
			if (
				find(
					_varyingKeys.begin(), _varyingKeys.end(), name
				) == _varyingKeys.end()
				&& 
				find(
					_consistentKeys.begin(), _consistentKeys.end(),name
				) == _consistentKeys.end()
			) {

				continue;
			}

			_vlen=MAX_VAL_LEN;
			bzero(_value,_vlen);
			GRIB_CHECK(grib_get_string(_h,name.c_str(),_value,&_vlen),name.c_str());
			std::string gribKey(name);
			std::string gribValue(_value);
			keyMap[gribKey] = gribValue;
		}   

		_recordKeys.push_back(keyMap);
		keyMap.clear();
		grib_keys_iterator_delete(kiter);
		grib_handle_delete(_h);
	}

	// if offset did not increment, we read no grib records and were given an invalid file
	if (offset == 0) {
		MyBase::SetErrMsg("ERROR: Unable to create grib_handle from file %s.",file.c_str());
		return -1;
	}

	_grib_count=0;  
	fclose(_in); 
	return 0;
}

int DCReaderGRIB::GribParser::_VerifyKeys() {
	int numRecords = _recordKeys.size(); 
	string dataDate = _recordKeys[0]["dataDate"];
	string dataTime = _recordKeys[0]["dataTime"];
	string P2 = _recordKeys[0]["P2"];
	bool dateChanged = 0;
	bool P2Changed = 0;
	_ignoreForecastTimes = 0;

	for (int i=0; i<numRecords; i++) {
		
		string grid;
		grid = _recordKeys[i]["gridType"];
		if ((strcmp(grid.c_str(),"regular_ll")!=0) && 
			(strcmp(grid.c_str(),"regular_gg")!=0) &&
			(strcmp(grid.c_str(),"polar_stereographic")!=0) &&
			(strcmp(grid.c_str(),"lambert")) && 
			(strcmp(grid.c_str(),"mercator"))) {
			MyBase::SetErrMsg("Error: Invalid grid specification ('%s') for Record No. %d",grid.c_str(),i);
			return -1;
		} 

		for (size_t k=0; k<_consistentKeys.size(); k++) {
			// Check for inconsistent key across multiple records
			if (_recordKeys[i][_consistentKeys[k]].compare(_recordKeys[0][_consistentKeys[k]]) != 0) {
				MyBase::SetErrMsg("Error: Inconsistent key found in Record No. %i, Key: %s",i,_consistentKeys[k].c_str());
				return -1;
			}
		}

		if ((dataDate.compare(_recordKeys[i]["dataDate"]) ||
			(dataTime.compare(_recordKeys[i]["dataTime"])))) dateChanged = 1;
		if (P2.compare(_recordKeys[i]["P2"])) P2Changed = 1;
		if (dateChanged && P2Changed) _ignoreForecastTimes = 1;

	}
	_recordKeysVerified = 1; 
	return 0;
}

DCReaderGRIB::GribParser::~GribParser() {
	if (_value) delete [] _value;
	if (_values) delete _values;
	_recordKeys.clear();
	_consistentKeys.clear();
	_varyingKeys.clear();
}
