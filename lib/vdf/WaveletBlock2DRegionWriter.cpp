//
//	$Id$
//
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "vapor/WaveletBlock2DRegionWriter.h"

using namespace VetsUtil;
using namespace VAPoR;

int	WaveletBlock2DRegionWriter::_WaveletBlock2DRegionWriter()
{
	int	j;

	SetClassName("WaveletBlock2DRegionWriter");

	_xform_timer = 0.0;
	_is_open = 0;

	for(j=0; j<MAX_LEVELS; j++) {
		_lambda_tiles[j] = NULL;
	}
	_padblock = NULL;

	return(my_alloc());
}

WaveletBlock2DRegionWriter::WaveletBlock2DRegionWriter(
	const Metadata *metadata
) : WaveletBlock2DIO(metadata) {

	_objInitialized = 0;
	if (WaveletBlock2DRegionWriter::GetErrCode()) return;

	SetDiagMsg("WaveletBlock2DRegionWriter::WaveletBlock2DRegionWriter()");

	if (_WaveletBlock2DRegionWriter() < 0) return;

	_objInitialized = 1;
}

WaveletBlock2DRegionWriter::WaveletBlock2DRegionWriter(
	const char *metafile
) : WaveletBlock2DIO(metafile) {

	_objInitialized = 0;
	if (WaveletBlock2DRegionWriter::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock2DRegionWriter::WaveletBlock2DRegionWriter(%s)",
		metafile
	);

	if (_WaveletBlock2DRegionWriter() < 0) return;

	_objInitialized = 1;
}

WaveletBlock2DRegionWriter::~WaveletBlock2DRegionWriter(
) {

	SetDiagMsg("WaveletBlock2DRegionWriter::~WaveletBlock2DRegionWriter()");
	if (! _objInitialized) return;

	WaveletBlock2DRegionWriter::CloseVariable();

	my_free();

	_objInitialized = 0;
}

int	WaveletBlock2DRegionWriter::OpenVariableWrite(
	size_t	timestep,
	const char	*varname,
	int reflevel
) {
	int	rc;

	SetDiagMsg(
		"WaveletBlock2DRegionWriter::OpenVariableWrite(%d, %s, %d)",
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = _num_reflevels - 1;

	WaveletBlock2DRegionWriter::CloseVariable();	// close any previously opened files.
	rc = WaveletBlock2DIO::OpenVariableWrite(timestep, varname, reflevel);
	if (rc<0) return(rc);

	_is_open = 1;

	return(0);
}

int	WaveletBlock2DRegionWriter::CloseVariable(
) {
	SetDiagMsg("WaveletBlock2DRegionWriter::CloseVariable()");

	if (! _is_open) return(0);

	//
	// We've already computed the block min & max at the native refinement
	// level. Now go back and compute the block min & max at refinement 
	// levels [0.._num_reflevels-2]
	//
	// N.B. this code computes min/max across entire area. Really only
	// need to do subarea...
	//
	for (int l=_num_reflevels-2; l>=0; l--) {
		size_t bdim3d[3];		// block dim at level l
		size_t bdimlp13d[3];	// block dim at level l+1
		size_t bdim[2];		// 2d block dim at level l
		size_t bdimlp1[2];	// 2d block dim at level l+1
		int blkidx;
		int blkidxlp1;

		VDFIOBase::GetDimBlk(bdim3d, l);
		VDFIOBase::GetDimBlk(bdimlp13d, l+1);

		switch (_vtype) {
		case VAR2D_XY:
			bdim[0] = bdim3d[0]; bdim[1] = bdim3d[1];
			bdimlp1[0] = bdimlp13d[0]; bdimlp1[1] = bdimlp13d[1];
		break;
		case VAR2D_XZ:
			bdim[0] = bdim3d[0]; bdim[1] = bdim3d[2];
			bdimlp1[0] = bdimlp13d[0]; bdimlp1[1] = bdimlp13d[2];
		break;
		case VAR2D_YZ:
			bdim[0] = bdim3d[1]; bdim[1] = bdim3d[2];
			bdimlp1[0] = bdimlp13d[1]; bdimlp1[1] = bdimlp13d[2];
		break;
		default:
			SetErrMsg("Invalid variable type");
			return(-1);
		}

		for(int y=0; y<bdim[1]; y++) {
		for(int x=0; x<bdim[0]; x++) {

			blkidx = y * bdim[0] + x;
			blkidxlp1 = (y<<1) * bdimlp1[0] + (x<<1);
			_mins[l][blkidx] = _mins[l+1][blkidxlp1];
			_maxs[l][blkidx] = _maxs[l+1][blkidxlp1];


			for(int yy=y<<1; yy < (y<<1)+2 && yy<bdimlp1[1]; yy++) {
			for(int xx=x<<1; xx < (x<<1)+2 && xx<bdimlp1[0]; xx++) {
				blkidxlp1 = yy * bdimlp1[0] + xx;

				if (_mins[l+1][blkidxlp1] < _mins[l][blkidx]) {
					_mins[l][blkidx] = _mins[l+1][blkidxlp1];
				}
				if (_maxs[l+1][blkidxlp1] > _maxs[l][blkidx]) {
					_maxs[l][blkidx] = _maxs[l+1][blkidxlp1];
				}
			}
			}

		}
		}
	}

	_is_open = 0;
	return(WaveletBlock2DIO::CloseVariable());
}

int	WaveletBlock2DRegionWriter::_WriteUntransformedRegion(
	const size_t bs[2],
	const size_t min[2],
	const size_t max[2],
	const float *region,
	int block
) {
	assert(block == 1);

	// bounds (in voxels) relative to smallest block-aligned 
	// enclosing region
	//
	size_t rmin[2];
	size_t rmax[2];	

	size_t regbsize[2];	// Size of region in blocks

	// Compute bounds (in voxels) relative to the smallest 
	// block-aligned enclosing region.
	//
	for (int i=0; i<2; i++) {
		rmin[i] = min[i] - (bs[i] * (min[i] / bs[i]));
		rmax[i] = max[i] - (min[i] - rmin[i]);

		// Compute size, in blocks, of the block-aligned 
		// enclosing region
		//
		regbsize[i] = rmax[i] / bs[i] + 1;
	}


	//
	// Block the data, one block at a time
	//
	for (int y=0; y<regbsize[1]; y++) {
	for (int x=0; x<regbsize[0]; x++) {
		size_t srcx, srcy;
		size_t dstx, dsty;

		srcx = x*_bs[0] - rmin[0];
		dstx = 0;
		srcy = y*_bs[1] - rmin[1];
		dsty = 0;

		// if a boundary block
		//
		if ((x==0 && rmin[0] != 0) ||
			(y==0 && rmin[1] != 0) ||
			(x==regbsize[0]-1 && ((rmax[0]+1) % _bs[0])) ||
			(y==regbsize[1]-1 && ((rmax[1]+1) % _bs[1]))) {

			if (x==0) {
				srcx = 0;
				dstx = rmin[0];
			}
			if (y==0) {
				srcy = 0;
				dsty = rmin[1];
			}

			brickit(
				region, bs, max[0]-min[0]+1, max[1]-min[1]+1,
				srcx, srcy, dstx, dsty, _lambda_tiles[0]
			);

		}
		else {

			// Interior of region (or a boundary piece that is already
			// aligned with the block-aligned enclosing region
			//
			brickit(
				region, bs, max[0]-min[0]+1, max[1]-min[1]+1,
				srcx, srcy, _lambda_tiles[0]
			);

		}

		compute_minmax( _lambda_tiles[0], _volBMin[0]+x, _volBMin[1]+y, 0);

		int rc;
		const size_t bcoord[] = {_volBMin[0]+x, _volBMin[1]+y};
		rc = seekLambdaBlocks(bcoord);
		if (rc < 0) return (rc);

		rc = writeLambdaBlocks(_lambda_tiles[0], 1);
		if (rc < 0) return (rc);

	}
	}

	return(0);

}

int	WaveletBlock2DRegionWriter::_WriteRegion(
	const float *region,
	const size_t min[2],
	const size_t max[2],
	int block
) {

	size_t quadmin[2], quadmax[2];	// bounds (in blocks) of
									// superblock enclosing subregion at
									// coarsest level, relative to
									// global volume at finest level
	int regstart[2];

	// initialize data range calculation
	//
	_dataRange[0] = _dataRange[1] = *region;

	size_t bs[2];

	size_t min3d[] = {0,0,0};
    size_t max3d[] = {0,0,0};
	size_t volBMin3d[3], volBMax3d[3];
	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0]; bs[1] = _bs[1];
		_validRegMin[0] = min[0];
		_validRegMax[0] = max[0];
		_validRegMin[1] = min[1];
		_validRegMax[1] = max[1];
		_validRegMin[2] = 0;
		_validRegMax[2] = _dim[2]-1;

        min3d[0] = min[0]; min3d[1] = min[1];
        max3d[0] = max[0]; max3d[1] = max[1];
		// Bounds (in blocks) of region relative to global volume
		VDFIOBase::MapVoxToBlk(min3d, volBMin3d);
		VDFIOBase::MapVoxToBlk(max3d, volBMax3d);
		_volBMin[0] = volBMin3d[0]; _volBMin[1] = volBMin3d[1];
		_volBMax[0] = volBMax3d[0]; _volBMax[1] = volBMax3d[1];
	break;
	case VAR2D_XZ:
		bs[0] = _bs[0]; bs[1] = _bs[2];
		_validRegMin[0] = min[0];
		_validRegMax[0] = max[0];
		_validRegMin[1] = 0;
		_validRegMax[1] = _dim[1]-1;
		_validRegMin[2] = min[1];
		_validRegMax[2] = max[1];

        min3d[0] = min[0]; min3d[2] = min[1];
        max3d[0] = max[0]; max3d[2] = max[1];
		// Bounds (in blocks) of region relative to global volume
		VDFIOBase::MapVoxToBlk(min3d, volBMin3d);
		VDFIOBase::MapVoxToBlk(max3d, volBMax3d);
		_volBMin[0] = volBMin3d[0]; _volBMin[1] = volBMin3d[2];
		_volBMax[0] = volBMax3d[0]; _volBMax[1] = volBMax3d[2];

	break;
	case VAR2D_YZ:
		bs[0] = _bs[1]; bs[1] = _bs[2];
		_validRegMin[0] = 0;
		_validRegMax[0] = _dim[0]-1;
		_validRegMin[1] = min[0];
		_validRegMax[1] = max[0];
		_validRegMin[2] = min[1];
		_validRegMax[2] = max[1];
        min3d[1] = min[0]; min3d[2] = min[1];
        max3d[1] = max[0]; max3d[2] = max[1];
		// Bounds (in blocks) of region relative to global volume
		VDFIOBase::MapVoxToBlk(min3d, volBMin3d);
		VDFIOBase::MapVoxToBlk(max3d, volBMax3d);
		_volBMin[0] = volBMin3d[1]; _volBMin[1] = volBMin3d[2];
		_volBMax[0] = volBMax3d[1]; _volBMax[1] = volBMax3d[2];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}



	// handle case where there is no transform
	//
	if ((_num_reflevels-1) <= 0) {
		return(_WriteUntransformedRegion(bs, min, max, region, block));
	}

	for (int i=0; i<2; i++) {
		// Compute bounds, in voxels, relative to the smallest 
		// superblock-aligned enclosing region.
		//
		_regMin[i] = min[i] - ((2*bs[i]) * (min[i] / (2*bs[i])));
		_regMax[i] = max[i] - (min[i] - _regMin[i]);

		// Compute bounds (in blocks), relative to global volume, of 
		// level 0 super-block-aligned volume that encloses the region.
		//
		if (IsOdd(_volBMin[i])) {	// not superblock aligned yet
			size_t tmp = (_volBMin[i]-1) >> (_num_reflevels-1);
			quadmin[i] = tmp * (1 << (_num_reflevels-1));
			regstart[i] = quadmin[i] - (_volBMin[i]-1);
		}
		else {
			size_t tmp = _volBMin[i] >> (_num_reflevels-1);
			quadmin[i] = tmp * (1 << (_num_reflevels-1));
			regstart[i] = quadmin[i] - _volBMin[i];
		}

		if (IsOdd(_volBMax[i])) {
			size_t tmp = (_volBMax[i]+1) >> (_num_reflevels-1);
			quadmax[i] = (tmp+1) * (1 << (_num_reflevels-1)) - 1;
		}
		else {
			size_t tmp = _volBMax[i] >> (_num_reflevels-1);
			quadmax[i] = (tmp+1) * (1 << (_num_reflevels-1)) - 1;
		}

	}


	int sz = 1<<(_num_reflevels-1);
																			
	for(int y=regstart[1], yy=quadmin[1]; yy<=quadmax[1]; y+=sz, yy+=sz) {
	for(int x=regstart[0], xx=quadmin[0]; xx<=quadmax[0]; x+=sz, xx+=sz) {
		int rc;
		rc = process_quadrant(sz,x,y, xx,yy, 0);
		if (rc < 0) return(rc);
	}
	}

	return(0);

}

int	WaveletBlock2DRegionWriter::WriteRegion(
	const float *region,
	const size_t min[2],
	const size_t max[2]
) {

	SetDiagMsg(
		"WaveletBlock2DRegionWriter::WriteRegion((%d,%d),(%d,%d))",
		min[0], min[1], max[0], max[1]
	);

	size_t min3d[3] = {0,0,0};
	size_t max3d[3] = {0,0,0};

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

	_regionData = region;

	return(_WriteRegion(region, min, max,1));
}

int	WaveletBlock2DRegionWriter::WriteRegion(
	const float *region
) {

	SetDiagMsg( "WaveletBlock2DRegionWriter::WriteRegion()" );

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

	return(WriteRegion(region, min, max));
}


int	WaveletBlock2DRegionWriter::BlockWriteRegion(
	const float *region,
	const size_t bmin[2],
	const size_t bmax[2],
	int block
) {
	SetDiagMsg(
		"WaveletBlock2DRegionWriter::BlockWriteRegion((%d,%d),%d,%d))",
		bmin[0], bmin[1], bmax[0], bmax[1]
	);

	size_t bmin3d[3], bmax3d[3];
	size_t bs[2];
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

	size_t min[2];
	size_t max[2];

	// Map block region coordinates to  voxels
	for(int i=0; i<2; i++) min[i] = bs[i]*bmin[i];
	for(int i=0; i<2; i++) max[i] = (bs[i]*(bmax[i]+1))-1;

	return(_WriteRegion(region, min, max, block));
}



void WaveletBlock2DRegionWriter::brickit(
	const float *srcptr,
	const size_t bs[2],
	size_t nx,
	size_t ny, 
	size_t srcx,
	size_t srcy, 
	size_t dstx,
	size_t dsty, 
	float *brickptr
) {
	const float *src;
	float *dst;

	assert (dstx < bs[0]);
	assert (dsty < bs[1]);

	int nxx = bs[0] - dstx;
	int nyy = bs[1] - dsty;
	if (nxx > nx - srcx) nxx = nx - srcx;
	if (nyy > ny - srcy) nyy = ny - srcy;

	for (int y=dsty, yy=srcy; y<dsty+nyy; y++, yy++) {

		src = srcptr + yy*nx + srcx;
		dst = brickptr + y*bs[0] + dstx;
		for (int x=dstx; x<dstx+nxx; x++) {

			*dst++ = *src++;

		}
	}
}

void WaveletBlock2DRegionWriter::brickit(
	const float *srcptr,
	const size_t bs[2],
	size_t nx,
	size_t ny, 
	size_t x,
	size_t y, 
	float *brickptr
) {

	srcptr += y*nx + x;

	for (int j=0; j<bs[1]; j++) {
		for (int i=0; i<bs[0]; i++) {
			*brickptr = *srcptr;
			brickptr++;
			srcptr++;

		}
		srcptr += nx - bs[0];
	}
}

void WaveletBlock2DRegionWriter::copy_top_superblock(
	const size_t bs[2],
	int srcx,
	int srcy,
	float *dst_super_blk
) {
	assert(srcx>=0);
	assert(srcy>=0);

	// intersection of region with superblock in voxel coordinates
	// relative to superblock-aligned enclosing region
	//
	int mincr[2], maxcr[2]; 

	// intersection of region with superblock in voxel coordinates
	// relative to the intersected superblock
	//
	int minsb[2], maxsb[2]; 

	// intersection of region with superblock in voxel coordinates relative
	// to region.
	int minreg[2]; 

	// default superblock bounds are the entire superblock
	for(int i=0; i<2; i++) {
		minsb[i] = 0;
		maxsb[i] = 2*bs[i] - 1;
	}

	
	

	// convert block coordinates to voxel coordinates
	//
	mincr[0] = srcx * bs[0];
	maxcr[0] = (srcx+2) * bs[0] - 1;
	mincr[1] = srcy * bs[1];
	maxcr[1] = (srcy+2) * bs[1] - 1;


	// Clamp coordinates to region bounds.
	//
	for(int i=0; i<2; i++) {
		if (mincr[i] < _regMin[i]) {
			minsb[i] += (_regMin[i] - mincr[i]); 
			mincr[i] = _regMin[i];
		}
		if (maxcr[i] > _regMax[i]) {
			maxsb[i] -= (maxcr[i] - _regMax[i]); 
			maxcr[i] = _regMax[i];
		}
		minreg[i] = mincr[i] - _regMin[i];
	}

	//
	// Copy data contained in intersection between superblock and region
	// to a padding block
	//
	int nx = _regMax[0] - _regMin[0] + 1;

	for(int y=minsb[1], yy=minreg[1]; y<=maxsb[1]; y++, yy++) {
	for(int x=minsb[0], xx=minreg[0]; x<=maxsb[0]; x++, xx++) {
		int i = y*bs[0]*2 + x;
		int ii = yy*nx + xx;

		_padblock[i] = _regionData[ii];
	}
	}

	// Pad 

	// First pad along X
	//
	if (minsb[0] > 0 || maxsb[0] < bs[0]*2-1) {
	int stride = 1;
	for (int y=minsb[1]; y<=maxsb[1]; y++) {
		float pad = _padblock[y*bs[0]*2 + minsb[0]];
		float *dstptr = _padblock +  y*bs[0]*2 + 0;

		for (int x=0; x<minsb[0]; x++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock[y*bs[0]*2 + maxsb[0]];
		dstptr = _padblock +  y*bs[0]*2 + maxsb[0]+1;

		for (int x=maxsb[0]+1; x<bs[0]*2 ; x++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}

	// Next pad along Y
	//
	if (minsb[1] > 0 || maxsb[1] < bs[1]*2-1) {
	int stride = bs[0]*2;
	for (int x=0; x<bs[0]*2; x++) {
		float pad = _padblock[minsb[1]*bs[0]*2 + x];
		float *dstptr = _padblock +  0 + x;

		for (int y=0; y<minsb[1]; y++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock[maxsb[1]*bs[0]*2 + x];
		dstptr = _padblock + (maxsb[1]+1)*bs[0]*2 + x;

		for (int y=maxsb[1]+1; y<bs[1]*2 ; y++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}


	// Finally, brick the data
	//
	size_t tile_size = bs[0]*bs[1];
	for(int y=0; y<2; y++) {
	for(int x=0; x<2; x++) {
		float *brickptr = dst_super_blk + (y*2 + x) * tile_size;
		brickit(
			_padblock, bs, bs[0]*2, bs[1]*2, 
			x*bs[0], y*bs[1],  brickptr
		);
	}
	}
}

int WaveletBlock2DRegionWriter::process_quadrant(
	size_t sz,
	int srcx,
	int srcy,
	int dstx,
	int dsty,
	int quad
) {
	int	rc;

	// Make sure quadrant is inside region. Hmm, this may not be right.
	// The quadrant may not be in the region, but it's ancestor (at 
	// a coarser level, covering more of the domain) probably is. This
	// quadrant should probably be padded as done elsewhere.
	//
	if (dstx > _volBMax[0] || dstx+sz <= _volBMin[0] ||
		dsty > _volBMax[1] || dsty+sz <= _volBMin[1]) {

		return(0);
	}

	WaveletBlock2D *wb2d;

	size_t bs[2];
	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0]; bs[1] = _bs[1];
		wb2d = _wb2dXY;
	break;
	case VAR2D_XZ:
		bs[0] = _bs[0]; bs[1] = _bs[2];
		wb2d = _wb2dXZ;
	break;
	case VAR2D_YZ:
		bs[0] = _bs[1]; bs[1] = _bs[2];
		wb2d = _wb2dYZ;
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	size_t tile_size = bs[0]*bs[1];

	// Recursively divide the input quadrant into another set of 4 
	// quadrants
	//
	if (sz > 2) {
		size_t h = sz>>1;

		rc = process_quadrant(h, srcx,srcy, dstx,dsty,0);
		if (rc<0) return(rc);

		rc = process_quadrant(h, srcx+h,srcy, dstx+h,dsty,1);
		if (rc<0) return(rc);

		rc = process_quadrant(h, srcx,srcy+h, dstx,dsty+h,2);
		if (rc<0) return(rc);

		rc = process_quadrant(h, srcx+h,srcy+h, dstx+h,dsty+h,3);
		if (rc<0) return(rc);

	}

	// finest level
	if (sz==2) {

		// Blocks input data and stores in _lambda_tiles[_num_reflevels-1]
		copy_top_superblock(bs, srcx, srcy, _lambda_tiles[_num_reflevels-1]);

		// Compute block min/max
		
		for(int y=0; y<2; y++) {
		for(int x=0; x<2; x++) {
			compute_minmax(
				_lambda_tiles[_num_reflevels-1] + (y*2+x)*tile_size, 
				dstx+x, dsty+y, _num_reflevels-1
			);
		}
		}
	}

	// Figure out what refinement level we're processing based on sz.
	// (j is the destination's level, not the source)
	int l = 0;
	for (unsigned int ui = sz; ui > 2; ui = ui>>1, l++);

	int j =  (_num_reflevels - 1) - l;

	const float *blkptr = _lambda_tiles[j];
	const float *src_super_blk[4];
	float *dst_super_blk[4];

	src_super_blk[0] = blkptr;
	src_super_blk[1] = blkptr + tile_size;
	src_super_blk[2] = blkptr + (tile_size * 2);
	src_super_blk[3] = blkptr + (tile_size * 3);

	// New lambda coefficients stored at appropriate quadrant
	//
	dst_super_blk[0] = _lambda_tiles[j-1] + quad * tile_size;
	for(int i=1; i<4; i++) {
		dst_super_blk[i] = _super_tile + (tile_size * (i-1));
	}

	wb2d->ForwardTransform(src_super_blk,dst_super_blk);

	// only save gamma coefficients for requested levels
	//
	if (j <= _reflevel) {
		int ldelta = _num_reflevels-1 - j + 1;

		const size_t bcoord[] = {dstx>>ldelta, dsty>>ldelta};
		rc = seekGammaBlocks(bcoord, j);
		if (rc<0) return(-1);
		rc = writeGammaBlocks(_super_tile, 1, j);
		if (rc<0) return(-1);
	}

	if (j-1==0) {
		int ldelta = _num_reflevels-1;
		const size_t bcoord[2] = {dstx>>ldelta, dsty>>ldelta};

		rc = seekLambdaBlocks(bcoord);
		if (rc<0) return(-1);
		rc = writeLambdaBlocks(_lambda_tiles[j-1], 1);
		if (rc<0) return (rc);
	}

	return(0);
}

void WaveletBlock2DRegionWriter::compute_minmax(
	const float *blockptr,
	size_t bx,
	size_t by,
	int	l
) {
	// Make sure quadrant is inside region
	//
	if (bx > _volBMax[0] || by > _volBMax[1] || 
		bx < _volBMin[0] || by < _volBMin[1] ) {
		return;
	}

	size_t bs[2] = {0,0};
	switch (_vtype) {
	case VAR2D_XY:
		bs[0] = _bs[0]; bs[1] = _bs[1];
	break;
	case VAR2D_XZ:
		bs[0] = _bs[0]; bs[1] = _bs[2];
	break;
	case VAR2D_YZ:
		bs[0] = _bs[1]; bs[1] = _bs[2];
	break;
	default:
	break;
	}

	const float *fptr = blockptr;
	size_t blkidx = by*_bdim[0] + bx;
	_mins[l][blkidx] = _maxs[l][blkidx] = *fptr;

	for(int y=0; y<bs[1]; y++) {
	for(int x=0; x<bs[0]; x++) {

		// block min/max
		if (*fptr < _mins[l][blkidx]) _mins[l][blkidx] = *fptr;
		if (*fptr > _maxs[l][blkidx]) _maxs[l][blkidx] = *fptr;

		// global (volume) min/max
		if (*fptr < _dataRange[0]) _dataRange[0] = *fptr;
		if (*fptr > _dataRange[1]) _dataRange[1] = *fptr;
		fptr++;
	}
	}
}



int	WaveletBlock2DRegionWriter::my_alloc(
) {

	// Maximum size of a tile regardless of type (XY, XZ, YZ)
	size_t max_tile_sz = max(_bs[0],_bs[1]) * max(_bs[2], min(_bs[0],_bs[1]));


	size_t size = max_tile_sz * 2 * 2;

	// alloc space from coarsest (j==0) to finest level
	for(int j=0; j<_num_reflevels; j++) {

		// allocate enough space for a superblock

		_lambda_tiles[j] = new(nothrow) float[size];
		if (! _lambda_tiles[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

	}

	_padblock = new(nothrow) float[size];
	if (! _padblock) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}
	return(0);
}

void	WaveletBlock2DRegionWriter::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_lambda_tiles[j]) delete [] _lambda_tiles[j];

		_lambda_tiles[j] = NULL;
	}
	if (_padblock) delete [] _padblock;
}
