#include <cstdio>
#include <cstring>
#include <cerrno>
#include <cassert>
#include "vapor/WaveletBlock3DRegionReader.h"

using namespace VetsUtil;
using namespace VAPoR;

void	WaveletBlock3DRegionReader::_WaveletBlock3DRegionReader()
{
	int	j;

	SetClassName("WaveletBlock3DRegionReader");

	xform_timer_c = 0.0;

	for(j=0; j<MAX_LEVELS; j++) {
		lambda_blks_c[j] = NULL;
		gamma_blks_c[j] = NULL;
	}
}

WaveletBlock3DRegionReader::WaveletBlock3DRegionReader(
	const Metadata *metadata,
	unsigned int	nthreads
) : WaveletBlock3DIO((Metadata *) metadata, nthreads) {

	SetDiagMsg("WaveletBlock3DRegionReader::WaveletBlock3DRegionReader()");

	_WaveletBlock3DRegionReader();
}

WaveletBlock3DRegionReader::WaveletBlock3DRegionReader(
	const char *metafile,
	unsigned int	nthreads
) : WaveletBlock3DIO(metafile, nthreads) {

	SetDiagMsg(
		"WaveletBlock3DRegionReader::WaveletBlock3DRegionReader(%s)",
		metafile
	);

	_WaveletBlock3DRegionReader();
}

WaveletBlock3DRegionReader::~WaveletBlock3DRegionReader(
) {
	SetDiagMsg("WaveletBlock3DRegionReader::~WaveletBlock3DRegionReader()");

	CloseVariable();
}

int	WaveletBlock3DRegionReader::OpenVariableRead(
	size_t	timestep,
	const char	*varname,
	size_t num_xforms
) {
	int	rc;

	SetDiagMsg(
		"WaveletBlock3DRegionReader::OpenVariableRead(%d, %s, %d)",
		timestep, varname, num_xforms
	);

	CloseVariable();	// close any previously opened files.
	rc = WaveletBlock3DIO::OpenVariableRead(timestep, varname, num_xforms);
	if (rc<0) return(rc);
	rc = my_alloc();
	if (rc<0) return(rc);

	return(0);
}

int	WaveletBlock3DRegionReader::CloseVariable(
) {
	SetDiagMsg("WaveletBlock3DRegionReader::CloseVariable()");

	my_free();
	return(WaveletBlock3DIO::CloseVariable());
}

int	WaveletBlock3DRegionReader::_ReadRegion(
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
	size_t dim[3] = {max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1};

	if (! unblock) {
		// sanity check. Region must be defined on block boundaries
		// to preserve blocking
		//
		for(int i=0; i<3; i++) assert((min[i] % bs_c) == 0);
		for(int i=0; i<3; i++) assert(((max[i]+1) % bs_c) == 0);
	}

	MapVoxToBlk(min, bmin);
	MapVoxToBlk(max, bmax);

	size_t x0 = bmin[0];
	size_t y0 = bmin[1];
	size_t z0 = bmin[2];
	size_t x1 = bmax[0];
	size_t y1 = bmax[1];
	size_t z1 = bmax[2];


	// Max transform request => read raw blocks directly into buffer. Do no
	// processing.
	//
	if (num_xforms_c == max_xforms_c) {
		TIMER_START(t0)

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
				regptr += block_size_c * (x1-x0+1);
			}
			else {
				float *srcptr = lambda_blks_c[0];

				rc = readLambdaBlocks(x1-bmin[0]+1, lambda_blks_c[0]);
				if (rc < 0) return (rc);
				for(x=0;x<(int)(x1-x0+1); x++) {
					const size_t bcoord[3] = {x,y-y0,z-z0};
					Block2NonBlock(srcptr, bcoord, dim, region);
					srcptr += block_size_c;
				}
					
			}
		}
		}
		TIMER_STOP(t0, xform_timer_c)
		return(0);
	}

	TIMER_START(t0)

	// Get bounds of subregion at coarsest level
	//
	l0x0 = (int)x0 >> (max_xforms_c - num_xforms_c);
	l0x1 = (int)x1 >> (max_xforms_c - num_xforms_c);
	l0y0 = (int)y0 >> (max_xforms_c - num_xforms_c);
	l0y1 = (int)y1 >> (max_xforms_c - num_xforms_c);
	l0z0 = (int)z0 >> (max_xforms_c - num_xforms_c);
	l0z1 = (int)z1 >> (max_xforms_c - num_xforms_c);

	// read in blocks from level 0 a row at a time.
	for(z=l0z0; z<=l0z1; z++) {
	for(y=l0y0; y<=l0y1; y++) {
		const size_t bcoord[3] = {l0x0, y, z};
		rc = seekLambdaBlocks(bcoord);
		if (rc < 0) return (rc);

		rc = readLambdaBlocks(l0x1-l0x0+1, lambda_blks_c[0]);
		if (rc < 0) return (rc);

		// transform blocks from level 0 to desired level
		rc = row_inv_xform(
			lambda_blks_c[0], l0x0, y, z, l0x1-l0x0+1, 0,
			region, min, max, max_xforms_c - num_xforms_c, unblock
		);
		if (rc < 0) return (rc);
	}
	}

	TIMER_STOP(t0, xform_timer_c)

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

	if (! IsValidRegion(num_xforms_c, min, max)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min[0], min[1], min[2], max[0], max[1], max[2]
		);
		return(-1);
	}

	return(_ReadRegion(min, max, region, 1));
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

	if (! IsValidRegionBlk(num_xforms_c, bmin, bmax)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			bmin[0], bmin[1], bmin[2], bmax[0], bmax[1], bmax[2]
		);
		return(-1);
	}

	for(int i=0; i<3; i++) min[i] = bs_c*bmin[i];
	for(int i=0; i<3; i++) max[i] = (bs_c*(bmax[i]+1))-1;

	return(_ReadRegion(min, max, region, unblock));

}

#ifdef	DEAD
int	WaveletBlock3DRegionReader::ReadRegionGamma(
	float *xregion,
	float *yregion,
	float *zregion,
	unsigned int x0,
	unsigned int y0,
	unsigned int z0,
	unsigned int x1,
	unsigned int y1,
	unsigned int z1,
	unsigned int level,
	int unblock
) {
	int	x,y,z;	// block coordinates in full domain space
	int	xx,yy,zz;	// block coordinates in subdomain space
	float *super_blk_ptr[8];
	size_t num_xforms = max_xforms_c - level;

    if (num_xforms > max_xforms_c) {
        SetErrMsg("Invalid transform level : %d", num_xforms);
        return(-1);
    }

	if (! IsValidRegionBlk(num_xforms, min, max)) {
		SetErrMsg(
			"Invalid region : (%d %d %d) (%d %d %d)", 
			min[0], min[1], min[2], max[0], max[1], max[2]
		);
		return(-1);
	}

	int nbx = x1-x0+1;
	int nby = y1-y0+1;

    super_blk_ptr[0] = NULL;
    for(int i=1; i<8; i++) {
        super_blk_ptr[i] = super_block_c + (block_size_c * (i-1));
    }


	// read in block clusters, one cluster (seven blocks) at a time
	//
	for(z=z0,zz=0; z<=z1; z++,zz++) {
	for(y=y0,yy=0; y<=y1; y++,yy++) {
	for(x=x0,xx=0; x<=x1; x++,xx++) {
		int	rc;

		rc = seekGammaBlocks(x, y, z, num_xforms);
		if (rc < 0) return (rc);

		rc = readGammaBlocks(super_block_c, 1, num_xforms);
		if (rc < 0) return (rc);

		// Gamma coefficients are tranposed and need to be
		// restored to X,Y,Z order
		//
		wb3d_c->Transpose(super_blk_ptr);

		if (! unblock) {
			size_t size = block_size_c * sizeof(super_block_c[0]);

			// Z detail coefficients
			//
			memcpy(
				zregion+((nbx*nby*zz)+(nbx*yy)+xx)*block_size_c, 
				super_block_c, size
			);

			// Y detail coefficients
			//
			memcpy(
				yregion+((nbx*nby*(zz*2))+(nbx*yy)+xx)*block_size_c, 
				super_block_c+block_size_c, size
			);
			memcpy(
				yregion+((nbx*nby*(zz*2+1))+(nbx*yy)+xx)*block_size_c, 
				super_block_c+(2*block_size_c), size
			);

			// X detail coefficients
			//
			memcpy(
				xregion+((nbx*nby*(zz*2))+(nbx*(yy*2))+xx)*block_size_c, 
				super_block_c+(3*block_size_c), size
			);
			memcpy(
				xregion+((nbx*nby*(zz*2))+(nbx*(yy*2+1))+xx)*block_size_c, 
				super_block_c+(4*block_size_c), size
			);
			memcpy(
				xregion+((nbx*nby*(zz*2+1))+(nbx*(yy*2))+xx)*block_size_c, 
				super_block_c+(5*block_size_c), size
			);
			memcpy(
				xregion+((nbx*nby*(zz*2+1))+(nbx*(yy*2+1))+xx)*block_size_c, 
				super_block_c+(6*block_size_c), size
			);
		}

		else {
			// Z detail coefficients
			//
			Block2NonBlock(super_block_c, zregion, xx,yy,zz,nbx,nby);

			// Y detail coefficients
			//
			Block2NonBlock(
				super_block_c+block_size_c, yregion, xx,yy,zz*2,nbx,nby
			);
			Block2NonBlock(
				super_block_c+(2*block_size_c), yregion, xx,yy,zz*2+1,nbx,nby
			);

			// X detail coefficients
			//
			Block2NonBlock(
				super_block_c+(3*block_size_c), 
				xregion, xx, yy*2, zz*2, nbx, nby*2
			);
			Block2NonBlock(
				super_block_c+(4*block_size_c), 
				xregion, xx, yy*2+1, zz*2, nbx, nby*2
			);
			Block2NonBlock(
				super_block_c+(5*block_size_c), 
				xregion, xx, yy*2, zz*2+1, nbx, nby*2
			);
			Block2NonBlock(
				super_block_c+(6*block_size_c), 
				xregion, xx, yy*2+1, zz*2+1, nbx, nby*2
			);
		}
	}
	}
	}

	return(0);
}
#endif


int	WaveletBlock3DRegionReader::row_inv_xform(
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
	size_t dim[3] = {max[0]-min[0]+1, max[1]-min[1]+1, max[2]-min[2]+1};

	MapVoxToBlk(min, bmin);
	MapVoxToBlk(max, bmax);

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
	rc = seekGammaBlocks(max_xforms_c - j - 1, bcoord);
	if (rc<0) return(rc);

	rc = readGammaBlocks(ljnx, max_xforms_c - j - 1, gamma_blks_c[j+1]);
	if (rc<0) return(rc);

	dst_nbx = ljp1nx;
	dst_nby = 2;

	slab1 = lambda_blks_c[j+1];
	slab2 = slab1 + (block_size_c * dst_nbx * dst_nby);

	// Apply inverse transform to go from level j to j+1
	//
	for(x=0; x<(int)ljnx; x++) {

		src_super_blk[0] = lambda_row + (x * block_size_c);
		for(i=1; i<8; i++) {
			src_super_blk[i] = gamma_blks_c[j+1] + 
				(x*block_size_c*7) + (block_size_c * (i-1));
		}

		dst_super_blk[0] = slab1;
		dst_super_blk[1] = slab1 + block_size_c;
		dst_super_blk[2] = slab1 + (block_size_c * dst_nbx);
		dst_super_blk[3] = slab1 + (block_size_c * dst_nbx) + block_size_c;
		dst_super_blk[4] = slab2;
		dst_super_blk[5] = slab2 + block_size_c;
		dst_super_blk[6] = slab2 + (block_size_c * dst_nbx);
		dst_super_blk[7] = slab2 + (block_size_c * dst_nbx) + block_size_c;


		wb3d_c->InverseTransform(src_super_blk,dst_super_blk);

		slab1 += 2*block_size_c;
		slab2 += 2*block_size_c;
	}


	if (j+1 < level) {
		for(z=0; z<2; z++) {
		for(y=0; y<2; y++) {

			// cull rows outside of subregion
			//
			if (z+ljp1z0 < sljp1z0 || z+ljp1z0 > sljp1z1) continue;
			if (y+ljp1y0 < sljp1y0 || y+ljp1y0 > sljp1y1) continue;

			srcptr = lambda_blks_c[j+1] + 
				(block_size_c * 
				((z*2*ljp1nx) + (y*ljp1nx)));

			if (sljp1x0 % 2) srcptr += block_size_c;

			rc = row_inv_xform(
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
				(block_size_c * 
				((z*2*ljp1nx) + (y*ljp1nx)));

			if (sljp1x0 % 2) srcptr += block_size_c;

			zz = z + ljp1z0 - sljp1z0;
			yy = y + ljp1y0 - sljp1y0;
			dstptr = region + 
				(block_size_c *
				((zz * regny * regnx) + 
				(yy * regnx)));

			if (! unblock) {
				memcpy(
					dstptr, srcptr, 
					sljp1nx*block_size_c*sizeof(srcptr[0])
				);
			}
			else {
				for(x=0;x<sljp1nx;x++) {
					const size_t bcoord[3] = {x,yy,zz};
					Block2NonBlock(srcptr, bcoord, dim, region);
					srcptr += block_size_c;
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



int	WaveletBlock3DRegionReader::my_alloc(
) {

	size_t nb_j[3];
	int     size;
	int	j;

	// alloc space from coarsest (j==0) to finest level
	for(j=0; j<=max_xforms_c-num_xforms_c; j++) {

		GetDimBlk(max_xforms_c - j, nb_j);
		nb_j[0] += 1;	// fudge (deal with odd size dimensions)

		size = (int)(nb_j[0] * block_size_c * 2 * 2);

		lambda_blks_c[j] = new float[size];
		if (! lambda_blks_c[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
		}

		size = (int)(nb_j[0] * block_size_c * 2 * 2 * 7);

		gamma_blks_c[j] = new float[size];
		if (! gamma_blks_c[j]) {
			SetErrMsg("new float[%d] : %s", size, strerror(errno));
			return(-1);
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
	}
}
