#ifndef VDFCREATE_H
#define VDFCREATE_H

#include <iostream>
#include <cstdio>
#include <vapor/OptionParser.h>
#include <vapor/MetadataVDC.h>
#include <vapor/DCReader.h>
#include <vapor/VDCFactory.h>

using namespace VetsUtil;
using namespace VAPoR;

//namespace VAPoR{
void vdfcreateUsage(OptionParser &op, const char * msg);
void populateVariables(std::vector<std::string> &vars, 
			std::vector<std::string> candidate_vars,
			MetadataVDC *file, 
			int (MetadataVDC::*SetVarFunction)(const std::vector<std::string>&));

void writeToScreen(DCReader *DCdata, MetadataVDC *file);
MetadataVDC *CreateMetadataVDC(const VDCFactory &vdcf,
				const DCReader *DCdata);

char ** argv_merge(int argc1, char **argv1, int argc2, char **argv2, int &newargc);
int VDF_API launchVdfCreate(int argc, char **argv, string NetCDFtype);
//};

#endif
