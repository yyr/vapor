//
//	$Id$
//
#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include <vapor/WaveletBlock3DRegionWriter.h>
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

int	WaveletBlock3DRegionWriter::_WaveletBlock3DRegionWriter()
{
	int	j;

	SetClassName("WaveletBlock3DRegionWriter");

	_is_open = 0;

	const size_t *bs = GetBlockSize();
	_block_size = bs[0]*bs[1]*bs[2];

	for(j=0; j<MAX_LEVELS; j++) {
		_lambda_blks[j] = NULL;
		_lambda_tiles[j] = NULL;
	}
	_padblock3d = NULL;
	_padblock2d = NULL;

	return(0);
}

WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter(
	const MetadataVDC &metadata
) : WaveletBlockIOBase(metadata) {

	if (WaveletBlock3DRegionWriter::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter()");

	if (_WaveletBlock3DRegionWriter() < 0) return;
}

WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter(
	const string &metafile
) : WaveletBlockIOBase(metafile) {

	if (WaveletBlock3DRegionWriter::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DRegionWriter::WaveletBlock3DRegionWriter(%s)",
		metafile.c_str()
	);

	if (_WaveletBlock3DRegionWriter() < 0) return;

}

WaveletBlock3DRegionWriter::~WaveletBlock3DRegionWriter(
) {

	SetDiagMsg("WaveletBlock3DRegionWriter::~WaveletBlock3DRegionWriter()");

	WaveletBlock3DRegionWriter::CloseVariable();

	my_free();

}

int	WaveletBlock3DRegionWriter::OpenVariableWrite(
	size_t	timestep,
	const char	*varname,
	int reflevel,
	int 
) {
	int	rc;

	SetDiagMsg(
		"WaveletBlock3DRegionWriter::OpenVariableWrite(%d, %s, %d)",
		timestep, varname, reflevel
	);

	_firstWrite = true;

	if (reflevel < 0) reflevel = GetNumTransforms();

	WaveletBlock3DRegionWriter::CloseVariable();	// close any previously opened files.
	rc = WaveletBlockIOBase::OpenVariableWrite(timestep, varname, reflevel);
	if (rc<0) return(rc);

	if (_vtype == VAR3D) {
		if (my_alloc3d() < 0) return(-1);
	}
	else {
		if (my_alloc2d() < 0) return(-1);
	}

	_is_open = 1;

	return(0);
}

void	WaveletBlock3DRegionWriter::_CloseVariable3D()
{
	//
	// We've already computed the block min & max at the native refinement
	// level. Now go back and compute the block min & max at refinement 
	// levels [0..GetNumTransforms()-1]
	//
	// N.B. this code computes min/max across entire volume. Really only
	// need to do subvolume...
	//
	for (int l=GetNumTransforms()-1; l>=0; l--) {
		size_t bdim[3];		// block dim at level l
		size_t bdimlp1[3];	// block dim at level l+1
		int blkidx;
		int blkidxlp1;

		WaveletBlockIOBase::GetDimBlk(bdim, l);
		WaveletBlockIOBase::GetDimBlk(bdimlp1, l+1);

		for(int z=0; z<bdim[2]; z++) {
		for(int y=0; y<bdim[1]; y++) {
		for(int x=0; x<bdim[0]; x++) {

			blkidx = z * bdim[1] * bdim[0] + y * bdim[0] + x;
			blkidxlp1 = (z<<1) * bdimlp1[1] * bdimlp1[0] + (y<<1) * bdimlp1[0] + (x<<1);
			_mins3d[l][blkidx] = _mins3d[l+1][blkidxlp1];
			_maxs3d[l][blkidx] = _maxs3d[l+1][blkidxlp1];


			for(int zz=z<<1; zz < (z<<1)+2 && zz<bdimlp1[2]; zz++) {
			for(int yy=y<<1; yy < (y<<1)+2 && yy<bdimlp1[1]; yy++) {
			for(int xx=x<<1; xx < (x<<1)+2 && xx<bdimlp1[0]; xx++) {
				blkidxlp1 = zz * bdimlp1[1] * bdimlp1[0] + yy * bdimlp1[0] + xx;

				if (_mins3d[l+1][blkidxlp1] < _mins3d[l][blkidx]) {
					_mins3d[l][blkidx] = _mins3d[l+1][blkidxlp1];
				}
				if (_maxs3d[l+1][blkidxlp1] > _maxs3d[l][blkidx]) {
					_maxs3d[l][blkidx] = _maxs3d[l+1][blkidxlp1];
				}
			}
			}
			}

		}
		}
		}
	}
}

void	WaveletBlock3DRegionWriter::_CloseVariable2D()
{

	//
	// We've already computed the block min & max at the native refinement
	// level. Now go back and compute the block min & max at refinement 
	// levels [0..GetNumTransforms()-1]
	//
	// N.B. this code computes min/max across entire area. Really only
	// need to do subarea...
	//
	for (int l=GetNumTransforms()-1; l>=0; l--) {
		size_t bdim3d[3];		// block dim at level l
		size_t bdimlp13d[3];	// block dim at level l+1
		size_t bdim[2];		// 2d block dim at level l
		size_t bdimlp1[2];	// 2d block dim at level l+1
		int blkidx;
		int blkidxlp1;

		WaveletBlockIOBase::GetDimBlk(bdim3d, l);
		WaveletBlockIOBase::GetDimBlk(bdimlp13d, l+1);

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
			return;
		}

		for(int y=0; y<bdim[1]; y++) {
		for(int x=0; x<bdim[0]; x++) {

			blkidx = y * bdim[0] + x;
			blkidxlp1 = (y<<1) * bdimlp1[0] + (x<<1);
			_mins2d[l][blkidx] = _mins2d[l+1][blkidxlp1];
			_maxs2d[l][blkidx] = _maxs2d[l+1][blkidxlp1];


			for(int yy=y<<1; yy < (y<<1)+2 && yy<bdimlp1[1]; yy++) {
			for(int xx=x<<1; xx < (x<<1)+2 && xx<bdimlp1[0]; xx++) {
				blkidxlp1 = yy * bdimlp1[0] + xx;

				if (_mins2d[l+1][blkidxlp1] < _mins2d[l][blkidx]) {
					_mins2d[l][blkidx] = _mins2d[l+1][blkidxlp1];
				}
				if (_maxs2d[l+1][blkidxlp1] > _maxs2d[l][blkidx]) {
					_maxs2d[l][blkidx] = _maxs2d[l+1][blkidxlp1];
				}
			}
			}

		}
		}
	}
}

int	WaveletBlock3DRegionWriter::CloseVariable(
) {
	SetDiagMsg("WaveletBlock3DRegionWriter::CloseVariable()");

	if (! _is_open) return(0);

	_is_open = 0;

	if (_vtype == VAR3D) {
		_CloseVariable3D();
	}
	else {
		_CloseVariable2D();
	}
	return(WaveletBlockIOBase::CloseVariable());
}

int	WaveletBlock3DRegionWriter::_WriteUntransformedRegion3D(
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
	const size_t *bs = GetBlockSize();
	for (int i=0; i<3; i++) {
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
	for (int z=0; z<regbsize[2]; z++) {
	for (int y=0; y<regbsize[1]; y++) {
	for (int x=0; x<regbsize[0]; x++) {
		size_t srcx, srcy, srcz;
		size_t dstx, dsty, dstz;

		srcx = x*bs[0] - rmin[0];
		dstx = 0;
		srcy = y*bs[1] - rmin[1];
		dsty = 0;
		srcz = z*bs[2] - rmin[2];
		dstz = 0;

		// if a boundary block
		//
		if ((x==0 && rmin[0] != 0) ||
			(y==0 && rmin[1] != 0) ||
			(z==0 && rmin[2] != 0) ||
			(x==regbsize[0]-1 && ((rmax[0]+1) % bs[0])) ||
			(y==regbsize[1]-1 && ((rmax[1]+1) % bs[1])) ||
			(z==regbsize[2]-1 && ((rmax[2]+1) % bs[2]))) {

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

			brickit3d(
				region, max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1,
				srcx, srcy, srcz, dstx, dsty, dstz, _lambda_blks[0]
			);

		}
		else {

			// Interior of region (or a boundary piece that is already
			// aligned with the block-aligned enclosing region
			//
			brickit3d(
				region, max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1,
				srcx, srcy, srcz, _lambda_blks[0]
			);

		}

		compute_minmax3d(
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

int	WaveletBlock3DRegionWriter::_WriteUntransformedRegion2D(
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

	const size_t *bs3d = GetBlockSize();

	//
	// Block the data, one block at a time
	//
	for (int y=0; y<regbsize[1]; y++) {
	for (int x=0; x<regbsize[0]; x++) {
		size_t srcx, srcy;
		size_t dstx, dsty;

		srcx = x*bs3d[0] - rmin[0];
		dstx = 0;
		srcy = y*bs3d[1] - rmin[1];
		dsty = 0;

		// if a boundary block
		//
		if ((x==0 && rmin[0] != 0) ||
			(y==0 && rmin[1] != 0) ||
			(x==regbsize[0]-1 && ((rmax[0]+1) % bs3d[0])) ||
			(y==regbsize[1]-1 && ((rmax[1]+1) % bs3d[1]))) {

			if (x==0) {
				srcx = 0;
				dstx = rmin[0];
			}
			if (y==0) {
				srcy = 0;
				dsty = rmin[1];
			}

			brickit2d(
				region, bs, max[0]-min[0]+1, max[1]-min[1]+1,
				srcx, srcy, dstx, dsty, _lambda_tiles[0]
			);

		}
		else {

			// Interior of region (or a boundary piece that is already
			// aligned with the block-aligned enclosing region
			//
			brickit2d(
				region, bs, max[0]-min[0]+1, max[1]-min[1]+1,
				srcx, srcy, _lambda_tiles[0]
			);

		}

		compute_minmax2d( _lambda_tiles[0], _volBMin[0]+x, _volBMin[1]+y, 0);

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

int	WaveletBlock3DRegionWriter::_WriteRegion3D(
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


	for (int i=0; i<3; i++) {
		_validRegMin[i] = min[i];
		_validRegMax[i] = max[i];
	}

	// Bounds (in blocks) of region relative to global volume
	VDFIOBase::MapVoxToBlk(min, _volBMin, _reflevel);
	VDFIOBase::MapVoxToBlk(max, _volBMax, _reflevel);

	// handle case where there is no transform
	//
	if ((GetNumTransforms()) <= 0) {
		return(_WriteUntransformedRegion3D(min, max, region, block));
	}

	const size_t *bs = GetBlockSize();

	for (int i=0; i<3; i++) {
		// Compute bounds, in voxels, relative to the smallest 
		// superblock-aligned enclosing region.
		//
		_regMin[i] = min[i] - ((2*bs[i]) * (min[i] / (2*bs[i])));
		_regMax[i] = max[i] - (min[i] - _regMin[i]);

		// Compute bounds (in blocks), relative to global volume, of 
		// level 0 super-block-aligned volume that encloses the region.
		//
		if (IsOdd(_volBMin[i])) {	// not superblock aligned yet
			size_t tmp = (_volBMin[i]-1) >> (GetNumTransforms());
			octmin[i] = tmp * (1 << (GetNumTransforms()));
			regstart[i] = octmin[i] - (_volBMin[i]-1);
		}
		else {
			size_t tmp = _volBMin[i] >> (GetNumTransforms());
			octmin[i] = tmp * (1 << (GetNumTransforms()));
			regstart[i] = octmin[i] - _volBMin[i];
		}

		if (IsOdd(_volBMax[i])) {
			size_t tmp = (_volBMax[i]+1) >> (GetNumTransforms());
			octmax[i] = (tmp+1) * (1 << (GetNumTransforms())) - 1;
		}
		else {
			size_t tmp = _volBMax[i] >> (GetNumTransforms());
			octmax[i] = (tmp+1) * (1 << (GetNumTransforms())) - 1;
		}

	}


	int sz = 1<<(GetNumTransforms());
																			
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

int	WaveletBlock3DRegionWriter::_WriteRegion2D(
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

	size_t bs2d[2];

	size_t min3d[] = {0,0,0};
    size_t max3d[] = {0,0,0};
	size_t volBMin3d[3], volBMax3d[3];
	const size_t *bs = GetBlockSize();

	size_t dim[3];
	WaveletBlockIOBase::GetDim(dim, -1);

	switch (_vtype) {
	case VAR2D_XY:
		bs2d[0] = bs[0]; bs2d[1] = bs[1];
		_validRegMin[0] = min[0];
		_validRegMax[0] = max[0];
		_validRegMin[1] = min[1];
		_validRegMax[1] = max[1];
		_validRegMin[2] = 0;
		_validRegMax[2] = dim[2]-1;

        min3d[0] = min[0]; min3d[1] = min[1];
        max3d[0] = max[0]; max3d[1] = max[1];
		// Bounds (in blocks) of region relative to global volume
		VDFIOBase::MapVoxToBlk(min3d, volBMin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, volBMax3d, _reflevel);
		_volBMin[0] = volBMin3d[0]; _volBMin[1] = volBMin3d[1];
		_volBMax[0] = volBMax3d[0]; _volBMax[1] = volBMax3d[1];
	break;
	case VAR2D_XZ:
		bs2d[0] = bs[0]; bs2d[1] = bs[2];
		_validRegMin[0] = min[0];
		_validRegMax[0] = max[0];
		_validRegMin[1] = 0;
		_validRegMax[1] = dim[1]-1;
		_validRegMin[2] = min[1];
		_validRegMax[2] = max[1];

        min3d[0] = min[0]; min3d[2] = min[1];
        max3d[0] = max[0]; max3d[2] = max[1];
		// Bounds (in blocks) of region relative to global volume
		VDFIOBase::MapVoxToBlk(min3d, volBMin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, volBMax3d, _reflevel);
		_volBMin[0] = volBMin3d[0]; _volBMin[1] = volBMin3d[2];
		_volBMax[0] = volBMax3d[0]; _volBMax[1] = volBMax3d[2];

	break;
	case VAR2D_YZ:
		bs2d[0] = bs[1]; bs2d[1] = bs[2];
		_validRegMin[0] = 0;
		_validRegMax[0] = dim[0]-1;
		_validRegMin[1] = min[0];
		_validRegMax[1] = max[0];
		_validRegMin[2] = min[1];
		_validRegMax[2] = max[1];
        min3d[1] = min[0]; min3d[2] = min[1];
        max3d[1] = max[0]; max3d[2] = max[1];
		// Bounds (in blocks) of region relative to global volume
		VDFIOBase::MapVoxToBlk(min3d, volBMin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, volBMax3d, _reflevel);
		_volBMin[0] = volBMin3d[1]; _volBMin[1] = volBMin3d[2];
		_volBMax[0] = volBMax3d[1]; _volBMax[1] = volBMax3d[2];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}



	// handle case where there is no transform
	//
	if ((GetNumTransforms()) <= 0) {
		return(_WriteUntransformedRegion2D(bs2d, min, max, region, block));
	}

	for (int i=0; i<2; i++) {
		// Compute bounds, in voxels, relative to the smallest 
		// superblock-aligned enclosing region.
		//
		_regMin[i] = min[i] - ((2*bs2d[i]) * (min[i] / (2*bs2d[i])));
		_regMax[i] = max[i] - (min[i] - _regMin[i]);

		// Compute bounds (in blocks), relative to global volume, of 
		// level 0 super-block-aligned volume that encloses the region.
		//
		if (IsOdd(_volBMin[i])) {	// not superblock aligned yet
			size_t tmp = (_volBMin[i]-1) >> (GetNumTransforms());
			quadmin[i] = tmp * (1 << (GetNumTransforms()));
			regstart[i] = quadmin[i] - (_volBMin[i]-1);
		}
		else {
			size_t tmp = _volBMin[i] >> (GetNumTransforms());
			quadmin[i] = tmp * (1 << (GetNumTransforms()));
			regstart[i] = quadmin[i] - _volBMin[i];
		}

		if (IsOdd(_volBMax[i])) {
			size_t tmp = (_volBMax[i]+1) >> (GetNumTransforms());
			quadmax[i] = (tmp+1) * (1 << (GetNumTransforms())) - 1;
		}
		else {
			size_t tmp = _volBMax[i] >> (GetNumTransforms());
			quadmax[i] = (tmp+1) * (1 << (GetNumTransforms())) - 1;
		}

	}


	int sz = 1<<(GetNumTransforms());
																			
	for(int y=regstart[1], yy=quadmin[1]; yy<=quadmax[1]; y+=sz, yy+=sz) {
	for(int x=regstart[0], xx=quadmin[0]; xx<=quadmax[0]; x+=sz, xx+=sz) {
		int rc;
		rc = process_quadrant(sz,x,y, xx,yy, 0);
		if (rc < 0) return(rc);
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

	if (_firstWrite) {
		_dataRange[0] = _dataRange[1] = *region;
	}
	_firstWrite = false;

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
	case VAR3D:
		min3d[0] = min[0]; min3d[1] = min[1]; min3d[2] = min[2];
		max3d[0] = max[0]; max3d[1] = max[1]; max3d[2] = max[2];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}


	if (! VDFIOBase::IsValidRegion(min3d, max3d, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min3d[0], min3d[1], min3d[2], max3d[0], max3d[1], max3d[2]
		);
		return(-1);
	}

	_regionData = region;

	if (_vtype == VAR3D) {
		return(_WriteRegion3D(region, min, max, 1));
	}
	else {
		return(_WriteRegion2D(region, min, max, 1));
	}
}



int	WaveletBlock3DRegionWriter::WriteRegion(
	const float *region
) {

	SetDiagMsg( "WaveletBlock3DRegionWriter::WriteRegion()" );

    size_t dim3d[3];
    WaveletBlockIOBase::GetDim(dim3d,_reflevel);

    size_t min[] = {0,0,0};
    size_t max[3];
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
    case VAR3D:
        max[0] = dim3d[0]-1; max[1] = dim3d[1]-1; max[2] = dim3d[2]-1;
    break;
    default:
        SetErrMsg("Invalid variable type");
        return(-1);
    }

	return(WriteRegion(region, min, max));
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

	if (_firstWrite) {
		_dataRange[0] = _dataRange[1] = *region;
	}
	_firstWrite = false;

	size_t bmin3d[3], bmax3d[3];
	size_t my_bs[3];
	const size_t *bs = GetBlockSize();

	switch (_vtype) {
	case VAR2D_XY:
		my_bs[0] = bs[0]; my_bs[1] = bs[1];
		bmin3d[0] = bmin[0]; bmin3d[1] = bmin[1];
		bmax3d[0] = bmax[0]; bmax3d[1] = bmax[1];
	break;
	case VAR2D_XZ:
		my_bs[0] = bs[0]; my_bs[1] = bs[2];
		bmin3d[0] = bmin[0]; bmin3d[2] = bmin[1];
		bmax3d[0] = bmax[0]; bmax3d[2] = bmax[1];
	break;
	case VAR2D_YZ:
		my_bs[0] = bs[1]; my_bs[1] = bs[2];
		bmin3d[1] = bmin[0]; bmin3d[2] = bmin[1];
		bmax3d[1] = bmax[0]; bmax3d[2] = bmax[1];
	break;
	case VAR3D:
		my_bs[0] = bs[0]; my_bs[1] = bs[1]; my_bs[2] = bs[2];
		bmin3d[0] = bmin[0]; bmin3d[1] = bmin[1]; bmin3d[2] = bmin[2];
		bmax3d[0] = bmax[0]; bmax3d[1] = bmax[1]; bmax3d[2] = bmax[2];
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}


	if (! VDFIOBase::IsValidRegionBlk(bmin3d, bmax3d, _reflevel)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}

	size_t min[3];
	size_t max[3];

	// Map block region coordinates to  voxels
	for(int i=0; i<3; i++) min[i] = my_bs[i]*bmin[i];
	for(int i=0; i<3; i++) max[i] = (my_bs[i]*(bmax[i]+1))-1;

	if (_vtype == VAR3D) {
		return(_WriteRegion3D(region, min, max, block));
	}
	else {
		return(_WriteRegion2D(region, min, max, block));
	}
}



void WaveletBlock3DRegionWriter::brickit3d(
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

	const size_t *bs = GetBlockSize();

	assert (dstx < bs[0]);
	assert (dsty < bs[1]);
	assert (dstz < bs[2]);

	int nxx = bs[0] - dstx;
	int nyy = bs[1] - dsty;
	int nzz = bs[2] - dstz;
	if (nxx > nx - srcx) nxx = nx - srcx;
	if (nyy > ny - srcy) nyy = ny - srcy;
	if (nzz > nz - srcz) nzz = nz - srcz;

	for (int z=dstz, zz=srcz; z<dstz+nzz; z++, zz++) {
		for (int y=dsty, yy=srcy; y<dsty+nyy; y++, yy++) {

			src = srcptr + zz*nx*ny + yy*nx + srcx;
			dst = brickptr + z*bs[0]*bs[1] + y*bs[0] + dstx;
			for (int x=dstx; x<dstx+nxx; x++) {

				*dst++ = *src++;

			}
		}
	}
}

void WaveletBlock3DRegionWriter::brickit3d(
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

	const size_t *bs = GetBlockSize();

	for (int k=0; k<bs[2]; k++) {
		for (int j=0; j<bs[1]; j++) {
			for (int i=0; i<bs[0]; i++) {
				*brickptr = *srcptr;
				brickptr++;
				srcptr++;

			}
			srcptr += nx - bs[0];
		}
		srcptr += nx*ny - nx*bs[1];
	}
}

void WaveletBlock3DRegionWriter::copy_top_superblock3d(
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
	const size_t *bs = GetBlockSize();

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
		maxsb[i] = 2*bs[i] - 1;
	}


	// convert block coordinates to voxel coordinates
	//
	mincr[0] = srcx * bs[0];
	maxcr[0] = (srcx+2) * bs[0] - 1;
	mincr[1] = srcy * bs[1];
	maxcr[1] = (srcy+2) * bs[1] - 1;
	mincr[2] = srcz * bs[2];
	maxcr[2] = (srcz+2) * bs[2] - 1;


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
			int i = z*bs[0]*bs[1]*4 + y*bs[0]*2 + x;
			int ii = zz*nx*ny + yy*nx + xx;

			_padblock3d[i] = _regionData[ii];
		}
		}
	}

	// Pad 

	// First pad along X
	//
	if (minsb[0] > 0 || maxsb[0] < bs[0]*2-1) {
	int stride = 1;
	for (int z=minsb[2]; z<=maxsb[2]; z++) {
	for (int y=minsb[1]; y<=maxsb[1]; y++) {
		float pad = _padblock3d[z*bs[0]*bs[1]*4 + y*bs[0]*2 + minsb[0]];
		float *dstptr = _padblock3d + z*bs[0]*bs[1]*4 + y*bs[0]*2 + 0;

		for (int x=0; x<minsb[0]; x++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock3d[z*bs[0]*bs[1]*4 + y*bs[0]*2 + maxsb[0]];
		dstptr = _padblock3d + z*bs[0]*bs[1]*4 + y*bs[0]*2 + maxsb[0]+1;

		for (int x=maxsb[0]+1; x<bs[0]*2 ; x++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}
	}

	// Next pad along Y
	//
	if (minsb[1] > 0 || maxsb[1] < bs[1]*2-1) {
	int stride = bs[0]*2;
	for (int z=minsb[2]; z<=maxsb[2]; z++) {
	for (int x=0; x<bs[0]*2; x++) {
		float pad = _padblock3d[z*bs[0]*bs[1]*4 + minsb[1]*bs[0]*2 + x];
		float *dstptr = _padblock3d + z*bs[0]*bs[1]*4 + 0 + x;

		for (int y=0; y<minsb[1]; y++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock3d[z*bs[0]*bs[1]*4 + maxsb[1]*bs[0]*2 + x];
		dstptr = _padblock3d + z*bs[0]*bs[1]*4 + (maxsb[1]+1)*bs[0]*2 + x;

		for (int y=maxsb[1]+1; y<bs[1]*2 ; y++) {
			*dstptr = pad;
			dstptr += stride;
		}
	}
	}
	}

	// Next pad along Z
	//
	if (minsb[2] > 0 || maxsb[2] < bs[2]*2-1) {
	int stride = bs[0]*bs[0]*4;
	for (int y=0; y<bs[1]*2; y++) {
	for (int x=0; x<bs[0]*2; x++) {
		float pad = _padblock3d[minsb[2]*bs[0]*bs[1]*4 + y*bs[0]*2 + x];
		float *dstptr = _padblock3d + 0 + y*bs[0]*2 + x;

		for (int z=0; z<minsb[2]; z++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock3d[maxsb[2]*bs[0]*bs[1]*4 + y*bs[0]*2 + x];
		dstptr = _padblock3d + (maxsb[2]+1)*bs[0]*bs[1]*4 + y*bs[0]*2 + x;
		for (int z=maxsb[2]+1; z<bs[2]*2 ; z++) {
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
		brickit3d(
			_padblock3d, bs[0]*2, bs[1]*2, bs[2]*2, 
			x*bs[0], y*bs[1], z*bs[2], brickptr
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

	// Figure out what refinement level we're processing based on sz.
	// (j is the destination's level, not the source)
	int l = 0;
	for (unsigned int ui = sz; ui > 2; ui = ui>>1, l++);

	int j =  (GetNumTransforms()) - l;
	int ldelta = GetNumTransforms() - j + 1;

	size_t bdim[3]; WaveletBlockIOBase::GetDimBlk(bdim, j-1);
	const size_t bcoord[] = {dstx>>ldelta, dsty>>ldelta, dstz>>ldelta};

	// Make sure quadrant is inside region.
	//
	if (bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1] || bcoord[2]>=bdim[2])return(0);


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

		// Blocks input data and stores in _lambda_blks[GetNumTransforms()]
		copy_top_superblock3d(srcx, srcy, srcz, _lambda_blks[GetNumTransforms()]);

		// Compute block min/max
		
		for(int z=0; z<2; z++) {
		for(int y=0; y<2; y++) {
		for(int x=0; x<2; x++) {
			compute_minmax3d(
				_lambda_blks[GetNumTransforms()] + (z*4+y*2+x)*_block_size, 
				dstx+x, dsty+y, dstz+z, GetNumTransforms()
			);
		}
		}
		}
	}

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
		dst_super_blk[i] = _super_block + (_block_size * (i-1));
	}

	_wb3d->ForwardTransform(src_super_blk,dst_super_blk);

	// only save gamma coefficients for requested levels
	//
	if (j <= _reflevel) {
		rc = seekGammaBlocks(bcoord, j);
		if (rc<0) return(-1);
		rc = writeGammaBlocks(_super_block, 1, j);
		if (rc<0) return(-1);
	}

	if (j-1==0) {

		rc = seekLambdaBlocks(bcoord);
		if (rc<0) return(-1);
		rc = writeLambdaBlocks(_lambda_blks[j-1], 1);
		if (rc<0) return (rc);
	}

	return(0);
}

void WaveletBlock3DRegionWriter::brickit2d(
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

void WaveletBlock3DRegionWriter::brickit2d(
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

void WaveletBlock3DRegionWriter::copy_top_superblock2d(
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

		_padblock2d[i] = _regionData[ii];
	}
	}

	// Pad 

	// First pad along X
	//
	if (minsb[0] > 0 || maxsb[0] < bs[0]*2-1) {
	int stride = 1;
	for (int y=minsb[1]; y<=maxsb[1]; y++) {
		float pad = _padblock2d[y*bs[0]*2 + minsb[0]];
		float *dstptr = _padblock2d +  y*bs[0]*2 + 0;

		for (int x=0; x<minsb[0]; x++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock2d[y*bs[0]*2 + maxsb[0]];
		dstptr = _padblock2d +  y*bs[0]*2 + maxsb[0]+1;

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
		float pad = _padblock2d[minsb[1]*bs[0]*2 + x];
		float *dstptr = _padblock2d +  0 + x;

		for (int y=0; y<minsb[1]; y++) {
			*dstptr = pad;
			dstptr += stride;
		}

		pad = _padblock2d[maxsb[1]*bs[0]*2 + x];
		dstptr = _padblock2d + (maxsb[1]+1)*bs[0]*2 + x;

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
		brickit2d(
			_padblock2d, bs, bs[0]*2, bs[1]*2, 
			x*bs[0], y*bs[1],  brickptr
		);
	}
	}
}

int WaveletBlock3DRegionWriter::process_quadrant(
	size_t sz,
	int srcx,
	int srcy,
	int dstx,
	int dsty,
	int quad
) {
	int	rc;

	// Figure out what refinement level we're processing based on sz.
	// (j is the destination's level, not the source)
	int l = 0;
	for (unsigned int ui = sz; ui > 2; ui = ui>>1, l++);

	int j =  (GetNumTransforms()) - l;
	int ldelta = GetNumTransforms() - j + 1;

	size_t bdim[3]; WaveletBlockIOBase::GetDimBlk(bdim, j-1);
	const size_t bcoord[] = {dstx>>ldelta, dsty>>ldelta};

	// Make sure quadrant is inside region.
	//
	if (bcoord[0]>=bdim[0] || bcoord[1]>=bdim[1]) return(0);

	WaveletBlock2D *wb2d;

	const size_t *bs = GetBlockSize();

	size_t bs2d[2];
	switch (_vtype) {
	case VAR2D_XY:
		bs2d[0] = bs[0]; bs2d[1] = bs[1];
		wb2d = _wb2dXY;
	break;
	case VAR2D_XZ:
		bs2d[0] = bs[0]; bs2d[1] = bs[2];
		wb2d = _wb2dXZ;
	break;
	case VAR2D_YZ:
		bs2d[0] = bs[1]; bs2d[1] = bs[2];
		wb2d = _wb2dYZ;
	break;
	default:
		SetErrMsg("Invalid variable type");
		return(-1);
	}

	size_t tile_size = bs2d[0]*bs2d[1];

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

		// Blocks input data and stores in _lambda_tiles[GetNumTransforms()]
		copy_top_superblock2d(bs2d, srcx, srcy, _lambda_tiles[GetNumTransforms()]);

		// Compute block min/max
		
		for(int y=0; y<2; y++) {
		for(int x=0; x<2; x++) {
			compute_minmax2d(
				_lambda_tiles[GetNumTransforms()] + (y*2+x)*tile_size, 
				dstx+x, dsty+y, GetNumTransforms()
			);
		}
		}
	}


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
		rc = seekGammaBlocks(bcoord, j);
		if (rc<0) return(-1);
		rc = writeGammaBlocks(_super_tile, 1, j);
		if (rc<0) return(-1);
	}

	if (j-1==0) {
		rc = seekLambdaBlocks(bcoord);
		if (rc<0) return(-1);
		rc = writeLambdaBlocks(_lambda_tiles[j-1], 1);
		if (rc<0) return (rc);
	}

	return(0);
}

void WaveletBlock3DRegionWriter::compute_minmax3d(
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

	size_t bdim[3];
	WaveletBlockIOBase::GetDimBlk(bdim, GetNumTransforms());

	const float *fptr = blockptr;
	size_t blkidx = bz*bdim[0]*bdim[1] + by*bdim[0] + bx;
	_mins3d[l][blkidx] = _maxs3d[l][blkidx] = *fptr;

	const size_t *bs = GetBlockSize();

	for(int z=0; z<bs[2]; z++) {
	for(int y=0; y<bs[1]; y++) {
	for(int x=0; x<bs[0]; x++) {

		// block min/max
		if (*fptr < _mins3d[l][blkidx]) _mins3d[l][blkidx] = *fptr;
		if (*fptr > _maxs3d[l][blkidx]) _maxs3d[l][blkidx] = *fptr;

		// global (volume) min/max
		if (*fptr < _dataRange[0]) _dataRange[0] = *fptr;
		if (*fptr > _dataRange[1]) _dataRange[1] = *fptr;
		fptr++;
	}
	}
	}
}

void WaveletBlock3DRegionWriter::compute_minmax2d(
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

	size_t bs2d[2] = {0,0};
	const size_t *bs = GetBlockSize();

	switch (_vtype) {
	case VAR2D_XY:
		bs2d[0] = bs[0]; bs2d[1] = bs[1];
	break;
	case VAR2D_XZ:
		bs2d[0] = bs[0]; bs2d[1] = bs[2];
	break;
	case VAR2D_YZ:
		bs2d[0] = bs[1]; bs2d[1] = bs[2];
	break;
	default:
	break;
	}

	size_t bdim[3];
	WaveletBlockIOBase::GetDimBlk(bdim, GetNumTransforms());

	const float *fptr = blockptr;
	size_t blkidx = by*bdim[0] + bx;
	_mins2d[l][blkidx] = _maxs2d[l][blkidx] = *fptr;

	for(int y=0; y<bs2d[1]; y++) {
	for(int x=0; x<bs2d[0]; x++) {

		// block min/max
		if (*fptr < _mins2d[l][blkidx]) _mins2d[l][blkidx] = *fptr;
		if (*fptr > _maxs2d[l][blkidx]) _maxs2d[l][blkidx] = *fptr;

		// global (volume) min/max
		if (*fptr < _dataRange[0]) _dataRange[0] = *fptr;
		if (*fptr > _dataRange[1]) _dataRange[1] = *fptr;
		fptr++;
	}
	}
}



int	WaveletBlock3DRegionWriter::my_alloc3d(
) {


	size_t size = _block_size * 2 * 2 * 2;

	// alloc space from coarsest (j==0) to finest level
	for(int j=0; j<GetNumTransforms()+1; j++) {

		// allocate enough space for a superblock

		_lambda_blks[j] = new(nothrow) float[size];
		if (! _lambda_blks[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

	}

	_padblock3d = new(nothrow) float[size];
	if (! _padblock3d) {
		SetErrMsg("new float[%d] : %s", size, strerror(errno));
		return(-1);
	}
	return(0);
}


int	WaveletBlock3DRegionWriter::my_alloc2d(
) {
	const size_t *bs = GetBlockSize();

	// Maximum size of a tile regardless of type (XY, XZ, YZ)
	size_t max_tile_sz = max(bs[0],bs[1]) * max(bs[2], min(bs[0],bs[1]));


	size_t size = max_tile_sz * 2 * 2;

	// alloc space from coarsest (j==0) to finest level
	for(int j=0; j<GetNumTransforms()+1; j++) {

		// allocate enough space for a superblock

		_lambda_tiles[j] = new(nothrow) float[size];
		if (! _lambda_tiles[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

	}

	_padblock2d = new(nothrow) float[size];
	if (! _padblock2d) {
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

		if (_lambda_tiles[j]) delete [] _lambda_tiles[j];

		_lambda_tiles[j] = NULL;

	}
	if (_padblock3d) delete [] _padblock3d;
	if (_padblock2d) delete [] _padblock2d;
}

void	WaveletBlock3DRegionWriter::_GetValidRegion(
    size_t minreg[3], size_t maxreg[3]
) const {
	for (int i=0; i<3; i++) {
		minreg[i] = _validRegMin[i];
		maxreg[i] = _validRegMax[i];
	}
} 


void WaveletBlock3DRegionWriter::_GetDataRange(float range[2]) const { 
	range[0] = _dataRange[0];
	range[1] = _dataRange[1];
}

