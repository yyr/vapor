//
//	$Id$
//
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "vapor/WaveletBlock3DRegionWriter.h"

using namespace VetsUtil;
using namespace VAPoR;

int	WaveletBlock3DRegionWriter::_WaveletBlock3DRegionWriter()
{
	int	j;

	SetClassName("WaveletBlock3DRegionWriter");

	_xform_timer = 0.0;
	_is_open = 0;

	for(j=0; j<MAX_LEVELS; j++) {
		_lambda_blks[j] = NULL;
	}
	_padblock = NULL;

	return(my_alloc());
}

WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter(
	const Metadata *metadata,
	unsigned int	nthreads
) : WaveletBlock3DIO(metadata, nthreads) {

	_objInitialized = 0;
	if (WaveletBlock3DRegionWriter::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter()");

	if (_WaveletBlock3DRegionWriter() < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter(
	const char *metafile,
	unsigned int	nthreads
) : WaveletBlock3DIO(metafile, nthreads) {

	_objInitialized = 0;
	if (WaveletBlock3DRegionWriter::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter(%s)",
		metafile
	);

	if (_WaveletBlock3DRegionWriter() < 0) return;

	_objInitialized = 1;
}

WaveletBlock3DRegionWriter::~WaveletBlock3DRegionWriter(
) {

	SetDiagMsg("WaveletBlock3DRegionWriter::~WaveletBlock3DRegionWriter()");
	if (! _objInitialized) return;

	WaveletBlock3DRegionWriter::CloseVariable();

	my_free();

	_objInitialized = 0;
}

int	WaveletBlock3DRegionWriter::OpenVariableWrite(
	size_t	timestep,
	const char	*varname,
	int reflevel
) {
	int	rc;

	SetDiagMsg(
		"WaveletBlock3DRegionWriter::OpenVariableWrite(%d, %s, %d)",
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = _num_reflevels - 1;

	WaveletBlock3DRegionWriter::CloseVariable();	// close any previously opened files.
	rc = WaveletBlock3DIO::OpenVariableWrite(timestep, varname, reflevel);
	if (rc<0) return(rc);

	_is_open = 1;

	return(0);
}

int	WaveletBlock3DRegionWriter::CloseVariable(
) {
	SetDiagMsg("WaveletBlock3DRegionWriter::CloseVariable()");

	if (! _is_open) return(0);

	//
	// We've already computed the block min & max at the native refinement
	// level. Now go back and compute the block min & max at refinement 
	// levels [0.._num_reflevels-2]
	//
	// N.B. this code computes min/max across entire volume. Really only
	// need to do subvolume...
	//
	for (int l=_num_reflevels-2; l>=0; l--) {
		size_t bdim[3];		// block dim at level l
		size_t bdimlp1[3];	// block dim at level l+1
		int blkidx;
		int blkidxlp1;

		GetDimBlk(bdim, l);
		GetDimBlk(bdimlp1, l+1);

		for(int z=0; z<bdim[2]; z++) {
		for(int y=0; y<bdim[1]; y++) {
		for(int x=0; x<bdim[0]; x++) {

			blkidx = z * bdim[1] * bdim[0] + y * bdim[0] + x;
			blkidxlp1 = (z<<1) * bdimlp1[1] * bdimlp1[0] + (y<<1) * bdimlp1[0] + (x<<1);
			_mins[l][blkidx] = _mins[l+1][blkidxlp1];
			_maxs[l][blkidx] = _maxs[l+1][blkidxlp1];


			for(int zz=z<<1; zz < (z<<1)+2 && zz<bdimlp1[2]; zz++) {
			for(int yy=y<<1; yy < (y<<1)+2 && yy<bdimlp1[1]; yy++) {
			for(int xx=x<<1; xx < (x<<1)+2 && xx<bdimlp1[0]; xx++) {
				blkidxlp1 = zz * bdimlp1[1] * bdimlp1[0] + yy * bdimlp1[0] + xx;

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
		}
	}

	_is_open = 0;
	return(WaveletBlock3DIO::CloseVariable());
}

int	WaveletBlock3DRegionWriter::_WriteUntransformedRegion(
	const size_t min[3],
	const size_t max[3],
	const float *region,
	int block
) {
	assert(block == 1);

	// bounds (in voxels) relative to smallest block-aligned 
	// enclosing region
	//
	size_t rmin[3];
	size_t rmax[3];	

	size_t regbsize[3];	// Size of region in blocks

	// Compute bounds (in voxels) relative to the smallest 
	// block-aligned enclosing region.
	//
	for (int i=0; i<3; i++) {
		rmin[i] = min[i] - (_bs[i] * (min[i] / _bs[i]));
		rmax[i] = max[i] - (min[i] - rmin[i]);

		// Compute size, in blocks, of the block-aligned 
		// enclosing region
		//
		regbsize[i] = rmax[i] / _bs[i] + 1;
	}


	//
	// Block the data, one block at a time
	//
	for (int z=0; z<regbsize[2]; z++) {
	for (int y=0; y<regbsize[1]; y++) {
	for (int x=0; x<regbsize[0]; x++) {
		size_t srcx, srcy, srcz;
		size_t dstx, dsty, dstz;

		srcx = x*_bs[0] - rmin[0];
		dstx = 0;
		srcy = y*_bs[1] - rmin[1];
		dsty = 0;
		srcz = z*_bs[2] - rmin[2];
		dstz = 0;

		// if a boundary block
		//
		if ((x==0 && rmin[0] != 0) ||
			(y==0 && rmin[1] != 0) ||
			(z==0 && rmin[2] != 0) ||
			(x==regbsize[0]-1 && ((rmax[0]+1) % _bs[0])) ||
			(y==regbsize[1]-1 && ((rmax[1]+1) % _bs[1])) ||
			(z==regbsize[2]-1 && ((rmax[2]+1) % _bs[2]))) {

			if (x==0) {
				srcx = 0;
				dstx = rmin[0];
			}
			if (y==0) {
				srcy = 0;
				dsty = rmin[1];
			}
			if (z==0) {
				srcz = 0;
				dstz = rmin[2];
			}

			brickit(
				region, max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1,
				srcx, srcy, srcz, dstx, dsty, dstz, _lambda_blks[0]
			);

		}
		else {

			// Interior of region (or a boundary piece that is already
			// aligned with the block-aligned enclosing region
			//
			brickit(
				region, max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1,
				srcx, srcy, srcz, _lambda_blks[0]
			);

		}

		compute_minmax(
			_lambda_blks[0], _volBMin[0]+x, _volBMin[1]+y, _volBMin[2]+z, 0
		);

		int rc;
		const size_t bcoord[3] = {_volBMin[0]+x, _volBMin[1]+y, _volBMin[2]+z};
		rc = seekLambdaBlocks(bcoord);
		if (rc < 0) return (rc);

		rc = writeLambdaBlocks(_lambda_blks[0], 1);
		if (rc < 0) return (rc);

	}
	}
	}

	return(0);

}

int	WaveletBlock3DRegionWriter::_WriteRegion(
	const float *region,
	const size_t min[3],
	const size_t max[3],
	int block
) {

	size_t octmin[3], octmax[3];	// bounds (in blocks) of
									// superblock enclosing subregion at
									// coarsest level, relative to
									// global volume at finest level
	int regstart[3];

	// initialize data range calculation
	//
	_dataRange[0] = _dataRange[1] = *region;

	for (int i=0; i<3; i++) {
		_validRegMin[i] = min[i];
		_validRegMax[i] = max[i];
	}

	// Bounds (in blocks) of region relative to global volume
	MapVoxToBlk(min, _volBMin);
	MapVoxToBlk(max, _volBMax);

	// handle case where there is no transform
	//
	if ((_num_reflevels-1) <= 0) {
		return(_WriteUntransformedRegion(min, max, region, block));
	}

	for (int i=0; i<3; i++) {
		// Compute bounds, in voxels, relative to the smallest 
		// superblock-aligned enclosing region.
		//
		_regMin[i] = min[i] - ((2*_bs[i]) * (min[i] / (2*_bs[i])));
		_regMax[i] = max[i] - (min[i] - _regMin[i]);

		// Compute bounds (in blocks), relative to global volume, of 
		// level 0 super-block-aligned volume that encloses the region.
		//
		if (IsOdd(_volBMin[i])) {	// not superblock aligned yet
			size_t tmp = (_volBMin[i]-1) >> _num_reflevels-1;
			octmin[i] = tmp * (1 << _num_reflevels-1);
			regstart[i] = octmin[i] - (_volBMin[i]-1);
		}
		else {
			size_t tmp = _volBMin[i] >> _num_reflevels-1;
			octmin[i] = tmp * (1 << _num_reflevels-1);
			regstart[i] = octmin[i] - _volBMin[i];
		}

		if (IsOdd(_volBMax[i])) {
			size_t tmp = (_volBMax[i]+1) >> _num_reflevels-1;
			octmax[i] = (tmp+1) * (1 << _num_reflevels-1) - 1;
		}
		else {
			size_t tmp = _volBMax[i] >> _num_reflevels-1;
			octmax[i] = (tmp+1) * (1 << _num_reflevels-1) - 1;
		}

	}


	int sz = 1<<(_num_reflevels-1);
																			
	for(int z=regstart[2], zz=octmin[2]; zz<=octmax[2]; z+=sz, zz+=sz) {
	for(int y=regstart[1], yy=octmin[1]; yy<=octmax[1]; y+=sz, yy+=sz) {
	for(int x=regstart[0], xx=octmin[0]; xx<=octmax[0]; x+=sz, xx+=sz) {
		int rc;
		rc = process_octant(sz,x,y,z, xx,yy,zz, 0);
		if (rc < 0) return(rc);
	}
	}
	}

	return(0);

}

int	WaveletBlock3DRegionWriter::WriteRegion(
	const float *region,
	const size_t min[3],
	const size_t max[3]
) {

	SetDiagMsg(
		"WaveletBlock3DRegionWriter::WriteRegion((%d,%d,%d),(%d,%d,%d))",
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	if (! IsValidRegion(min, max, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min[0], min[1], min[2], max[0], max[1], max[2]
		);
		return(-1);
	}

	_regionData = region;

	return(_WriteRegion(region, min, max, 1));
}


int	WaveletBlock3DRegionWriter::BlockWriteRegion(
	const float *region,
	const size_t bmin[3],
	const size_t bmax[3],
	int block
) {
	SetDiagMsg(
		"WaveletBlock3DRegionWriter::BlockWriteRegion((%d,%d,%d),(%d,%d,%d))",
		bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
	);

	if (! IsValidRegionBlk(bmin, bmax, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}

	size_t min[3];
	size_t max[3];

	// Map block region coordinates to  voxels
	for(int i=0; i<3; i++) min[i] = _bs[i]*bmin[i];
	for(int i=0; i<3; i++) max[i] = (_bs[i]*(bmax[i]+1))-1;

	return(_WriteRegion(region, min, max, block));
}



void WaveletBlock3DRegionWriter::brickit(
	const float *srcptr,
	size_t nx,
	size_t ny, 
	size_t nz,
	size_t srcx,
	size_t srcy, 
	size_t srcz,
	size_t dstx,
	size_t dsty, 
	size_t dstz,
	float *brickptr
) {
	const float *src;
	float *dst;

	assert (dstx < _bs[0]);
	assert (dsty < _bs[1]);
	assert (dstz < _bs[2]);

	int nxx = _bs[0] - dstx;
	int nyy = _bs[1] - dsty;
	int nzz = _bs[2] - dstz;
	if (nxx > nx - srcx) nxx = nx - srcx;
	if (nyy > ny - srcy) nyy = ny - srcy;
	if (nzz > nz - srcz) nzz = nz - srcz;

	for (int z=dstz, zz=srcz; z<dstz+nzz; z++, zz++) {
		for (int y=dsty, yy=srcy; y<dsty+nyy; y++, yy++) {

			src = srcptr + zz*nx*ny + yy*nx + srcx;
			dst = brickptr + z*_bs[0]*_bs[1] + y*_bs[0] + dstx;
			for (int x=dstx; x<dstx+nxx; x++) {

				*dst++ = *src++;

			}
		}
	}
}

void WaveletBlock3DRegionWriter::brickit(
	const float *srcptr,
	size_t nx,
	size_t ny, 
	size_t nz,
	size_t x,
	size_t y, 
	size_t z,
	float *brickptr
) {

	srcptr += z*nx*ny + y*nx + x;

	for (int k=0; k<_bs[2]; k++) {
		for (int j=0; j<_bs[1]; j++) {
			for (int i=0; i<_bs[0]; i++) {
				*brickptr = *srcptr;
				brickptr++;
				srcptr++;

			}
			srcptr += nx - _bs[0];
		}
		srcptr += nx*ny - nx*_bs[1];
	}
}

void WaveletBlock3DRegionWriter::copy_top_superblock(
	int srcx,
	int srcy,
	int srcz,
	float *dst_super_blk
) {
	assert(srcx>=0);
	assert(srcy>=0);
	assert(srcz>=0);
#ifdef	DEAD
	// Ugh. hack to deal with two different coordinate spaces
	//
	if (srcx<0) srcx *= -1;
	if (srcy<0) srcy *= -1;
	if (srcz<0) srcz *= -1;
#endif

	// intersection of region with superblock in voxel coordinates
	// relative to superblock-aligned enclosing region
	//
	int mincr[3], maxcr[3]; 

	// intersection of region with superblock in voxel coordinates
	// relative to the intersected superblock
	//
	int minsb[3], maxsb[3]; 

	// intersection of region with superblock in voxel coordinates relative
	// to region.
	int minreg[3]; 

	// default superblock bounds are the entire superblock
	for(int i=0; i<3; i++) {
		minsb[i] = 0;
		maxsb[i] = 2*_bs[i] - 1;
	}

	
	

	// convert block coordinates to voxel coordinates
	//
	mincr[0] = srcx * _bs[0];
	maxcr[0] = (srcx+2) * _bs[0] - 1;
	mincr[1] = srcy * _bs[1];
	maxcr[1] = (srcy+2) * _bs[1] - 1;
	mincr[2] = srcz * _bs[2];
	maxcr[2] = (srcz+2) * _bs[2] - 1;


	// Clamp coordinates to region bounds.
	//
	for(int i=0; i<3; i++) {
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
	for(int z=minsb[2], zz=minreg[2]; z<=maxsb[2]; z++, zz++) {
		int nx = _regMax[0] - _regMin[0] + 1;
		int ny = _regMax[1] - _regMin[1] + 1;

		for(int y=minsb[1], yy=minreg[1]; y<=maxsb[1]; y++, yy++) {
		for(int x=minsb[0], xx=minreg[0]; x<=maxsb[0]; x++, xx++) {
			int i = z*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + x;
			int ii = zz*nx*ny + yy*nx + xx;

			_padblock[i] = _regionData[ii];
		}
		}
	}

	// Pad 

	// First pad along X
	//
	if (minsb[0] > 0 || maxsb[0] < _bs[0]*2-1) {
	int stride = 1;
	for (int z=minsb[2]; z<=maxsb[2]; z++) {
	for (int y=minsb[1]; y<=maxsb[1]; y++) {
		float pad = _padblock[z*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + minsb[0]];
		float *dstptr = _padblock + z*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + 0;

		for (int x=0; x<minsb[0]; x++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock[z*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + maxsb[0]];
		dstptr = _padblock + z*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + maxsb[0]+1;

		for (int x=maxsb[0]+1; x<_bs[0]*2 ; x++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}
	}

	// Next pad along Y
	//
	if (minsb[1] > 0 || maxsb[1] < _bs[1]*2-1) {
	int stride = _bs[0]*2;
	for (int z=minsb[2]; z<=maxsb[2]; z++) {
	for (int x=0; x<_bs[0]*2; x++) {
		float pad = _padblock[z*_bs[0]*_bs[1]*4 + minsb[1]*_bs[0]*2 + x];
		float *dstptr = _padblock + z*_bs[0]*_bs[1]*4 + 0 + x;

		for (int y=0; y<minsb[1]; y++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock[z*_bs[0]*_bs[1]*4 + maxsb[1]*_bs[0]*2 + x];
		dstptr = _padblock + z*_bs[0]*_bs[1]*4 + (maxsb[1]+1)*_bs[0]*2 + x;

		for (int y=maxsb[1]+1; y<_bs[1]*2 ; y++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}
	}

	// Next pad along Z
	//
	if (minsb[2] > 0 || maxsb[2] < _bs[2]*2-1) {
	int stride = _bs[0]*_bs[0]*4;
	for (int y=0; y<_bs[1]*2; y++) {
	for (int x=0; x<_bs[0]*2; x++) {
		float pad = _padblock[minsb[2]*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + x];
		float *dstptr = _padblock + 0 + y*_bs[0]*2 + x;

		for (int z=0; z<minsb[2]; z++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock[maxsb[2]*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + x];
		dstptr = _padblock + (maxsb[2]+1)*_bs[0]*_bs[1]*4 + y*_bs[0]*2 + x;
		for (int z=maxsb[2]+1; z<_bs[2]*2 ; z++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}
	}


	// Finally, brick the data
	//
	for(int z=0; z<2; z++) {
	for(int y=0; y<2; y++) {
	for(int x=0; x<2; x++) {
		float *brickptr = dst_super_blk + (z*4 + y*2 + x) * _block_size;
		brickit(
			_padblock, _bs[0]*2, _bs[1]*2, _bs[2]*2, 
			x*_bs[0], y*_bs[1], z*_bs[2], brickptr
		);
	}
	}
	}
}

int WaveletBlock3DRegionWriter::process_octant(
	size_t sz,
	int srcx,
	int srcy,
	int srcz,
	int dstx,
	int dsty,
	int dstz,
	int oct
) {
	int	rc;

	// Make sure octant is inside region. Hmm, this may not be right.
	// The octant may not be in the region, but it's ancestor (at 
	// a coarser level, covering more of the domain) probably is. This
	// octant should probably be padded as done elsewhere.
	//
	if (dstx > _volBMax[0] || dstx+sz <= _volBMin[0] ||
		dsty > _volBMax[1] || dsty+sz <= _volBMin[1] ||
		dstz > _volBMax[2] || dstz+sz <= _volBMin[2]) { 

		return(0);
	}

	// Recursively divide the input octant into another set of 8 
	// octants
	//
	if (sz > 2) {
		size_t h = sz>>1;

		rc = process_octant(h, srcx,srcy,srcz, dstx,dsty,dstz,0);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx+h,srcy,srcz, dstx+h,dsty,dstz,1);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx,srcy+h,srcz, dstx,dsty+h,dstz,2);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx+h,srcy+h,srcz, dstx+h,dsty+h,dstz,3);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx,srcy,srcz+h, dstx,dsty,dstz+h,4);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx+h,srcy,srcz+h, dstx+h,dsty,dstz+h,5);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx,srcy+h,srcz+h, dstx,dsty+h,dstz+h,6);
		if (rc<0) return(rc);

		rc = process_octant(h, srcx+h,srcy+h,srcz+h, dstx+h,dsty+h,dstz+h,7);
		if (rc<0) return(rc);
	}

	// finest level
	if (sz==2) {

		// Blocks input data and stores in _lambda_blks[_num_reflevels-1]
		copy_top_superblock(srcx, srcy, srcz, _lambda_blks[_num_reflevels-1]);

		// Compute block min/max
		
		for(int z=0; z<2; z++) {
		for(int y=0; y<2; y++) {
		for(int x=0; x<2; x++) {
			compute_minmax(
				_lambda_blks[_num_reflevels-1] + (z*4+y*2+x)*_block_size, 
				dstx+x, dsty+y, dstz+z, _num_reflevels-1
			);
		}
		}
		}
	}

	// Figure out what refinement level we're processing based on sz.
	// (j is the destination's level, not the source)
	int l = 0;
	for (unsigned int ui = sz; ui > 2; ui = ui>>1, l++);

	int j =  (_num_reflevels - 1) - l;

	const float *blkptr = _lambda_blks[j];
	const float *src_super_blk[8];
	float *dst_super_blk[8];

	src_super_blk[0] = blkptr;
	src_super_blk[1] = blkptr + _block_size;
	src_super_blk[2] = blkptr + (_block_size * 2);
	src_super_blk[3] = blkptr + (_block_size * 3);
	blkptr += 4 * _block_size;
	src_super_blk[4] = blkptr;
	src_super_blk[5] = blkptr + _block_size;
	src_super_blk[6] = blkptr + (_block_size * 2);
	src_super_blk[7] = blkptr + (_block_size * 3);

	// New lambda coefficients stored at appropriate octant
	//
	dst_super_blk[0] = _lambda_blks[j-1] + oct * _block_size;
	for(int i=1; i<8; i++) {
		dst_super_blk[i] = super_block_c + (_block_size * (i-1));
	}

	wb3d_c->ForwardTransform(src_super_blk,dst_super_blk);

	// only save gamma coefficients for requested levels
	//
	if (j <= _reflevel) {
		int ldelta = _num_reflevels-1 - j + 1;

		const size_t bcoord[3] = {dstx>>ldelta, dsty>>ldelta, dstz>>ldelta};
		rc = seekGammaBlocks(bcoord, j);
		if (rc<0) return(-1);
		rc = writeGammaBlocks(super_block_c, 1, j);
		if (rc<0) return(-1);
	}

	if (j-1==0) {
		int ldelta = _num_reflevels-1;
		const size_t bcoord[3] = {dstx>>ldelta, dsty>>ldelta, dstz>>ldelta};

		rc = seekLambdaBlocks(bcoord);
		if (rc<0) return(-1);
		rc = writeLambdaBlocks(_lambda_blks[j-1], 1);
		if (rc<0) return (rc);
	}

	return(0);
}

void WaveletBlock3DRegionWriter::compute_minmax(
	const float *blockptr,
	size_t bx,
	size_t by,
	size_t bz,
	int	l
) {
	// Make sure octant is inside region
	//
	if (bx > _volBMax[0] || by > _volBMax[1] || bz > _volBMax[2] ||
		bx < _volBMin[0] || by < _volBMin[1] || bz < _volBMin[2]) {
		return;
	}

	const float *fptr = blockptr;
	size_t blkidx = bz*_bdim[0]*_bdim[1] + by*_bdim[0] + bx;
	_mins[l][blkidx] = _maxs[l][blkidx] = *fptr;

	for(int z=0; z<_bs[2]; z++) {
	for(int y=0; y<_bs[1]; y++) {
	for(int x=0; x<_bs[0]; x++) {

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
}



int	WaveletBlock3DRegionWriter::my_alloc(
) {


	size_t size = _block_size * 2 * 2 * 2;

	// alloc space from coarsest (j==0) to finest level
	for(int j=0; j<_num_reflevels; j++) {

		// allocate enough space for a superblock

		_lambda_blks[j] = new(nothrow) float[size];
		if (! _lambda_blks[j]) {
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

void	WaveletBlock3DRegionWriter::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (_lambda_blks[j]) delete [] _lambda_blks[j];

		_lambda_blks[j] = NULL;
	}
	if (_padblock) delete [] _padblock;
}
