//
//      $Id$
//

#ifndef	PYTHONPIPELINE_H
#define	PYTHONPIPELINE_H


#include <Python.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <vapor/MyBase.h>
#include <vapor/RegularGrid.h>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include "datastatus.h"
class QMutex;

namespace VAPoR {

class DataMgr;

//
//! \class PythonPipeline
//! \brief Methods to enable derived variables from Python scripts 
//! \author Alan Norton 
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides several methods that can be
//! used to derive variables using Python scripts 
//! The Python scripts are invoked by the DataMgr
//! creating derived variables in the cache as needed.
//



class PARAMS_API PythonPipeLine : public PipeLine {
public:
	PythonPipeLine(string name, vector<string> inputs, vector < pair < string, DataMgr::VarType_T > > outputs, DataMgr*);

	static void terminate(){ if (everInitialized) Py_Finalize();}
		
	virtual int Calculate (
	   vector <const RegularGrid *> input_blks,
	   vector <RegularGrid *> output_blks,	// space for the output variables
	   size_t ts, // current time step
	   int reflevel, // refinement level
	   int lod
	  );

	std::string& python_test_wrapper(const string& script, 
						const vector<string>& inputVars2,
						const vector<string>& inputVars3, 
						vector<pair<string, DataMgr::VarType_T> > outputs,
						size_t ts, int reflevel, int compression,
						size_t rmin[3], size_t rmax[3]);

	static std::string& getStartupScript() {return startupScript;}
	static void setStartupScript(const std::string& newScript) {
		startupScript = newScript;
		initialized = false;
	}
	static void setEverInitialized() {everInitialized = true;}
protected:
	int python_wrapper(int scriptId,size_t ts,int reflevel, int compression, 
  		const vector<string > inputs, 
		vector <const RegularGrid *> inData,
  		vector<pair<string, DataMgr::VarType_T> > outputs, 
		vector <RegularGrid *> outData);
	static PyObject* vaporModule;
	void initialize();
	std::string pythonOutputText;
	static bool needCheckArrays() {return (arrayAllocMap.size()>0);}
	static void tryDeleteArrayStorage(PyObject*);
	static void mapArrayObject(PyObject*, float*);
	static void mapMaskObject(PyObject*, bool*);
	bool usingMissingValues(size_t ts,const std::vector<string>& inputVars2,const std::vector<string>& inputVars3,size_t* rmin,size_t* rmax);
	
	
	//Initialize the python vapor module:
	//Windows may need PyMODINIT_FUNC?
	static void initvapor(void);
	
	//Methods called by Python interpreter:
	static PyObject* get_3Dvariable(PyObject *self, PyObject* args);
	static PyObject* get_2Dvariable(PyObject *self, PyObject* args);
	static PyObject* get_3Dmask(PyObject *self, PyObject* args);
	static PyObject* get_2Dmask(PyObject *self, PyObject* args);
	static PyObject* mapUserToVox(PyObject *self, PyObject* args);
	static PyObject* mapVoxToUser(PyObject *self, PyObject* args);
	static PyObject* getFullVDCDims(PyObject *self, PyObject* args);
	static PyObject* getValidRegionMin(PyObject *self, PyObject* args);
	static PyObject* getValidRegionMax(PyObject *self, PyObject* args);
	static PyObject* variableExists(PyObject *self, PyObject* args);
	
	static void copyTo3DGrid(const float* srcArray, bool *srcMask, RegularGrid* destGrid);
	static void copyFrom3DGrid(const RegularGrid* srcGrid, float* destArray, bool* destMask = 0);
	static void copyTo2DGrid(const float* srcArray, bool* srcMask, RegularGrid* destGrid);
	static void copyFrom2DGrid(const RegularGrid* srcGrid, float* destArray, bool* destMask = 0);
	
	static string startupScript;
	
	//Define the vapor methods:
	static PyMethodDef vaporMethodDefinitions[];
	
	static DataMgr* currentDataMgr;
	static bool initialized;
	static bool everInitialized;
	
	static std::map<PyObject*, float*> arrayAllocMap;
	static std::map<PyObject*, bool*> maskAllocMap;
	static QMutex* arrayAllocMutex;
       
		
};
	

};

#endif	// PYTHONPIPELINE_H	
