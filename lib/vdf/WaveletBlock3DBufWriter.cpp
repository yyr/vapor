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
	_writer2D = NULL;
	_vartype = VAR3D;
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
	if (_writer2D) delete _writer2D;

}

int	WaveletBlock3DBufWriter::_OpenVariableWrite2D(
	size_t timestep,
	const char *varname,
	int reflevel
) {
	if (! _writer2D) {
		_writer2D = new WaveletBlock3DRegionWriter(*this);
	}

	return (_writer2D->OpenVariableWrite(timestep, varname, reflevel));
}

int	WaveletBlock3DBufWriter::OpenVariableWrite(
	size_t timestep,
	const char *varname,
	int reflevel,
	int
) {
	_vartype = GetVarType(varname);
	if (_vartype != VAR3D) {
		return(_OpenVariableWrite2D(timestep, varname, reflevel));
	} 

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
	WaveletBlockIOBase::GetDimBlk(bdim, GetNumTransforms());
	const size_t *bs = GetBlockSize();

	size = bdim[0] * bdim[1] * bs[0] * bs[1] * bs[2] * 2;
	buf_c = new float[size];
	if (! buf_c) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}

	return(0);
}

int     WaveletBlock3DBufWriter::_CloseVariable2D() {

	int rc = _writer2D->CloseVariable();
	const float *r = _writer2D->GetDataRange();
	_dataRange[0] = r[0];
	_dataRange[1] = r[1];
	return 0;
}


int     WaveletBlock3DBufWriter::CloseVariable(
) {
	if (_vartype != VAR3D) {
		return(_CloseVariable2D());
	} 

	SetDiagMsg(
		"WaveletBlock3DBufWriter::CloseVariable()"
	);

	if (! is_open_c) return(0);

	size_t dim[3];
	WaveletBlockIOBase::GetDim(dim, -1);
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

int	WaveletBlock3DBufWriter::_WriteSlice2D(
	const float *slice
) {
	size_t dim[3];
	WaveletBlockIOBase::GetDim(dim, -1);

	size_t min[] = {0,0,0};
	size_t max[] = {0,0,0};

	switch (_vartype) {
	case Metadata::VAR2D_XY:
		max[0] = dim[0] - 1;
		max[1] = dim[1] - 1;
	break;

	case Metadata::VAR2D_XZ:
		max[0] = dim[0] - 1;
		max[2] = dim[2] - 1;
	break;
	case Metadata::VAR2D_YZ:
		max[1] = dim[1] - 1;
		max[2] = dim[2] - 1;
	break;
	default:
	break;

	}
	
	return(_writer2D->WriteRegion(slice, min, max));
}

int	WaveletBlock3DBufWriter::WriteSlice(
	const float *slice
) {
	if (_vartype != VAR3D) {
		return(_WriteSlice2D(slice));
	} 
	size_t	size;

	SetDiagMsg(
		"WaveletBlock3DBufWriter::WriteSlice()"
	);

	if (! is_open_c) {
		SetErrMsg("File must be open before writing");
		return(-1);
	}

	size_t dim[3];
	WaveletBlockIOBase::GetDim(dim, -1);
	if (slice_cntr_c >= (int)dim[2]) {
		SetErrMsg("WriteSlice() must be invoked exactly %d times", dim[2]);
		return(-1);
	}

	size_t bdim[3];
	WaveletBlockIOBase::GetDimBlk(bdim, GetNumTransforms());
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
