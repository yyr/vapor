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
#include <vapor/BlkMemMgr.h>
#include <vapor/Metadata.h>
#include <vapor/DataMgr.h>
#include <vapor/common.h>
#include "datastatus.h"

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
	PythonPipeLine(string name, vector<string> inputs, vector < pair < string, Metadata::VarType_T > > outputs, DataMgr*);
		
	virtual int Calculate (
	   vector <const float *> input_blks,
	   vector <float *> output_blks,	// space for the output variables
	   size_t ts, // current time step
	   int reflevel, // refinement level
	   int lod, //
	   const size_t bs[3], // block dimensions
	   const size_t min[3],	// dimensions of all variables (in blocks)
	   const size_t max[3]
	   );
        
	std::string& python_test_wrapper(const string& script, 
						const vector<string>& inputVars2,
						const vector<string>& inputVars3, 
						vector<pair<string, Metadata::VarType_T> > outputs,
						size_t ts, int reflevel, int compression, const size_t min[3],const size_t max[3]);

	static std::string& getStartupScript() {return startupScript;}
	static void setStartupScript(const std::string& newScript) {
		startupScript = newScript;
		initialized = false;
	}
	
protected:
	int python_wrapper(int scriptId,size_t ts,int reflevel, int compression,
		const size_t min[3],const size_t max[3], 
  		const vector<string > inputs, 
		vector<const float*> inData,
  		vector<pair<string, Metadata::VarType_T> > outputs, 
		vector<float*> outData);
	
	void initialize();
	std::string pythonOutputText;
	
	
	//Initialize the python vapor module:
	//Windows may need PyMODINIT_FUNC?
	static void initvapor(void);
	
	//Methods called by Python interpreter:
	static PyObject* get_3Dvariable(PyObject *self, PyObject* args);
	static PyObject* get_2Dvariable(PyObject *self, PyObject* args);
	static PyObject* mapUserToVox(PyObject *self, PyObject* args);
	static PyObject* mapVoxToUser(PyObject *self, PyObject* args);
	
	static void realign3DArray(const float* srcArray, size_t srcSize[3],float* destArray, size_t destSize[3]);
	static void realign2DArray(const float* srcArray, size_t srcSize[2],float* destArray, size_t destSize[2]);
	
	static string startupScript;
	
	//Define the vapor methods:
	static PyMethodDef vaporMethodDefinitions[];
	
	static DataMgr* currentDataMgr;
	static bool initialized;
       
		
};
	

};

#endif	// PYTHONPIPELINE_H	
