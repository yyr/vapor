#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cerrno>
#include "vapor/WaveletBlock3DBufReader.h"

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
	const Metadata *metadata,
	unsigned int    nthreads
) : WaveletBlock3DReader(metadata, nthreads) {

	_objInitialized = 0;

	SetDiagMsg(
		"WaveletBlock3DBufReader::WaveletBlock3DBufReader(,%d)", nthreads
	);

	_WaveletBlock3DBufReader();
	_objInitialized = 1;
}

WaveletBlock3DBufReader::WaveletBlock3DBufReader(
	const char	*metafile,
	unsigned int    nthreads
) : WaveletBlock3DReader(metafile, nthreads) {

	_objInitialized = 0;

	SetDiagMsg(
		"WaveletBlock3DBufReader::WaveletBlock3DBufReader(%s,%d)", 
		metafile, nthreads
	);

	_WaveletBlock3DBufReader();
	_objInitialized = 1;
}

WaveletBlock3DBufReader::~WaveletBlock3DBufReader(
) {
	SetDiagMsg("WaveletBlock3DBufReader::~WaveletBlock3DBufReader()");
	if (! _objInitialized) return;

	CloseVariable();
	_objInitialized = 0;
}

int	WaveletBlock3DBufReader::OpenVariableRead(
	size_t timestep,
	const char *varname,
	size_t num_xforms
) {
	int	rc;
	size_t size;

	SetDiagMsg(
		"WaveletBlock3DBufReader::OpenVariableRead(%d,%s,%d)",
		timestep, varname, num_xforms
	);

	slice_cntr_c = 0;
	is_open_c = 1;

	rc = WaveletBlock3DReader::OpenVariableRead(timestep, varname, num_xforms);
	if (rc<0) return(rc);


	// allocate a buffer large enough to hold two slabs of the 
	// volume at the desired resolution.
	//
	size = xbdim_c[0] * xbdim_c[1] * bs_c * bs_c * bs_c * 2;
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

	if (! is_open_c) {
		SetErrMsg("File must be open before reading");
		return(-1);
	}

	if (slice_cntr_c >= (int)xdim_c[2]) return (0);	

	// Read slabs as needed
	//
	if (slice_cntr_c % (bs_c*2) == 0) {	
		int	rc;

		rc = ReadSlabs(buf_c, 1);
		if (rc < 0) return (rc);
		bufptr_c = buf_c;
	}

	// Copy data to user space. If volume isn't padded along X axis we
	// can perform a single copy
	//
	if ((xdim_c[0] % bs_c) == 0) {
		size = xdim_c[0] * xdim_c[1] * sizeof(*bufptr_c);
		memcpy(slice, bufptr_c, size);
	}
	else {
		size = xdim_c[0] * sizeof(*bufptr_c);
		for(int y=0;y<(int)xdim_c[1]; y++) {
			memcpy(slice+(y*xdim_c[0]), bufptr_c+(y*xbdim_c[0]*bs_c), size);
		}
	}
	bufptr_c += xbdim_c[0]*bs_c * xbdim_c[1]*bs_c;
	slice_cntr_c++;

	return(0);
}
