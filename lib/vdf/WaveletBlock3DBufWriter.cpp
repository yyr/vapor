#include <cstdio>
#include <cstring>
#include <cerrno>
#include "vapor/WaveletBlock3DBufWriter.h"

using namespace VAPoR;

void	WaveletBlock3DBufWriter::_WaveletBlock3DBufWriter()
{
	SetClassName("WaveletBlock3DBufWriter");

	slice_cntr_c = 0;
	is_open_c = 0;
	buf_c = NULL;
}

WaveletBlock3DBufWriter::WaveletBlock3DBufWriter(
	Metadata *metadata,
	unsigned int    nthreads
) : WaveletBlock3DWriter(metadata, nthreads) {

	_WaveletBlock3DBufWriter();

}

WaveletBlock3DBufWriter::WaveletBlock3DBufWriter(
	const char *metafile,
	unsigned int    nthreads
) : WaveletBlock3DWriter(metafile, nthreads) {

	_WaveletBlock3DBufWriter();
}

WaveletBlock3DBufWriter::~WaveletBlock3DBufWriter(
) {
	CloseVariable();
}

int	WaveletBlock3DBufWriter::OpenVariableWrite(
	size_t timestep,
	const char *varname
) {
	int	rc;
	size_t size;

	slice_cntr_c = 0;
	is_open_c = 1;

	rc = WaveletBlock3DWriter::OpenVariableWrite(timestep, varname);
	if (rc<0) return(rc);

	size = bdim_c[0] * bdim_c[1] * bs_c * bs_c * bs_c * 2;
	buf_c = new float[size];
	if (! buf_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	return(0);
}

int     WaveletBlock3DBufWriter::CloseVariable(
) {

	if (! is_open_c) return(0);

	if (slice_cntr_c != dim_c[2]) {
		SetErrMsg("File closed before exactly %d slices written", dim_c[2]);
	}

	// May need to flush the buffer
	//
	if (slice_cntr_c % bs_c != 0) {	
		int rc; 

		rc = WaveletBlock3DWriter::WriteSlabs(buf_c);
		if (rc < 0) return (rc);
	}

	if (buf_c) delete [] buf_c;
	buf_c = NULL;
	slice_cntr_c = 0;
	is_open_c = 0;

	return(WaveletBlock3DWriter::CloseVariable());
}

static void	blockit(
	const float	*slice,
	float	*slab,
	int	nx,
	int	ny,
	int	nbx,
	int	nby,
	int	bs,
	int z
) {
	const float *slcptr, *sptr;
	float *blkptr, *bptr;
	int xx,yy;
	int x,y;

	int block_size = bs * bs * bs;

	for(yy=0; yy<nby; yy++) {
	for(xx=0; xx<nbx; xx++) {

		blkptr = slab + ((yy*nbx + xx) * block_size) + (z*bs*bs);
		slcptr = slice + (yy*nx + xx) * bs;

		for(y=0;y<bs && yy*bs+y<ny; y++) {
			sptr = slcptr + (nx*y);
			bptr = blkptr + (bs*y);

			for(x=0;x<bs && xx*bs+x<nx; x++) {
				*bptr++ = *sptr++;
			}
		}
	}
	}
}

int	WaveletBlock3DBufWriter::WriteSlice(
	const float *slice
) {
	size_t	size;

	if (! is_open_c) {
		SetErrMsg("File must be open before writing");
		return(-1);
	}

	if (slice_cntr_c >= (int)dim_c[2]) {
		SetErrMsg("WriteSlice() must be invoked exactly %d times", dim_c[2]);
		return(-1);
	}

	if (slice_cntr_c % (bs_c*2) == 0) {	
		size = bdim_c[0] * bdim_c[1] * bs_c * bs_c * bs_c * 2;
		memset(buf_c, 0, size*sizeof(buf_c[0]));
		bufptr_c = buf_c;
	}

	if ((slice_cntr_c + bs_c) % (bs_c*2) == 0) {
		size = bdim_c[0] * bdim_c[1] * bs_c * bs_c * bs_c;
		bufptr_c = buf_c + size;
	}

	blockit(
		slice, bufptr_c, (int)dim_c[0], (int)dim_c[1], (int)bdim_c[0],(int)bdim_c[1],(int)bs_c,
		slice_cntr_c % bs_c
	);

	slice_cntr_c++;

	if (slice_cntr_c % (bs_c*2) == 0) {	
		int rc; 

		rc = WaveletBlock3DWriter::WriteSlabs(buf_c);
		if (rc < 0) return (rc);
	}


	return(0);

}
