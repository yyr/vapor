//
//      $Id$
//

#ifndef	PYTHONCONTROL_H
#define	PYTHONCONTROL_H


#include <Python.h>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <vapor/MyBase.h>
#include "vapor/BlkMemMgr.h"
#include "vapor/Metadata.h"
#include "vaporinternal/common.h"

namespace VAPoR {

class DataMgr;

//
//! \class PythonControl
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
class VDF_API PythonControl {

public:

 //! Constructor for the PythonControl class. 
 //!
 //
 PythonControl(DataMgr*
 );

 virtual ~PythonControl(); 


std::string pythonOutputText;
std::string& python_test_wrapper(const string& script, const vector<string>& inputVars2, const vector<string>& outputVars2, 
						const vector<string>& inputVars3, const vector<string>& outputVars3, 
						size_t ts,int reflevel,const size_t min[3],const size_t max[3]);


int python_wrapper(int scriptId,size_t ts,int reflevel,const size_t min[3],const size_t max[3]);
protected:
 const vector<string> emptyVec;

private:

 static DataMgr* derVarDataMgr;
 //Methods called by Python interpreter:
 static PyObject* get_3Dvariable(PyObject *self, PyObject* args);
 static PyObject* set_3Dvariable(PyObject *self, PyObject* args);
 static PyObject* get_2Dvariable(PyObject *self, PyObject* args);
 static PyObject* set_2Dvariable(PyObject *self, PyObject* args);
 static void realign3DArray(float* srcArray, size_t srcSize[3],float* destArray, size_t destSize[3]);
 static void realign2DArray(float* srcArray, size_t srcSize[2],float* destArray, size_t destSize[2]);
 
//Initialize the python vapor module:
//Windows needs PyMODINIT_FUNC here?
static void initvapor(void);
 //Define the vapor methods:
 static PyMethodDef vaporMethodDefinitions[];

};

};

#endif	// PYTHONCONTROL_H	
