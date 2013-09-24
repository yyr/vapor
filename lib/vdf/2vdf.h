#ifndef TOVDF_H
#define TOVDF_H

#include <iostream>
#include <cstdio>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReader.h>
#include <vapor/WaveCodecIO.h>
#include <vapor/WaveletBlock3DBufWriter.h>
#include <vapor/CFuncs.h>
 
//using namespace VetsUtil;
using namespace VAPoR;

//namespace VAPoR{
void TovdfUsage(OptionParser &op, const char * msg);
void GetTimeMap(const VDFIOBase *vdfio,
          	const DCReader *DCData,
          	int startts,
          	int numts,
          	map <size_t, size_t> &timemap);
void VDF_API GetVariables(const VDFIOBase *vdfio,
	          const DCReader *DCData,
        	  const vector <string> &in_varnames,
        	  vector <string> &out_varnames);
void MissingValue(VDFIOBase *vdfio,
	         DCReader *DCData,
        	 size_t vdcTS,
       		 string vdcVar,
         	 string ncdfVar,
            	 VDFIOBase::VarType_T vtype,
         	 int slice,
         	 float *buf);
int CopyVar(VDFIOBase *vdfio,
         DCReader *DCData,
         int vdcTS,
         int ncdfTS,
         string vdcVar,
         string ncdfVar,
         int level,
         int lod);
int VDF_API launch2vdf(int argc, char **argv, string dataType);

//};

#endif
