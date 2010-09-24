#include <Python.h>
#include "pythonpipeline.h"
#include <numpy/arrayobject.h>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include <vapor/errorcodes.h>

using namespace VetsUtil;
using namespace VAPoR;

std::string PythonPipeLine::startupScript = "";
//myErr = StringIO.StringIO()"+"sys.stderr = myErr";

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
							   int lod, //
							   const size_t bs[3], // block dimensions
							   const size_t min[3],	// dimensions of all variables (in blocks)
							   const size_t max[3]
							   ) {
	//call python wrapper.
	int scriptId = DataStatus::getInstance()->getDerivedScriptId(GetName());
	return python_wrapper(scriptId,ts,reflevel,min,max, 
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
				if(szval > 0){
					//Get the text
					PyObject* txt = PyObject_CallMethod(myErr,"getvalue",NULL);
					const char* strtext = PyString_AsString(txt);
					//post it as Error 
					
					MyBase::SetErrMsg(" Python startup execution error:\n%s\n",strtext);
				}
			}
			return;
		}
	}
	initialized = true;
		
}
//Wrapper for calling a python script from the PipeLine
int PythonPipeLine::python_wrapper(
	int scriptId,size_t ts,int reflevel,
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
	
	
	string pretext = "import sys\n";
	pretext += "import StringIO\n";
	pretext += "import vapor\n";
	pretext += "myIO = StringIO.StringIO()\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stdout = myIO\n";
	pretext += "sys.stderr = myErr\n";

	for (int i = 0; i< inputData.size(); i++){
		string vname = inputs[i];
		Metadata::VarType_T datatype = currentDataMgr->GetVarType(vname);
		if (datatype == Metadata::VAR3D){
			// convert the float array a python array:
			//Create a new array to pass into python:
			float* pyData = new float[pydims[0]*pydims[1]*pydims[2]];
			if(!pyData) return NULL;
			//Now realign the array...
			realign3DArray(inputData[i],blkregsize, pyData, (size_t*)pydims);
			pyRegion = PyArray_SimpleNewFromData(3, pydims, PyArray_FLOAT, pyData);
		} else {
			float* pyData = new float[pydims[0]*pydims[1]];
			if(!pyData) return NULL;
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
	
	int rc;
	
	rc = PyDict_SetItemString(mainDict, "__TIMESTEP__", timestep);
	rc = PyDict_SetItemString(mainDict, "__REFINEMENT__",refinement);
	rc = PyDict_SetItemString(mainDict, "__BOUNDS__",exts);

	string program = pretext+pythonMethod;
	
    PyObject* retObj = PyRun_String(program.c_str(),Py_file_input, mainDict,mainDict);
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
			
				MyBase::SetErrMsg(" Python execution error:\n%s\n",strtext);
			}
		}
	return -1;
    }
   
	
	//Retrieve all the output variables using the dictionary.  First do 3d, then 2d
	for (int i = 0; i< outputs.size(); i++) {
		if (outputs[i].second == Metadata::VAR3D) {
			const char* vname = outputs[i].first.c_str();
		
			PyObject* ky = Py_BuildValue("s",vname);
			
			PyObject* varArray = PyDict_GetItem(mainDict, ky);
			float *dataArray = (float*)PyArray_DATA(varArray);
			//Realign, put into DataMgr's allocated region
			realign3DArray(dataArray, regsize, outputData[i], blkregsize);
			
		} else if (outputs[i].second == Metadata::VAR2D_XY) {
			PyObject* ky = Py_BuildValue("s",(outputs[i].first).c_str());
			
			PyObject* varArray = PyDict_GetItem(mainDict, ky);
			float *dataArray = (float*)PyArray_DATA(varArray);
			//Realign, put into DataMgr's allocated region
			realign2DArray(dataArray, regsize, outputData[i], blkregsize);
		}
		else assert(0);  // no other 2d orientations supported
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
					size_t ts,int reflevel,const size_t min[3],const size_t max[3]){
		
	if(!initialized) initialize();
	
	
	string pretext = "import sys\n";
	pretext += "import StringIO\n";
	pretext += "import vapor\n";
	pretext += "myIO = StringIO.StringIO()\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stdout = myIO\n";
	pretext += "sys.stderr = myErr\n";
	for (int i = 0; i< inputVars3.size(); i++){
		pretext += inputVars3[i] + " = vapor.Get3DVariable('" + inputVars3[i] + "',__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	for (int i = 0; i< inputVars2.size(); i++){
		pretext += inputVars2[i] + " = vapor.Get2DVariable('" + inputVars2[i] + "',__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	
	
	string pythonMethod = pretext + script ;
	
	
	//must convert input block extents to actual region extents
	size_t dim[3];
	size_t blksize[3];

	//get the module dictionary...
	PyObject* mainModule = PyImport_AddModule("__main__");
	PyObject* mainDict = PyModule_GetDict(mainModule);

	if (currentDataMgr){
		currentDataMgr->GetBlockSize(blksize,reflevel);
		
		currentDataMgr->GetDim(dim, reflevel);	
		
		int regmin[3],regmax[3];
		for (int i = 0; i<3; i++){
			regmin[i] = min[i]*blksize[i];
			regmax[i] = (max[i]+1)*blksize[i]-1;
			if (regmax[i] >= dim[i]) regmax[i] = dim[i]-1; 
		}
		PyObject* exts = Py_BuildValue("(iiiiii)",regmin[0],regmin[1],regmin[2],regmax[0],regmax[1],regmax[2]);
		PyObject* refinement = Py_BuildValue("i",reflevel);
		PyObject* timestep = Py_BuildValue("i",ts);
		
		int rc = PyDict_SetItemString(mainDict, "__TIMESTEP__", timestep);
		rc = PyDict_SetItemString(mainDict, "__REFINEMENT__",refinement);
		rc = PyDict_SetItemString(mainDict, "__BOUNDS__",exts);
	}
	
	// RUN THE INTERPRETER!!!
    PyObject* retObj = PyRun_String(pythonMethod.c_str(),Py_file_input, mainDict,mainDict);
	
	
    if (!retObj){
		PyErr_Print();
		MyBase::SetErrMsg(VAPOR_ERROR,"Python interpreter failure");
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
				MyBase::SetErrMsg(" Python execution error:\n%s\n",strtext);
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

    int tstep, reflevel;
    int minreg[3],maxreg[3];
    size_t minblkreg[3],maxblkreg[3];
    if (!PyArg_ParseTuple(args,"sii(iiiiii)",&varname,&tstep,&reflevel,
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
    
    regData = currentDataMgr->GetRegion(tstep, varname, reflevel, 0, minblkreg, maxblkreg, 0);
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

    int tstep, reflevel;
    int minreg[3],maxreg[3];
    size_t minblkreg[3],maxblkreg[3];
    if (!PyArg_ParseTuple(args,"sii(iiiiii)",&varname,&tstep,&reflevel,
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
    
    regData = currentDataMgr->GetRegion(tstep, varname, reflevel, 0, minblkreg, maxblkreg, 0);
    
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

