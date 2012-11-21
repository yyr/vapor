#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include <vapor/WaveletBlock3DBufReader.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif

using namespace VAPoR;

void	WaveletBlock3DBufReader::_WaveletBlock3DBufReader() 
{
	SetClassName("WaveletBlock3DBufReader");

	slice_cntr_c = 0;
	is_open_c = 0;
	buf_c = NULL;
	bufptr_c = NULL;
}

WaveletBlock3DBufReader::WaveletBlock3DBufReader(
	const MetadataVDC &metadata
) : WaveletBlock3DReader(metadata) {

	if (WaveletBlock3DReader::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DBufReader::WaveletBlock3DBufReader()");

	_WaveletBlock3DBufReader();
}

WaveletBlock3DBufReader::WaveletBlock3DBufReader(
	const string &metafile
) : WaveletBlock3DReader(metafile) {

	if (WaveletBlock3DReader::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DBufReader::WaveletBlock3DBufReader(%s)", metafile.c_str()
	);

	_WaveletBlock3DBufReader();
}

WaveletBlock3DBufReader::~WaveletBlock3DBufReader(
) {
	SetDiagMsg("WaveletBlock3DBufReader::~WaveletBlock3DBufReader()");

	WaveletBlock3DBufReader::CloseVariable();
}

int	WaveletBlock3DBufReader::OpenVariableRead(
	size_t timestep,
	const char *varname,
	int reflevel,
	int
) {
	int	rc;
	size_t size;

	SetDiagMsg(
		"WaveletBlock3DBufReader::OpenVariableRead(%d,%s,%d)",
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = GetNumTransforms();

	slice_cntr_c = 0;
	is_open_c = 1;

	rc = WaveletBlock3DReader::OpenVariableRead(timestep, varname, reflevel);
	if (rc<0) return(rc);


	// allocate a buffer large enough to hold two slabs of the 
	// volume at the desired resolution.
	//
	size_t bdim[3];
	WaveletBlockIOBase::GetDimBlk(bdim, reflevel);
	const size_t *bs = GetBlockSize();

	size = bdim[0] * bdim[1] * bs[0] * bs[1] * bs[2] * 2;
	buf_c = new float[size];
	if (! buf_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	return(0);
}

int     WaveletBlock3DBufReader::CloseVariable(
) {
	SetDiagMsg("WaveletBlock3DBufReader::CloseVariable()");

	if (! is_open_c) return(0);

	if (buf_c) delete [] buf_c;
	buf_c = NULL;
	bufptr_c = NULL;
	slice_cntr_c = 0;
	is_open_c = 0;

	return(WaveletBlock3DReader::CloseVariable());
}

int	WaveletBlock3DBufReader::ReadSlice(
	float *slice
) {
	size_t	size;

	SetDiagMsg("WaveletBlock3DBufReader::ReadSlice()");

	size_t bdim[3];
	size_t dim[3];
	WaveletBlockIOBase::GetDimBlk(bdim, _reflevel);
	WaveletBlockIOBase::GetDim(dim, _reflevel);

	const size_t *bs = GetBlockSize();


	if (! is_open_c) {
		SetErrMsg("File must be open before reading");
		return(-1);
	}

	if (slice_cntr_c >= (int)dim[2]) return (0);	

	// Read slabs as needed
	//
	if (slice_cntr_c % (bs[2]*2) == 0) {	
		int	rc;

		rc = ReadSlabs(buf_c, 1);
		if (rc < 0) return (rc);
		bufptr_c = buf_c;
	}

	// Copy data to user space. If volume isn't padded along X axis we
	// can perform a single copy
	//
	if ((bdim[0] % bs[0]) == 0) {
		size = dim[0] * dim[1] * sizeof(*bufptr_c);
		memcpy(slice, bufptr_c, size);
	}
	else {
		size = dim[0] * sizeof(*bufptr_c);
		for(int y=0;y<(int)dim[1]; y++) {
			memcpy(slice+(y*dim[0]), bufptr_c+(y*bdim[0]*bs[0]), size);
		}
	}
	bufptr_c += bdim[0]*bs[0] * bdim[1]*bs[1];
	slice_cntr_c++;

	return(0);
}
