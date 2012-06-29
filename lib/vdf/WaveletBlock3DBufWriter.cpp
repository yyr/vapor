#include <cstdio>
#include <cstring>
#include <cerrno>
#include <vapor/WaveletBlock3DBufWriter.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VAPoR;

void	WaveletBlock3DBufWriter::_WaveletBlock3DBufWriter()
{
	SetClassName("WaveletBlock3DBufWriter");

	slice_cntr_c = 0;
	is_open_c = 0;
	buf_c = NULL;
}

WaveletBlock3DBufWriter::WaveletBlock3DBufWriter(
	const MetadataVDC &metadata
) : WaveletBlock3DWriter(metadata) {

	if (WaveletBlock3DWriter::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DBufWriter::WaveletBlock3DBufWriter()");

	_WaveletBlock3DBufWriter();
}

WaveletBlock3DBufWriter::WaveletBlock3DBufWriter(
	const string &metafile
) : WaveletBlock3DWriter(metafile) {

	if (WaveletBlock3DWriter::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DBufWriter::WaveletBlock3DBufWriter(%s)", 
		metafile.c_str()
	);

	_WaveletBlock3DBufWriter();

}

WaveletBlock3DBufWriter::~WaveletBlock3DBufWriter(
) {
	SetDiagMsg("WaveletBlock3DBufWriter::~WaveletBlock3DBufWriter()");


	WaveletBlock3DWriter::CloseVariable();

}

int	WaveletBlock3DBufWriter::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel,
	int
) {
	int	rc;
	size_t size;

	SetDiagMsg(
		"WaveletBlock3DBufWriter::OpenVariableWrite(%d, %s,%d)", 
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = GetNumTransforms();

	slice_cntr_c = 0;
	is_open_c = 1;

	rc = WaveletBlock3DWriter::OpenVariableWrite(timestep, varname, reflevel);
	if (rc<0) return(rc);

	size_t bdim[3];
	GetDimBlk(bdim, GetNumTransforms());
	const size_t *bs = GetBlockSize();

	size = bdim[0] * bdim[1] * bs[0] * bs[1] * bs[2] * 2;
	buf_c = new float[size];
	if (! buf_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	return(0);
}

int     WaveletBlock3DBufWriter::CloseVariable(
) {

	SetDiagMsg(
		"WaveletBlock3DBufWriter::CloseVariable()"
	);

	if (! is_open_c) return(0);

	size_t dim[3];
	GetDim(dim, -1);
	const size_t *bs = GetBlockSize();
	if (slice_cntr_c != dim[2]) {
		SetErrMsg("File closed before exactly %d slices written", dim[2]);
	}

	// May need to flush the buffer
	//
	if ((slice_cntr_c % (bs[2]*2)) != 0) {	
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
	const size_t	bs[3],
	int z
) {
	const float *slcptr, *sptr;
	float *blkptr, *bptr;
	int xx,yy;
	int x,y;

	int block_size = bs[0] * bs[1] * bs[2];

	for(yy=0; yy<nby; yy++) {
	for(xx=0; xx<nbx; xx++) {

		blkptr = slab + ((yy*nbx + xx) * block_size) + (z*bs[0]*bs[1]);
		slcptr = slice + (yy*nx + xx) * bs[0];

		for(y=0;y<bs[1] && yy*bs[1]+y<ny; y++) {
			sptr = slcptr + (nx*y);
			bptr = blkptr + (bs[1]*y);

			for(x=0;x<bs[0] && xx*bs[0]+x<nx; x++) {
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

	SetDiagMsg(
		"WaveletBlock3DBufWriter::WriteSlice()"
	);

	if (! is_open_c) {
		SetErrMsg("File must be open before writing");
		return(-1);
	}

	size_t dim[3];
	GetDim(dim, -1);
	if (slice_cntr_c >= (int)dim[2]) {
		SetErrMsg("WriteSlice() must be invoked exactly %d times", dim[2]);
		return(-1);
	}

	size_t bdim[3];
	GetDimBlk(bdim, GetNumTransforms());
	const size_t *bs = GetBlockSize();

	if (slice_cntr_c % (bs[2]*2) == 0) {	
		size = bdim[0] * bdim[1] * bs[0] * bs[1] * bs[2] * 2;
		memset(buf_c, 0, size*sizeof(buf_c[0]));
		bufptr_c = buf_c;
	}

	if ((slice_cntr_c + bs[2]) % (bs[2]*2) == 0) {
		size = bdim[0] * bdim[1] * bs[0] * bs[1] * bs[2];
		bufptr_c = buf_c + size;
	}

	blockit(
		slice, bufptr_c, (int)dim[0], (int)dim[1], (int)bdim[0],(int)bdim[1],bs,
		slice_cntr_c % bs[2]
	);

	slice_cntr_c++;

	if (slice_cntr_c % (bs[2]*2) == 0) {	
		int rc; 

		rc = WaveletBlock3DWriter::WriteSlabs(buf_c);
		if (rc < 0) return (rc);
	}


	return(0);

}
