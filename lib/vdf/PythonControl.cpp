#include <Python.h>
#include "vapor/PythonControl.h"
#include <numpy/arrayobject.h>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <vector>
#include <map>
#include "vapor/DataMgr.h"
#include "vaporinternal/common.h"
#include "vapor/errorcodes.h"

using namespace VetsUtil;
using namespace VAPoR;

DataMgr* PythonControl::derVarDataMgr = 0;
PyMethodDef PythonControl::vaporMethodDefinitions[] = { 
         {"Get3DVariable",PythonControl::get_3Dvariable, METH_VARARGS,
                        "Get a variable from the DataMgr cache"},
		{"Set3DVariable",PythonControl::set_3Dvariable, METH_VARARGS,
                        "Set a variable into the DataMgr cache"},
		 {"Get2DVariable",PythonControl::get_2Dvariable, METH_VARARGS,
                        "Get a variable from the DataMgr cache"},
         {"Set2DVariable",PythonControl::set_2Dvariable, METH_VARARGS,
                        "Set a variable into the DataMgr cache"},
         {NULL,NULL,0,NULL}
 };



PythonControl::PythonControl(DataMgr* dataMgr
) {
	derVarDataMgr = dataMgr;
}


PythonControl::~PythonControl()
 {

}

// windows needs: pymodinitfunc
static PyObject *vaporerror;
void PythonControl::initvapor(void)
{
	PyObject *m;
        m = Py_InitModule("vapor", vaporMethodDefinitions);
        if (m == NULL)
                return;
        import_array();
        fprintf(stderr, "initialized vapor in initregion\n");
        vaporerror = PyErr_NewException((char*)"vapor.error",NULL,NULL);
        Py_INCREF(vaporerror);
        PyModule_AddObject(m, "error", vaporerror);
}

//Wrapper for calling a python script from the DataMgr:
int PythonControl::python_wrapper(int scriptId,size_t ts,int reflevel,const size_t min[3],const size_t max[3]){
   static int numtries = 0;
    printf("Execute Python program");
	//Insert getVar and setVar for inputs and outputs
	const vector<string>& inputVars3 = derVarDataMgr->getDerived3DInputs(scriptId);
	const vector<string>& outputVars3 = derVarDataMgr->getDerived3DOutputs(scriptId);
	const vector<string>& inputVars2 = derVarDataMgr->getDerived2DInputs(scriptId);
	const vector<string>& outputVars2 = derVarDataMgr->getDerived2DOutputs(scriptId);
	string pretext = "import numpy\n";
	pretext += "import vapor\n";
	pretext += "import sys\n";
	pretext += "import StringIO\n";
	pretext += "myErr = StringIO.StringIO()\n";
	pretext += "sys.stderr = myErr\n";

	for (int i = 0; i< inputVars3.size(); i++){
		pretext += inputVars3[i] + " = vapor.Get3DVariable('" + inputVars3[i] + "',__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	for (int i = 0; i< inputVars2.size(); i++){
		pretext += inputVars2[i] + " = vapor.Get2DVariable('" + inputVars2[i] + "',__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	string posttext = "\n";
	for (int i = 0; i< outputVars3.size(); i++){
		posttext += "vapor.Set3DVariable('" + outputVars3[i] + "',"+outputVars3[i]+",__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	
	for (int i = 0; i< outputVars2.size(); i++){
		posttext += "vapor.Set2DVariable('" + outputVars2[i] + "',"+outputVars2[i]+",__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	string pythonMethod = pretext + derVarDataMgr->getDerivedScript(scriptId) + posttext;
    printf("In Wrapper, Program is: %s", pythonMethod.c_str());
	printf(" min, max: %d %d %d %d %d %d\n",min[0],min[1],min[2],max[0],max[1],max[2]);
	printf(" timestep, reflevel: %d %d\n",ts, reflevel);

// Name the built-in module to be imported:
        if(numtries == 0){
                PyImport_AppendInittab((char*)"vapor",&(PythonControl::initvapor));
                fprintf(stderr,"called PyImport_AppendInittab\n");
                Py_Initialize();
                fprintf(stderr,"called PyInitialize\n");
        }
   //     numtries++;
	//See if we can put something in the dictionary...
    PyObject* val = Py_BuildValue("s","value");
	PyObject* refinement = Py_BuildValue("i",reflevel);
	PyObject* timestep = Py_BuildValue("i",ts);
	//must convert input block extents to actual region extents
	size_t dim[3];

	const size_t* blksize = derVarDataMgr->GetBlockSize();
	derVarDataMgr->GetDim(dim, reflevel);	
	printf("dim is: %d %d %d\n",dim[0],dim[1],dim[2]);
	int regmin[3],regmax[3];
	for (int i = 0; i<3; i++){
		regmin[i] = min[i]*blksize[i];
		regmax[i] = (max[i]+1)*blksize[i]-1;
		if (regmax[i] >= dim[i]) regmax[i] = dim[i]-1; 
	}
	PyObject* exts = Py_BuildValue("(iiiiii)",regmin[0],regmin[1],regmin[2],regmax[0],regmax[1],regmax[2]);
//get the module dictionary...
    PyObject* mainModule = PyImport_AddModule("__main__");
        int rc = PyObject_SetAttrString(mainModule,"foo",val);
        printf("setattrstring rc = %d\n",rc);

    PyObject* mainDict = PyModule_GetDict(mainModule);
	rc = PyDict_SetItemString(mainDict, "__TIMESTEP__", timestep);
	rc = PyDict_SetItemString(mainDict, "__REFINEMENT__",refinement);
	rc = PyDict_SetItemString(mainDict, "__BOUNDS__",exts);
// Print all the string keys or values in the dictionary:
        Py_ssize_t pos = 0;
        PyObject* k, *v;
        while (PyDict_Next(mainDict, &pos, &k, &v)){
                if(PyString_Check(k)){
                        fprintf(stderr,"Key %d: %s\n",pos,PyString_AS_STRING(k));
                } else {
                        fprintf(stderr,"Key %d is not a string\n",pos);
                }
        }
    PyObject* retObj = PyRun_String(pythonMethod.c_str(),Py_file_input, mainDict,mainDict);
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
			printf(" Python error text:\n %s\n",strtext);
			MyBase::SetErrMsg(" Python execution error:\n%s\n",strtext);
		}
	}
	return -1;
    }
    //Py_Finalize(); //never call this until vapor exits?
        pos = 0;
        fprintf(stderr," results:\n");
        while (PyDict_Next(mainDict, &pos, &k, &v)){
                if(PyString_Check(k)){
                        fprintf(stderr,"Key %d: %s\n",pos,PyString_AS_STRING(k));
                } else {
                        fprintf(stderr,"Key %d is not a string\n",pos);
                }
        }
        return 0;
}

//Wrapper for testing a python script, called from python editor
//Returns text output resulting from execution
//Supplies its own variables, not those saved to datamgr
//
std::string& PythonControl::
python_test_wrapper(const string& script, const vector<string>& inputVars2,const vector<string>& outputVars2, 
								 const vector<string>& inputVars3,const vector<string>& outputVars3, 
								 size_t ts,int reflevel,const size_t min[3],const size_t max[3]){
   static int numtries = 0;
    printf("Execute Python program");
	//Insert getVar and setVar for inputs and outputs
	
	string pretext = "import numpy\n";
	pretext += "import vapor\n";
	pretext += "import sys\n";
	pretext += "import StringIO\n";
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
	string posttext = "\n";
	for (int i = 0; i< outputVars2.size(); i++){
		posttext += "vapor.Set2DVariable('" + outputVars2[i] + "',"+outputVars2[i]+",__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	for (int i = 0; i< outputVars3.size(); i++){
		posttext += "vapor.Set3DVariable('" + outputVars3[i] + "',"+outputVars3[i]+",__TIMESTEP__,__REFINEMENT__,__BOUNDS__)\n";
	}
	string pythonMethod = pretext + script + posttext;
    printf("In test wrapper, Program is: %s", pythonMethod.c_str());
	printf(" min, max: %d %d %d %d %d %d\n",min[0],min[1],min[2],max[0],max[1],max[2]);
	printf(" timestep, reflevel: %d %d\n",ts, reflevel);

// Name the built-in module to be imported:
        if(numtries == 0){
                PyImport_AppendInittab((char*)"vapor",&(PythonControl::initvapor));
                fprintf(stderr,"called PyImport_AppendInittab\n");
                Py_Initialize();
                fprintf(stderr,"called PyInitialize\n");
        }
   //     numtries++;
	//See if we can put something in the dictionary...
    PyObject* val = Py_BuildValue("s","value");
	PyObject* refinement = Py_BuildValue("i",reflevel);
	PyObject* timestep = Py_BuildValue("i",ts);
	//must convert input block extents to actual region extents
	size_t dim[3];

	const size_t* blksize = derVarDataMgr->GetBlockSize();
	derVarDataMgr->GetDim(dim, reflevel);	
	printf("dim is: %d %d %d\n",dim[0],dim[1],dim[2]);
	int regmin[3],regmax[3];
	for (int i = 0; i<3; i++){
		regmin[i] = min[i]*blksize[i];
		regmax[i] = (max[i]+1)*blksize[i]-1;
		if (regmax[i] >= dim[i]) regmax[i] = dim[i]-1; 
	}
	PyObject* exts = Py_BuildValue("(iiiiii)",regmin[0],regmin[1],regmin[2],regmax[0],regmax[1],regmax[2]);
//get the module dictionary...
    PyObject* mainModule = PyImport_AddModule("__main__");
        int rc = PyObject_SetAttrString(mainModule,"foo",val);
        printf("setattrstring rc = %d\n",rc);

    PyObject* mainDict = PyModule_GetDict(mainModule);
	rc = PyDict_SetItemString(mainDict, "__TIMESTEP__", timestep);
	rc = PyDict_SetItemString(mainDict, "__REFINEMENT__",refinement);
	rc = PyDict_SetItemString(mainDict, "__BOUNDS__",exts);
// Print all the string keys or values in the dictionary:
        Py_ssize_t pos = 0;
        PyObject* k, *v;
        while (PyDict_Next(mainDict, &pos, &k, &v)){
                if(PyString_Check(k)){
                        fprintf(stderr,"Key %d: %s\n",pos,PyString_AS_STRING(k));
                } else {
                        fprintf(stderr,"Key %d is not a string\n",pos);
                }
        }

// RUN THE INTERPRETER!!!
    PyObject* retObj = PyRun_String(pythonMethod.c_str(),Py_file_input, mainDict,mainDict);
//

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
			printf(" Python error text:\n %s\n",strtext);
			MyBase::SetErrMsg(" Python execution error:\n%s\n",strtext);
		}
	}
	}
    //Py_Finalize(); //never call this until vapor exits?
        pos = 0;
        fprintf(stderr," results:\n");
        while (PyDict_Next(mainDict, &pos, &k, &v)){
                if(PyString_Check(k)){
                        fprintf(stderr,"Key %d: %s\n",pos,PyString_AS_STRING(k));
                } else {
                        fprintf(stderr,"Key %d is not a string\n",pos);
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
			printf(" Python output text:\n %s\n",strtext);
			MyBase::SetDiagMsg(" Python output text:\n%s\n",strtext);
			pythonOutputText = strtext;
		}
	}
	//invoke getvalue(), putting text into a string, then return that string
	//Also provide that text as diagnostic
        return pythonOutputText;
}

//get/set called by Python interpreter:
//Note this is static
PyObject* PythonControl::get_3Dvariable(PyObject *self, PyObject* args){
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
    printf("in get_3Dvariable, tstep = %d, reflevel = %d extents = %d %d %d %d %d %d\n",
	tstep, reflevel, minreg[0],minreg[1],minreg[2], maxreg[0],maxreg[1],maxreg[2]);
    //Need to convert min,max extents to block extents
    const size_t* blksize = derVarDataMgr->GetBlockSize();
    derVarDataMgr->GetDim(dims,reflevel);
    for (int i = 0; i<3; i++){
	minblkreg[i] = minreg[i]/blksize[i];
	maxblkreg[i] = maxreg[i]/blksize[i];
	blockedRegionSize[i] = (maxblkreg[i]-minblkreg[i]+1)*blksize[i];
	pydims[i] = blockedRegionSize[i];
	int maxPySize = (dims[i]-minreg[i]); 
	if (pydims[i] > maxPySize) pydims[i] = maxPySize;
    }
    
    regData = derVarDataMgr->GetRegion(tstep, varname, reflevel, -1, minblkreg, maxblkreg, 0);
    if (regData) fprintf(stderr,"obtained region data, dims %d %d %d\n",
                pydims[0],pydims[1],pydims[2]);
    //Create a new array to pass to python:
    float* pyData = new float[pydims[0]*pydims[1]*pydims[2]];
    if(!pyData) return NULL;

	//Now realign the array...
    realign3DArray(regData,blockedRegionSize, pyData, (size_t*)pydims); 
    pyRegion = PyArray_SimpleNewFromData(3, pydims, PyArray_FLOAT, pyData);
    fprintf(stderr, "converted to pyRegion\n");
    return Py_BuildValue("O", pyRegion);
}
PyObject* PythonControl::set_3Dvariable(PyObject *self, PyObject* args){
	//Currently assume variable is 3D
	//Need to get varname, data, timestep, reflevel, extents
	//from the argument list 
	const char* varname;
	PyObject* arr;
	int tstep, reflevel;
	int minreg[3],maxreg[3];
	size_t minblkreg[3],maxblkreg[3],blkregsize[3],regsize[3];
	printf(" called set_3Dvariable\n");
	if (!PyArg_ParseTuple(args,"sO!ii(iiiiii)",&varname,&PyArray_Type,&arr,&tstep,&reflevel,
		minreg,minreg+1,minreg+2,maxreg,maxreg+1,maxreg+2)) {
		fprintf(stderr,"failed to get all args\n");
		return NULL;
	} 
    	printf("in set_3Dvariable, varname = %s, tstep = %d, reflevel = %d extents = %d %d %d %d %d %d\n",
		varname, tstep, reflevel, minreg[0],minreg[1],minreg[2], maxreg[0],maxreg[1],maxreg[2]);
	//Calculate the block extents
	//The minreg should already be based on block dims
	//The maxreg should already be truncated to fit inside domain
	const size_t *blksize = derVarDataMgr->GetBlockSize();
	for(int i = 0; i<3; i++){
		regsize[i] = maxreg[i]-minreg[i]+1;
		minblkreg[i] = minreg[i]/blksize[i];
		maxblkreg[i] = maxreg[i]/blksize[i];
		blkregsize[i] = (maxblkreg[i]-minblkreg[i]+1)*blksize[i];
	}
	printf(" block extents are %d %d %d %d %d %d\n",
		minblkreg[0],minblkreg[1],minblkreg[2],maxblkreg[0],maxblkreg[1],maxblkreg[2]);
	//convert the data to a C-array:
	float* data = (float*)PyArray_DATA(arr);
	//(May also want to check that the Python strides agree with minreg, maxreg) 
	//Allocate a buffer in the cache to put it
	float* blks = (float *) derVarDataMgr->alloc_region(
		tstep,varname,Metadata::VAR3D, reflevel,-1, DataMgr::FLOAT32,minblkreg,maxblkreg,0);
	if (! blks) return(NULL);
	realign3DArray(data,regsize,blks,blkregsize);
	//Unref the python array, done with it. 
	//Py_XDECREF(arr);
	return Py_BuildValue("i",1);
}
//get/set 2D called by Python interpreter:
//Note this is static
//the z coord of extents is ignored
PyObject* PythonControl::get_2Dvariable(PyObject *self, PyObject* args){
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
    printf("in get_2Dvariable, tstep = %d, reflevel = %d extents = %d %d %d %d %d %d\n",
	tstep, reflevel, minreg[0],minreg[1],minreg[2], maxreg[0],maxreg[1],maxreg[2]);
    //Need to convert min,max extents to block extents
    const size_t* blksize = derVarDataMgr->GetBlockSize();
    derVarDataMgr->GetDim(dims,reflevel);
    for (int i = 0; i<2; i++){
		minblkreg[i] = minreg[i]/blksize[i];
		maxblkreg[i] = maxreg[i]/blksize[i];
		blockedRegionSize[i] = (maxblkreg[i]-minblkreg[i]+1)*blksize[i];
		pydims[i] = blockedRegionSize[i];
		int maxPySize = (dims[i]-minreg[i]); 
		if (pydims[i] > maxPySize) pydims[i] = maxPySize;
    }
    
    regData = derVarDataMgr->GetRegion(tstep, varname, reflevel, -1, minblkreg, maxblkreg, 0);
    if (regData) fprintf(stderr,"obtained 2D region data, dims %d %d\n",
                pydims[0],pydims[1]);
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
    
    fprintf(stderr, "converted to pyRegion\n");
    float* val10ptr = (float*) PyArray_GETPTR2(pyRegion,1,0);
    printf(" value in pyarray(1,0): %f\n",*val10ptr);
    val10ptr = (float*) PyArray_GETPTR2(pyRegion,0,1);
    printf(" value in pyarray(0,1): %f\n",*val10ptr);
    return Py_BuildValue("O", pyRegion);
}
PyObject* PythonControl::set_2Dvariable(PyObject *self, PyObject* args){
	
	//Need to get varname, data, timestep, reflevel, extents
	//from the argument list 
	//Ignore 3rd dimension
	const char* varname;
	PyObject* arr;
	int tstep, reflevel;
	int minreg[3],maxreg[3];
	size_t minblkreg[3],maxblkreg[3],blkregsize[3],regsize[3];
	printf(" called set_2Dvariable\n");
	if (!PyArg_ParseTuple(args,"sO!ii(iiiiii)",&varname,&PyArray_Type,&arr,&tstep,&reflevel,
		minreg,minreg+1,minreg+2,maxreg,maxreg+1,maxreg+2)) {
		printf("failed to get all args\n");
		return NULL;
	} 
    printf("in set_2Dvariable, varname = %s, tstep = %d, reflevel = %d extents = %d %d %d %d %d %d\n",
		varname, tstep, reflevel, minreg[0],minreg[1],minreg[2], maxreg[0],maxreg[1],maxreg[2]);
	//Calculate the block extents
	//The minreg should already be based on block dims
	//The maxreg should already be truncated to fit inside domain
	const size_t *blksize = derVarDataMgr->GetBlockSize();
	for(int i = 0; i<2; i++){
		regsize[i] = maxreg[i]-minreg[i]+1;
		minblkreg[i] = minreg[i]/blksize[i];
		maxblkreg[i] = maxreg[i]/blksize[i];
		blkregsize[i] = (maxblkreg[i]-minblkreg[i]+1)*blksize[i];
	}
	minblkreg[2]=0;
	maxblkreg[2]=0;
	printf(" block extents are %d %d %d %d\n",
		minblkreg[0],minblkreg[1],maxblkreg[0],maxblkreg[1]);
	//convert the data to a C-array:
	float* data = (float*)PyArray_DATA(arr);
	//(May also want to check that the Python strides agree with minreg, maxreg) 
	//Allocate a buffer in the cache to put it
	float* blks = (float *) derVarDataMgr->alloc_region(
		tstep,varname,Metadata::VAR2D_XY, reflevel,-1, DataMgr::FLOAT32,minblkreg,maxblkreg,0);
	if (! blks) return(NULL);
	printf(" successful cache allocation\n");
	realign2DArray(data,regsize,blks,blkregsize);
	//Unref the python array, done with it. 
	//Py_XDECREF(arr);
	return Py_BuildValue("i",1);
}
// static method to copy an array into another one with different dimensioning.
// Useful to convert a blocked region to a smaller region that intersects full domain bounds.
// Also useful to copy smaller region back to full domain bounds.  Source and
// destination region share the same (0,0,0) origin, but dest may be larger or smaller.
void PythonControl::realign3DArray(float* srcArray, size_t srcSize[3], float* destArray, size_t destSize[3]){
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

void PythonControl::realign2DArray(float* srcArray, size_t srcSize[2], float* destArray, size_t destSize[2]){
        int xmax = (srcSize[0] < destSize[0]) ? srcSize[0] : destSize[0];
        int ymax = (srcSize[1] < destSize[1]) ? srcSize[1] : destSize[1];
        for (int j = 0; j < ymax; j++){
                for (int i= 0; i< xmax; i++){
                        destArray[i+destSize[0]*j]= srcArray[i+srcSize[0]*j];
                }
        }

}

