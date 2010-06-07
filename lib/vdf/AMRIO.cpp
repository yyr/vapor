//
//      $Id$
//
//***********************************************************************
//                                                                      *
//                      Copyright (C)  2006	                        *
//          University Corporation for Atmospheric Research             *
//                      All Rights Reserved                             *
//                                                                      *
//***********************************************************************
//
//	File:		
//
//	Author:		John Clyne
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		Thu Jan 5 17:00:37 MST 2006
//
//	Description:	
//
//
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <sstream>
#include <cassert>
#ifndef WIN32
#include <unistd.h>
#else
#include "windows.h"
#include "vaporinternal/common.h"
#endif
#include <sys/types.h>
#include <sys/stat.h>
#include "vapor/AMRIO.h"
#include "vapor/MyBase.h"

using namespace VetsUtil;
using namespace VAPoR;


int	AMRIO::_AMRIO(
) {
	SetClassName("AMRIO");

	if (GetNumTransforms() >= MAX_LEVELS) {
		SetErrMsg("Too many refinement levels");
		return(-1);
	}

	_treeIsOpen = 0;
	_dataIsOpen = 0;

	return(0);
}

AMRIO::AMRIO(
	const MetadataVDC	&metadata
) : VDFIOBase(metadata) {

	SetDiagMsg("AMRIO::AMRIO()");

	if (VDFIOBase::GetErrCode()) return;

	if (_AMRIO() < 0) return;

}

AMRIO::AMRIO(
	const string &metafile
) : VDFIOBase(metafile) {

	SetDiagMsg("AMRIO::AMRIO(%s)", metafile.c_str());

	if (VDFIOBase::GetErrCode()) return;

	if (_AMRIO() < 0) return;

}

AMRIO::~AMRIO() {


}

int    AMRIO::VariableExists(
    size_t timestep,
    const char *varname,
    int reflevel,
	int
) const {

	SetDiagMsg("AMRIO::VariableExists(%d, %s, %d)", timestep, varname,reflevel);

	string basename;

	// Noop since we store all levels in the same file, for now
	//
    if (reflevel < 0) reflevel = GetNumTransforms();

	struct STAT64 statbuf;

	// 
	// First check for file containing field data
	//
	if (mkpath(timestep, varname, &basename) < 0) return(0);
	if (STAT64(basename.c_str(), &statbuf) < 0) return(0);

	// 
	// Next check for file containing the AMR tree
	//
	if (mkpath(timestep, &basename) < 0) return(0);
	if (STAT64(basename.c_str(), &statbuf) < 0) return(0);

	return(1);
}

int	AMRIO::OpenTreeWrite(
	size_t timestep
) {
	SetDiagMsg("AMRIO::OpenTreeWrite(%d)", timestep);

	string dir;
	string basename;

	_treeWriteMode = 1;

	(void) AMRIO::CloseTree(); // close any previously opened files

	if (mkpath(timestep, &basename) < 0) {
			SetErrMsg("Failed to find octree in metadata object at time step %d",
			(int) timestep
		);
		return(-1);
	}

	DirName(basename, dir);
	if (MkDirHier(dir) < 0) return(-1);

	_treeFileName = basename;
	
	_treeIsOpen = 1;

	return(0);
}

int	AMRIO::OpenTreeRead(
	size_t timestep
) {
	SetDiagMsg("AMRIO::OpenTreeWrite(%d)", timestep);

	string dir;
	string basename;

	_treeWriteMode = 0;

	(void) AMRIO::CloseTree(); // close any previously opened files

	if (mkpath(timestep, &basename) < 0) {
			SetErrMsg("Failed to find octree in metadata object at time step %d",
			(int) timestep
		);
		return(-1);
	}

	DirName(basename, dir);

	_treeFileName = basename;
	
	_treeIsOpen = 1;

	return(0);
}


int	AMRIO::CloseTree() {

	SetDiagMsg("AMRIO::CloseTree()");

	_treeIsOpen = 0;
	return(0);
}


int	AMRIO::TreeRead(AMRTree *tree) {

	SetDiagMsg("AMRIO::TreeRead()");

	if (! _treeIsOpen || _treeWriteMode) {
		SetErrMsg("Tree file not open for reading");
		return(-1);
	}

	_ReadTimerStart();
	int rc = tree->Read(_treeFileName);
	_ReadTimerStop();
	return(rc);
}

int	AMRIO::TreeWrite(const AMRTree *tree) {

	SetDiagMsg("AMRIO::TreeWrite()");

	if (! _treeIsOpen || ! _treeWriteMode) {
		SetErrMsg("Tree file not open for writing");
		return(-1);
	}

	_WriteTimerStart();
	int rc = tree->Write(_treeFileName);
	_WriteTimerStop();
	return(rc);

}

int	AMRIO::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel
) {

	SetDiagMsg("AMRIO::OpenVariableWrite(%d, %s, %d)",timestep,varname,reflevel);

	string dir;
	string basename;

    if (reflevel < 0) reflevel = GetNumTransforms();

	_dataWriteMode = 1;

	_varName.assign(varname);
	_reflevel = reflevel;

	(void) AMRIO::CloseVariable(); // close any previously opened files

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (mkpath(timestep, varname, &basename) < 0) {
		SetErrMsg(
			"Failed to find variable \"%s\" at time step %d", 
			varname, (int) timestep
		);
		return(-1);
	}

	DirName(basename, dir);
	if (MkDirHier(dir) < 0) return(-1);

	_dataFileName = basename;

	size_t num_nodes, cell_dim[3];
	int dummy;
	int rc = AMRData::ReadAttributesNCDF(
		_dataFileName, cell_dim, _validRegMin, _validRegMax, _dataRange, 
		dummy, num_nodes
	);
	if (rc<0) {
		SetErrMsg(
			"Failed to stat variable \"%s\" at time step %d", 
			varname, (int) timestep
		);
		return(-1);
	}
	
	
	_dataIsOpen = 1;

	return(0);
}

int	AMRIO::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel,
	int
) {

	SetDiagMsg("AMRIO::OpenVariableRead(%d, %s, %d)",timestep,varname,reflevel);

	string dir;
	string basename;


    if (reflevel < 0) reflevel = GetNumTransforms();

	_dataWriteMode = 0;

	_varName.assign(varname);
	_reflevel = reflevel;

	(void) AMRIO::CloseVariable(); // close any previously opened files

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (mkpath(timestep, varname, &basename) < 0) {
		SetErrMsg(
			"Failed to find variable \"%s\" at time step %d", 
			varname, (int) timestep
		);
		return(-1);
	}

	DirName(basename, dir);

	_dataFileName = basename;

	_dataIsOpen = 1;

	return(0);
}

int	AMRIO::CloseVariable()
{
	SetDiagMsg("AMRIO::CloseVariable()");

	if (! _dataIsOpen) return(0);

	_dataIsOpen = 0;

	return(0);
}

int	AMRIO::VariableRead(AMRData *data) {

	SetDiagMsg("AMRIO::VariableRead()");

	if (! _dataIsOpen || _dataWriteMode) {
		SetErrMsg("AMR data file not open for reading");
		return(-1);
	}

	_ReadTimerStart();
	int rc = data->ReadNCDF(_dataFileName,_reflevel);
	_ReadTimerStop();

	return(rc);

}


int	AMRIO::VariableWrite(AMRData *data) {

	SetDiagMsg("AMRIO::VariableWrite()");

	if (! _dataIsOpen || ! _dataWriteMode) {
		SetErrMsg("AMR data file not open for writing");
		return(-1);
	}

	_WriteTimerStart();
	int rc = data->WriteNCDF(_dataFileName,_reflevel);
	_WriteTimerStop();

	const float *fptr = data->GetDataRange();
	_dataRange[0] = fptr[0];
	_dataRange[1] = fptr[1];

	data->GetBounds(_validRegMin, _validRegMax);

	return(rc);
}


int	AMRIO::GetBlockMins(
	const float	**mins,
	int reflevel
) const {
    if (reflevel < 0) reflevel = GetNumTransforms();

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}
#ifdef DEAD
	*mins = _mins[reflevel];
	return(0);
#else
	*mins = NULL;
	return(-1);
#endif
}

int	AMRIO::GetBlockMaxs(
	const float	**maxs,
	int reflevel
) const {
    if (reflevel < 0) reflevel = GetNumTransforms();

	if (reflevel > GetNumTransforms()) {
		SetErrMsg("Invalid refinement level : %d", reflevel);
		return(-1);
	}

#ifdef DEAD
	*maxs = _maxs[reflevel];
	return(0);
#else
	*maxs = NULL;
	return(-1);
#endif
}

void    AMRIO::GetValidRegion(
	size_t minreg[3], size_t maxreg[3], int reflevel
) const {


	if (reflevel < 0) reflevel = GetNumTransforms();

	int  ldelta = GetNumTransforms() - reflevel;

	for (int i=0; i<3; i++) {
		minreg[i] = _validRegMin[i] >> ldelta;
		maxreg[i] = _validRegMax[i] >> ldelta;
	}
}


int AMRIO::mkpath(size_t timestep, const char *varname, string *path) const {

	path->clear();

	if (ConstructFullVBase(timestep, varname, path) < 0) {
		SetErrCode(0);
		return (-1);
	}

	path->append(".nc");

	return(0);
}

int AMRIO::mkpath(size_t timestep, string *path) const {

	path->clear();

	const string &bp = GetTSAuxBasePath(timestep);
	if (GetErrCode() != 0 || bp.length() == 0) {
		SetErrCode(0);
		return(-1);
	}
	if (ConstructFullAuxBase(timestep, path) < 0) {
		SetErrCode(0);
		return (-1);
	}

	path->append(".amr");
	return(0);
}
