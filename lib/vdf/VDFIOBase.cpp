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
#include "vapor/VDFIOBase.h"
#include "vapor/MyBase.h"

using namespace VetsUtil;
using namespace VAPoR;


int	VDFIOBase::_VDFIOBase(
	const Metadata	*metadata,
	unsigned int	nthreads
) {

	SetClassName("VDFIOBase");

	_nthreads = nthreads;

	_read_timer = 0.0;
	_write_timer = 0.0;
	_seek_timer = 0.0;
	_dataRange[0] = _dataRange[1] = 0.0;

	const size_t *dim = _metadata->GetDimension();
	for(int i=0; i<3; i++) _dim[i] = dim[i];

	const size_t *bs = _metadata->GetBlockSize();
	for(int i=0; i<3; i++) _bs[i] = bs[i];

	_block_size = _bs[0]*_bs[1]*_bs[2];

	_num_reflevels = _metadata->GetNumTransforms() + 1;
	_version = _metadata->GetVDFVersion();

	GetDimBlk(_bdim, _num_reflevels-1);

	return(0);
}

VDFIOBase::VDFIOBase(
	Metadata	*metadata,
	unsigned int	nthreads
) {
	_objInitialized = 0;
	_doFreeMeta = 0;

	_metadata = metadata;
	if (_VDFIOBase(metadata, nthreads) < 0) return;

	_objInitialized = 1;
}

VDFIOBase::VDFIOBase(
	const char *metafile,
	unsigned int	nthreads
) {
	_objInitialized = 0;
	_doFreeMeta = 1;

	_metadata = new Metadata(metafile);
	if (_metadata->GetErrCode() != 0) return;

	if (_VDFIOBase(_metadata, nthreads) < 0) return;
	_objInitialized = 1;
}

VDFIOBase::~VDFIOBase() {

	if (! _objInitialized) return;


	if (_doFreeMeta && _metadata) delete _metadata;
	_objInitialized = 0;
}


void	VDFIOBase::GetDim(
	size_t dim[3], int reflevel
) const {

	if (reflevel < 0) reflevel = _num_reflevels - 1;
	int	 ldelta = _num_reflevels - 1 - reflevel;

	for (int i=0; i<3; i++) {
		dim[i] = _dim[i] >> ldelta;

		// Deal with odd dimensions
		if (_version > 1) {
			if ((dim[i] << ldelta) < _dim[i]) dim[i]++;
		}
	}

}

void	VDFIOBase::GetDimBlk(
	size_t bdim[3], int reflevel
) const {
	size_t dim[3];

	GetDim(dim, reflevel);

	for (int i=0; i<3; i++) {
		bdim[i] = (size_t) ceil ((double) dim[i] / (double) _bs[i]);
	}
}

void	VDFIOBase::MapVoxToBlk(
	const size_t vcoord[3], size_t bcoord[3]
) const {
	for (int i=0; i<3; i++) {
		bcoord[i] = vcoord[i] / _bs[i];
	}
}

void	VDFIOBase::MapVoxToUser(
    size_t timestep, 
	const size_t vcoord0[3], double vcoord1[3],
	int	reflevel
) const {
	string stretched = "stretched";

	if (reflevel < 0) reflevel = _num_reflevels - 1;
	int	 ldelta = _num_reflevels - 1 - reflevel;

	const string &sptr = _metadata->GetGridType();
	if (StrCmpNoCase(sptr, stretched) == 0) {
		//const vector <double> &xcoords = _metadata->GetTSXCoords(timestep);
		//const vector <double> &ycoords = _metadata->GetTSYCoords(timestep);
		//const vector <double> &zcoords = _metadata->GetTSZCoords(timestep);

		cerr << "MapVoxToUser : Function not implemented yet" << endl;
		
	} 
	else {
		size_t	dim[3];

		const vector <double> &extents = _metadata->GetExtents();
		GetDim(dim, _num_reflevels-1);	// finest dimension
		for(int i = 0; i<3; i++) {

			// distance between voxels along dimension 'i' in user coords
			double deltax = (extents[i+3] - extents[i]) / (dim[i] - 1);

			// coordinate of first voxel in user space
			double x0 = extents[i];

			// Boundary shrinks and step size increases with each transform
			for(int j=0; j<(int)ldelta; j++) {
				x0 += 0.5 * deltax;
				deltax *= 2.0;
			}
			vcoord1[i] = x0 + (vcoord0[i] * deltax);
		}
	}
}

void	VDFIOBase::MapUserToVox(
    size_t timestep, const double vcoord0[3], size_t vcoord1[3],
	int	reflevel
) const {
	string stretched = "stretched";

	if (reflevel < 0) reflevel = _num_reflevels - 1;
	int	 ldelta = _num_reflevels - 1 - reflevel;

	const string &sptr = _metadata->GetGridType();
	if (StrCmpNoCase(sptr, stretched) == 0) {
		//const vector <double> &xcoords = _metadata->GetTSXCoords(timestep);
		//const vector <double> &ycoords = _metadata->GetTSYCoords(timestep);
		//const vector <double> &zcoords = _metadata->GetTSZCoords(timestep);
		//assert(_metadata->GetErrCode() == 0);

		cerr << "MapVoxToUser : Function not implemented yet" << endl;
		
	} 
	else {
		size_t	dim[3];
		const vector <double> &extents = _metadata->GetExtents();
		assert(_metadata->GetErrCode() == 0);

		vector <double> lextents = extents;

		GetDim(dim, reflevel);	
		for(int i = 0; i<3; i++) {
			double a, b;

			// distance between voxels along dimension 'i' in user coords
			double deltax = (lextents[i+3] - lextents[i]) / (_dim[i] - 1);

			// coordinate of first voxel in user space
			double x0 = lextents[i];

			// Boundary shrinks and step size increases with each transform
			for(int j=0; j<(int)ldelta; j++) {
				x0 += 0.5 * deltax;
				deltax *= 2.0;
			}
			lextents[i] += x0;
			lextents[i+3] = lextents[i] + (deltax * (dim[i]-1));

			a = (double) (dim[i]-1) / (lextents[i+3] - lextents[i]);
			b = a * extents[i];

			vcoord1[i] = (size_t) rint (a*vcoord0[i] + b);
			if (vcoord1[i] > (dim[i]-1)) vcoord1[i] = dim[i]-1;
		}
	}
}


int	VDFIOBase::IsValidRegion(
	const size_t min[3],
	const size_t max[3],
	int reflevel
) const {
	size_t dim[3];

	GetDim(dim, reflevel);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);

}

int	VDFIOBase::IsValidRegionBlk(
	const size_t min[3],
	const size_t max[3],
	int reflevel
) const {
	size_t dim[3];

	GetDimBlk(dim, reflevel);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);
}
