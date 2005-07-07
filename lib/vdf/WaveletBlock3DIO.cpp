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
#include "vapor/WaveletBlock3DIO.h"
#include "vapor/MyBase.h"

using namespace VetsUtil;
using namespace VAPoR;

int    VDF_API mkdirhier(const string &dir);
void	VDF_API mkpath(const string &basename, int level, string &path);
void	VDF_API dirname(const string &path, string &dir);


int	WaveletBlock3DIO::_WaveletBlock3DIO(
	const Metadata	*metadata,
	unsigned int	nthreads
) {
	int	j;

	SetClassName("WaveletBlock3DIO");

	nthreads_c = nthreads;

	read_timer_c = 0.0;
	write_timer_c = 0.0;
	seek_timer_c = 0.0;
	xform_timer_c = 0.0;

	for(j=0; j<MAX_LEVELS; j++) {
		file_ptrs_c[j] = NULL;
		mins_c[j] = NULL;
		maxs_c[j] = NULL;
		_fileOffsets[j] = 0;
	}
	file_ptrs_c[MAX_LEVELS] = NULL;	// file_ptrs_c is size MAX_LEVELS+1

	super_block_c = NULL;
	block_c = NULL;
	wb3d_c = NULL;

	const size_t *dim = metadata_c->GetDimension();
	for(int i=0; i<3; i++) dim_c[i] = dim[i];

	bs_c = (int)metadata_c->GetBlockSize();

	block_size_c = bs_c * bs_c * bs_c;

	ntilde_c = metadata_c->GetLiftingCoef();
	n_c = metadata_c->GetFilterCoef();
	max_xforms_c = metadata_c->GetNumTransforms();
	_msbFirst = metadata_c->GetMSBFirst();

	// Backwords compatibility for pre version 1 files
	//
	if (metadata_c->GetVDFVersion() < 1) {
		for (j=0; j<max_xforms_c+1; j++) {
			_fileOffsets[j] = get_file_offset(j);
		}
	}

	GetDimBlk(0, bdim_c);

	if (this->my_alloc() < 0) {
		this->my_free();
		return (-1);
	}

	is_open_c = 0;

	return(0);
}

WaveletBlock3DIO::WaveletBlock3DIO(
	Metadata	*metadata,
	unsigned int	nthreads
) {
	_objInitialized = 0;
	_doFreeMeta = 0;

	metadata_c = metadata;
	if (_WaveletBlock3DIO(metadata, nthreads) < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DIO::WaveletBlock3DIO(
	const char *metafile,
	unsigned int	nthreads
) {
	_objInitialized = 0;
	_doFreeMeta = 1;

	metadata_c = new Metadata(metafile);
	if (metadata_c->GetErrCode() != 0) return;

	if (_WaveletBlock3DIO(metadata_c, nthreads) < 0) return;
	_objInitialized = 1;
}

WaveletBlock3DIO::~WaveletBlock3DIO() {

	if (! _objInitialized) return;

#ifdef	TIMER

	fprintf(stderr, "Read timer: %f\n", (float) read_timer_c);
	fprintf(stderr, "Write timer: %f\n", (float) write_timer_c);
	fprintf(stderr, "Seek timer: %f\n", (float) seek_timer_c);
	fprintf(stderr, "Transform timer: %f\n", (float) xform_timer_c);

#endif
	my_free();

	if (_doFreeMeta && metadata_c) delete metadata_c;
	_objInitialized = 0;
}

int    WaveletBlock3DIO::VariableExists(
    size_t timestep,
    const char *varname,
    size_t num_xforms
) {
	string basename;

	const string &bp = metadata_c->GetVBasePath(timestep, varname);
	if (wb3d_c->GetErrCode() != 0 || bp.length() == 0) {
		wb3d_c->SetErrCode(0);
		return (0);
	}

	// Path to variable file is relative to xml file, if it exists
	if (metadata_c->GetParentDir() && bp[0] != '/') {
		basename.append(metadata_c->GetParentDir());
		basename.append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (metadata_c->GetMetafileName() && bp[0] != '/') {
		string s = metadata_c->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		basename.append(t);
		basename.append("_data");
		basename.append("/");

    }
	
	basename.append(bp);
	//AN:  Need to check for existence of *.wbn where n goes from 0 to
	// (max num Transforms) - (requested num transforms)
	// requested num transforms is num_xforms
	// max num transforms is max_xforms_c
	//for(int j=0; j< (int)(num_xforms+1); j++) {
	for (int j = 0; j< (int) (max_xforms_c - num_xforms + 1); j++){
#ifdef WIN32
		struct _stat statbuf;
#else
		struct stat64 statbuf;
#endif
		string path;

		mkpath(basename, j, path);
#ifndef WIN32
		if (lstat64(path.c_str(), &statbuf) < 0) return(0);
#else
		if (_stat(path.c_str(), &statbuf) < 0) return (0);
#endif
	}
	return(1);
}

int	WaveletBlock3DIO::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	size_t num_xforms
) {
	string dir;
	int	min;
	int	j;
	string basename;

	write_mode_c = 1;

	num_xforms_c = 0;

	_timeStep = timestep;
	_varName.assign(varname);

	CloseVariable(); // close any previously opened files

	min = Min((int)bdim_c[0],(int) bdim_c[1]);
	min = Min((int)min, (int)bdim_c[2]);

	if (num_xforms > max_xforms_c) {
		SetErrMsg("Requested transform level out of range (%d)", num_xforms);
		return(-1);
	}

	if (LogBaseN(min, 2.0) < max_xforms_c) {
		SetErrMsg("Too many levels (%d) in transform ", max_xforms_c);
		return(-1);
	}

	const string &bp = metadata_c->GetVBasePath(timestep, varname);
	if (wb3d_c->GetErrCode() != 0 || bp.length() == 0) {
		SetErrMsg(
			"Failed to find variable \"%s\" in metadata object", 
			varname
		);
		return(-1);
	}

	// Path to variable file is relative to xml file, if it exists
	if (metadata_c->GetParentDir() && bp[0] != '/') {
		basename.append(metadata_c->GetParentDir());
		basename.append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (metadata_c->GetMetafileName() && bp[0] != '/') {
		string s = metadata_c->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		basename.append(t);
		basename.append("_data");
		basename.append("/");

    }

	basename.append(bp);

	dirname(basename, dir);
	if (mkdirhier(dir) < 0) return(-1);
	

	for(j=0; j<(max_xforms_c-num_xforms)+1; j++) {
		string path;

		mkpath(basename, j, path);

#ifdef	WIN32
		file_ptrs_c[j] = fopen(path.c_str(), "wb");
#else
		file_ptrs_c[j] = fopen64(path.c_str(), "wb");
#endif
		if (! file_ptrs_c[j]) {
			SetErrMsg("fopen(%s, wb) : %s", path.c_str(), strerror(errno));
			return(-1);
		}

	}
	num_xforms_c = (int) num_xforms;

	GetDim(num_xforms_c, xdim_c);
	GetDimBlk(num_xforms_c, xbdim_c);

	is_open_c = 1;

	return(0);
}

int	WaveletBlock3DIO::OpenVariableRead(
	size_t timestep,
	const char *varname,
	size_t num_xforms
) {
	int	j;
	string basename;

	write_mode_c = 0;

	CloseVariable(); // close previously opened files

	if (num_xforms > max_xforms_c) {
		SetErrMsg("Requested transform level out of range (%d)", num_xforms);
		return(-1);
	}

	if (!VariableExists(timestep, varname, num_xforms)) {
		SetErrMsg(
			"Variable \"%s\" not present at requested timestep or level",
			varname
		);
		return(-1);
	}


	const string &bp = metadata_c->GetVBasePath(timestep, varname);
	if (wb3d_c->GetErrCode() != 0 || bp.length() == 0) {
		SetErrMsg(
			"Failed to find variable \"%s\" in metadata object", 
			varname
		);
		return(-1);
	}

	// Path to variable file is relative to xml file, if it exists
	if (metadata_c->GetParentDir() && bp[0] != '/') {
		basename.append(metadata_c->GetParentDir());
		basename.append("/");
	}

	// Path to variable file is relative to xml file, if it exists
	if (metadata_c->GetMetafileName() && bp[0] != '/') {
		string s = metadata_c->GetMetafileName();
		string t;
		size_t p = s.find_first_of(".");
		if (p != string::npos) {
			t = s.substr(0, p);
		}
		else {
			t = s;
		}
		basename.append(t);
		basename.append("_data");
		basename.append("/");

    }

	basename.append(bp);

	for(j=0; j<(int)((max_xforms_c-num_xforms)+1); j++) {
		string path;

		mkpath(basename, j, path);

#ifdef	WIN32
		file_ptrs_c[j] = fopen(path.c_str(), "rb");
#else
		file_ptrs_c[j] = fopen64(path.c_str(), "rb");
#endif
		if (! file_ptrs_c[j]) {
			if (errno != ENOENT) {
				SetErrMsg("fopen(%s, rb) : %s", path.c_str(), strerror(errno));
				return(-1);
			}
			else {
				break;
			}
		}
		if (_fileOffsets[j]) {	// seek to start of data
			int rc = fseek(file_ptrs_c[j], _fileOffsets[j], SEEK_SET);
			if (rc<0) {
				SetErrMsg("fseek(%d) : %s",_fileOffsets[j],strerror(errno));
				return(-1);
			}
		}
	}
	num_xforms_c = (int)num_xforms;

	GetDim(num_xforms_c, xdim_c);
	GetDimBlk(num_xforms_c, xbdim_c);

	is_open_c = 1;

	return(0);
}

int	WaveletBlock3DIO::CloseVariable()
{
	int	j;

	if (! is_open_c) return(0);

	for(j=0; j<max_xforms_c+1; j++) {
		if (file_ptrs_c[j]) (void) fclose (file_ptrs_c[j]);
		file_ptrs_c[j] = NULL;
	}

	is_open_c = 0;

	return(0);
}


int	WaveletBlock3DIO::GetBlockMins(
	size_t num_xforms,
	const float	**mins
) {
	if ((int)num_xforms > max_xforms_c) {
		SetErrMsg("Invalid transform level : %d", num_xforms);
		return(-1);
	}

	*mins = mins_c[num_xforms];
	return(0);
}

int	WaveletBlock3DIO::GetBlockMaxs(
	size_t num_xforms,
	const float	**maxs
) {
	if ((int)num_xforms > max_xforms_c) {
		SetErrMsg("Invalid transform level : %d", num_xforms);
		return(-1);
	}

	*maxs = maxs_c[num_xforms];
	return(0);
}

void	WaveletBlock3DIO::GetDim(
	size_t num_xforms, size_t dim[3]
) const {
	for (int i=0; i<3; i++) {
		dim[i] = dim_c[i] >> num_xforms;
	}
}

void	WaveletBlock3DIO::GetDimBlk(
	size_t num_xforms, size_t bdim[3]
) const {
	size_t dim[3];

	GetDim(num_xforms, dim);

	for (int i=0; i<3; i++) {
		bdim[i] = (size_t) ceil ((double) dim[i] / (double) bs_c);
	}
}

void	WaveletBlock3DIO::TransformCoord(
	size_t num_xforms, const size_t vcoord0[3], size_t vcoord1[3]
) const {
	vcoord1[0] = vcoord0[0] >> num_xforms;
	vcoord1[1] = vcoord0[1] >> num_xforms;
	vcoord1[2] = vcoord0[2] >> num_xforms;
}

void	WaveletBlock3DIO::MapVoxToBlk(
	const size_t vcoord[3], size_t bcoord[3]
) const {
	bcoord[0] = vcoord[0] / bs_c;
	bcoord[1] = vcoord[1] / bs_c;
	bcoord[2] = vcoord[2] / bs_c;
}

void	WaveletBlock3DIO::MapVoxToUser(
    size_t num_xforms, size_t timestep, 
	const size_t vcoord0[3], double vcoord1[3]
) const {
	string stretched = "stretched";

	const string &sptr = metadata_c->GetGridType();
	if (StrCmpNoCase(sptr, stretched) == 0) {
		//const vector <double> &xcoords = metadata_c->GetTSXCoords(timestep);
		//const vector <double> &ycoords = metadata_c->GetTSYCoords(timestep);
		//const vector <double> &zcoords = metadata_c->GetTSZCoords(timestep);

		cerr << "MapVoxToUser : Function not implemented yet" << endl;
		
	} 
	else {
		size_t	dim[3];

		const vector <double> &extents = metadata_c->GetExtents();
		GetDim(0, dim);
		for(int i = 0; i<3; i++) {

			// distance between voxels along dimension 'i' in user coords
			double deltax = (extents[i+3] - extents[i]) / (dim[i] - 1);

			// coordinate of first voxel in user space
			double x0 = extents[i];

			// Boundary shrinks and step size increases with each transform
			for(int j=0; j<(int)num_xforms; j++) {
				x0 += 0.5 * deltax;
				deltax *= 2.0;
			}
			vcoord1[i] = x0 + (vcoord0[i] * deltax);
		}
	}
}

void	WaveletBlock3DIO::MapUserToVox(
    size_t num_xforms, size_t timestep,
	const double vcoord0[3], size_t vcoord1[3]
) const {
	string stretched = "stretched";

	const string &sptr = metadata_c->GetGridType();
	if (StrCmpNoCase(sptr, stretched) == 0) {
		//const vector <double> &xcoords = metadata_c->GetTSXCoords(timestep);
		//const vector <double> &ycoords = metadata_c->GetTSYCoords(timestep);
		//const vector <double> &zcoords = metadata_c->GetTSZCoords(timestep);
		//assert(metadata_c->GetErrCode() == 0);

		cerr << "MapVoxToUser : Function not implemented yet" << endl;
		
	} 
	else {
		size_t	dim[3];
		const vector <double> &extents = metadata_c->GetExtents();
		assert(metadata_c->GetErrCode() == 0);

		vector <double> lextents = extents;

		GetDim(0, dim);
		for(int i = 0; i<3; i++) {

			// distance between voxels along dimension 'i' in user coords
			double deltax = (lextents[i+3] - lextents[i]) / (dim[i] - 1);

			// coordinate of first voxel in user space
			double x0 = lextents[i];

			// Boundary shrinks and step size increases with each transform
			for(int j=0; j<(int)num_xforms; j++) {
				x0 += 0.5 * deltax;
				deltax *= 2.0;
			}
			lextents[i] += x0;
			lextents[i+3] -= x0;

			vcoord1[i] = (size_t) rint (
				(vcoord0[i]-lextents[i])/(lextents[i+3]-lextents[i])*(dim[i]-1)
			);
		}
	}
}


int	WaveletBlock3DIO::IsValidRegion(
	size_t num_xforms,
	const size_t min[3],
	const size_t max[3]
) const {
	size_t dim[3];

	GetDim(num_xforms, dim);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);

}

int	WaveletBlock3DIO::IsValidRegionBlk(
	size_t num_xforms,
	const size_t min[3],
	const size_t max[3]
) const {
	size_t dim[3];

	GetDimBlk(num_xforms, dim);

	for(int i=0; i<3; i++) {
		if (min[i] > max[i]) return (0);
		if (max[i] >= dim[i]) return (0);
	}
	return(1);
}



int	WaveletBlock3DIO::seekBlocks(
	unsigned int offset, 
	size_t num_xforms
) {
	int	rc;
	FILE	*fp;
	size_t level = max_xforms_c - num_xforms;

	long long byteoffset = offset * block_size_c * sizeof(float);
	byteoffset += _fileOffsets[level];

	//cerr << level << " seeking " << offset << " blocks" << endl;

	if (! is_open_c) {
		SetErrMsg("File not open");
		return(-1);
	}

	if ((int)num_xforms > max_xforms_c) {
		SetErrMsg("Invalid transform level : %d", num_xforms);
		return(-1);
	}

	fp = file_ptrs_c[level];

	TIMER_START(t0);
#ifdef	WIN32
	rc = fseek(fp, (long) byteoffset, SEEK_SET);
#else
#if     defined(Linux) || defined(AIX)
	rc = fseeko64(fp, byteoffset, SEEK_SET);
#else
	rc = fseek64(fp, byteoffset, SEEK_SET);
#endif
#endif
	if (rc<0) {
		SetErrMsg("fseek(%lld) : %s",(long long) byteoffset,strerror(errno));
		return(-1);
	}
	TIMER_STOP(t0, seek_timer_c);
	return(0);
}

int	WaveletBlock3DIO::seekLambdaBlocks(
	const size_t bcoord[3]
) {

	unsigned int	offset;
	size_t bdim[3];

	GetDimBlk(max_xforms_c, bdim);

	for(int i=0; i<3; i++) {
		if (bcoord[i] >= bdim[i]) {
			SetErrMsg(
				"Invalid block address : %dx%dx%d", 
				bcoord[0], bcoord[1], bcoord[2]
			);
			return(-1);
		}
	}

	offset = (int)(bcoord[2] * bdim[0] * bdim[1] + (bcoord[1]*bdim[0]) + (bcoord[0]));

	return(seekBlocks(offset, max_xforms_c));
}

int	WaveletBlock3DIO::seekGammaBlocks(
	size_t num_xforms,
	const size_t bcoord[3]
) {
	size_t bdim[3];

	unsigned int	offset;

	// there are num_xforms_c-1 levels of gamma blocks
	//
	if ((int)num_xforms >= max_xforms_c) {
		SetErrMsg("Invalid transform number : %d\n", num_xforms);
		return(-1);
	}

	GetDimBlk(num_xforms+1, bdim);

	for(int i=0; i<3; i++) {
		if (bcoord[i] >= bdim[i]) {
			SetErrMsg(
				"Invalid block address : %dx%dx%d", 
				bcoord[0], bcoord[1], bcoord[2]
			);
			return(-1);
		}
	}

	offset = (unsigned int)((bcoord[2] * bdim[0] * bdim[1]) + (bcoord[1]*bdim[0]) + (bcoord[0])) *  7;

	return(seekBlocks(offset, num_xforms));
}

int	WaveletBlock3DIO::readBlocks(
	size_t n,
	size_t num_xforms,
	float *blks
) {
	int	rc;
	FILE	*fp;
	unsigned long    lsbFirstTest = 1;
	size_t level = max_xforms_c - num_xforms;

	//cerr << level << " reading " << n << " blocks" << endl;

	if (! is_open_c) {
		SetErrMsg("File not open");
		return(-1);
	}

	if (write_mode_c) {
		SetErrMsg("WaveletBlock3DIO : object created for reading");
		return(-1);
	}

	if ((int)num_xforms > max_xforms_c) {
		SetErrMsg("Invalid transform number : %d\n", num_xforms);
		return(-1);
	}

	fp = file_ptrs_c[level];

	TIMER_START(t0)
	rc = (int)fread(blks, sizeof(blks[0]), (block_size_c*n), fp);
	if (rc != block_size_c*n) {
		SetErrMsg("fread(%d) : %s",block_size_c*n,strerror(errno));
		return(-1);
	}

	// Deal with endianess
	//
	if ((*(char *) &lsbFirstTest && _msbFirst) || 
		(! *(char *) &lsbFirstTest && ! _msbFirst)) {

		swapbytescopy(blks, blks, sizeof(blks[0]), block_size_c*n);
	}
	TIMER_STOP(t0, read_timer_c)

	return(0);
}

int	WaveletBlock3DIO::readLambdaBlocks(
	size_t n,
	float *blks
) {
	//cerr << "Read lambda ";
	return(readBlocks(n, max_xforms_c, blks));
}

int	WaveletBlock3DIO::readGammaBlocks(
	size_t n, 
	size_t num_xforms,
	float *blks
) {

	//cerr << "Read gamma ";
	if ((int)num_xforms >= max_xforms_c) {
		SetErrMsg("No gamma blocks");
		return(-1);
	}
	return(readBlocks(n*7, num_xforms, blks));
}


int	WaveletBlock3DIO::writeBlocks(
	const float *blks, 
	size_t n,
	size_t num_xforms
) {
	int	rc;
	FILE	*fp;
	unsigned long lsbFirstTest = 1;
	size_t level = max_xforms_c - num_xforms;

	//cerr << level << " writing " << n << " blocks" << endl;

	if (! is_open_c) {
		SetErrMsg("File not open");
		return(-1);
	}

	if (! write_mode_c) {
		SetErrMsg("WaveletBlock3DIO : object created for reading");
		return(-1);
	}

	if ((int)num_xforms > max_xforms_c) {
		SetErrMsg("Invalid transform number : %d\n", num_xforms);
		return(-1);
	}

	fp = file_ptrs_c[level];

	TIMER_START(t0)

	// Deal with endianess
	//
	if ((*(char *) &lsbFirstTest && _msbFirst) || 
		(! *(char *) &lsbFirstTest && ! _msbFirst)) {
		int	i;

		for(i=0;i<(int)n;i++) {
			swapbytescopy(
				&blks[i*block_size_c], block_c, sizeof(blks[0]), block_size_c
			);
			rc = (int)fwrite(block_c, sizeof(block_c[0]), block_size_c, fp);
			if (rc != block_size_c) {
				SetErrMsg("fwrite(%d) : %s",block_size_c*n,strerror(errno));
				return(-1);
			}
		}
	}
	else {
		rc = (int)fwrite(blks, sizeof(blks[0]), block_size_c*n, fp);
		if (rc != block_size_c*n) {
			SetErrMsg("fwrite(%d) : %s",block_size_c*n,strerror(errno));
			return(-1);
		}
	}
	TIMER_STOP(t0, write_timer_c)

	return(0);
}

int	WaveletBlock3DIO::writeLambdaBlocks(
	const float *blks, 
	size_t n
) {
	//cerr << "Write lambda ";
	return(writeBlocks(blks, n, max_xforms_c));
}

int	WaveletBlock3DIO::writeGammaBlocks(
	const float *blks, 
	size_t n, 
	size_t num_xforms
) {
	//cerr << "Write gamma ";
	if ((int)num_xforms >= max_xforms_c) {
		SetErrMsg("No gamma blocks");
		return(-1);
	}
	return(writeBlocks(blks, n*7, num_xforms));
}

void	WaveletBlock3DIO::Block2NonBlock(
	const float *blk, 
	const size_t bcoord[3],
	const size_t min[3],
	const size_t max[3],
	float	*voxels
) const {

	SetDiagMsg(
		"WaveletBlock3DIO::Block2NonBlock(,[%d,%d,%d],[%d,%d,%d],[%d,%d,%d])",
		bcoord[0], bcoord[1], bcoord[2],
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	size_t y,z;

	assert((min[0] < bs_c) && (max[0] > min[0]) && (max[0]/bs_c >= (bcoord[0])));
	assert((min[1] < bs_c) && (max[1] > min[1]) && (max[1]/bs_c >= (bcoord[1])));
	assert((min[2] < bs_c) && (max[2] > min[2]) && (max[2]/bs_c >= (bcoord[2])));

	size_t dim[] = {max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1};
	size_t xsize = bs_c;
	size_t ysize = bs_c;
	size_t zsize = bs_c;

	//
	// Address of first destination voxel
	//
	voxels += (bcoord[2]*bs_c * dim[1] * dim[0]) + 
		(bcoord[1]*bs_c*dim[0]) + 
		(bcoord[0]*bs_c);

	if (bcoord[0] == 0) {
		blk += min[0];
		xsize = bs_c - min[0];
	}
	else {
		if (bcoord[0] == max[0]/bs_c) xsize = max[0] % bs_c + 1;
		voxels -= min[0];
	}

	if (bcoord[1] == 0) {
		blk += min[1] * bs_c;
		ysize = bs_c - min[1];
	}
	else {
		if (bcoord[1] == max[1]/bs_c) ysize = max[1] % bs_c + 1;
		voxels -= dim[0] * min[1];
	}

	if (bcoord[2] == 0) {
		blk += min[2] * bs_c * bs_c;
		zsize = bs_c - min[2];
	}
	else {
		if (bcoord[2] == max[2]/bs_c) zsize = max[2] % bs_c + 1;
		voxels -= dim[1] * dim[0] * min[2];
	}

	float *voxptr;
	const float *blkptr;

	for(z=0; z<zsize; z++) {
		voxptr = voxels + (dim[0]*dim[1]*z); 
		blkptr = blk + (bs_c*bs_c*z);

		for(y=0; y<ysize; y++) {

			memcpy(voxptr,blkptr,xsize*sizeof(blkptr[0]));
			blkptr += bs_c;
			voxptr += dim[0];
		}
	}
}




int	WaveletBlock3DIO::my_alloc(
) {

	int     size;
	unsigned long	lsbFirstTest = 1;
	int	j;


	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<=max_xforms_c; j++) {
		size_t nb_j[3];

		GetDimBlk(max_xforms_c - j, nb_j);

		size = (int)(nb_j[0] * nb_j[1] * nb_j[2]);

		mins_c[j] = new float[size];
		maxs_c[j] = new float[size];
		if (! mins_c[j] || ! maxs_c[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}
	}

	size = block_size_c * 8;
	super_block_c = new float[size];
	if (! super_block_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	// Deal with endianess
	//
	if ((*(char *) &lsbFirstTest && _msbFirst) || 
		(! *(char *) &lsbFirstTest && ! _msbFirst)) {

		block_c = new float[block_size_c];
		if (! block_c) {
			SetErrMsg("new float[%d] : %s", block_size_c, strerror(errno));
			return(-1);
		}
	}

	wb3d_c = new WaveletBlock3D(bs_c, n_c, ntilde_c, nthreads_c);
	if (wb3d_c->GetErrCode() != 0) {
		SetErrMsg(
			"WaveletBlock3D(%d,%d,%d,%d) : %s",
			bs_c, n_c, ntilde_c, nthreads_c, wb3d_c->GetErrMsg()
		);
		return(-1);
	}
	return(0);
}

void	WaveletBlock3DIO::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (mins_c[j]) delete [] mins_c[j];
		if (maxs_c[j]) delete [] maxs_c[j];
		mins_c[j] = NULL;
		maxs_c[j] = NULL;
	}
	if (super_block_c) delete [] super_block_c;
	if (block_c) delete [] block_c;
	if (wb3d_c) delete wb3d_c;

	super_block_c = NULL;
	block_c = NULL;
	wb3d_c = NULL;
}



void    WaveletBlock3DIO::swapbytes(
        void    *vptr,
        int     n
) const {
        unsigned char   *ucptr = (unsigned char *) vptr;
        unsigned char   uc;
        int             i;

        for (i=0; i<n/2; i++) {
                uc = ucptr[i];
                ucptr[i] = ucptr[n-i-1];
                ucptr[n-i-1] = uc;
        }
}

void    WaveletBlock3DIO::swapbytescopy(
        const void    *src,
        void    *dst,
        int	size,
		int	n
) const {
	const unsigned char   *ucsrc = (unsigned char *) src;
	unsigned char   *ucdst = (unsigned char *) dst;
	unsigned char   uc;
	int             i,j;

	for(j=0;j<n;j++) {
		for (i=0; i<size/2; i++) {
			uc = ucsrc[i];
			ucdst[i] = ucsrc[size-i-1];
			ucdst[size-i-1] = uc;
		}
		ucsrc += size;
		ucdst += size;
	}
}

int	WaveletBlock3DIO::get_file_offset(
	size_t level
) {
	int	fixed_part;
	int	var_part;
	size_t bdim[3];
	const int BLOCK_FACTOR = 8192;
	const int STATIC_HEADER_SIZE = 512;

	int	minsize;

	fixed_part = STATIC_HEADER_SIZE;

	GetDimBlk(max_xforms_c-level, bdim);

	minsize = bdim[0] * bdim[1] * bdim[2] * sizeof(float) * 2;

	for(var_part = -fixed_part; var_part<(minsize + fixed_part); var_part += BLOCK_FACTOR);

	return(fixed_part + var_part);
}

int    mkdirhier(const string &dir) {

    stack <string> dirs;

    string::size_type idx;
    string s = dir;

	dirs.push(s);
    while ((idx = s.find_last_of("/")) != string::npos) {
        s = s.substr(0, idx);
		if (! s.empty()) dirs.push(s);
    }

    while (! dirs.empty()) {
        s = dirs.top();
		dirs.pop();
#ifndef WIN32
        if ((mkdir(s.c_str(), 0777) < 0) && errno != EEXIST) {
			MyBase::SetErrMsg("mkdir(%s) : %M", s.c_str());
            return(-1);
        }
#else 
		//Windows version of mkdir:
		//If it succeeds, return value is nonzero
		if (!CreateDirectory(( LPCSTR)s.c_str(), 0)){
			DWORD dw = GetLastError();
			if (dw != 183){ //183 means file already exists
				MyBase::SetErrMsg("mkdir(%s) : %M", s.c_str());
				return(-1);
			}
		}
#endif
    }
    return(0);
}

void    mkpath(const string &basename, int level, string &path) {
	ostringstream oss;

	oss << basename << ".wb" << level;
	path = oss.str();
}

void    dirname(const string &path, string &dir) {
	
	string::size_type idx = path.find_last_of('/');
	if (idx == string::npos) {
		dir.assign("./");
	}
	else {
		dir = path.substr(0, idx+1);
	}
}


