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

	if (_num_reflevels > MAX_LEVELS) {
		SetErrMsg("Too many refinement levels");
		return(-1);
	}

	_xform_timer = 0.0;

	_treeIsOpen = 0;
	_dataIsOpen = 0;

	return(0);
}

AMRIO::AMRIO(
	const Metadata	*metadata,
	unsigned int	nthreads
) : VDFIOBase(metadata, nthreads) {

	SetDiagMsg("AMRIO::AMRIO(, %d)", nthreads);

	_objInitialized = 0;

	if (VDFIOBase::GetErrCode()) return;

	if (_AMRIO() < 0) return;

	_objInitialized = 1;
}

AMRIO::AMRIO(
	const char *metafile,
	unsigned int	nthreads
) : VDFIOBase(metafile, nthreads) {

	SetDiagMsg("AMRIO::AMRIO(%s, %d)", metafile, nthreads);

	_objInitialized = 0;

	if (VDFIOBase::GetErrCode()) return;

	if (_AMRIO() < 0) return;

	_objInitialized = 1;
}

AMRIO::~AMRIO() {

	if (! _objInitialized) return;

	_objInitialized = 0;
}

int    AMRIO::VariableExists(
    size_t timestep,
    const char *varname,
    int reflevel
) {

	SetDiagMsg("AMRIO::VariableExists(%d, %s, %d)", timestep, varname,reflevel);

	string basename;

	// Noop since we store all levels in the same file, for now
	//
    if (reflevel < 0) reflevel = _num_reflevels - 1;

	if (mkpath(timestep, varname, &basename) < 0) return(0);

#ifdef WIN32
	struct _stat statbuf;
#else
	struct stat64 statbuf;
#endif

#ifndef WIN32
	if (stat64(basename.c_str(), &statbuf) < 0) return(0);
#else
	if (_stat(basename.c_str(), &statbuf) < 0) return (0);
#endif
	return(1);
}

int	AMRIO::OpenTreeWrite(
	size_t timestep
) {
	SetDiagMsg("AMRIO::OpenTreeWrite(%d)", timestep);

	string dir;
	string basename;

	_treeWriteMode = 1;

	_treeTimeStep = timestep;

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

	_treeTimeStep = timestep;

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

	TIMER_START(t0)
	int rc = tree->Read(_treeFileName);
	TIMER_STOP(t0,_read_timer)
	return(rc);
}

int	AMRIO::TreeWrite(const AMRTree *tree) {

	SetDiagMsg("AMRIO::TreeWrite()");

	if (! _treeIsOpen || ! _treeWriteMode) {
		SetErrMsg("Tree file not open for writing");
		return(-1);
	}

	TIMER_START(t0)
	int rc = tree->Write(_treeFileName);
	TIMER_STOP(t0,_write_timer)
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

	_dataRange[0] = _dataRange[1] = 0.0;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	_dataWriteMode = 1;

	_dataTimeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	(void) AMRIO::CloseVariable(); // close any previously opened files

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (mkpath(timestep, varname, &basename) < 0) {
			SetErrMsg("Failed to find octree in metadata object at time step %d",
			(int) timestep
		);
		return(-1);
	}

	DirName(basename, dir);
	if (MkDirHier(dir) < 0) return(-1);

	_dataFileName = basename;
	
	_dataIsOpen = 1;

	return(0);
}

int	AMRIO::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel
) {

	SetDiagMsg("AMRIO::OpenVariableRead(%d, %s, %d)",timestep,varname,reflevel);

	string dir;
	string basename;

	_dataRange[0] = _dataRange[1] = 0.0;

    if (reflevel < 0) reflevel = _num_reflevels - 1;

	_dataWriteMode = 0;

	_dataTimeStep = timestep;
	_varName.assign(varname);
	_reflevel = reflevel;

	(void) AMRIO::CloseVariable(); // close any previously opened files

	if (reflevel >= _num_reflevels) {
		SetErrMsg("Requested refinement level out of range (%d)", reflevel);
		return(-1);
	}

	if (mkpath(timestep, varname, &basename) < 0) {
			SetErrMsg("Failed to find octree in metadata object at time step %d",
			(int) timestep
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

	TIMER_START(t0)
	int rc = data->ReadNCDF(_dataFileName,_reflevel);
	TIMER_STOP(t0, _read_timer)

	const float *r = data->GetDataRange();
	_dataRange[0] = r[0];
	_dataRange[1] = r[1];

	return(rc);

}


int	AMRIO::VariableWrite(const AMRData *data) {

	SetDiagMsg("AMRIO::VariableWrite()");

	if (! _dataIsOpen || ! _dataWriteMode) {
		SetErrMsg("AMR data file not open for writing");
		return(-1);
	}

	TIMER_START(t0)
	int rc = data->WriteNCDF(_dataFileName,_reflevel);
	TIMER_STOP(t0, _write_timer)

	const float *r = data->GetDataRange();
	_dataRange[0] = r[0];
	_dataRange[1] = r[1];

	return(rc);
}


int	AMRIO::GetBlockMins(
	const float	**mins,
	int reflevel
) const {
    if (reflevel < 0) reflevel = _num_reflevels - 1;

	if (reflevel >= _num_reflevels) {
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
    if (reflevel < 0) reflevel = _num_reflevels - 1;

	if (reflevel >= _num_reflevels) {
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


int AMRIO::mkpath(size_t timestep, const char *varname, string *path) {

	path->clear();

	const string &bp = _metadata->GetVBasePath(timestep, varname);
	if (_metadata->GetErrCode() != 0 || bp.length() == 0) {
		_metadata->SetErrCode(0);
		return (-1);
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetParentDir() && bp[0] != '/') {
		path->append(_metadata->GetParentDir());
		path->append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (_metadata->GetMetafileName() && bp[0] != '/') {
		string s = _metadata->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		path->append(t);
		path->append("_data");
		path->append("/");

    }
	
	path->append(bp);
	path->append(".nc");

	return(0);
}

int AMRIO::mkpath(size_t timestep, string *path) {

	path->clear();

	const string &bp = _metadata->GetTSAuxBasePath(timestep);
	if (_metadata->GetErrCode() != 0 || bp.length() == 0) {
		_metadata->SetErrCode(0);
		return(-1);
	}

	// Path to octree file is relative to xml file, if it exists
	if (_metadata->GetParentDir() && bp[0] != '/') {
		path->append(_metadata->GetParentDir());
		path->append("/");
	}

	// Path to octree file is relative to xml file, if it exists
	if (_metadata->GetMetafileName() && bp[0] != '/') {
		string s = _metadata->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		path->append(t);
		path->append("_data");
		path->append("/");

    }

	path->append(bp);
	path->append(".amr");
	return(0);
}
