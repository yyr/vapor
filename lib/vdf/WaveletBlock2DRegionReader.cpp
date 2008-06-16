#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "vapor/WaveletBlock2DRegionReader.h"

using namespace VetsUtil;
using namespace VAPoR;

void	WaveletBlock2DRegionReader::_WaveletBlock2DRegionReader()
{
	int	j;

	SetClassName("WaveletBlock2DRegionReader");

	_xform_timer = 0.0;

	for(j=0; j<MAX_LEVELS; j++) {
		_lambda_tiles[j] = NULL;
		_gamma_tiles[j] = NULL;
	}
}

WaveletBlock2DRegionReader::WaveletBlock2DRegionReader(
	const Metadata *metadata
) : WaveletBlock2DIO(metadata) {

	_objInitialized = 0;
	if (WaveletBlock2DRegionReader::GetErrCode()) return;

	SetDiagMsg("WaveletBlock2DRegionReader::WaveletBlock2DRegionReader()");

	_WaveletBlock2DRegionReader();

	_objInitialized = 1;
}

WaveletBlock2DRegionReader::WaveletBlock2DRegionReader(
	const char *metafile
) : WaveletBlock2DIO(metafile) {

	_objInitialized = 0;
	if (WaveletBlock2DRegionReader::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock2DRegionReader::WaveletBlock2DRegionReader(%s)",
		metafile
	);

	_WaveletBlock2DRegionReader();

	_objInitialized = 1;
}

WaveletBlock2DRegionReader::~WaveletBlock2DRegionReader(
) {

	SetDiagMsg("WaveletBlock2DRegionReader::~WaveletBlock2DRegionReader()");
	if (! _objInitialized) return;

	WaveletBlock2DRegionReader::CloseVariable();
	my_free();

	_objInitialized = 0;
}

int	WaveletBlock2DRegionReader::OpenVariableRead(
	size_t	timestep,
	const char	*varname,
	int reflevel
) {
	int	rc;

	SetDiagMsg(
		"WaveletBlock2DRegionReader::OpenVariableRead(%d, %s, %d)",
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = _num_reflevels - 1;

	WaveletBlock2DRegionReader::CloseVariable();	// close any previously opened files.
	rc = WaveletBlock2DIO::OpenVariableRead(timestep, varname, reflevel);
	if (rc<0) return(rc);
	rc = my_realloc();
	if (rc<0) return(rc);

	return(0);
}

int	WaveletBlock2DRegionReader::CloseVariable(
) {
	SetDiagMsg("WaveletBlock2DRegionReader::CloseVariable()");

	return(WaveletBlock2DIO::CloseVariable());
}

int	WaveletBlock2DRegionReader::_ReadRegion(
	const size_t min[2],
	const size_t max[2],
	float *region,
	int unblock
) {
	size_t bs[2];
	size_t bmin[2];
	size_t bmax[2];

	size_t min3d[] = {0,0,0};
	size_t max3d[] = {0,0,0};
	size_t bmin3d[] = {0,0,0};
	size_t bmax3d[] = {0,0,0};
	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0]; bs[1] = _bs[1];
		min3d[0] = min[0]; min3d[1] = min[1]; 
		max3d[0] = max[0]; max3d[1] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[1];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[1];
	break;
	case VAR2D_XZ:
		bs[0] = _bs[0]; bs[1] = _bs[2];
		min3d[0] = min[0]; min3d[2] = min[1];
		max3d[0] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[2];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[2];
	break;
	case VAR2D_YZ:
		bs[0] = _bs[1]; bs[1] = _bs[2];
		min3d[1] = min[0]; min3d[2] = min[1];
		max3d[1] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d);
		bmin[0] = bmin3d[1]; bmin[1] = bmin3d[2];
		bmax[0] = bmax3d[1]; bmax[1] = bmax3d[2];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	int	x,y;
	int	rc;
	int	l0x0, l0x1, l0y0, l0y1;
	size_t minsub[] = {min[0], min[1]};
	size_t maxsub[] = {max[0], max[1]};

	while (minsub[0] >= bs[0]) {
		minsub[0] -= bs[0];
		maxsub[0] -= bs[0];
	}
	while (minsub[1] >= bs[1]) {
		minsub[1] -= bs[1];
		maxsub[1] -= bs[1];
	}

	if (! unblock) {
		// sanity check. Region must be defined on block boundaries
		// to preserve blocking
		//
		for(int i=0; i<2; i++) assert((min[i] % bs[i]) == 0);
		for(int i=0; i<2; i++) assert(((max[i]+1) % bs[i]) == 0);
	}


	size_t x0 = bmin[0];
	size_t y0 = bmin[1];
	size_t x1 = bmax[0];
	size_t y1 = bmax[1];


	// Max transform request => read raw blocks directly into buffer. Do no
	// processing.
	//
	size_t tile_size = bs[0]*bs[1];
	if (_reflevel == 0) {

		float *regptr = region;

		// read in blocks a row at a time.
		for(y=(int)y0; y<=(int)y1; y++) {
			const size_t bcoord[] = {x0, y};
			rc = seekLambdaBlocks(bcoord);
			if (rc < 0) return (rc);

			if (! unblock) {
				rc = readLambdaBlocks(x1-x0+1, regptr);
				if (rc < 0) return (rc);
				regptr += tile_size * (x1-x0+1);
			}
			else {
				float *srcptr = _lambda_tiles[0];

				rc = readLambdaBlocks(x1-bmin[0]+1, _lambda_tiles[0]);
				if (rc < 0) return (rc);
				for(x=0;x<(int)(x1-x0+1); x++) {
					const size_t bcoord[] = {x,y-y0};
					Tile2NonTile(srcptr,bcoord,minsub,maxsub,_vtype,region);
					srcptr += tile_size;
				}
					
			}
		}
		return(0);
	}

	// Get bounds of subregion at coarsest level
	//
	l0x0 = (int)x0 >> _reflevel;
	l0x1 = (int)x1 >> _reflevel;
	l0y0 = (int)y0 >> _reflevel;
	l0y1 = (int)y1 >> _reflevel;

	// read in blocks from level 0 a row at a time.
	for(y=l0y0; y<=l0y1; y++) {
		const size_t bcoord[] = {l0x0, y};
		rc = seekLambdaBlocks(bcoord);
		if (rc < 0) return (rc);

		rc = readLambdaBlocks(l0x1-l0x0+1, _lambda_tiles[0]);
		if (rc < 0) return (rc);

		// transform blocks from level 0 to desired level
		rc = row_inv_xform(
			_lambda_tiles[0], l0x0, y, l0x1-l0x0+1, 0,
			region, min, max, _reflevel, unblock
		);
		if (rc < 0) return (rc);
	}

	return(0);
}

int	WaveletBlock2DRegionReader::ReadRegion(
	const size_t min[2],
	const size_t max[2],
	float *region
) {
	SetDiagMsg(
		"WaveletBlock2DRegionReader::ReadRegion((%d,%d),(%d,%d))",
		min[0], min[1], max[0], max[1]
	);

	// Make sure 3rd, unused coord is valid
	//
	size_t min3d[3], max3d[3];
	VDFIOBase::GetValidRegion(min3d, max3d, _reflevel); 

	switch (_vtype) {
	case VAR2D_XY:
		min3d[0] = min[0]; min3d[1] = min[1]; 
		max3d[0] = max[0]; max3d[1] = max[1];
	break;
	case VAR2D_XZ:
		min3d[0] = min[0]; min3d[2] = min[1];
		max3d[0] = max[0]; max3d[2] = max[1];
	break;
	case VAR2D_YZ:
		min3d[1] = min[0]; min3d[2] = min[1];
		max3d[1] = max[0]; max3d[2] = max[1];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (! VDFIOBase::IsValidRegion(min3d, max3d, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d) (%d %d)", 
			min[0], min[1], max[0], max[1]
		);
		return(-1);
	}

	return(_ReadRegion(min, max, region, 1));
}

int	WaveletBlock2DRegionReader::ReadRegion(
	float *region
) {
	SetDiagMsg( "WaveletBlock2DRegionReader::ReadRegion()");

	size_t dim3d[3];
	VDFIOBase::GetDim(dim3d,_reflevel);

	size_t min[] = {0,0};
	size_t max[2];
	switch (_vtype) {
	case VAR2D_XY:
		max[0] = dim3d[0]-1; max[1] = dim3d[1]-1;
	break;
	case VAR2D_XZ:
		max[0] = dim3d[0]-1; max[1] = dim3d[2]-1;
	break;
	case VAR2D_YZ:
		max[0] = dim3d[1]-1; max[1] = dim3d[2]-1;
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	return(ReadRegion(min, max, region));
}


int	WaveletBlock2DRegionReader::BlockReadRegion(
	const size_t bmin[2],
	const size_t bmax[2],
	float *region,
	int unblock
) {
	size_t min[2], max[2];

	SetDiagMsg(
		"WaveletBlock2DRegionReader::BlockReadRegion((%d,%d),(%d,%d))",
		bmin[0], bmin[1], bmax[0], bmax[1]
	);


	size_t min3d[3], max3d[3];
	size_t bmin3d[3], bmax3d[3];
	size_t bs[2];
	// Make sure 3rd, unused coord is valid
	//
	VDFIOBase::GetValidRegion(min3d, max3d, _reflevel);
	VDFIOBase::MapVoxToBlk(min3d, bmin3d);
	VDFIOBase::MapVoxToBlk(max3d, bmax3d);

	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0]; bs[1] = _bs[1];
		bmin3d[0] = bmin[0]; bmin3d[1] = bmin[1]; 
		bmax3d[0] = bmax[0]; bmax3d[1] = bmax[1];
	break;
	case VAR2D_XZ:
		bs[0] = _bs[0]; bs[1] = _bs[2];
		bmin3d[0] = bmin[0]; bmin3d[2] = bmin[1];
		bmax3d[0] = bmax[0]; bmax3d[2] = bmax[1];
	break;
	case VAR2D_YZ:
		bs[0] = _bs[1]; bs[1] = _bs[2];
		bmin3d[1] = bmin[0]; bmin3d[2] = bmin[1];
		bmax3d[1] = bmax[0]; bmax3d[2] = bmax[1];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	if (! VDFIOBase::IsValidRegionBlk(bmin3d, bmax3d, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d) (%d %d)", 
			bmin[0], bmin[1], bmax[0], bmax[1]
		);
		return(-1);
	}

	for(int i=0; i<2; i++) min[i] = bs[i]*bmin[i];
	for(int i=0; i<2; i++) max[i] = (bs[i]*(bmax[i]+1))-1;

	return(_ReadRegion(min, max, region, unblock));

}

int	WaveletBlock2DRegionReader::row_inv_xform(
	const float *lambda_row,
	unsigned int ljx0,
	unsigned int ljy0,
	unsigned int ljnx,
	unsigned int j,
	float *region,
	const size_t min[2],
	const size_t max[2],
	unsigned int level,
	int unblock
) {

	size_t bs[2];
	size_t bmin[2];
	size_t bmax[2];
	WaveletBlock2D *wb2d;

	size_t min3d[] = {0,0,0};
	size_t max3d[] = {0,0,0};
	size_t bmin3d[] = {0,0,0};
	size_t bmax3d[] = {0,0,0};

	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0]; bs[1] = _bs[1];
		min3d[0] = min[0]; min3d[1] = min[1]; 
		max3d[0] = max[0]; max3d[1] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[1];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[1];
		wb2d = _wb2dXY;
	break;
	case VAR2D_XZ:
		bs[0] = _bs[0]; bs[1] = _bs[2];
		min3d[0] = min[0]; min3d[2] = min[1];
		max3d[0] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[2];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[2];
		wb2d = _wb2dXZ;
	break;
	case VAR2D_YZ:
		bs[0] = _bs[1]; bs[1] = _bs[2];
		min3d[1] = min[0]; min3d[2] = min[1];
		max3d[1] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d);
		bmin[0] = bmin3d[1]; bmin[1] = bmin3d[2];
		bmax[0] = bmax3d[1]; bmax[1] = bmax3d[2];
		wb2d = _wb2dYZ;
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	size_t minsub[] = {min[0], min[1], min[2]};
	size_t maxsub[] = {max[0], max[1], max[2]};

	while (minsub[0] >= bs[0]) {
		minsub[0] -= bs[0];
		maxsub[0] -= bs[0];
	}
	while (minsub[1] >= bs[1]) {
		minsub[1] -= bs[1];
		maxsub[1] -= bs[1];
	}


	size_t x0 = bmin[0];
	size_t y0 = bmin[1];
	size_t x1 = bmax[0];
	size_t y1 = bmax[1];

	//	Coords at level j + 1  (May be outside subregion)
	//
	int	ljp1y0 = ljy0<<1;
	int	ljp1nx = ljnx<<1;

	//	Coords at level j + 1 that are contained in subregion
	//
	int	sljp1x0 = (int)(x0>>(level-(j+1)));
	int	sljp1y0 = (int)(y0>>(level-(j+1)));
	int	sljp1x1 = (int)(x1>>(level-(j+1)));
	int	sljp1y1 = (int)(y1>>(level-(j+1)));
	int	sljp1nx = (int)(sljp1x1 - sljp1x0 + 1);

	const float *src_super_tile[4];
	float	*dst_super_tile[4];

	float	*srcptr, *dstptr;
	float	*slice;
	int	dst_nbx;
	int	i,x,y;
	int	rc;

	size_t tile_size = bs[0]*bs[1];

	// Get gamma coefficients for next level
	//
	const size_t bcoord[] = {ljx0, ljy0};
	rc = seekGammaBlocks(bcoord, j+1);
	if (rc<0) return(rc);

	rc = readGammaBlocks(ljnx, _gamma_tiles[j+1], j+1);
	if (rc<0) return(rc);

	dst_nbx = ljp1nx;

	slice = _lambda_tiles[j+1];

	// Apply inverse transform to go from level j to j+1
	//
	for(x=0; x<(int)ljnx; x++) {

		src_super_tile[0] = lambda_row + (x * tile_size);
		for(i=1; i<4; i++) {
			src_super_tile[i] = _gamma_tiles[j+1] + 
				(x*tile_size*3) + (tile_size * (i-1));
		}

		dst_super_tile[0] = slice;
		dst_super_tile[1] = slice + tile_size;
		dst_super_tile[2] = slice + (tile_size * dst_nbx);
		dst_super_tile[3] = slice + (tile_size * dst_nbx) + tile_size;


		TIMER_START(t0)
		wb2d->InverseTransform(src_super_tile,dst_super_tile);
		TIMER_STOP(t0, _xform_timer)

		slice += 2*tile_size;
	}


	if (j+1 < level) {
		for(y=0; y<2; y++) {

			// cull rows outside of subregion
			//
			if (y+ljp1y0 < sljp1y0 || y+ljp1y0 > sljp1y1) continue;

			srcptr = _lambda_tiles[j+1] + (tile_size *  y*ljp1nx);

			if (sljp1x0 % 2) srcptr += tile_size;

			rc = row_inv_xform(
				srcptr, sljp1x0, ljp1y0+y, sljp1nx,
				j+1, region, min, max, level,
				unblock
			);
			if (rc < 0) return(rc);
		}
		return(0);
	}

	// Last iteration. Copy *valid* blocks (those within the subregion) 
	// to region buffer. 
	//
	if (j+1 == level) {
		int	yy;

		for(y=0,yy=0; y<2; y++) {
			int	regnx = (int)(x1-x0+1);

			// cull rows outside of subregion
			// n.b. y0==sljp1y0, etc.
			//
			if (y+ljp1y0 < sljp1y0 || y+ljp1y0 > sljp1y1) continue;


			srcptr = _lambda_tiles[j+1] + 
				(tile_size * y*ljp1nx);

			if (sljp1x0 % 2) srcptr += tile_size;

			yy = y + ljp1y0 - sljp1y0;
			dstptr = region + 
				(tile_size * yy * regnx);

			if (! unblock) {
				memcpy(
					dstptr, srcptr, 
					sljp1nx*tile_size*sizeof(srcptr[0])
				);
			}
			else {
				for(x=0;x<sljp1nx;x++) {
					const size_t bcoord[] = {x,yy};
					Tile2NonTile(srcptr, bcoord, minsub, maxsub, _vtype,region);
					srcptr += tile_size;
				}
			}
			yy++;

		}
		return(0);
	}

	return(0);
}



//
// allocate memory, only if needed.
//
int	WaveletBlock2DRegionReader::my_realloc(
) {
	// Maximum size of a tile regardless of type (XY, XZ, YZ)
	size_t max_tile_sz = max(_bs[0],_bs[1]) * max(_bs[2], min(_bs[0],_bs[1]));

	// alloc space from coarsest (j==0) to finest level
	for(int j=0; j<=_reflevel; j++) {

		if (! _lambda_tiles[j]) {
			size_t nb_j[3];
			size_t     size;

			VDFIOBase::GetDimBlk(nb_j, j);
			nb_j[0] += 1;	// fudge (deal with odd size dimensions)
			nb_j[1] += 1;	// fudge (deal with odd size dimensions)

			size = max(nb_j[0],nb_j[1]);
			size *= max_tile_sz * 2;

			_lambda_tiles[j] = new float[size];
			if (! _lambda_tiles[j]) {
				SetErrMsg("new float[%d] : %s", size, strerror(errno));
				return(-1);
			}

			size = max(nb_j[0],nb_j[1]);
			size *= max_tile_sz * 2 * 3;

			_gamma_tiles[j] = new float[size];
			if (! _gamma_tiles[j]) {
				SetErrMsg("new float[%d] : %s", size, strerror(errno));
				return(-1);
			}
		}
	}

	return(0);
}

void	WaveletBlock2DRegionReader::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_lambda_tiles[j]) delete [] _lambda_tiles[j];
		if (_gamma_tiles[j]) delete [] _gamma_tiles[j];

		_lambda_tiles[j] = NULL;
		_gamma_tiles[j] = NULL;
	}
}
