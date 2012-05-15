#include <Python.h>
#include "pythonpipeline.h"
#include <numpy/arrayobject.h>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include <vapor/errorcodes.h>
#include <vapor/GetAppPath.h>
#include "pythonpipeline.h"
#include <QMutex>

using namespace VetsUtil;
using namespace VAPoR;

std::string PythonPipeLine::startupScript = "";
bool PythonPipeLine::everInitialized = false;
PyObject* PythonPipeLine::vaporModule = 0;

DataMgr* PythonPipeLine::currentDataMgr = 0;
bool PythonPipeLine::initialized = false;
std::map<PyObject*, float*> PythonPipeLine::arrayAllocMap;
QMutex* PythonPipeLine::arrayAllocMutex = new QMutex();

PythonPipeLine::PythonPipeLine(string name, vector<string> inputs, 
		vector < pair < string, DataMgr::VarType_T > > outputs, DataMgr* dmgr) :
PipeLine(name, inputs, outputs)
{
	currentDataMgr = dmgr;
}

int PythonPipeLine::Calculate (
	vector <const RegularGrid *> input_grids,
	vector <RegularGrid *> output_grids,	// space for the output variables
	size_t ts, // current time step
	int reflevel, // refinement level
	int lod
 ) {
	//call python wrapper.
	int scriptId = DataStatus::getInstance()->getDerivedScriptId(GetName());
	return python_wrapper(scriptId,ts,reflevel,lod,
						  GetInputs(), input_grids,
						  GetOutputs(), output_grids);
}

PyMethodDef PythonPipeLine::vaporMethodDefinitions[] = { 
		{"Get3DVariable",PythonPipeLine::get_3Dvariable, METH_VARARGS,
                        "Get a variable from the DataMgr cache"},
		{"Get2DVariable",PythonPipeLine::get_2Dvariable, METH_VARARGS,
                        "Get a variable from the DataMgr cache"},
		{"MapVoxToUser",PythonPipeLine::mapVoxToUser, METH_VARARGS,
						"Map voxel to user coordinates at specified refinement level"},
		{"MapUserToVox",PythonPipeLine::mapUserToVox, METH_VARARGS,
						"Map user to voxel coordinates at specified refinement level"},
		{"GetValidRegionMin",PythonPipeLine::getValidRegionMin, METH_VARARGS,
						"Get min valid coordinate bounds of variable at specified refinement level and timestep"},
		{"GetValidRegionMax",PythonPipeLine::getValidRegionMax, METH_VARARGS,
						"Get max valid coordinate bounds of variable at specified refinement level and timestep"},
		{"VariableExists",PythonPipeLine::variableExists, METH_VARARGS,
						"Determine if the specified variable exists in the VDC"},
		
		{NULL,NULL,0,NULL}
 };




// windows needs: pymodinitfunc
static PyObject *vaporerror;
void PythonPipeLine::initvapor(void)
{
	PyObject *m;
        m = Py_InitModule("vapor", vaporMethodDefinitions);
        if (m == NULL)
                return;
        import_array();
        
        vaporerror = PyErr_NewException((char*)"vapor.error",NULL,NULL);
        Py_INCREF(vaporerror);
        PyModule_AddObject(m, "error", vaporerror);
}

void PythonPipeLine::initialize(){
	// Specify some standard imports
	if(initialized) return;
	
	PyImport_AppendInittab((char*)"vapor",&(PythonPipeLine::initvapor));
		
	Py_Initialize();
	everInitialized = true;
	PyObject* vaporname = PyString_FromFormat("vapor");
	vaporModule = PyImport_Import(vaporname);
	
	PyObject* mainModule = PyImport_AddModule("__main__");
	PyObject* mainDict = PyModule_GetDict(mainModule);

	//Put standard variables into the vapor module, with zeroed values...
	PyObject* lodObj = Py_BuildValue("i", 0);
	int rc = PyModule_AddObject(vaporModule, "LOD", lodObj);
	PyObject* refObj = Py_BuildValue("i", 0);
	rc = PyModule_AddObject(vaporModule, "REFINEMENT", refObj);
	PyObject* tsObj = Py_BuildValue("i", 0);
	rc = PyModule_AddObject(vaporModule, "TIMESTEP", tsObj);
	PyObject* extObj = Py_BuildValue("(iiiiii)",0,0,0,0,0,0);
	rc = PyModule_AddObject(vaporModule, "BOUNDS", extObj);

	//See if there is a system startup file:
	//Get the path from the environment:
	vector <string> paths;
	paths.push_back("python");
	string pythonSystemPath = GetAppPath("VAPOR", "share", paths);
	string pythonSysFilename = pythonSystemPath+"/pythonSystemStartup.py";
	string sysProg;
	ifstream pythonSysFile(pythonSysFilename.c_str(),ios_base::in);
	if (pythonSysFile.is_open()){
		char readBuffer[400];
		while (1){
			pythonSysFile.getline(readBuffer, 400);
			if (pythonSysFile.gcount()<=0) break;
			sysProg += readBuffer;
			sysProg += "\n";
		}
		pythonSysFile.close();
	}
	//Insert lines to specify PYTHONPATH to include $VAPOR_HOME/share/python:
	string pathstring("import sys\n");
	pathstring += "sys.path += ['";
	pathstring += pythonSystemPath;
	pathstring += "']\n";
	sysProg = pathstring + sysProg;
	if (sysProg.size()>0){
		PyObject* retObj = PyRun_String(sysProg.c_str(),Py_file_input, mainDict,mainDict);
		if (!retObj){
			PyErr_Print();
			MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Error executing Python system startup program.\n%s\n%s\n",
				"PYTHONHOME or VAPOR_HOME may not be properly set",sysProg.substr(0,400).c_str());
		}
	}
	
	if (startupScript != ""){
		PyObject* retObj = PyRun_String(startupScript.c_str(),Py_file_input, mainDict,mainDict);
		if (!retObj){
			PyErr_Print();
			//Put stderr into MyBase error message
			//Find myErr in the dictionary
			PyObject* myErrString = PyString_FromFormat("myErr");
			PyObject* myErr = PyDict_GetItem(mainDict, myErrString);
			if (myErr){
				//Find size of StringIO:
				PyObject* sz = PyObject_CallMethod(myErr,(char*)"tell",NULL);
				int szval = PyInt_AsLong(sz);
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Python startup script execution error\n");
				if(szval > 0){
					//Get the text
					PyObject* txt = PyObject_CallMethod(myErr,(char*)"getvalue",NULL);
					const char* strtext = PyString_AsString(txt);
					MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," error output:\n%s\n",strtext);
				}
			}
			return;
		}
	}
	initialized = true;
		
}
//Wrapper for calling a python script from the PipeLine
int PythonPipeLine::python_wrapper(
	int scriptId,size_t ts,int reflevel, int compression,
  	vector<string>  inputs, 
	vector<const RegularGrid*> inputData,
  	vector<pair<string, DataMgr::VarType_T> > outputs, 
	vector<RegularGrid*> outputData)
{
   
	DataStatus* ds = DataStatus::getInstance();	
	string pythonMethod =  ds->getDerivedScript(scriptId);
	
	vector<float*> allocatedArrays;
   
    if(!initialized) initialize();
	
	//Determine sizes of arrays to allocate for python:
	RegularGrid *rg = outputData[0];
	size_t dims[3];
	rg->GetDimensions(dims);
	size_t mins[3],maxs[3];
	double umins[3],umaxs[3];
	for (int i = 0; i<3; i++){
		mins[i] = 0;
		maxs[i] = dims[i]-1;
	}
	//convert to relative integer coordinates:
	currentDataMgr->MapVoxToUser(ts,mins,umins,reflevel);
	currentDataMgr->MapVoxToUser(ts,maxs,umaxs,reflevel);
	currentDataMgr->MapUserToVox(ts,umins,mins,reflevel);
	currentDataMgr->MapUserToVox(ts,umaxs,maxs,reflevel);
	int regmin[3], regmax[3];
	Py_ssize_t pydims[3];
	PyObject* pyRegion;
	
	//reverse order so that regmin, pydims, etc. are in python coordinate order, not user coordinate order.
	for(int i = 0; i< 3; i++){
		pydims[i] = dims[2-i];
		regmin[i] = mins[2-i];
		regmax[i] = maxs[2-i];
	}
			
	//Convert the input arrays and put into dictionary:
	PyObject* mainModule = PyImport_AddModule("__main__");
	PyObject* mainDict = PyModule_GetDict(mainModule);
	
	//Make a copy (needed for later cleanup)
	PyObject* copyDict = PyDict_Copy(mainDict);

	
	string pretext = "import sys\n";
	pretext += "import StringIO\n";
	pretext += "import vapor\n";
	pretext += "import numpy\n";
	pretext += "myIO = StringIO.StringIO()\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stdout = myIO\n";
	pretext += "sys.stderr = myErr\n";
	int flags;
	
    PyObject* retObj = PyRun_String(pretext.c_str(),Py_file_input, mainDict,mainDict);
	if (!retObj){
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Python interpreter preparation error");
		return -1;
	}
	for (int i = 0; i< inputData.size(); i++){
		string vname = inputs[i];
		DataMgr::VarType_T datatype = currentDataMgr->GetVarType(vname);
		if (datatype == DataMgr::VAR3D){
			//Create a new array to pass into python:
			float* pyData = new float[pydims[0]*pydims[1]*pydims[2]];
			if(!pyData) return 0;
			allocatedArrays.push_back(pyData);
			//Now realign the array...
			copyFrom3DGrid(inputData[i], pyData);
			pyRegion = PyArray_New(&PyArray_Type,3,pydims,PyArray_FLOAT,NULL,pyData,0,
								   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
			flags = PyArray_FLAGS(pyRegion);
		} else {
			//The last two python dimension are the first two user dimensions:
			float* pyData = new float[pydims[1]*pydims[2]];
			if(!pyData) return 0;
			allocatedArrays.push_back(pyData);
			//Now copy the data from the grid.
			copyFrom2DGrid(inputData[i], pyData);
			pyRegion = PyArray_New(&PyArray_Type,2,pydims+1,PyArray_FLOAT,NULL,pyData,0,
								   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
			
		}
		
		//Put it into the dictionary:
		PyObject* ky = Py_BuildValue("s",inputs[i].c_str());
		PyObject_SetItem(mainDict,ky,pyRegion);
	}
	//Extents are in python coordinate order, so they can be used directly to allocate arrays inside the
	//Python script:
	PyObject* exts = Py_BuildValue("(iiiiii)",regmin[0],regmin[1],regmin[2],regmax[0],regmax[1],regmax[2]);
	PyObject* refinement = Py_BuildValue("i",reflevel);
	PyObject* timestep = Py_BuildValue("i",ts);
	PyObject* lod = Py_BuildValue("i",compression);
	
	int rc;
	PyObject* vaporDict = PyModule_GetDict(vaporModule);
	rc = PyDict_SetItemString(vaporDict, "TIMESTEP", timestep);
	rc = PyDict_SetItemString(vaporDict, "REFINEMENT",refinement);
	rc = PyDict_SetItemString(vaporDict, "BOUNDS",exts);
	rc = PyDict_SetItemString(vaporDict, "LOD", lod);
	
	retObj = PyRun_String(pythonMethod.c_str(),Py_file_input, mainDict,mainDict);
    if (!retObj){
		PyErr_Print();
		//Put stderr into MyBase diagnostic message
		//Find myErr in the dictionary
		PyObject* myErrString = PyString_FromFormat("myErr");
		PyObject* myErr = PyDict_GetItem(mainDict, myErrString);
		if (myErr){
			//Find size of StringIO:
			PyObject* sz = PyObject_CallMethod(myErr,(char*)"tell",NULL);
			int szval = PyInt_AsLong(sz);
			if(szval > 0){
				//Get the text
				PyObject* txt = PyObject_CallMethod(myErr,(char*)"getvalue",NULL);
				const char* strtext = PyString_AsString(txt);
				//post it as Diagnostic.
				//There will be an error message too, but
				//the error callback fcn is disabled here.
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Python execution error:\n%s\n",strtext);
			}
		}
		return -1;
    }
	//Find myIO in the dictionary, send output to diagnostics
	PyObject* myIOString = PyString_FromFormat("myIO");
	PyObject* myIO = PyDict_GetItem(mainDict, myIOString);
	pythonOutputText.clear();
	if (myIO){
		//Find size of StringIO:
		PyObject* sz = PyObject_CallMethod(myIO,(char*)"tell",(char*)NULL);
		int szval = PyInt_AsLong(sz);
		if(szval > 0){
			//Get the text
			PyObject* txt = PyObject_CallMethod(myIO,(char*)"getvalue",NULL);
			const char* strtext = PyString_AsString(txt);
			
			//post it to diagnostics
			MyBase::SetDiagMsg(" Python output text:\n%s\n",strtext);
		}
	}
	
   
	
	//Retrieve all the output variables using the dictionary.  First do 3d, then 2d
	for (int i = 0; i< outputs.size(); i++) {
		const char* vname = outputs[i].first.c_str();
		int dimen = 2;
		if (outputs[i].second == DataMgr::VAR3D) dimen = 3;
		PyObject* ky = Py_BuildValue("s",vname);
		
		PyObject* varArray = PyDict_GetItem(mainDict, ky);
		if (! varArray){
			MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Variable %s not produced by script",vname);
			return -1;
		}
		//check shape and type of data:
		int nd = PyArray_NDIM(varArray);
		if (nd != dimen){
			MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Variable %s is not %d-dimensional", vname, dimen);
			return -1;
		}
		int datatype = PyArray_TYPE(varArray);
		if (datatype != 11){
			MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Variable %s data is not float32",vname);
			return -1;
		}
		npy_intp* dims = PyArray_DIMS(varArray);
		for (int j = 0; j< nd; j++) {
			if (dims[j] != (regmax[j-nd+3] - regmin[j-nd+3]+1)){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Shape of %s array does not conform with __BOUNDS__",vname);
				return -1;
			}
		}
		flags = PyArray_FLAGS(varArray);
		float *dataArray = (float*)PyArray_DATA(varArray);
			
		if (dimen == 3) {
			//Realign, put into DataMgr's allocated region
			copyTo3DGrid(dataArray,  outputData[i]);
			
		} else {
			copyTo2DGrid(dataArray, outputData[i]);
		}
		
	}
	//Cleanup:  Remove anything that was added to the dictionary:
	Py_ssize_t pos = 0;
	std::vector<PyObject*> newObjects;
	std::vector<PyObject*> newKeys;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&pos, &key, &val);
		if (!rc) break;
		if (PyDict_Contains(copyDict,key)) {
			continue;
		}
		newKeys.push_back(key);
		newObjects.push_back(val);
	}
	for (int i = 0; i<newObjects.size(); i++){
		PyObject_DelItem(mainDict,newKeys[i]);
		if(needCheckArrays()){
			tryDeleteArrayStorage(newObjects[i]);
		}
	}
	//We need to release the copies of the input arrays:
	for (int i = 0; i< allocatedArrays.size(); i++){
		delete allocatedArrays[i];
	}
		
	return 0;
}
//Wrapper for testing a python script, called from python editor
//Returns text output resulting from execution
//Supplies its own variables, not those saved to datamgr
//
std::string& PythonPipeLine::
python_test_wrapper(const string& script, const vector<string>& inputVars2, 
					const vector<string>& inputVars3, 
					vector<pair<string, DataMgr::VarType_T> > outputs,
					size_t ts,int reflevel,int compression, size_t rmin[3], size_t rmax[3]){
	
	bool executionError=false;
	if(!initialized) {
		initialize();
	}
	
	string pretext = "import sys\n";
	pretext += "import StringIO\n";
	pretext += "import vapor\n";
	pretext += "import numpy\n";
	pretext += "myIO = StringIO.StringIO()\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stdout = myIO\n";
	pretext += "sys.stderr = myErr\n";
	
	for (int i = 0; i< inputVars3.size(); i++){
		pretext += inputVars3[i] + " = vapor.Get3DVariable(vapor.TIMESTEP,'" + inputVars3[i] + "',vapor.REFINEMENT,vapor.LOD,vapor.BOUNDS)\n";
	}
	for (int i = 0; i< inputVars2.size(); i++){
		pretext += inputVars2[i] + " = vapor.Get2DVariable(vapor.TIMESTEP,'" + inputVars2[i] + "',vapor.REFINEMENT,vapor.LOD,vapor.BOUNDS)\n";
	}
	
	
	string pythonMethod = script ;
	

	//get the module dictionary...
	PyObject* mainModule = PyImport_AddModule("__main__");
	PyObject* mainDict = PyModule_GetDict(mainModule);
	
	//Make a copy (needed for later cleanup)
	PyObject* copyDict = PyDict_Copy(mainDict);
	//Print out the contents of the dictionary:
	/*printf("Dictionary prior to test execution\n");
	Py_ssize_t poss = 0;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&poss, &key, &val);
		if (!rc) break;
		printf("Key: %s\n", PyString_AsString(key));
	}*/

	int regmin[3],regmax[3];
	for (int i = 0; i<3; i++){
			regmin[i] = rmin[i];
			regmax[i] = rmax[i];
	}
	if (currentDataMgr){
		
		PyObject* exts = Py_BuildValue("(iiiiii)",regmin[2],regmin[1],regmin[0],regmax[2],regmax[1],regmax[0]);
		PyObject* refinement = Py_BuildValue("i",reflevel);
		PyObject* timestep = Py_BuildValue("i",ts);
		PyObject* lod = Py_BuildValue("i",compression);
		PyObject* vaporDict = PyModule_GetDict(vaporModule);
		int rc = PyDict_SetItemString(vaporDict, "TIMESTEP", timestep);
		rc = PyDict_SetItemString(vaporDict, "REFINEMENT",refinement);
		rc = PyDict_SetItemString(vaporDict, "LOD",lod);
		rc = PyDict_SetItemString(vaporDict, "BOUNDS",exts);
		
	}
	PyObject* retObj = PyRun_String(pretext.c_str(), Py_file_input, mainDict,mainDict);
	if (!retObj){
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Python interpreter preparation error");
		pythonOutputText = "preparation script error";
		return pythonOutputText;
	}
		
	// execute the program...
    retObj = PyRun_String(pythonMethod.c_str(),Py_file_input, mainDict,mainDict);
	
	
    if (!retObj){
		executionError=true;
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Python interpreter failure");
		//Put stderr into MyBase error message
		//Find myErr in the dictionary
		PyObject* myErrString = PyString_FromFormat("myErr");
		PyObject* myErr = PyDict_GetItem(mainDict, myErrString);
		if (myErr){
			//Find size of StringIO:
			PyObject* sz = PyObject_CallMethod(myErr,(char*)"tell",NULL);
			int szval = PyInt_AsLong(sz);
			if(szval > 0){
				//Get the text
				PyObject* txt = PyObject_CallMethod(myErr,(char*)"getvalue",NULL);
				const char* strtext = PyString_AsString(txt);
				//post it as Error 
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Python execution error:\n%s\n",strtext);
			}
		}
	} else {
		//Check all the output variables using the dictionary.  First do 3d, then 2d
		for (int i = 0; i< outputs.size(); i++) {
			const char* vname = outputs[i].first.c_str();
			int dimen = 2;
			if (outputs[i].second == DataMgr::VAR3D) dimen = 3;
			PyObject* ky = Py_BuildValue("s",vname);
		
			PyObject* varArray = PyDict_GetItem(mainDict, ky);
			if (! varArray){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Variable %s not produced by script",vname);
				pythonOutputText = "Output data error";
				return pythonOutputText;
			}
			//check shape and type of data:
			int nd = PyArray_NDIM(varArray);
			if (nd != dimen){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Variable %s is not %d-dimensional", vname, dimen);
				pythonOutputText = "Output data error";
				return pythonOutputText;
			}
			int datatype = PyArray_TYPE(varArray);
			if (datatype != 11){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Variable %s data is not float32",vname);
				pythonOutputText = "Output data error";
				return pythonOutputText;
			}
			npy_intp* dims = PyArray_DIMS(varArray);
			for (int i = 0; i< nd; i++) {
				if (dims[i] != (regmax[nd-1-i] - regmin[nd-1-i]+1)){
					MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Shape of %s array does not conform with BOUNDS",vname);
					pythonOutputText = "Output data error";
					return pythonOutputText;
				}
			}
		}
	}
	
    
	
	//Find myIO in the dictionary
	PyObject* myIOString = PyString_FromFormat("myIO");
	PyObject* myIO = PyDict_GetItem(mainDict, myIOString);
	pythonOutputText.clear();
	if (myIO){
		//Find size of StringIO:
		PyObject* sz = PyObject_CallMethod(myIO,(char*)"tell",(char*)NULL);
		int szval = PyInt_AsLong(sz);
		if(szval > 0){
			//Get the text
			PyObject* txt = PyObject_CallMethod(myIO,(char*)"getvalue",NULL);
			const char* strtext = PyString_AsString(txt);
			//post it to diagnostics
			
			MyBase::SetDiagMsg(" Python output text:\n%s\n",strtext);
			pythonOutputText = strtext;
		}
	}
	Py_ssize_t pos = 0;
	//Print out the contents of the dictionary:
	/*
	printf("Dictionary following test execution\n");
	Py_ssize_t poss=0;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&poss, &key, &val);
		if (!rc) break;
		printf("Key: %s\n", PyString_AsString(key));
	}
	*/
	//Cleanup:  Remove anything that was added to the dictionary:
	
	std::vector<PyObject*> newObjects;
	std::vector<PyObject*> newKeys;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&pos, &key, &val);
		if (!rc) break;
		if (PyDict_Contains(copyDict,key)) {
			//printf("Matching key found: %s\n", PyString_AsString(key));
			continue;
		}
		newKeys.push_back(key);
		newObjects.push_back(val);
	}
	for (int i = 0; i<newObjects.size(); i++){
		//char* keyString = PyString_AsString(newObjects[i]);
		//printf("deleting object with key %s\n",keyString);
		PyObject_DelItem(mainDict,newKeys[i]);
		if(needCheckArrays()){
			tryDeleteArrayStorage(newObjects[i]);
		}	
	}
	if (pythonOutputText.length() == 0 && executionError)
		pythonOutputText = "TEST_SCRIPT_ERROR";
	return pythonOutputText;
}



//get/set called by Python interpreter:
//Note this is static
PyObject* PythonPipeLine::get_3Dvariable(PyObject *self, PyObject* args){
	const char *varname;
    static float *regData = 0;
    PyObject *pyRegion;
    Py_ssize_t pydims[3];
    int tstep, reflevel, lod;
    int minreg[3],maxreg[3];
	size_t regmin[3],regmax[3];

	//Arguments are in python order.  Reverse them for use with VAPOR
    if (!PyArg_ParseTuple(args,"isii(iiiiii)",&tstep,&varname,&reflevel,&lod,
		minreg+2,minreg+1,minreg,maxreg+2,maxreg+1,maxreg)) return NULL; 
    string vname(varname);
	for (int i = 0; i<3; i++){
		//pydims are in python order, minreg, maxreg are in user order
		pydims[i] = maxreg[2-i]-minreg[2-i]+1;
		regmin[i] = minreg[i];
		regmax[i] = maxreg[i];
    }
    RegularGrid* rg = currentDataMgr->GetGrid((size_t)tstep, vname, reflevel,lod, regmin,regmax, true);

	if (!rg){
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Error obtaining variable %s from VDC",varname);
		return NULL;
	}
	//Create a new array to pass to python:
    float* pyData = new float[pydims[0]*pydims[1]*pydims[2]];
    if(!pyData) return NULL;

	//Now copy in the data:
	copyFrom3DGrid(rg, pyData);
    currentDataMgr->UnlockGrid(rg);
	pyRegion = PyArray_New(&PyArray_Type,3,pydims,PyArray_FLOAT,NULL,pyData,0,
						   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
	mapArrayObject(pyRegion, pyData);
    return Py_BuildValue("O", pyRegion);
}
//get/set 2D called by Python interpreter:
//Note this is static
//the z coord of extents is ignored
PyObject* PythonPipeLine::get_2Dvariable(PyObject *self, PyObject* args){
	const char *varname;
    static float *regData = 0;
    PyObject *pyRegion;
    Py_ssize_t pydims[2];
    int tstep, reflevel,lod;
    int minreg[3],maxreg[3];
    size_t regmin[3],regmax[3];
    if (!PyArg_ParseTuple(args,"isii(iiiiii)",&tstep,&varname,&reflevel,&lod,
		minreg+2,minreg+1,minreg,maxreg+2,maxreg+1,maxreg)) return NULL; 
	string vname(varname);
    
	for (int i = 0; i<2; i++){
		pydims[i] = maxreg[1-i]-minreg[1-i]+1;
		regmin[i]=minreg[i];
		regmax[i]=maxreg[i];
    }
    
    RegularGrid* rg = currentDataMgr->GetGrid((size_t)tstep, vname, reflevel,lod, regmin,regmax, true);
	if (!rg){
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Error obtaining variable %s from VDC",varname);
		return NULL;
	}

    //Create a new array to pass to python:
    float* pyData = new float[pydims[0]*pydims[1]];
    if(!pyData) return NULL;

	//Now copy in the data:
	copyFrom2DGrid(rg, pyData);
    currentDataMgr->UnlockGrid(rg);

    pyRegion = PyArray_New(&PyArray_Type,2,pydims,PyArray_FLOAT,NULL,pyData,0,
						   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
    mapArrayObject(pyRegion, pyData);
    return Py_BuildValue("O", pyRegion);
}
//convert voxel to user coordinates at specified refinement level.
//Note this is static
//The python coordinates are reversed, then converted by DataMgr, then converted values are reversed for Python
PyObject* PythonPipeLine::mapVoxToUser(PyObject *self, PyObject* args){
	
    int reflevel;
    int ivox[3];
	size_t voxCrds[3];
	double userCrds[3];
    
    if (!PyArg_ParseTuple(args,"(iii)i",
						  ivox,ivox+1,ivox+2,&reflevel)) return NULL; 
    
	for (int i = 0; i< 3; i++) voxCrds[i] = (size_t)ivox[2-i];

	currentDataMgr->MapVoxToUser(0,voxCrds,userCrds, reflevel);
	
    return Py_BuildValue("[ddd]", userCrds[2],userCrds[1],userCrds[0]);
}
//convert user to voxel coordinates at specified refinement level.
//Note this is static
//The python coordinates are reversed, then converted by DataMgr, then converted values are reversed for Python
PyObject* PythonPipeLine::mapUserToVox(PyObject *self, PyObject* args){
	
    int reflevel;
	double userCrds[3];
	size_t vox[3];
	int ivox[3];
    
    if (!PyArg_ParseTuple(args,"(ddd)i",
						  userCrds+2,userCrds+1,userCrds,&reflevel)) return NULL; 
	
	currentDataMgr->MapUserToVox(0, userCrds, vox, reflevel);
	for(int i = 0; i< 3; i++) ivox[i] = (int)vox[2-i];
	
    return Py_BuildValue("[iii]", ivox[0],ivox[1],ivox[2]);
}
//Returns min valid coordinate bound (in voxels) for specified variable name, refinement, timestep
//Arguments are: timestep, variableName, refinementLevel. 
PyObject* PythonPipeLine::getValidRegionMin(PyObject *self, PyObject* args){
	
    int reflevel;
	size_t minvox[3];
	size_t maxvox[3];
	int ivox[3];
	char* varname;
	int timestep;
	size_t ts;
    
    if (!PyArg_ParseTuple(args,"isi",
						  &timestep, &varname,&reflevel)) return NULL; 
	ts = timestep;
	currentDataMgr->GetValidRegion(ts, varname, reflevel, minvox, maxvox);
	for(int i = 0; i< 3; i++) ivox[i] = (int)minvox[i];
	
    return Py_BuildValue("[iii]", ivox[0],ivox[1],ivox[2]);
}
//Returns max valid coordinate bound (in voxels) for specified variable name, refinement, timestep
//Arguments are: timestep, variableName, refinementLevel. 
PyObject* PythonPipeLine::getValidRegionMax(PyObject *self, PyObject* args){
	
    int reflevel;
	size_t minvox[3];
	size_t maxvox[3];
	size_t ts;
	int ivox[3];
	char* varname;
	int timestep;
	
    
    if (!PyArg_ParseTuple(args,"isi",
						  &timestep,&varname,&reflevel)) return NULL; 
	ts = timestep;
	currentDataMgr->GetValidRegion(ts, varname, reflevel, minvox, maxvox);
	for(int i = 0; i< 3; i++) ivox[i] = (int)maxvox[i];
	
    return Py_BuildValue("[iii]", ivox[0],ivox[1],ivox[2]);
}
//Returns boolean indicating that specified variable name exists at a specified timestep
//Arguments are: timestep, variableName, and optionally: refinement level, level of detail, which default to 0
//
PyObject* PythonPipeLine::variableExists(PyObject *self, PyObject* args){
	
    int reflevel = 0;
	int lod = 0;
	size_t ts;
	char* varname;
	int timestep;
	
    
    if (!PyArg_ParseTuple(args,"is|ii",
						  &timestep,&varname,&reflevel,&lod)) return NULL; 
	ts = timestep;
	int retval = currentDataMgr->VariableExists(ts, varname, reflevel, lod);
	
    return Py_BuildValue("i", retval);
}
//Static methods for converting between float arrays and RegularGrid data.
//Coordinate order is always reversed.  They will need to deal with missing values soon.
void PythonPipeLine::copyTo3DGrid(const float* srcArray, RegularGrid* destGrid){
	size_t dims[3];
	destGrid->GetDimensions(dims);
	for (int k =0; k<dims[2]; k++){
		for (int j = 0; j< dims[1]; j++){
			for (int i = 0; i< dims[0]; i++){
				//get index in srcArray (reversed, i.e. python order)
				int srcIndex = k+j*dims[2]+ i*dims[2]*dims[1];
				//obtain from Vapor grid coordinate order
				float& val = destGrid->AccessIJK(i,j,k);
				val = srcArray[srcIndex];
			}
		}
	}
}
void PythonPipeLine::copyFrom3DGrid(const RegularGrid* srcGrid, float* destArray){
	size_t dims[3];
	srcGrid->GetDimensions(dims);
	for (int k = 0; k < dims[2]; k++){
		for (int j = 0; j < dims[1]; j++){
			for (int i = 0; i < dims[0]; i++){
				//destIndex is in python order
				int destIndex = k+j*dims[2]+ i*dims[2]*dims[1];
				float& val = srcGrid->AccessIJK(i,j,k);
				destArray[destIndex] = val;
			}
		}
	}
}
void PythonPipeLine::copyTo2DGrid(const float* srcArray, RegularGrid* destGrid){
	size_t dims[3];
	destGrid->GetDimensions(dims);
	//For 2D data, only use first two VAPOR dimensions
	for (int j = 0; j<  dims[1]; j++){
		for (int i = 0; i< dims[0]; i++){
			int srcIndex = j+ i*dims[1];
			float& val = destGrid->AccessIJK(i,j,0);
			val = srcArray[srcIndex];
		}
	}

}
void PythonPipeLine::copyFrom2DGrid(const RegularGrid* srcGrid, float* destArray){
	size_t dims[3];
	srcGrid->GetDimensions(dims);
	for (int j = 0; j< dims[1]; j++){
		for (int i = 0; i< dims[0]; i++){
			int destIndex = j+ i*dims[1];
			float& val = srcGrid->AccessIJK(i,j,0);
			destArray[destIndex] = val;
		}
	}

}
// static method to copy an array into another one with different dimensioning.
// Useful to convert a blocked region to a smaller region that intersects full domain bounds.
// Also useful to copy smaller region back to full domain bounds.  Source and
// destination region share the same (0,0,0) origin, but dest may be larger or smaller.
// Source and destination have opposite ordering of array bounds
void PythonPipeLine::realign3DArray(const float* srcArray, size_t srcSize[3], float* destArray, size_t destSize[3]){
        int xmax = (srcSize[0] < destSize[0]) ? srcSize[0] : destSize[0];
        int ymax = (srcSize[1] < destSize[1]) ? srcSize[1] : destSize[1];
        int zmax = (srcSize[2] < destSize[2]) ? srcSize[2] : destSize[2];
        for (int k = 0; k < zmax; k++){
                for (int j = 0; j < ymax; j++){
                        for (int i= 0; i< xmax; i++){
                                destArray[i+destSize[0]*(j+destSize[1]*k)]=
                                        srcArray[i+srcSize[0]*(j+srcSize[1]*k)];
                        }
                }
        }
}

void PythonPipeLine::realign2DArray(const float* srcArray, size_t srcSize[2], float* destArray, size_t destSize[2]){
        int xmax = (srcSize[0] < destSize[0]) ? srcSize[0] : destSize[0];
        int ymax = (srcSize[1] < destSize[1]) ? srcSize[1] : destSize[1];
        for (int j = 0; j < ymax; j++){
                for (int i= 0; i< xmax; i++){
                        destArray[i+destSize[0]*j]= srcArray[i+srcSize[0]*j];
                }
        }

}
void PythonPipeLine::tryDeleteArrayStorage(PyObject* pyobj){
	if (!arrayAllocMutex->tryLock(10000)){//  <= 10 second wait..
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Thread conflict in array deallocation");
		return;
	}
	map< PyObject*,float*> :: iterator mapiter = arrayAllocMap.find(pyobj);
	if (mapiter != arrayAllocMap.end()){
		float* ary = mapiter->second;
		delete ary;
		arrayAllocMap.erase(mapiter);
	}
	arrayAllocMutex->unlock();
}
void PythonPipeLine::mapArrayObject(PyObject* pyobj, float* ary){
	if (!arrayAllocMutex->tryLock(10000)){//  <= 10 second wait..
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Thread conflict in array allocation");
		return;
	}
	arrayAllocMap[pyobj] = ary;
	arrayAllocMutex->unlock();
}
