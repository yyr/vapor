#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "vapor/WaveletBlock3DRegionReader.h"
#ifdef WIN32
#pragma warning(disable : 4996)
#endif
using namespace VetsUtil;
using namespace VAPoR;

void	WaveletBlock3DRegionReader::_WaveletBlock3DRegionReader()
{
	int	j;

	SetClassName("WaveletBlock3DRegionReader");

	const size_t *bs = GetBlockSize();
	_block_size = bs[0]*bs[1]*bs[2];

	for(j=0; j<MAX_LEVELS; j++) {
		lambda_blks_c[j] = NULL;
		gamma_blks_c[j] = NULL;
		_lambda_tiles[j] = NULL;
		_gamma_tiles[j] = NULL;
	}
}

WaveletBlock3DRegionReader::WaveletBlock3DRegionReader(
	const MetadataVDC &metadata
) : WaveletBlockIOBase(metadata) {

	if (WaveletBlock3DRegionReader::GetErrCode()) return;

	SetDiagMsg("WaveletBlock3DRegionReader::WaveletBlock3DRegionReader()");

	_WaveletBlock3DRegionReader();
}

WaveletBlock3DRegionReader::WaveletBlock3DRegionReader(
	const string &metafile
) : WaveletBlockIOBase(metafile) {

	if (WaveletBlock3DRegionReader::GetErrCode()) return;

	SetDiagMsg(
		"WaveletBlock3DRegionReader::WaveletBlock3DRegionReader(%s)",
		metafile.c_str()
	);

	_WaveletBlock3DRegionReader();

}

WaveletBlock3DRegionReader::~WaveletBlock3DRegionReader(
) {

	SetDiagMsg("WaveletBlock3DRegionReader::~WaveletBlock3DRegionReader()");

	WaveletBlock3DRegionReader::CloseVariable();
	my_free();

}

int	WaveletBlock3DRegionReader::OpenVariableRead(
	size_t	timestep,
	const char	*varname,
	int reflevel,
	int
) {
	int	rc;

	SetDiagMsg(
		"WaveletBlock3DRegionReader::OpenVariableRead(%d, %s, %d)",
		timestep, varname, reflevel
	);

	if (reflevel < 0) reflevel = GetNumTransforms();

	CloseVariable();	// close any previously opened files.
	rc = WaveletBlockIOBase::OpenVariableRead(timestep, varname, reflevel);
	if (rc<0) return(rc);

	if (_vtype == VAR3D) {
		rc = my_realloc3d();
	}
	else {
		rc = my_realloc2d();
	}
	if (rc<0) return(rc);

	return(0);
}

int	WaveletBlock3DRegionReader::CloseVariable(
) {
	SetDiagMsg("WaveletBlock3DRegionReader::CloseVariable()");

	return(WaveletBlockIOBase::CloseVariable());
}

int	WaveletBlock3DRegionReader::_ReadRegion3D(
	const size_t min[3],
	const size_t max[3],
	float *region,
	int unblock
) {
	size_t	bmin[3];
	size_t	bmax[3];

	int	x,y,z;
	int	rc;
	int	l0x0, l0x1, l0y0, l0y1, l0z0, l0z1;
	size_t minsub[] = {min[0], min[1], min[2]};
	size_t maxsub[] = {max[0], max[1], max[2]};

	const size_t *bs = GetBlockSize();
	while (minsub[0] >= bs[0]) {
		minsub[0] -= bs[0];
		maxsub[0] -= bs[0];
	}
	while (minsub[1] >= bs[1]) {
		minsub[1] -= bs[1];
		maxsub[1] -= bs[1];
	}
	while (minsub[2] >= bs[2]) {
		minsub[2] -= bs[2];
		maxsub[2] -= bs[2];
	}

	if (! unblock) {
		// sanity check. Region must be defined on block boundaries
		// to preserve blocking
		//
		for(int i=0; i<3; i++) assert((min[i] % bs[i]) == 0);
		for(int i=0; i<3; i++) assert(((max[i]+1) % bs[i]) == 0);
	}

	VDFIOBase::MapVoxToBlk(min, bmin, _reflevel);
	VDFIOBase::MapVoxToBlk(max, bmax, _reflevel);

	size_t x0 = bmin[0];
	size_t y0 = bmin[1];
	size_t z0 = bmin[2];
	size_t x1 = bmax[0];
	size_t y1 = bmax[1];
	size_t z1 = bmax[2];


	// Max transform request => read raw blocks directly into buffer. Do no
	// processing.
	//
	if (_reflevel == 0) {

		float *regptr = region;

		// read in blocks a row at a time.
		for(z=(int)z0; z<=(int)z1; z++) {
		for(y=(int)y0; y<=(int)y1; y++) {
			const size_t bcoord[3] = {x0, y, z};
			rc = seekLambdaBlocks(bcoord);
			if (rc < 0) return (rc);

			if (! unblock) {
				rc = readLambdaBlocks(x1-x0+1, regptr);
				if (rc < 0) return (rc);
				regptr += _block_size * (x1-x0+1);
			}
			else {
				float *srcptr = lambda_blks_c[0];

				rc = readLambdaBlocks(x1-bmin[0]+1, lambda_blks_c[0]);
				if (rc < 0) return (rc);
				for(x=0;x<(int)(x1-x0+1); x++) {
					const size_t bcoord[3] = {x,y-y0,z-z0};
					Block2NonBlock(srcptr, bcoord, minsub, maxsub, region);
					srcptr += _block_size;
				}
					
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
	l0z0 = (int)z0 >> _reflevel;
	l0z1 = (int)z1 >> _reflevel;

	// read in blocks from level 0 a row at a time.
	for(z=l0z0; z<=l0z1; z++) {
	for(y=l0y0; y<=l0y1; y++) {
		const size_t bcoord[3] = {l0x0, y, z};
		rc = seekLambdaBlocks(bcoord);
		if (rc < 0) return (rc);

		rc = readLambdaBlocks(l0x1-l0x0+1, lambda_blks_c[0]);
		if (rc < 0) return (rc);

		// transform blocks from level 0 to desired level
		rc = row_inv_xform3d(
			lambda_blks_c[0], l0x0, y, z, l0x1-l0x0+1, 0,
			region, min, max, _reflevel, unblock
		);
		if (rc < 0) return (rc);
	}
	}

	return(0);
}

int	WaveletBlock3DRegionReader::_ReadRegion2D(
	const size_t min[2],
	const size_t max[2],
	float *region,
	int unblock
) {
	size_t bs2d[2];
	size_t bmin[2];
	size_t bmax[2];

	size_t min3d[] = {0,0,0};
	size_t max3d[] = {0,0,0};
	size_t bmin3d[] = {0,0,0};
	size_t bmax3d[] = {0,0,0};

	const size_t *bs = GetBlockSize();
	switch (_vtype) {
	case VAR2D_XY:
		bs2d[0] = bs[0]; bs2d[1] = bs[1];
		min3d[0] = min[0]; min3d[1] = min[1]; 
		max3d[0] = max[0]; max3d[1] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[1];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[1];
	break;
	case VAR2D_XZ:
		bs2d[0] = bs[0]; bs2d[1] = bs[2];
		min3d[0] = min[0]; min3d[2] = min[1];
		max3d[0] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[2];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[2];
	break;
	case VAR2D_YZ:
		bs2d[0] = bs[1]; bs2d[1] = bs[2];
		min3d[1] = min[0]; min3d[2] = min[1];
		max3d[1] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
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

	while (minsub[0] >= bs2d[0]) {
		minsub[0] -= bs2d[0];
		maxsub[0] -= bs2d[0];
	}
	while (minsub[1] >= bs2d[1]) {
		minsub[1] -= bs2d[1];
		maxsub[1] -= bs2d[1];
	}

	if (! unblock) {
		// sanity check. Region must be defined on block boundaries
		// to preserve blocking
		//
		for(int i=0; i<2; i++) assert((min[i] % bs2d[i]) == 0);
		for(int i=0; i<2; i++) assert(((max[i]+1) % bs2d[i]) == 0);
	}


	size_t x0 = bmin[0];
	size_t y0 = bmin[1];
	size_t x1 = bmax[0];
	size_t y1 = bmax[1];


	// Max transform request => read raw blocks directly into buffer. Do no
	// processing.
	//
	size_t tile_size = bs2d[0]*bs2d[1];
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
		rc = row_inv_xform2d(
			_lambda_tiles[0], l0x0, y, l0x1-l0x0+1, 0,
			region, min, max, _reflevel, unblock
		);
		if (rc < 0) return (rc);
	}

	return(0);
}

int	WaveletBlock3DRegionReader::ReadRegion(
	const size_t min[3],
	const size_t max[3],
	float *region
) {
	SetDiagMsg(
		"WaveletBlock3DRegionReader::ReadRegion((%d,%d,%d),(%d,%d,%d))",
		min[0], min[1], min[2], max[0], max[1], max[2]
	);

	
	size_t min3d[3], max3d[3];

	switch (_vtype) {
	case VAR2D_XY:
		// Make sure 3rd, unused coord is valid
		//
		WaveletBlockIOBase::GetValidRegion(min3d, max3d, _reflevel);
		min3d[0] = min[0]; min3d[1] = min[1];
		max3d[0] = max[0]; max3d[1] = max[1];
	break;
	case VAR2D_XZ:
		WaveletBlockIOBase::GetValidRegion(min3d, max3d, _reflevel);
		min3d[0] = min[0]; min3d[2] = min[1];
		max3d[0] = max[0]; max3d[2] = max[1];
	break;
	case VAR2D_YZ:
		WaveletBlockIOBase::GetValidRegion(min3d, max3d, _reflevel);
		min3d[1] = min[0]; min3d[2] = min[1];
		max3d[1] = max[0]; max3d[2] = max[1];
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

	if (_vtype == VAR3D) {
		return(_ReadRegion3D(min, max, region, 1));
	}
	else {
		return(_ReadRegion2D(min, max, region, 1));
	}
}

int WaveletBlock3DRegionReader::ReadRegion(
    float *region
) {
	SetDiagMsg( "WaveletBlock3DRegionReader::ReadRegion()");

	size_t dim3d[3]; 
	VDFIOBase::GetDim(dim3d,_reflevel);

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

	return(ReadRegion(min, max, region));
}



int	WaveletBlock3DRegionReader::BlockReadRegion(
	const size_t bmin[3],
	const size_t bmax[3],
	float *region,
	int unblock
) {
	size_t min[3], max[3];

	SetDiagMsg(
		"WaveletBlock3DRegionReader::BlockReadRegion((%d,%d,%d),(%d,%d,%d))",
		bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
	);


	size_t min3d[3], max3d[3];
	size_t bmin3d[3], bmax3d[3];
	size_t my_bs[3] = {0,0,0};

	// If this is a 2D region, make sure 3rd, unused coord is valid
	//
	WaveletBlockIOBase::GetValidRegion(min3d, max3d, _reflevel);
	WaveletBlockIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
	WaveletBlockIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
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
			"Invalid region : (%d %d) (%d %d)", 
			bmin[0], bmin[1], bmax[0], bmax[1]
		);
		return(-1);
	}

	for(int i=0; i<3; i++) min[i] = my_bs[i]*bmin[i];
	for(int i=0; i<3; i++) max[i] = (my_bs[i]*(bmax[i]+1))-1;

	if (_vtype == VAR3D) {
		return(_ReadRegion3D(min, max, region, unblock));
	}
	else {
		return(_ReadRegion2D(min, max, region, unblock));
	}

}


int	WaveletBlock3DRegionReader::row_inv_xform3d(
	const float *lambda_row,
	unsigned int ljx0,
	unsigned int ljy0,
	unsigned int ljz0,
	unsigned int ljnx,
	unsigned int j,
	float *region,
	const size_t min[3],
	const size_t max[3],
	unsigned int level,
	int unblock
) {
	size_t bmin[3], bmax[3];
	size_t minsub[] = {min[0], min[1], min[2]};
	size_t maxsub[] = {max[0], max[1], max[2]};
	const size_t *bs = GetBlockSize();

	while (minsub[0] >= bs[0]) {
		minsub[0] -= bs[0];
		maxsub[0] -= bs[0];
	}
	while (minsub[1] >= bs[1]) {
		minsub[1] -= bs[1];
		maxsub[1] -= bs[1];
	}
	while (minsub[2] >= bs[2]) {
		minsub[2] -= bs[2];
		maxsub[2] -= bs[2];
	}

	VDFIOBase::MapVoxToBlk(min, bmin, _reflevel);
	VDFIOBase::MapVoxToBlk(max, bmax, _reflevel);

	size_t x0 = bmin[0];
	size_t y0 = bmin[1];
	size_t z0 = bmin[2];
	size_t x1 = bmax[0];
	size_t y1 = bmax[1];
	size_t z1 = bmax[2];

	//	Coords at level j + 1  (May be outside subregion)
	//
	int	ljp1y0 = ljy0<<1;
	int	ljp1z0 = ljz0<<1;
	int	ljp1nx = ljnx<<1;

	//	Coords at level j + 1 that are contained in subregion
	//
	int	sljp1x0 = (int)(x0>>(level-(j+1)));
	int	sljp1y0 = (int)(y0>>(level-(j+1)));
	int	sljp1z0 = (int)(z0>>(level-(j+1)));
	int	sljp1x1 = (int)(x1>>(level-(j+1)));
	int	sljp1y1 = (int)(y1>>(level-(j+1)));
	int	sljp1z1 = (int)(z1>>(level-(j+1)));
	int	sljp1nx = (int)(sljp1x1 - sljp1x0 + 1);

	const float *src_super_blk[8];
	float	*dst_super_blk[8];

	float	*srcptr, *dstptr;
	float	*slab1, *slab2;
	int	dst_nbx, dst_nby;
	int	i,x,y,z;
	int	rc;


	// Get gamma coefficients for next level
	//
	const size_t bcoord[3] = {ljx0, ljy0, ljz0};
	rc = seekGammaBlocks(bcoord, j+1);
	if (rc<0) return(rc);

	rc = readGammaBlocks(ljnx, gamma_blks_c[j+1], j+1);
	if (rc<0) return(rc);

	dst_nbx = ljp1nx;
	dst_nby = 2;

	slab1 = lambda_blks_c[j+1];
	slab2 = slab1 + (_block_size * dst_nbx * dst_nby);

	// Apply inverse transform to go from level j to j+1
	//
	for(x=0; x<(int)ljnx; x++) {

		src_super_blk[0] = lambda_row + (x * _block_size);
		for(i=1; i<8; i++) {
			src_super_blk[i] = gamma_blks_c[j+1] + 
				(x*_block_size*7) + (_block_size * (i-1));
		}

		dst_super_blk[0] = slab1;
		dst_super_blk[1] = slab1 + _block_size;
		dst_super_blk[2] = slab1 + (_block_size * dst_nbx);
		dst_super_blk[3] = slab1 + (_block_size * dst_nbx) + _block_size;
		dst_super_blk[4] = slab2;
		dst_super_blk[5] = slab2 + _block_size;
		dst_super_blk[6] = slab2 + (_block_size * dst_nbx);
		dst_super_blk[7] = slab2 + (_block_size * dst_nbx) + _block_size;


		_XFormTimerStart();
		_wb3d->InverseTransform(src_super_blk,dst_super_blk);
		_XFormTimerStop();

		slab1 += 2*_block_size;
		slab2 += 2*_block_size;
	}


	if (j+1 < level) {
		for(z=0; z<2; z++) {
		for(y=0; y<2; y++) {

			// cull rows outside of subregion
			//
			if (z+ljp1z0 < sljp1z0 || z+ljp1z0 > sljp1z1) continue;
			if (y+ljp1y0 < sljp1y0 || y+ljp1y0 > sljp1y1) continue;

			srcptr = lambda_blks_c[j+1] + 
				(_block_size * 
				((z*2*ljp1nx) + (y*ljp1nx)));

			if (sljp1x0 % 2) srcptr += _block_size;

			rc = row_inv_xform3d(
				srcptr, sljp1x0, ljp1y0+y, ljp1z0+z, sljp1nx,
				j+1, region, min, max, level,
				unblock
			);
			if (rc < 0) return(rc);
		}
		}
		return(0);
	}

	// Last iteration. Copy *valid* blocks (those within the subregion) 
	// to region buffer. 
	//
	if (j+1 == level) {
		int	yy,zz;

		for(z=0,zz=0; z<2; z++) {
		for(y=0,yy=0; y<2; y++) {
			int	regnx = (int)(x1-x0+1);
			int	regny = (int)(y1-y0+1);

			// cull rows outside of subregion
			// n.b. x0 == sljp1z0, y0==sljp1y0, etc.
			//
			if (z+ljp1z0 < sljp1z0 || z+ljp1z0 > sljp1z1) continue;
			if (y+ljp1y0 < sljp1y0 || y+ljp1y0 > sljp1y1) continue;


			srcptr = lambda_blks_c[j+1] + 
				(_block_size * 
				((z*2*ljp1nx) + (y*ljp1nx)));

			if (sljp1x0 % 2) srcptr += _block_size;

			zz = z + ljp1z0 - sljp1z0;
			yy = y + ljp1y0 - sljp1y0;
			dstptr = region + 
				(_block_size *
				((zz * regny * regnx) + 
				(yy * regnx)));

			if (! unblock) {
				memcpy(
					dstptr, srcptr, 
					sljp1nx*_block_size*sizeof(srcptr[0])
				);
			}
			else {
				for(x=0;x<sljp1nx;x++) {
					const size_t bcoord[3] = {x,yy,zz};
					Block2NonBlock(srcptr, bcoord, minsub, maxsub, region);
					srcptr += _block_size;
				}
			}
			yy++;
			zz++;

		}
		}
		return(0);
	}

	return(0);
}


int	WaveletBlock3DRegionReader::row_inv_xform2d(
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

	size_t bs2d[2];
	size_t bmin[2];
	size_t bmax[2];
	WaveletBlock2D *wb2d;

	size_t min3d[] = {0,0,0};
	size_t max3d[] = {0,0,0};
	size_t bmin3d[] = {0,0,0};
	size_t bmax3d[] = {0,0,0};

	const size_t *bs = GetBlockSize();

	switch (_vtype) {
	case VAR2D_XY:
		bs2d[0] = bs[0]; bs2d[1] = bs[1];
		min3d[0] = min[0]; min3d[1] = min[1]; 
		max3d[0] = max[0]; max3d[1] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[1];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[1];
		wb2d = _wb2dXY;
	break;
	case VAR2D_XZ:
		bs2d[0] = bs[0]; bs2d[1] = bs[2];
		min3d[0] = min[0]; min3d[2] = min[1];
		max3d[0] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
		bmin[0] = bmin3d[0]; bmin[1] = bmin3d[2];
		bmax[0] = bmax3d[0]; bmax[1] = bmax3d[2];
		wb2d = _wb2dXZ;
	break;
	case VAR2D_YZ:
		bs2d[0] = bs[1]; bs2d[1] = bs[2];
		min3d[1] = min[0]; min3d[2] = min[1];
		max3d[1] = max[0]; max3d[2] = max[1];
		VDFIOBase::MapVoxToBlk(min3d, bmin3d, _reflevel);
		VDFIOBase::MapVoxToBlk(max3d, bmax3d, _reflevel);
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

	while (minsub[0] >= bs2d[0]) {
		minsub[0] -= bs2d[0];
		maxsub[0] -= bs2d[0];
	}
	while (minsub[1] >= bs2d[1]) {
		minsub[1] -= bs2d[1];
		maxsub[1] -= bs2d[1];
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

	size_t tile_size = bs2d[0]*bs2d[1];

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


		_XFormTimerStart();
		wb2d->InverseTransform(src_super_tile,dst_super_tile);
		_XFormTimerStop();

		slice += 2*tile_size;
	}


	if (j+1 < level) {
		for(y=0; y<2; y++) {

			// cull rows outside of subregion
			//
			if (y+ljp1y0 < sljp1y0 || y+ljp1y0 > sljp1y1) continue;

			srcptr = _lambda_tiles[j+1] + (tile_size *  y*ljp1nx);

			if (sljp1x0 % 2) srcptr += tile_size;

			rc = row_inv_xform2d(
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
int	WaveletBlock3DRegionReader::my_realloc3d(
) {

	// alloc space from coarsest (j==0) to finest level
	for(int j=0; j<=_reflevel; j++) {

		if (! lambda_blks_c[j]) {
			size_t nb_j[3];
			size_t     size;

			VDFIOBase::GetDimBlk(nb_j, j);
			nb_j[0] += 1;	// fudge (deal with odd size dimensions)

			size = nb_j[0] * _block_size * 2 * 2;

			lambda_blks_c[j] = new float[size];
			if (! lambda_blks_c[j]) {
				SetErrMsg("new float[%d] : %s", size, strerror(errno));
				return(-1);
			}

			size = nb_j[0] * _block_size * 2 * 2 * 7;

			gamma_blks_c[j] = new float[size];
			if (! gamma_blks_c[j]) {
				SetErrMsg("new float[%d] : %s", size, strerror(errno));
				return(-1);
			}
		}
	}

	return(0);
}

int	WaveletBlock3DRegionReader::my_realloc2d(
) {
	const size_t *bs = GetBlockSize();

	// Maximum size of a tile regardless of type (XY, XZ, YZ)
	size_t max_tile_sz = max(bs[0],bs[1]) * max(bs[2], min(bs[0],bs[1]));

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

void	WaveletBlock3DRegionReader::my_free(
) {
	int	j;

	for(j=0; j<MAX_LEVELS; j++) {
		if (lambda_blks_c[j]) delete [] lambda_blks_c[j];
		if (gamma_blks_c[j]) delete [] gamma_blks_c[j];

		lambda_blks_c[j] = NULL;
		gamma_blks_c[j] = NULL;

		if (_lambda_tiles[j]) delete [] _lambda_tiles[j];
		if (_gamma_tiles[j]) delete [] _gamma_tiles[j];

		_lambda_tiles[j] = NULL;
		_gamma_tiles[j] = NULL;

	}
}
