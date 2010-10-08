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
#include "GetAppPath.h"

using namespace VetsUtil;
using namespace VAPoR;

std::string PythonPipeLine::startupScript = "";

DataMgr* PythonPipeLine::currentDataMgr = 0;
bool PythonPipeLine::initialized = false;

PythonPipeLine::PythonPipeLine(string name, vector<string> inputs, 
		vector < pair < string, Metadata::VarType_T > > outputs, DataMgr* dmgr) :
PipeLine(name, inputs, outputs)
{
	currentDataMgr = dmgr;
}
int PythonPipeLine::Calculate (
							   vector <const float *> input_blks,
							   vector <float *> output_blks,	// space for the output variables
							   size_t ts, // current time step
							   int reflevel, // refinement level
							   int lod, // compression level
							   const size_t bs[3], // block dimensions
							   const size_t min[3],	// dimensions of all variables (in blocks)
							   const size_t max[3]
							   ) {
	//call python wrapper.
	int scriptId = DataStatus::getInstance()->getDerivedScriptId(GetName());
	return python_wrapper(scriptId,ts,reflevel,lod,min,max, 
						  GetInputs(), input_blks,
						  GetOutputs(), output_blks);
}

PyMethodDef PythonPipeLine::vaporMethodDefinitions[] = { 
		{"Get3DVariable",PythonPipeLine::get_3Dvariable, METH_VARARGS,
                        "Get a variable from the DataMgr cache"},
		{"Get2DVariable",PythonPipeLine::get_2Dvariable, METH_VARARGS,
                        "Get a variable from the DataMgr cache"},
		{"GetExtents",PythonPipeLine::get_extents, METH_VARARGS,
						"Return extents at specified timestep, refinement and bounds"},
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
	PyObject* numpyname = PyString_FromFormat("numpy");
	PyObject* mod = PyImport_Import(numpyname);
	PyObject* vaporname = PyString_FromFormat("vapor");
	mod = PyImport_Import(vaporname);
	
	PyObject* mainModule = PyImport_AddModule("__main__");
	PyObject* mainDict = PyModule_GetDict(mainModule);

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
	if (sysProg.size()>0){
		PyObject* retObj = PyRun_String(sysProg.c_str(),Py_file_input, mainDict,mainDict);
		if (!retObj){
			PyErr_Print();
			MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Error executing Python system startup program:\n%s\n",sysProg.c_str());
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
				PyObject* sz = PyObject_CallMethod(myErr,"tell",NULL);
				int szval = PyInt_AsLong(sz);
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Python startup script execution error\n");
				if(szval > 0){
					//Get the text
					PyObject* txt = PyObject_CallMethod(myErr,"getvalue",NULL);
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
	const size_t min[3],const size_t max[3],
  	vector<string>  inputs, 
	vector<const float*> inputData,
  	vector<pair<string, Metadata::VarType_T> > outputs, 
	vector<float*> outputData)
{
   	
		
	DataStatus* ds = DataStatus::getInstance();	
	string pythonMethod =  ds->getDerivedScript(scriptId);
   
		
    if(!initialized) initialize();
	
	//must convert input block extents to actual region extents
	size_t dim[3];
	size_t blksize[3];
	size_t regsize[3];
	size_t minblkreg[3],maxblkreg[3], blkregsize[3];
	int regmin[3],regmax[3];
	Py_ssize_t pydims[3];
	PyObject* pyRegion;
	
	currentDataMgr->GetBlockSize(blksize,reflevel);
	currentDataMgr->GetDim(dim, reflevel);	
	
	for (int i = 0; i<3; i++){
		regmin[i] = min[i]*blksize[i];
		regmax[i] = (max[i]+1)*blksize[i]-1;
		if (regmax[i] >= dim[i]) regmax[i] = dim[i]-1; 
		regsize[i] = regmax[i]-regmin[i]+1;
		minblkreg[i] = regmin[i]/blksize[i];
		maxblkreg[i] = regmax[i]/blksize[i];
		blkregsize[i] = (maxblkreg[i] - minblkreg[i] + 1)*blksize[i];
		assert(minblkreg[i] == min[i]);
		assert(maxblkreg[i] == max[i]);
		pydims[i] = blkregsize[i];
        int maxPySize = (dim[i]-regmin[i]);
        if (pydims[i] > maxPySize) pydims[i] = maxPySize;
		
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
	
	
    PyObject* retObj = PyRun_String(pretext.c_str(),Py_file_input, mainDict,mainDict);
	if (!retObj){
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Python interpreter preparation error");
		return -1;
	}
	for (int i = 0; i< inputData.size(); i++){
		string vname = inputs[i];
		Metadata::VarType_T datatype = currentDataMgr->GetVarType(vname);
		if (datatype == Metadata::VAR3D){
			// convert the float array a python array:
			//Create a new array to pass into python:
			float* pyData = new float[pydims[0]*pydims[1]*pydims[2]];
			if(!pyData) return 0;
			//Now realign the array...
			realign3DArray(inputData[i],blkregsize, pyData, (size_t*)pydims);
			pyRegion = PyArray_SimpleNewFromData(3, pydims, PyArray_FLOAT, pyData);
		} else {
			float* pyData = new float[pydims[0]*pydims[1]];
			if(!pyData) return 0;

			//Now realign the array...
			realign2DArray(inputData[i],blkregsize, pyData, (size_t*)pydims);
			pyRegion = PyArray_SimpleNewFromData(2, pydims, PyArray_FLOAT, pyData);
			
		}
		;
		//Put it into the dictionary:
		PyObject* ky = Py_BuildValue("s",inputs[i].c_str());
		PyObject_SetItem(mainDict,ky,pyRegion);
	}
	 
	PyObject* exts = Py_BuildValue("(iiiiii)",regmin[0],regmin[1],regmin[2],regmax[0],regmax[1],regmax[2]);
    PyObject* refinement = Py_BuildValue("i",reflevel);
	PyObject* timestep = Py_BuildValue("i",ts);
	PyObject* lod = Py_BuildValue("i",compression);
	
	int rc;
	
	rc = PyDict_SetItemString(mainDict, "__TIMESTEP__", timestep);
	rc = PyDict_SetItemString(mainDict, "__REFINEMENT__",refinement);
	rc = PyDict_SetItemString(mainDict, "__BOUNDS__",exts);
	rc = PyDict_SetItemString(mainDict, "__LOD__", lod);

	
	retObj = PyRun_String(pythonMethod.c_str(),Py_file_input, mainDict,mainDict);
    if (!retObj){
		PyErr_Print();
		//Put stderr into MyBase error message
		//Find myErr in the dictionary
		PyObject* myErrString = PyString_FromFormat("myErr");
		PyObject* myErr = PyDict_GetItem(mainDict, myErrString);
		if (myErr){
			//Find size of StringIO:
			PyObject* sz = PyObject_CallMethod(myErr,"tell",NULL);
			int szval = PyInt_AsLong(sz);
			if(szval > 0){
				//Get the text
				PyObject* txt = PyObject_CallMethod(myErr,"getvalue",NULL);
				const char* strtext = PyString_AsString(txt);
				//post it as Error 
			
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING," Python execution error:\n%s\n",strtext);
			}
		}
		return -1;
    }
   
	
	//Retrieve all the output variables using the dictionary.  First do 3d, then 2d
	for (int i = 0; i< outputs.size(); i++) {
		const char* vname = outputs[i].first.c_str();
		int dimen = 2;
		if (outputs[i].second == Metadata::VAR3D) dimen = 3;
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
			if (dims[j] != (regmax[j] - regmin[j]+1)){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Shape of %s array does not conform with __BOUNDS__",vname);
				return -1;
			}
		}
		float *dataArray = (float*)PyArray_DATA(varArray);
			
		if (dimen == 3) {
			//Realign, put into DataMgr's allocated region
			realign3DArray(dataArray, regsize, outputData[i], blkregsize);
			
		} else {
			realign2DArray(dataArray, regsize, outputData[i], blkregsize);
		}
		
	}
	//Cleanup:  Remove anything that was added to the dictionary:
	Py_ssize_t pos = 0;
	std::vector<PyObject*> newObjects;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&pos, &key, &val);
		if (!rc) break;
		if (PyDict_Contains(copyDict,key)) {
			continue;
		}
		newObjects.push_back(key);
	}
	for (int i = 0; i<newObjects.size(); i++){
		PyObject_DelItem(mainDict,newObjects[i]);
		if (PyDict_Contains(mainDict,newObjects[i])) assert(0);
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
					vector<pair<string, Metadata::VarType_T> > outputs,
					size_t ts,int reflevel,int compression, const size_t min[3],const size_t max[3]){
		
	if(!initialized) initialize();
	
	
	string pretext = "import sys\n";
	pretext += "import StringIO\n";
	pretext += "import vapor\n";
	pretext += "import numpy\n";
	pretext += "myIO = StringIO.StringIO()\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stdout = myIO\n";
	pretext += "sys.stderr = myErr\n";
	
	for (int i = 0; i< inputVars3.size(); i++){
		pretext += inputVars3[i] + " = vapor.Get3DVariable('" + inputVars3[i] + "',__TIMESTEP__,__REFINEMENT__,__LOD__,__BOUNDS__)\n";
	}
	for (int i = 0; i< inputVars2.size(); i++){
		pretext += inputVars2[i] + " = vapor.Get2DVariable('" + inputVars2[i] + "',__TIMESTEP__,__REFINEMENT__,__LOD__,__BOUNDS__)\n";
	}
	
	
	string pythonMethod = script ;
	
	
	//must convert input block extents to actual region extents
	size_t dim[3];
	size_t blksize[3];

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
	
	if (currentDataMgr){
		currentDataMgr->GetBlockSize(blksize,reflevel);
		
		currentDataMgr->GetDim(dim, reflevel);	
		
		
		for (int i = 0; i<3; i++){
			regmin[i] = min[i]*blksize[i];
			regmax[i] = (max[i]+1)*blksize[i]-1;
			if (regmax[i] >= dim[i]) regmax[i] = dim[i]-1; 
		}
		PyObject* exts = Py_BuildValue("(iiiiii)",regmin[0],regmin[1],regmin[2],regmax[0],regmax[1],regmax[2]);
		PyObject* refinement = Py_BuildValue("i",reflevel);
		PyObject* timestep = Py_BuildValue("i",ts);
		PyObject* lod = Py_BuildValue("i",compression);
		
		int rc = PyDict_SetItemString(mainDict, "__TIMESTEP__", timestep);
		rc = PyDict_SetItemString(mainDict, "__REFINEMENT__",refinement);
		rc = PyDict_SetItemString(mainDict, "__LOD__",lod);
		rc = PyDict_SetItemString(mainDict, "__BOUNDS__",exts);
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
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Python interpreter failure");
		//Put stderr into MyBase error message
		//Find myErr in the dictionary
		PyObject* myErrString = PyString_FromFormat("myErr");
		PyObject* myErr = PyDict_GetItem(mainDict, myErrString);
		if (myErr){
			//Find size of StringIO:
			PyObject* sz = PyObject_CallMethod(myErr,"tell",NULL);
			int szval = PyInt_AsLong(sz);
			if(szval > 0){
				//Get the text
				PyObject* txt = PyObject_CallMethod(myErr,"getvalue",NULL);
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
			if (outputs[i].second == Metadata::VAR3D) dimen = 3;
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
				if (dims[i] != (regmax[i] - regmin[i]+1)){
					MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Shape of %s array does not conform with __BOUNDS__",vname);
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
		PyObject* sz = PyObject_CallMethod(myIO,"tell",(char*)NULL);
		int szval = PyInt_AsLong(sz);
		if(szval > 0){
			//Get the text
			PyObject* txt = PyObject_CallMethod(myIO,"getvalue",NULL);
			const char* strtext = PyString_AsString(txt);
			//post it to diagnostics
			
			MyBase::SetDiagMsg(" Python output text:\n%s\n",strtext);
			pythonOutputText = strtext;
		}
	}
	//Print out the contents of the dictionary:
	/*
	printf("Dictionary following test execution\n");
	poss=0;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&poss, &key, &val);
		if (!rc) break;
		printf("Key: %s\n", PyString_AsString(key));
	}*/
	
	//Cleanup:  Remove anything that was added to the dictionary:
	Py_ssize_t pos = 0;
	std::vector<PyObject*> newObjects;
	while(1){
		PyObject* key;
		PyObject* val;
		int rc = PyDict_Next(mainDict,&pos, &key, &val);
		if (!rc) break;
		if (PyDict_Contains(copyDict,key)) {
			//printf("Matching key found: %s\n", PyString_AsString(key));
			continue;
		}
		newObjects.push_back(key);
	}
	for (int i = 0; i<newObjects.size(); i++){
		//char* keyString = PyString_AsString(newObjects[i]);
		//printf("deleting object with key %s\n",keyString);
		PyObject_DelItem(mainDict,newObjects[i]);
		if (PyDict_Contains(mainDict,newObjects[i])) assert(0);
	}
	
	return pythonOutputText;
}



//get/set called by Python interpreter:
//Note this is static
PyObject* PythonPipeLine::get_3Dvariable(PyObject *self, PyObject* args){
	const char *varname;
    static float *regData = 0;
    PyObject *pyRegion;
    size_t dims[3];
    Py_ssize_t pydims[3];
    size_t blockedRegionSize[3];

    int tstep, reflevel, lod;
    int minreg[3],maxreg[3];
    size_t minblkreg[3],maxblkreg[3];
    if (!PyArg_ParseTuple(args,"siii(iiiiii)",&varname,&tstep,&reflevel,&lod,
		minreg,minreg+1,minreg+2,maxreg,maxreg+1,maxreg+2)) return NULL; 
    
    //Need to convert min,max extents to block extents
	size_t blksize[3];
	
	currentDataMgr->GetBlockSize(blksize,reflevel);
    
    currentDataMgr->GetDim(dims,reflevel);
    for (int i = 0; i<3; i++){
		minblkreg[i] = minreg[i]/blksize[i];
		maxblkreg[i] = maxreg[i]/blksize[i];
		blockedRegionSize[i] = (maxblkreg[i]-minblkreg[i]+1)*blksize[i];
		pydims[i] = blockedRegionSize[i];
		int maxPySize = (dims[i]-minreg[i]); 
		if (pydims[i] > maxPySize) pydims[i] = maxPySize;
    }
    
    regData = currentDataMgr->GetRegion(tstep, varname, reflevel, lod, minblkreg, maxblkreg, 0);
	if (!regData){
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Error obtaining variable %s from VDC",varname);
		return NULL;
	}
	//Create a new array to pass to python:
    float* pyData = new float[pydims[0]*pydims[1]*pydims[2]];
    if(!pyData) return NULL;

	//Now realign the array...
    realign3DArray(regData,blockedRegionSize, pyData, (size_t*)pydims); 
    pyRegion = PyArray_SimpleNewFromData(3, pydims, PyArray_FLOAT, pyData);
    return Py_BuildValue("O", pyRegion);
}
//get/set 2D called by Python interpreter:
//Note this is static
//the z coord of extents is ignored
PyObject* PythonPipeLine::get_2Dvariable(PyObject *self, PyObject* args){
	const char *varname;
    static float *regData = 0;
    PyObject *pyRegion;
    size_t dims[3];
    Py_ssize_t pydims[2];
    size_t blockedRegionSize[2];

    int tstep, reflevel,lod;
    int minreg[3],maxreg[3];
    size_t minblkreg[3],maxblkreg[3];
    if (!PyArg_ParseTuple(args,"siii(iiiiii)",&varname,&tstep,&reflevel,&lod,
		minreg,minreg+1,minreg+2,maxreg,maxreg+1,maxreg+2)) return NULL; 
	//Need to convert min,max extents to block extents
    size_t blksize[3];
	currentDataMgr->GetBlockSize(blksize,reflevel);
    currentDataMgr->GetDim(dims,reflevel);
    for (int i = 0; i<2; i++){
		minblkreg[i] = minreg[i]/blksize[i];
		maxblkreg[i] = maxreg[i]/blksize[i];
		blockedRegionSize[i] = (maxblkreg[i]-minblkreg[i]+1)*blksize[i];
		pydims[i] = blockedRegionSize[i];
		int maxPySize = (dims[i]-minreg[i]); 
		if (pydims[i] > maxPySize) pydims[i] = maxPySize;
    }
    
    regData = currentDataMgr->GetRegion(tstep, varname, reflevel, lod, minblkreg, maxblkreg, 0);
	if (!regData){
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Error obtaining variable %s from VDC",varname);
		return NULL;
	}
    
    //fill in unused space:
    for (int j = 0; j< blockedRegionSize[1]; j++){
    	for (int i = 0; i< blockedRegionSize[0]; i++){
			if(i> maxreg[0]) regData[i+j*blockedRegionSize[0]] = -1.f;
			if(j> maxreg[1]) regData[i+j*blockedRegionSize[0]] = -1.f;
		}
    }

    //Create a new array to pass to python:
    float* pyData = new float[pydims[0]*pydims[1]];
    assert(pyData);

	//Now realign the array...
    realign2DArray(regData,blockedRegionSize, pyData, (size_t*)pydims); 
    pyRegion = PyArray_SimpleNewFromData(2, pydims, PyArray_FLOAT, pyData);
    
    return Py_BuildValue("O", pyRegion);
}
//calculate extents of specified bounds and refinement level
//Note this is static
PyObject* PythonPipeLine::get_extents(PyObject *self, PyObject* args){
	
    int reflevel;
	int timestep;
    int minreg[3],maxreg[3];
    
    if (!PyArg_ParseTuple(args,"ii(iiiiii)",&timestep, &reflevel,
		minreg,minreg+1,minreg+2,maxreg,maxreg+1,maxreg+2)) return NULL; 
    
	size_t voxmin[3], voxmax[3];
	double userExts[6];
	for (int i = 0; i< 3; i++){
		voxmin[i] = minreg[i];
		voxmax[i] = maxreg[i];
	}
	currentDataMgr->MapVoxToUser(timestep,voxmin, userExts, reflevel);
	currentDataMgr->MapVoxToUser(timestep, voxmax, userExts+3, reflevel);
    
    return Py_BuildValue("(dddddd)", userExts[0],userExts[1],userExts[2],userExts[3],userExts[4],userExts[5]);
}
// static method to copy an array into another one with different dimensioning.
// Useful to convert a blocked region to a smaller region that intersects full domain bounds.
// Also useful to copy smaller region back to full domain bounds.  Source and
// destination region share the same (0,0,0) origin, but dest may be larger or smaller.
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

