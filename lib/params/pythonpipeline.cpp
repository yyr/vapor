#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <sstream>
#include <map>
#include <fstream>
#include <iostream>
#include <QMutex>
#include <Python.h>
#include <numpy/arrayobject.h>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include <vapor/errorcodes.h>
#include <vapor/GetAppPath.h>
#include "pythonpipeline.h"

using namespace VetsUtil;
using namespace VAPoR;

#ifdef _WINDOWS //Define INFINITY
#include <limits>
	float INFINITY = numeric_limits<float>::infinity( );
#endif

std::string PythonPipeLine::startupScript = "";
bool PythonPipeLine::everInitialized = false;
PyObject* PythonPipeLine::vaporModule = 0;

DataMgr* PythonPipeLine::currentDataMgr = 0;
bool PythonPipeLine::initialized = false;
std::map<PyObject*, float*> PythonPipeLine::arrayAllocMap;
std::map<PyObject*, bool*> PythonPipeLine::maskAllocMap;
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
		{"Get3DMask",PythonPipeLine::get_3Dmask, METH_VARARGS,
                        "Get a 3d mask for a variable"},
		{"Get2DMask",PythonPipeLine::get_2Dmask, METH_VARARGS,
                        "Get a 2d mask for a variable"},
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
		if (PyErr_Occurred()) { MyBase::SetErrMsg("error importing NumPy");
			return;
		}
        
        vaporerror = PyErr_NewException((char*)"vapor.error",NULL,NULL);
        Py_INCREF(vaporerror);
        PyModule_AddObject(m, "error", vaporerror);
}

void PythonPipeLine::initialize(){
	// Specify some standard imports
	if(initialized) return;
	char* pyhome2 = 0;
	//Always set PYTHONHOME to VAPOR_PYTHONHOME, if it is set
	char* pyhome = getenv("VAPOR_PYTHONHOME");
	if (pyhome) Py_SetPythonHome(pyhome);
	else {
		char* pyset = getenv("PYTHONHOME");
		if(!pyset) { //neither PYTHONHOME nor VAPOR_PYTHONHOME are set:
			//Set pythonhome to the vapor installation (based on VAPOR_HOME)
			vector<string> pths;
//On windows use VAPOR_HOME/lib/python2.7; VAPOR_HOME works on Linux and Mac
#ifdef _WINDOWS
			pths.push_back("python2.7");
			string phome = GetAppPath("VAPOR","lib", pths, true);
#else
			string phome = GetAppPath("VAPOR","", pths, true);
			//On linux this is VAPOR_HOME;
			//On MacOS this is VAPOR/VAPOR.app/Contents/MacOS/
#endif
			pyhome2 = new char[phome.length()+1];
			strncpy(pyhome2,phome.c_str(),phome.length()+1);
			// if VAPOR_HOME is not set, then don't set PYTHONHOME either
			if (phome.length() > 0){
				//Note:  Python seems very picky about the strings accepted for this.
				//It must remain for a while (at least until after Py_Initialize is called).
				//It's also important to use forward slashes even on Windows.
				//printf("Setting PYTHONHOME in the vaporgui app to %s\n", pyhome2);
				Py_SetPythonHome(pyhome2);
				char* newhome = Py_GetPythonHome();
			}
		}
	}
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
				"VAPOR_HOME may not be properly set",sysProg.substr(0,400).c_str());
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
	if (pyhome2) delete pyhome2;
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
	vector<bool*> allocatedMasks;

	//If any variable has missing data we will use numpy.ma
	
	bool useMask = false;
	for (int i = 0; i< inputData.size(); i++){
		if (inputData[i]->HasMissingData()) useMask = true;
	}
   
    if(!initialized) initialize();
	//See if there are any 3D outputs.  If so use the
	//first 3D output var to determine the dimensions.  If not,
	//just use the first output variable
	int first3DVar = -1, outputIndex = 0;
	for (int i = 0; i<outputs.size(); i++){
		if (outputs[i].second == DataMgr::VAR3D){
			outputIndex = i;
			first3DVar = i;
			break;
		}
	}
	//Determine sizes of arrays to allocate for python:
	RegularGrid *rg = outputData[outputIndex];
	size_t dims[3], orig[3];
	rg->GetDimensions(dims);
	rg->GetIJKOrigin(orig);
	size_t mins[3],maxs[3];
	//double umins[3],umaxs[3];
	for (int i = 0; i<3; i++){
		mins[i] = orig[i];
		maxs[i] = dims[i]+orig[i]-1;
	}
	
	int regmin[3], regmax[3];
	Py_ssize_t pydims[3];
	PyObject* pyRegion, *pyRegionMask;
	
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
	if (useMask) pretext += "import numpy.ma\n";
	pretext += "myIO = StringIO.StringIO()\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stdout = myIO\n";
	pretext += "sys.stderr = myErr\n";
	
	
   
	// Create the data that will be associated arrays and the masks
	for (int i = 0; i< inputData.size(); i++){
		string vname = inputs[i];
		DataMgr::VarType_T datatype = currentDataMgr->GetVarType(vname);
		if (datatype == DataMgr::VAR3D){
			//Check if there are no 3D outputs; if so, make the 
			//3D vars be full in the Z dimension
			if (first3DVar < 0){
				//Find the full height of the data:
				size_t dim[3];
				currentDataMgr->GetDim(dim, reflevel); 
				pydims[0] = (Py_ssize_t) dim[2];
			}
			//Create a new array to pass into python:
			//If the output variable is 2D then make the vertical extents full in the domain

			float* pyData = new(nothrow) float[pydims[0]*pydims[1]*pydims[2]];
			if(!pyData) return 0;
			bool* pyMask = 0;
			if (useMask){
				pyMask = new(nothrow) bool[pydims[0]*pydims[1]*pydims[2]];
				if (!pyMask) return 0;
				allocatedMasks.push_back(pyMask);
			}
			allocatedArrays.push_back(pyData);
			//Now get the data and its mask
			copyFrom3DGrid(inputData[i], pyData, pyMask);
			pyRegion = PyArray_New(&PyArray_Type,3,pydims,PyArray_FLOAT,NULL,pyData,0,
								   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
			if (useMask){
				pyRegionMask = PyArray_New(&PyArray_Type,3,pydims,PyArray_BOOL,NULL,pyMask,0,
								   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
			}
			
		} else { // 2D variable
			//The last two python dimension are the first two user dimensions:
			float* pyData = new(nothrow) float[pydims[1]*pydims[2]];
			if(!pyData) return 0;
			bool* pyMask = 0;
			if (useMask){
				pyMask = new(nothrow) bool[pydims[1]*pydims[2]];
				if (!pyMask) return 0;
				allocatedMasks.push_back(pyMask);
			}
			allocatedArrays.push_back(pyData);
			//Now copy the data from the grid.
			copyFrom2DGrid(inputData[i], pyData, pyMask);
			pyRegion = PyArray_New(&PyArray_Type,2,pydims+1,PyArray_FLOAT,NULL,pyData,0,
								   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
			if (useMask){
				pyRegionMask = PyArray_New(&PyArray_Type,2,pydims+1,PyArray_BOOL,NULL,pyMask,0,
								   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
			}
			
		}
		
		//Put it into the dictionary, with modified name for masking
		if(useMask){
			string vname= (inputs[i]+"_origdata");
			string mname= (inputs[i]+"_mask");
			PyObject* ky1 = Py_BuildValue("s",vname.c_str());
			PyObject* ky2 = Py_BuildValue("s",mname.c_str());
			PyObject_SetItem(mainDict,ky1,pyRegion);
			PyObject_SetItem(mainDict,ky2,pyRegionMask);
			pretext += inputs[i] + " = numpy.ma.masked_array(" + vname +",mask="+mname+")\n";
		} else {
			PyObject* ky = Py_BuildValue("s",inputs[i].c_str());
			PyObject_SetItem(mainDict,ky,pyRegion);
		}
	}
	
	

	
    PyObject* retObj = PyRun_String(pretext.c_str(),Py_file_input, mainDict,mainDict);
	if (!retObj){
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Python interpreter preparation error");
		return -1;
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

	//If useMask is true, then, for each output variable insert a statement that retrieves the mask.  Cast it to a
	//masked array in case it did not already have a mask
	if (useMask){
		for (int i = 0; i< outputs.size(); i++) {
			string vname = outputs[i].first;
			string mname = vname+"_mask";
			//insert eos before, because the python string may not have one.
			pythonMethod += "\n"+ mname+ "="+"numpy.ma.masked_array("+vname+").mask";
		}
	}
	
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
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Shape of %s array does not conform with vapor.BOUNDS",vname);
				return -1;
			}
		}
		//If useMask is true, then find the corresponding mask variable in the data
		bool* dataMask = 0;
		if (useMask){
			//find the mask in the dictionary:
			string mname = outputs[i].first+"_mask";
			
			PyObject* ky = Py_BuildValue("s",mname.c_str());
		
			PyObject* maskArray = PyDict_GetItem(mainDict, ky);
			if (! maskArray){
				MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING, "Mask %s not produced by script",mname.c_str());
				return -1;
			}
			dataMask = (bool*)PyArray_DATA(maskArray);
		}
		float *dataArray = (float*)PyArray_DATA(varArray);
			
		if (dimen == 3) {
			//Realign, put into DataMgr's allocated region
			copyTo3DGrid(dataArray,  dataMask, outputData[i]);
			
		} else {
			copyTo2DGrid(dataArray, dataMask, outputData[i]);
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
		delete [] allocatedArrays[i];
	}
	for (int i = 0; i< allocatedMasks.size(); i++){
		delete [] allocatedMasks[i];
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
	
	
	
	//Determine whether or not missing values are in use:
	bool useMask = usingMissingValues(ts,inputVars2,inputVars3,rmin,rmax);
	//useMask = true;  //Only for testing!
	
	
	if (useMask) {
		pretext += "import numpy.ma\n";
	}

	for (int i = 0; i< inputVars3.size(); i++){
		string varname3d = inputVars3[i];
		string maskname3d;
		if (useMask) {
			varname3d += "_origdata";
			maskname3d = inputVars3[i]+"_mask";
		}
		pretext += varname3d + " = vapor.Get3DVariable(vapor.TIMESTEP,'" + inputVars3[i] + "',vapor.REFINEMENT,vapor.LOD,vapor.BOUNDS)\n";
		if (useMask){
			pretext += maskname3d + " = vapor.Get3DMask(vapor.TIMESTEP,'" + inputVars3[i] + "',vapor.REFINEMENT,vapor.LOD,vapor.BOUNDS)\n";
			pretext += inputVars3[i] + " = numpy.ma.masked_array(" + varname3d +",mask="+maskname3d+")\n";
		}
	}
	for (int i = 0; i< inputVars2.size(); i++){
		string varname2d = inputVars2[i];
		string maskname2d;
		if (useMask) {
			varname2d += "_origdata";
			maskname2d = inputVars2[i]+"_mask";
		}
		pretext += varname2d + " = vapor.Get2DVariable(vapor.TIMESTEP,'" + inputVars2[i] + "',vapor.REFINEMENT,vapor.LOD,vapor.BOUNDS)\n";
		if (useMask){
			pretext += maskname2d + " = vapor.Get2DMask(vapor.TIMESTEP,'" + inputVars2[i] + "',vapor.REFINEMENT,vapor.LOD,vapor.BOUNDS)\n";
			pretext += inputVars2[i] + " = numpy.ma.masked_array(" + varname2d +",mask="+maskname2d+")\n";
		} 


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
    float* pyData = new(nothrow) float[pydims[0]*pydims[1]*pydims[2]];
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
    float* pyData = new(nothrow) float[pydims[0]*pydims[1]];
    if(!pyData) return NULL;

	//Now copy in the data:
	copyFrom2DGrid(rg, pyData);
    currentDataMgr->UnlockGrid(rg);

    pyRegion = PyArray_New(&PyArray_Type,2,pydims,PyArray_FLOAT,NULL,pyData,0,
						   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
    mapArrayObject(pyRegion, pyData);
    return Py_BuildValue("O", pyRegion);
}
//get/set called by Python interpreter:
//Note this is static
PyObject* PythonPipeLine::get_3Dmask(PyObject *self, PyObject* args){
	const char *varname;
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
    float* pyData = new(nothrow) float[pydims[0]*pydims[1]*pydims[2]];
    if(!pyData) return NULL;
	//Create a new array to pass to python:
    bool* pyMask = new(nothrow) bool[pydims[0]*pydims[1]*pydims[2]];
    if(!pyMask) return NULL;

	//Now copy in the data:
	copyFrom3DGrid(rg, pyData,pyMask);
    currentDataMgr->UnlockGrid(rg);
	delete [] pyData;
	pyRegion = PyArray_New(&PyArray_Type,3,pydims,PyArray_BOOL,NULL,pyMask,0,
						   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
	mapMaskObject(pyRegion, pyMask);
    return Py_BuildValue("O", pyRegion);
}
//get/set 2D called by Python interpreter:
//Note this is static
//the z coord of extents is ignored
PyObject* PythonPipeLine::get_2Dmask(PyObject *self, PyObject* args){
	const char *varname;
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
    float* pyData = new(nothrow) float[pydims[0]*pydims[1]];
    if(!pyData) return NULL;
	bool* pyMask = new(nothrow) bool[pydims[0]*pydims[1]];
    if(!pyMask) return NULL;

	//Now copy in the data:
	copyFrom2DGrid(rg, pyData, pyMask);
    currentDataMgr->UnlockGrid(rg);
	delete [] pyData;
    pyRegion = PyArray_New(&PyArray_Type,2,pydims,PyArray_BOOL,NULL,pyMask,0,
						   NPY_C_CONTIGUOUS|NPY_ALIGNED|NPY_WRITEABLE, NULL);
    mapMaskObject(pyRegion, pyMask);
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
    //The final optional argument is LOD
	int lod = 0;
    if (!PyArg_ParseTuple(args,"(iii)i|i",
						  ivox,ivox+1,ivox+2,&reflevel,&lod)) return NULL; 
    
	for (int i = 0; i< 3; i++) voxCrds[i] = (size_t)ivox[2-i];

	currentDataMgr->MapVoxToUser(0,voxCrds,userCrds, reflevel,lod);
	
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
	//Optional arg is lod
	int lod = 0;
    
    if (!PyArg_ParseTuple(args,"(ddd)i|i",
						  userCrds+2,userCrds+1,userCrds,&reflevel,&lod)) return NULL; 
	
	currentDataMgr->MapUserToVox(0, userCrds, vox, reflevel,lod);
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
//Coordinate order is always reversed. 
void PythonPipeLine::copyTo3DGrid(const float* srcArray, bool* srcMask, RegularGrid* destGrid){
	size_t dims[3];
	destGrid->GetDimensions(dims);
	for (int k =0; k<dims[2]; k++){
		for (int j = 0; j< dims[1]; j++){
			for (int i = 0; i< dims[0]; i++){
				//get index in srcArray (reversed, i.e. python order)
				int srcIndex = i+j*dims[0]+k*dims[0]*dims[1];
				//obtain from Vapor grid coordinate order
				float& val = destGrid->AccessIJK(i,j,k);
				if (srcMask && srcMask[srcIndex])
					val = destGrid->GetMissingValue();
				else val = srcArray[srcIndex];
			}
		}
	}
}
void PythonPipeLine::copyFrom3DGrid(const RegularGrid* srcGrid, float* destArray, bool* destMask){
	size_t dims[3];
	srcGrid->GetDimensions(dims);
	if (!destMask){
		for (int k = 0; k < dims[2]; k++){
			for (int j = 0; j < dims[1]; j++){
				for (int i = 0; i < dims[0]; i++){
					//destIndex is in python order
					int destIndex = i+j*dims[0]+k*dims[0]*dims[1];
					float& val = srcGrid->AccessIJK(i,j,k);
					destArray[destIndex] = val;
				}
			}
		}
	} else {
		float maskValue = srcGrid->GetMissingValue();
		for (int k = 0; k < dims[2]; k++){
			for (int j = 0; j < dims[1]; j++){
				for (int i = 0; i < dims[0]; i++){
					//destIndex is in python order
					int destIndex = i+j*dims[0]+k*dims[0]*dims[1];
					float& val = srcGrid->AccessIJK(i,j,k);
					destArray[destIndex] = val;
					if (val == maskValue)
						destMask[destIndex] = true;
					else
						destMask[destIndex] = false;
				}
			}
		}
	}
}
void PythonPipeLine::copyTo2DGrid(const float* srcArray, bool* srcMask, RegularGrid* destGrid){
	size_t dims[3];
	destGrid->GetDimensions(dims);
	//For 2D data, only use first two VAPOR dimensions
	for (int j = 0; j<  dims[1]; j++){
		for (int i = 0; i< dims[0]; i++){
			int srcIndex = i+ j*dims[0];
			float& val = destGrid->AccessIJK(i,j,0);
			if (srcMask && srcMask[srcIndex])
				val = destGrid->GetMissingValue();
			else val = srcArray[srcIndex];
		}
	}
}
void PythonPipeLine::copyFrom2DGrid(const RegularGrid* srcGrid, float* destArray, bool* destMask){
	size_t dims[3];
	srcGrid->GetDimensions(dims);
	if (!destMask){
		for (int j = 0; j< dims[1]; j++){
			for (int i = 0; i< dims[0]; i++){
				int destIndex = i+ j*dims[0];
				float& val = srcGrid->AccessIJK(i,j,0);
				destArray[destIndex] = val;
			}
		}
	} else {
		float maskValue = srcGrid->GetMissingValue();
		for (int j = 0; j < dims[1]; j++){
			for (int i = 0; i < dims[0]; i++){
				//destIndex is in python order
				int destIndex = i+ j*dims[0];
				float& val = srcGrid->AccessIJK(i,j,0);
				destArray[destIndex] = val;
				if (val == maskValue)
					destMask[destIndex] = true;
				else
					destMask[destIndex] = false;
			}
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
		delete [] ary;
		arrayAllocMap.erase(mapiter);
	}
	map< PyObject*,bool*> :: iterator mapiter2 = maskAllocMap.find(pyobj);
	if (mapiter2 != maskAllocMap.end()){
		bool* ary = mapiter2->second;
		delete [] ary;
		maskAllocMap.erase(mapiter2);
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
void PythonPipeLine::mapMaskObject(PyObject* pyobj, bool* ary){
	if (!arrayAllocMutex->tryLock(10000)){//  <= 10 second wait..
		MyBase::SetErrMsg(VAPOR_ERROR_SCRIPTING,"Thread conflict in array allocation");
		return;
	}
	maskAllocMap[pyobj] = ary;
	arrayAllocMutex->unlock();
}
bool PythonPipeLine::usingMissingValues(size_t ts,const std::vector<string>& inputVars2,const std::vector<string>& inputVars3,size_t* rmin,size_t* rmax){
	
	if (currentDataMgr){
		for (int i = 0; i< inputVars2.size(); i++){
			RegularGrid* rg = currentDataMgr->GetGrid(ts, inputVars2[i], 0,0, rmin,rmax, false);
			if (rg && rg->HasMissingData()){
				delete rg;
				return true;
			}
			delete rg;
		}
		for (int i = 0; i< inputVars3.size(); i++){
			RegularGrid* rg = currentDataMgr->GetGrid(ts, inputVars3[i], 0,0, rmin,rmax, false);
			if (rg && rg->HasMissingData()){
				delete rg;
				return true;
			}
			delete rg;
		}
	}
	return false;
}
