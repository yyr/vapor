#include <cassert>
#include <sstream>
#include <sstream>
#include <iterator>
#include "vapor/WASP.h"
#include "vapor/MatWaveBase.h"
#include "vapor/Compressor.h"

using namespace VAPoR;
using namespace VetsUtil;


namespace {

//
// Map possibly unaligned hyperslab coords (start and count) 
// into block-aligned coordinates
//
void block_align(
	const vector <size_t> &start, const vector <size_t> &count,
	const vector <size_t> &bs, vector <size_t> &astart, vector <size_t> &acount
) {
	astart.clear();
	acount.clear();

	assert(start.size() == count.size());
	assert(start.size() == bs.size());

	for (int i=0; i<start.size(); i++) {
		astart.push_back(start[i] / bs[i] * bs[i]);
		acount.push_back(count[i] + start[i] - astart[i]);
	}
}


// convert multi-dimensional coordinates, 'coords', for a space
// with dimensions, 'dims', to a linear offset from the origin
// of dims
//
size_t _linearize(
	vector <size_t> coords, vector <size_t> dims
) {
	size_t offset = 0;

	assert (coords.size() == dims.size());
	for (int i=0; i<coords.size(); i++) {
		assert(coords[i] < dims[i]);
	}

	size_t factor = 1;
	for (int i=coords.size()-1; i>=0; i--) {
		offset += factor * coords[i];
		factor *= dims[i];
	}
	return(offset);
}

 //
 // Helper class for NetCDF style hyperslab indexing arithmetic 
 //
 class vectorinc {
 public:
  // start and count define hyperslab coordinates
  // dims define overall dimensions of array
  // inc is the increment step to be added to start 
  //
  vectorinc(
	vector <size_t> start, vector <size_t> count,
	vector <size_t> dims, vector <size_t> inc
  );

  // Compute next starting coordinates by adding inc to previous
  // starting coordinates. As a convenience return the linear offset
  // of the new starting coordinates coordinates.
  //
  bool next(vector <size_t> &newstart, size_t &offset);

  // Computh i'th coordinate after 'index' increments by 'inc'
  //
  void ith(
    size_t index, vector <size_t> &start, size_t &offset
  ) const;

  // Number of iterations required to increment 'start' by 'inc' 
  // to get to 'count'
  //
  size_t num() const {return(_num);};

 private:
  vector <size_t> _start;
  vector <size_t> _count;
  vector <size_t> _end;	// ending coordinates (start + count)
  vector <size_t> _dims;
  vector <size_t> _next;
  vector <size_t> _inc;
  size_t _num;
 };

vectorinc::vectorinc(
	vector <size_t> start,
	vector <size_t> count,
	vector <size_t> dims,
	vector <size_t> inc
) {
	assert(start.size() == count.size());
	assert(start.size() == dims.size());
	assert(start.size() >= inc.size());
	while (inc.size() < start.size()) {
		inc.insert(inc.begin(), 1);
	}

	for (int i=0; i<start.size(); i++) {
		_end.push_back(start[i] + count[i]);
		assert(_end[i] <= dims[i]);
	}
		
	_start = start;
	_next = start;
	_count = count;
	_dims = dims;
	_inc = inc;
	_num = 1;

	bool done;
	do {	
		done = false;
		int i = start.size()-1;
		while (i >= 0 && ! done) {
			start[i] += _inc[i];
			if (start[i] >= _end[i]) {
				start[i] = _start[i];
			}
			else { 
				done = true;
				_num++;
			}
			i--;
		}
	} while (done);
} 

void vectorinc::ith(
	size_t index, vector <size_t> &start, size_t &offset
) const {
	assert (index < _num);

	start.clear();
	offset = 0;

	start = _start;
	for (size_t idx = 0; idx < index; idx++) {

		int i = start.size()-1;
		bool done = false;
		while (i >= 0 && ! done) {
			start[i] += _inc[i];
			if (start[i] >= _end[i]) {
				start[i] = _start[i];
			}
			else {
				done = true;
			}
			i--;
		}
	}

	offset = _linearize(start, _dims);
}

bool vectorinc::next(vector <size_t> &start, size_t &offset) {

	offset = 0;

	int i = _next.size()-1;
	bool done = false;
	while (i >= 0 && ! done) {
		_next[i] += _inc[i];
		if (_next[i] >= _end[i]) {
			_next[i] = _start[i];
		}
		else { 
			done = true;
		}
		i--;
	}
	start = _next;

	offset = _linearize(start, _dims);

	return(done);
}

// Execution thread state for data reads and writes
//
class thread_state {
public:
 int _id;
 EasyThreads *_et;
 string _varname;
 vector <NetCDFCpp *> _ncdfcptrs;	// one for each file
 vector <size_t> _start;
 vector <size_t> _count;
 vector <size_t> _bs;
 vector <size_t> _udims;
 vector <size_t> _ncoeffs;
 vector <size_t> _encoded_dims;
 vector <Compressor *> _compressors;	// one for each thread
 float *_data;
 float *_block;
 float *_coeffs;
 unsigned char *_maps;
 int _level;
 static int _status;

 thread_state(
	int id, EasyThreads *et, string &varname, 
	const vector <NetCDFCpp *> &ncdfcptrs, 
	const vector <size_t> &start, 
	const vector <size_t> &count, 
	const vector <size_t> &bs, const vector <size_t> &udims,
	const vector <size_t> &ncoeffs, const vector <size_t> &encoded_dims,
	const vector <Compressor *> &compressors,  
	float *data, float *block, float *coeffs,
	unsigned char *maps, int level
 ) : _id(id), _et(et), _varname(varname), _ncdfcptrs(ncdfcptrs), 
	_start(start), _count(count), _bs(bs), _udims(udims),
	_ncoeffs(ncoeffs), _encoded_dims(encoded_dims),
	_compressors(compressors), _data(data), _block(block), _coeffs(coeffs),
	_maps(maps), _level(level)
 {_status = 0;}

};
int thread_state::_status = 0;



// Convert voxel coordinates, 'vcoords', to block coordinates, 'bcoords', 
// assuming a block size of 'bs'. Handles case where 
// rank(bs) < rank(vcoords)
//
void _to_block_coords(
	vector <size_t> vcoords,
	vector <size_t> bs,
	vector <size_t> &bcoords
) { 
	bcoords = vcoords;

	size_t factor = 1;
	size_t offset = 0;
	for (int i=bcoords.size()-1; i>=0; i--) {
		offset += factor * (bcoords[i] % bs[i]);
		factor *= bs[i];

		bcoords[i] /= bs[i];
	}

	bcoords.push_back(offset);	// offset from start of block
}


//
// Pad a line using the appropriate boundary extension method
// based on 'mode'
//
void _pad_line(
	string mode,
	float *line_start, 
	size_t l1,	// length of valid data
	size_t l2,	// total length of array
	long stride
) {
	float *ptr = line_start + ((long) l1 * stride);

	long index;
	int inc;

	assert(l1>0 && stride != 0);
	if (l1==l2) return;

	if (l1 == 1) {
		int dir = stride < 0 ? -1 : 1;
		for (size_t l=l1; l<l2; l+=dir) {
			*ptr = *line_start;
			ptr += stride;
		}
		return;
	}
		

	//
	// Symmetric-halfpoint. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	EEDCBAABCDEEDCBAA
	//	      ^^^^^
	//
	if (mode.compare("symh") == 0) {
		index = (long) l1 - 1;
		inc = 0;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
			if (index == 0) {
				if (inc == 0) inc = 1;
				else inc = 0;
			}
			else if (index == (long) l1 - 1) {
				if (inc == 0) inc = -1;
				else inc = 0;
			}
			index += inc;
		}
	}
	//
	// Symmetric-wholepoint. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	DEDCBABCDEDCBAB
	//	     ^^^^^
	else if (mode.compare("symw") == 0) {
		index = (long) l1 - 2;
		inc = -1;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
			if (index == 0) {
				inc = 1;
			}
			else if (index == (long) l1 - 1) {
				inc = -1;
			}
			index += inc;
		}
	}
	//
	// Periodic. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	...ABCDABCDEABCDABCD...
	//	       ^^^^^
	else if (mode.compare("per") == 0) {
		index = (int) 0;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
			index++;
		}
	}
	//
	// SP0. If a signal looks like ABCDE, the extended signal
	// looks like:
	//
	//	...AAAAABCDEEEEE...
	//	       ^^^^^
	else if (mode.compare("sp0") == 0) {
		index = (long) l1-1;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
		}
	}
	// Default to sp0
	//
	else {
		index = (long) l1-1;

		for (size_t l=l1; l<l2; l++) {
			*ptr = line_start[(size_t) index * stride];
			ptr += stride;
		}
	}
}


void _swapbytes(void *ptr, size_t ws, size_t nelem) {

	for (size_t i = 0; i<nelem; i++) {
		unsigned char *uptr = ((unsigned char *) ptr) + (i*ws);

		unsigned char *p1 = uptr;
		unsigned char *p2 = uptr + ws-1;
		unsigned char t;
		for (int j = 0; j< ws >> 1; j++) {
			t = *p1;
			*p1 = *p2;
			*p2 = t;
			p1++;
			p2--;
		}
	}
}

// Sum elements in a vector
//
size_t vsum(vector <size_t> v) {
	size_t ntotal = 0;

	for (int i=0; i<v.size(); i++) ntotal += v[i];
	return(ntotal);
}

// Product of elements in a vector
//
size_t vproduct(vector <size_t> a) {
	size_t ntotal = 1;

	for (int i=0; i<a.size(); i++) ntotal *= a[i];
	return(ntotal);
}

// Elementwise difference between vector a and b (return (a-b));
//
vector <size_t> vdiff(vector <size_t> a, vector <size_t> b) {
	assert(a.size() == b.size());

	vector <size_t> c(a.size(), 0);

	for (int i=0; i<a.size(); i++) c[i] = a[i] - b[i];
	return(c);
}

// Extract a single block of data from an array. Perform padding as
// needed based on mode value if this is a boundary block
// Handles 1D, 2D, and 3D arrays
//
// dims : dimensions of array
// start: starting coordinates of block within array (need not be block aligned)
// bs : dimensions of block. May have rank <= dims, in which case only
// fastest varying dimensions are used
// data : pointer to start of array
// block : pointer to start of block where data should be copied
// min, max : range of data values within block
//
void _Block(
	vector <size_t> origin,
	vector <size_t> dims, 
	vector <size_t> start,
	vector <size_t> bs, 
	string mode, 
	const float *data, float *block,
	float &min, float &max
) {
	min = 0.0;
	max = 0.0;

	// Only 1D, 2D, and 3D blocks handled
	//
	assert(bs.size() >=1 && bs.size() <= 3);
	int rank = bs.size();

	// dimensions of volume
	//
	size_t nx = rank >= 1 ? dims[rank-1] : 1;
	size_t ny = rank >= 2 ? dims[rank-2] : 1;
	size_t nz = rank >= 3 ? dims[rank-3] : 1;

	// dimensions of block
	//
	size_t nbx = rank >= 1 ? bs[rank-1] : 1;
	size_t nby = rank >= 2 ? bs[rank-2] : 1;
	size_t nbz = rank >= 3 ? bs[rank-3] : 1;

	// starting coordinates within 'data'
	//
	size_t x0 = (rank >= 1 && start[rank-1] > origin[rank-1]) ?
		start[rank-1] - origin[rank-1] : 0; 
	size_t y0 = (rank >= 2 && start[rank-2] > origin[rank-2]) ?
		start[rank-2] - origin[rank-2] : 0;
	size_t z0 = (rank >= 3 && start[rank-3] > origin[rank-3]) ?
		start[rank-3] - origin[rank-3] : 0; 


	// starting and stop coordinates within block, handling boundary cases
	// for non-block-aligned regions
	//
	size_t xstart = 0;
	size_t ystart = 0;
	size_t zstart = 0;

	if (rank>=1 && (origin[rank-1] > start[rank-1]))    // non-aligned boundary
		xstart = origin[rank-1] - start[rank-1];
	if (rank>=2 && (origin[rank-2] > start[rank-2]))    // non-aligned boundary
		ystart = origin[rank-2] - start[rank-2];
	if (rank>=3 && (origin[rank-3] > start[rank-3]))    // non-aligned boundary
		zstart = origin[rank-3] - start[rank-3];


	size_t xstop = nbx;
	size_t ystop = nby;
	size_t zstop = nbz;

	if (rank>=1 && (start[rank-1] + nbx) > (origin[rank-1] + dims[rank-1])) {
		xstop = origin[rank-1] + dims[rank-1] - start[rank-1];
	}
	if (rank>=2 && (start[rank-2] + nby) > (origin[rank-2] + dims[rank-2])) {
		ystop = origin[rank-2] + dims[rank-2] - start[rank-2];
	}
	if (rank>=3 && (start[rank-3] + nbz) > (origin[rank-3] + dims[rank-3])) {
		zstop = origin[rank-3] + dims[rank-3] - start[rank-3];
	}

	//
	// These flags are true if this is a boundary block && the volume
	// dimensions are not block aligned.
	//
	bool xbdry = rank >= 1 && start[rank-1] + nbx > nx;
	bool ybdry = rank >= 2 && start[rank-2] + nby > ny;
	bool zbdry = rank >= 3 && start[rank-3] + nbz > nz;

	min = data[nx*ny*z0 + nx*y0 + x0];
	max = data[nx*ny*z0 + nx*y0 + x0];
	for (size_t z = zstart, zz=0; z<zstop; z++,zz++) {
	for (size_t y = ystart, yy=0; y<ystop; y++,yy++) {
	for (size_t x = xstart, xx=0; x<xstop; x++,xx++) {
		float v = data[nx*ny*(z0+zz) + nx*(y0+yy) + (x0+xx)];

		if (v < min) min = v;
		if (v > max) max = v;

		block[z*nbx*nby + y*nbx + x] = v;
	}
	}
	}

	if (xbdry) {

		float *line_start;
		for (size_t z = 0; z<nbz; z++) {  
		for (size_t y = 0; y<nby; y++) {  
			line_start = block + z*nby*nbx + y*nby;

			_pad_line(mode, line_start, xstop, nbx, 1);
		}
		}
	}

	if (ybdry ) {

		float *line_start;
		for (size_t z = 0; z<nbz; z++) {  
		for (size_t x = 0; x<nbx; x++) {  
			line_start = block + z*nby*nbx + x;

			_pad_line(mode, line_start, ystop, nby, nbx);
		}
		}
	}

	if (zbdry) {

		float *line_start;
		for (size_t y = 0; y<nby; y++) {  
		for (size_t x = 0; x<nbx; x++) {  
			line_start = block + y*nbx + x;

			_pad_line(mode, line_start, zstop, nbz, nby*nbx);
		}
		}
	}
}

// Copy a block of blocked data into a contiguous array (unblocking
// the data). Handles 1D, 2D, and 3D arrays
//
// origin : coordinates of origin of data array
// dims : dimensions of array
// start: starting coordinates of block within array
// bs : dimensions of block. May have rank <= dims, in which case only
// fastest varying dimensions are used
// data : pointer to start of array
// block : pointer to start of block where data should be copied
// min, max : range of data values within block
//
void _UnBlock(
	vector <size_t> origin, 
	vector <size_t> dims, 
	vector <size_t> start,
	vector <size_t> bs, 
	float *data, const float *block
) {

	// Only 1D, 2D, and 3D blocks handled
	//
	assert(bs.size() >=1 && bs.size() <= 3);
	int rank = bs.size();

	// dimensions of volume
	//
	size_t nx = rank >= 1 ? dims[rank-1] : 1;
	size_t ny = rank >= 2 ? dims[rank-2] : 1;
	size_t nz = rank >= 3 ? dims[rank-3] : 1;

	// dimensions of block
	//
	size_t nbx = rank >= 1 ? bs[rank-1] : 1;
	size_t nby = rank >= 2 ? bs[rank-2] : 1;
	size_t nbz = rank >= 3 ? bs[rank-3] : 1;

	// starting coordinates within 'data'
	//
	size_t x0 = (rank >= 1 && start[rank-1] > origin[rank-1]) ? 
		start[rank-1] - origin[rank-1] : 0;
	size_t y0 = (rank >= 2 && start[rank-2] > origin[rank-2]) ? 
		start[rank-2] - origin[rank-2] : 0;
	size_t z0 = (rank >= 3 && start[rank-3] > origin[rank-3]) ? 
		start[rank-3] - origin[rank-3] : 0;


	// starting and stop coordinates within block, handling boundary cases
	// for non-block-aligned regions
	//
	size_t xstart = 0;
	size_t ystart = 0;
	size_t zstart = 0;

	if (rank>=1 && (origin[rank-1] > start[rank-1])) 	// non-aligned boundary
		xstart = origin[rank-1] - start[rank-1];
	if (rank>=2 && (origin[rank-2] > start[rank-2])) 	// non-aligned boundary
		ystart = origin[rank-2] - start[rank-2];
	if (rank>=3 && (origin[rank-3] > start[rank-3])) 	// non-aligned boundary
		zstart = origin[rank-3] - start[rank-3];


	size_t xstop = nbx;
	size_t ystop = nby;
	size_t zstop = nbz;

	if (rank>=1 && (start[rank-1] + nbx) > (origin[rank-1] + dims[rank-1])) {
		xstop = origin[rank-1] + dims[rank-1] - start[rank-1];
	}
	if (rank>=2 && (start[rank-2] + nby) > (origin[rank-2] + dims[rank-2])) {
		ystop = origin[rank-2] + dims[rank-2] - start[rank-2];
	}
	if (rank>=3 && (start[rank-3] + nbz) > (origin[rank-3] + dims[rank-3])) {
		zstop = origin[rank-3] + dims[rank-3] - start[rank-3];
	}

	for (size_t z = zstart, zz=0; z<zstop; z++,zz++) {
	for (size_t y = ystart, yy=0; y<ystop; y++,yy++) {
	for (size_t x = xstart, xx=0; x<xstop; x++,xx++) {

		data[nx*ny*(z0+zz) + nx*(y0+yy) + (x0+xx)] = block[z*nbx*nby + y*nbx + x];
	}
	}
	}
}

// Apply forward wavelet transfor to a block of data
//
// cmp : Compressor for wavelet transform
// block : block of data
// ncoeffs : vector describing partitioning of coefficients in 'coeffs'
// encoded_dims : vector describing dimension of encoded block at
// each compression level.
// coeffs : storage for transformed coefficients
// maps : storage for encoded significance maps
//
int _DecomposeBlock(
	Compressor *cmp,
	const float *block,
	vector <size_t> ncoeffs,
	vector <size_t> encoded_dims,
	float *coeffs,
	unsigned char *maps
	
) {

	vector <SignificanceMap> sigmaps(4);

	int rc = cmp->Decompose(block, coeffs, ncoeffs, sigmaps);
	if (rc<0) return(-1);

	unsigned char *mapptr = maps;
	for (int i=0; i<ncoeffs.size(); i++) {
		if (encoded_dims[i] != ncoeffs[i]) {	// last map not stored
			memset(mapptr, 0, sizeof(float) * (encoded_dims[i] - ncoeffs[i]));
			sigmaps[i].GetMap(mapptr);
			mapptr += sizeof(float) * (encoded_dims[i] - ncoeffs[i]);
		}
	}

	return(0);
}

// Apply inverse wavelet transfor to a block of data
//
// cmp : Compressor for wavelet transform
// block : block of data
// ncoeffs : vector describing partitioning of coefficients in 'coeffs'
// encoded_dims : vector describing dimension of encoded block at
// each compression level.
// coeffs : storage for transformed coefficients
// maps : storage for encoded significance maps
//
int _ReconstructBlock(
	Compressor *cmp,
	float *block,
	vector <size_t> ncoeffs,
	vector <size_t> encoded_dims,
	const float *coeffs,
	const unsigned char *maps,
	int level
	
) {

	vector <SignificanceMap> sigmaps(4);

	const unsigned char *mapptr = maps;
	bool reconstruct_map = false;
	for (int i=0; i<ncoeffs.size(); i++) {
		if (encoded_dims[i] != ncoeffs[i]) {	// last map not stored
			sigmaps[i].SetMap(mapptr);
			mapptr += sizeof(float) * (encoded_dims[i] - ncoeffs[i]);
		}
		else {
			reconstruct_map = true;
		}
	}

	//
	// In some cases the significance map need not be explicity stored,
	// and can be reconstructed from previous sigmaps
	//
	if (reconstruct_map) {
		sigmaps[ncoeffs.size()-1].Clear();

		for (int i=0; i<ncoeffs.size()-1; i++) {
			sigmaps[ncoeffs.size()-1].Append(sigmaps[i]);
		}
		sigmaps[ncoeffs.size()-1].Sort();
		sigmaps[ncoeffs.size()-1].Invert();
	}

	int rc = cmp->Reconstruct(coeffs, block, sigmaps, level);
	if (rc<0) return(-1);

	return(0);
}

// Write a single transformed & compressed block to disk
//
// varname : name of variable
// bcoords : coordinates of block
// ncoeffs : vector describing partitioning of coefficients in 'coeffs'
// encoded_dims : vector describing dimension of encoded block at
// each compression level.
// coeffs : transformed coefficients for each compression level
// maps : encoded significance maps for each compression level
//
int _StoreBlock(
	string varname, vector <NetCDFCpp *> ncdfcptrs, vector <size_t> bcoords, 
	vector <size_t> ncoeffs, vector <size_t> encoded_dims,
	const float *coeffs, unsigned char *maps
	
) {
    unsigned long LSBTest = 1;
    bool do_swapbytes = false;
    if (! (*(char *) &LSBTest)) {
        // swap to MSBFirst
        do_swapbytes = true;
    }

	vector <size_t> start = bcoords;
	vector <size_t> count;
	count.resize(start.size(), 1);

	// 
	// Current code assumes each wavelet decomposition is stored in a 
	// different file
	//
	assert(ncdfcptrs.size() >= ncoeffs.size());
	for (int i=0; i<ncoeffs.size(); i++) {
		start[start.size()-1] = 0;
		count[start.size()-1] = ncoeffs[i];

		int rc = ncdfcptrs[i]->NetCDFCpp::PutVara(
			varname, start, count, (const void *) coeffs
		);
		if (rc<0) return(rc);

		coeffs += ncoeffs[i];

		// Sigmap size (in words) is difference between encoded_dims and 
		// number of coefficients
		//
		assert(encoded_dims[i] >= ncoeffs[i]);
		size_t n = encoded_dims[i] - ncoeffs[i];

		//
		// If sigmap size is zero don't write it!
		//
		if (n != 0) {
			start[start.size()-1] += ncoeffs[i];
			count[start.size()-1] = n;

			//
			// Should be checking size of external type for var
			//
			if (do_swapbytes) {
				_swapbytes((void *) maps, sizeof(float), n);
			}

			// Signficance map is concatenated to the wavelet coefficients
			// variable to improve IO performance
			//
			int rc = ncdfcptrs[i]->NetCDFCpp::PutVara(
				varname, start, count, (const void *) maps
			);
			if (rc<0) return(rc);

			maps += n * sizeof(float);
		}
	}
	return(0);
}

// Read a single transformed & compressed block from disk
//
// varname : name of variable
// bcoords : coordinates of block
// ncoeffs : vector describing partitioning of coefficients in 'coeffs'
// encoded_dims : vector describing dimension of encoded block at
// each compression level.
// coeffs : transformed coefficients for each compression level
// maps : encoded significance maps for each compression level
//
int _FetchBlock(
	string varname, vector <NetCDFCpp *> ncdfcptrs, vector <size_t> bcoords, 
	vector <size_t> ncoeffs, vector <size_t> encoded_dims,
	float *coeffs, unsigned char *maps
	
) {
    unsigned long LSBTest = 1;
    bool do_swapbytes = false;
    if (! (*(char *) &LSBTest)) {
        // swap to MSBFirst
        do_swapbytes = true;
    }

	vector <size_t> start = bcoords;
	vector <size_t> count;
	count.resize(start.size(), 1);

	// 
	// Current code assumes each wavelet decomposition is stored in a 
	// different file
	//
	assert(ncdfcptrs.size() >= ncoeffs.size());
	for (int i=0; i<ncoeffs.size(); i++) {
		start[start.size()-1] = 0;
		count[start.size()-1] = ncoeffs[i];

		int rc = ncdfcptrs[i]->NetCDFCpp::GetVara(
			varname, start, count, (void *) coeffs
		);
		if (rc<0) return(rc);

		coeffs += ncoeffs[i];

		// Sigmap size (in words) is difference between encoded_dims and 
		// number of coefficients
		//
		assert(encoded_dims[i] >= ncoeffs[i]);
		size_t n = encoded_dims[i] - ncoeffs[i];

		//
		// If sigmap size is zero don't read it!
		//
		if (n != 0) {
			start[start.size()-1] += ncoeffs[i];
			count[start.size()-1] = n;


			// Signficance map is concatenated to the wavelet coefficients
			// variable to improve IO performance
			//
			int rc = ncdfcptrs[i]->NetCDFCpp::GetVara(
				varname, start, count, maps
			);
			if (rc<0) return(rc);

			//
			// Should be checking size of external type for var
			//
			if (do_swapbytes) {
				_swapbytes((void *) maps, sizeof(float), n);
			}

			maps += n * sizeof(float);
		}
	}
	return(0);
}

// Thread execution function for data writes
//
void *_RunWriteThread(void *arg) {
	thread_state &s = *(thread_state *) arg;

	// Align start and count coordinates to block boundaries to
	// facilitate blocking
	//
	vector <size_t> aligned_start;
	vector <size_t> aligned_count;
	block_align(s._start, s._count, s._bs, aligned_start, aligned_count);

	vectorinc vec(aligned_start, aligned_count, s._udims, s._bs);

	s._status = 0;

	int n = vec.num();
	for (int i=s._id; i<n; i += s._et->GetNumThreads()) {

		size_t offset;
		vector <size_t> start;

		// Get starting coordinates of i'th block
		//
		vec.ith(i, start, offset);

		// Convert from voxel to block coordinates
		//
		vector <size_t> bcoords;
		_to_block_coords(start, s._bs, bcoords);

		//
		// Extract the block with coordinates 'start' from the 
		// array, 's._data'
		//
		float min, max;
		_Block(
			s._start, s._count, start, s._bs, s._compressors[s._id]->dwtmode(),
			s._data, s._block, min, max
		);

		int rc = _DecomposeBlock(
			s._compressors[s._id], (const float *) s._block, s._ncoeffs, 
			s._encoded_dims, s._coeffs, s._maps
		);
		if (rc<0) {
			s._status = -1;
			break;
		}

		s._et->MutexLock();
			rc = _StoreBlock(
				s._varname, s._ncdfcptrs, bcoords, s._ncoeffs, s._encoded_dims,
				s._coeffs, s._maps
			);
			if (rc<0) {
				s._status = -1;
			}
		s._et->MutexUnlock();
		if (s._status < 0) break;
	}
	return(0);
}

// Thread execution function for data writes
//
void *_RunReadThread(void *arg) {
	thread_state &s = *(thread_state *) arg;


	// Align start and count coordinates to block boundaries
	//
	vector <size_t> aligned_start;
	vector <size_t> aligned_count;
	block_align(s._start, s._count, s._bs, aligned_start, aligned_count);

	vectorinc vec(aligned_start, aligned_count, s._udims, s._bs);

	s._status = 0;

	int n = vec.num();
	for (int i=s._id; i<n; i += s._et->GetNumThreads()) {

		size_t offset;
		vector <size_t> start;

		vec.ith(i, start, offset);
		
		vector <size_t> bcoords;
		_to_block_coords(start, s._bs, bcoords);

		s._et->MutexLock();
			int rc = _FetchBlock(
				s._varname, s._ncdfcptrs, bcoords, s._ncoeffs, 
				s._encoded_dims, s._coeffs, s._maps
			);
			if (rc<0) s._status = -1;
		s._et->MutexUnlock();
		if (s._status < 0) break;

		rc = _ReconstructBlock(
			s._compressors[s._id], s._block, s._ncoeffs, s._encoded_dims, 
			s._coeffs, s._maps, s._level
		);
		if (rc<0) {
			s._status = -1;
            break;
        }

		_UnBlock(s._start, s._count, start, s._bs, s._data, s._block);

	}
	return(NULL);
}

};

WASP::WASP(int nthreads) {
	_ncdfcs.clear();
	_ncdfcptrs.clear();

	_compressionMode = false;
	_wname.clear();
	_bs.clear();
	_ncratios = 0;
	_nthreads = 1;

	_open = false;
	_open_bs.clear();
	_open_cratios.clear();
	_open_udims.clear();
	_open_dims.clear();
	_open_lod = 0;
	_open_level = 0;
	_open_write = false;
	_open_slice_count = 0;
	_open_varname.clear();

	_et = NULL;

	// Set up execution threads for parallel execution
	//
	if (nthreads < 1) nthreads = EasyThreads::NProc();
	if (nthreads < 1) nthreads = 1;

	_et = new EasyThreads(nthreads);

	_nthreads = nthreads;

	// One Compressor instance for each thread
	//
	_open_compressors.resize(nthreads, NULL);
}

WASP::~WASP() {
	for (int i=0; i<_open_compressors.size(); i++) {
		if (_open_compressors[i]) delete _open_compressors[i];
	}
	if (_et) delete _et;
}

int WASP::Create(
    string path, int cmode, size_t initialsz,
    size_t &bufrsizehintp, string wname,
    vector <size_t> bs, int ncratios, bool multifile
) {
	int rc = WASP::Close();
	if (rc<0) return(rc);

	_ncdfcs.clear();
	_ncdfcptrs.clear();
	_ncdfcptrs.push_back(this);

	if (ncratios < 1) {
		SetErrMsg("Invalid cratios specification");
		return(-1);
	}

	if (bs.size() < 1 || bs.size() > 3) {
		SetErrMsg("Invalid block specification");
		return(-1);
	}

	MatWaveBase mwb(wname);
	if (! mwb.wavelet()) {
		SetErrMsg("Invalid wname specification");
		return(-1);
	}

	
	vector <string> paths;
	if (multifile) {
		paths = mkmultipaths(path, ncratios);
	}
	else {
		paths.push_back(path);
	}

	rc = NetCDFCpp::Create(paths[0], cmode, initialsz, bufrsizehintp);
	if (rc<0) return(rc);
	

	for (int i=1; i<paths.size(); i++) {
		NetCDFCpp netcdfcpp;

		rc = netcdfcpp.Create(
			paths[i], cmode, initialsz, bufrsizehintp
		);
		if (rc<0) return(rc);
		_ncdfcs.push_back(netcdfcpp);
	}

	for (int i=0; i<_ncdfcs.size(); i++) {
		_ncdfcptrs.push_back(&(_ncdfcs[i]));
	}

	_wname = wname;
	_bs = bs;
	_ncratios = ncratios;
	_compressionMode = true;

	// Attributes describing the compressed data
	//
	rc = PutAtt("", AttNameCompressed(), 1);
	if (rc<0) return(rc);

	rc = PutAtt("", AttNameWavelet(), wname);
	if (rc<0) return(rc);

	rc = PutAtt("", AttNameBlockSize(), bs);
	if (rc<0) return(rc);

	rc = PutAtt("", AttNameNumCRatios(), ncratios);
	if (rc<0) return(rc);

	rc = PutAtt("", AttNameMultifile(), multifile);
	if (rc<0) return(rc);

	return(NC_NOERR);
}

int WASP::Open(
	string path, int mode
) {
	int rc = WASP::Close();
	if (rc<0) return(rc);

	_ncdfcs.clear();
	_ncdfcptrs.clear();
	_ncdfcptrs.push_back(this);

	_compressionMode = false;
	_wname.clear();
	_bs.clear();
	_ncratios = 0;

    rc = NetCDFCpp::Open(path.c_str(), mode);
	if (rc<0) return(rc);

	bool compressed; 
	rc = InqVarCompressed("", compressed);
	if (rc<0) return(-1);

	bool multifile = false;
	int ncratios = 0;
	vector <size_t> bs;
	string wname;
	if (compressed) {
		rc = GetAtt("", AttNameMultifile(), (int &) multifile);
		if (rc<0) return(rc);

		rc = GetAtt("", AttNameNumCRatios(), ncratios);
		if (rc<0) return(rc);

		rc = GetAtt("", AttNameWavelet(), wname);
		if (rc<0) return(rc);

		rc = GetAtt("", AttNameBlockSize(), bs);
		if (rc<0) return(rc);
	}
    _wname = wname;
    _bs = bs;
    _ncratios = ncratios;
    _compressionMode = compressed;
	

    vector <string> paths;
	if (multifile) {
		paths = mkmultipaths(path, ncratios);
	}

	for (int i=1; i<paths.size(); i++) {
		NetCDFCpp netcdfcpp;

		rc = netcdfcpp.Open(paths[i], mode);
		if (rc<0) return(rc);

		_ncdfcs.push_back(netcdfcpp);
	}

	for (int i=0; i<_ncdfcs.size(); i++) {
		_ncdfcptrs.push_back(&(_ncdfcs[i]));
	}

	return(NC_NOERR);
}

int WASP::SetFill(int fillmode, int &old_modep) {

	// Set old_modep only for first file
	//
	int rc = NetCDFCpp::SetFill(fillmode, old_modep);
	if (rc<0) return(rc);

	for (int i=1; i<_ncdfcptrs.size(); i++) {
		int my_old_modep;

		rc = _ncdfcptrs[i]->NetCDFCpp::SetFill(fillmode, my_old_modep);
		if (rc<0) return(rc);
	}
	return(NC_NOERR);
}

int WASP::EndDef() const {

	for (int i=0; i<_ncdfcptrs.size(); i++) {

		int rc = _ncdfcptrs[i]->NetCDFCpp::EndDef();
		if (rc<0) return(rc);
	}
	return(NC_NOERR);
}

int WASP::Close() {

	int rc = 0;
	for (int i=0; i<_ncdfcptrs.size(); i++) {

		int my_rc = _ncdfcptrs[i]->NetCDFCpp::Close();
		if (my_rc<0) rc = -1;
	}
	_ncdfcptrs.clear();

	return(rc);
}

int WASP::DefDim(string name, size_t len) const {

	//
	// Dimensions get defined identically in every file
	//
	for (int i=0; i<_ncdfcptrs.size(); i++) {
		int rc = _ncdfcptrs[i]->NetCDFCpp::DefDim(name, len);
		if (rc<0) return(rc);
	}
	return(NC_NOERR);
}

int WASP::DefVar(
    string name, int xtype, vector <string> dimnames, vector <size_t> cratios
) {
	vector <string> cdimnames;
	vector <size_t> cdims;

	if (! _compressionMode) {
		SetErrMsg("Invalid compression mode");
		return(-1);
	}

	if (! (xtype == NC_FLOAT || xtype == NC_DOUBLE)) {
		SetErrMsg("Unsupported xtype specification");
		return(-1);
	}

	if (! _validate_cratios(cratios)) {
		SetErrMsg("Invalid cratios specification");
		return(-1);
	}

	sort(cratios.begin(), cratios.end()); 
	reverse(cratios.begin(), cratios.end());


	// Get dimensions for compressed version of variable. Compressed
	// variables are decomposed into blocks. The dimension of the compressed
	// variable in blocks is given by cdimnames and cdims. The dimension
	// of the blocks themselves varies with compression level and is given
	// by encoded_dim_names and encoded_dims
	//
	vector <string> encoded_dim_names;
	vector <size_t> encoded_dims;
	int rc = _GetCompressedDims(
		dimnames, cratios, xtype, cdimnames, cdims,
		encoded_dim_names, encoded_dims
	);

	// Implicitly define dimensions for compressed variable. One set of
	// dimensions for each compression level. Finally, define 
	// compressed variable itself.
	//
	for (int j=0; j<cdimnames.size(); j++) {
		size_t len;

		rc = _InqDimlen(cdimnames[j], len);
		if (len == 0) {
			rc = WASP::DefDim(cdimnames[j], cdims[j]);
			if (rc<0) return(rc);
		}
		else if (len != cdims[j]) {
			SetErrMsg("Can't implicitly redefine compression dimensions");
			return(-1);
		}
	}

	for (int i=0; i<encoded_dim_names.size(); i++) {
		
		size_t len;
		rc = _InqDimlen(encoded_dim_names[i], len);
		if (len == 0) {
			rc = WASP::DefDim(encoded_dim_names[i], encoded_dims[i]);
			if (rc<0) return(rc);
		}
		else if (len != encoded_dims[i]) {
			SetErrMsg("Can't implicitly redefine compression dimensions");
			return(-1);
		}
	}

	// Now define the variable, one in each file
	//
	assert(_ncdfcptrs.size() == encoded_dim_names.size());
	for (int i=0; i<encoded_dim_names.size(); i++) {
		vector <string> newdimnames;
		newdimnames = cdimnames;
		newdimnames.push_back(encoded_dim_names[i]);

		rc = _ncdfcptrs[i]->NetCDFCpp::DefVar(name, xtype, newdimnames);
		if (rc<0) return(rc);

	}

	// Attributes needed to encode or decode the variable later
	//

	rc = PutAtt(name, AttNameCompressed(), 1);
	if (rc<0) return(rc);

	rc = PutAtt(name, AttNameDimNames(), dimnames);
	if (rc<0) return(rc);

	rc = PutAtt(name, AttNameCRatios(), cratios);
	if (rc<0) return(rc);

	return(NC_NOERR);
}

int WASP::DefVar(
    string name, int xtype, vector <string> dimnames, vector <size_t> cratios,
	double missing_value
) {

	int rc = WASP::DefVar(name, xtype, dimnames, cratios);
	if (rc<0) return(rc);

	rc = PutAtt(name, AttNameMissingValue(), missing_value);
	if (rc<0) return(rc);

	return(NC_NOERR);

}

int WASP::InqVarDims(
    string name, vector <string> &dimnames, vector <size_t> &dims
) const {
	dimnames.clear();
	dims.clear();

	bool compressed = false;

	int rc = InqVarCompressed(name, compressed);
	if (rc<0) return(rc);

	if (! compressed) return(NetCDFCpp::InqVarDims(name, dimnames, dims));

	rc = GetAtt(name, AttNameDimNames(), dimnames);
	if (rc<0) return(rc);
	for (int i=0; i<dimnames.size(); i++) {
		size_t len;

		rc = InqDimlen(dimnames[i], len);
		if (rc<0) return(rc);
		dims.push_back(len);
	}
	return(NC_NOERR);
}

int WASP::InqVarDimlens(
	string name, vector <size_t> &dims, int level
) const {

	vector <string> dimnames;
	int rc = WASP::InqVarDims(name, dimnames, dims);
	if (rc<0) return(rc);


	vector <size_t> bs;
	rc = GetAtt("", AttNameBlockSize(), bs);
	if (rc<0) return(rc);

	vector <size_t> bs_level;
	rc = _dims_at_level(dims, bs, level, dims, bs_level);
	if (rc<0) return(rc);

	return(0);
}

int WASP::_dims_at_level(
	vector <size_t> dims,
	vector <size_t> bs,
	int level,
	vector <size_t> &dims_level,
	vector <size_t> &bs_level
) const {
	dims_level.clear();
	bs_level.clear();

	Compressor cmp(bs, _wname);

	if (level < 0) level = cmp.GetNumLevels();

    if (level > cmp.GetNumLevels()) {
        SetErrMsg("Invalid refinement level: (%d)", level);
        return(-1);
    }

	int  ldelta = cmp.GetNumLevels() - level;

	for (int i0 = bs.size()-1, i1 = dims.size()-1; i0>=0 && i1>=0; i0--,i1--) {
		dims[i1] = dims[i1] >> ldelta;
		if (dims[i1] == 0) dims[i1] = 1;
	}
	dims_level = dims;

	cmp.GetDimension(bs_level, level);

	return(0);
}

// Same as WASP::InqDimlen(), but returns 0 and sets len to 0
// if dimension 'name' does not exist
//
int WASP::_InqDimlen(string name, size_t &len) const {
	len = 0;

	// disable error reporting
	//
	bool enabled = MyBase::EnableErrMsg(false);

	int rc = WASP::InqDimlen(name, len);
	if (rc == NC_EBADDIM) rc = 0;

	(void) MyBase::EnableErrMsg(enabled);

	return(rc);

}

int WASP::InqVarCompressed(
	string varname, bool &compressed 
) const {
	compressed = false;

	int varid;
	int rc = NetCDFCpp::InqVarid(varname, varid);
	if (rc<0) return(rc);

	// disable error reporting
	//
	bool enabled = MyBase::EnableErrMsg(false);

	int xtype;
	size_t len;
	rc = NetCDFCpp::InqAtt(varname, AttNameCompressed(), xtype, len);

	(void) MyBase::EnableErrMsg(enabled);

	// Assume att doesn't exist if InqAtt fails of if len != 1
	//
	if (rc<0 || len != 1) return(NC_NOERR);

	int cflag;
	rc = NetCDFCpp::GetAtt(varname, AttNameCompressed(), cflag);
	compressed = cflag;

	return(rc);
}


int WASP::GetMaxCRatio(
    vector <size_t> bs, string wavename, size_t &cratio
) const {
	cratio = 1;

    Compressor cmp = Compressor(bs, wavename);
    if (Compressor::GetErrCode() != 0) return(NC_NOERR);
 
    // Total number of wavelet coefficients in a forward transform
    //
    size_t ntotal = cmp.GetNumWaveCoeffs();
 
    size_t mincoeff = cmp.GetMinCompression();
    if (mincoeff == 0.0)  mincoeff = 1;

	cratio = ntotal / mincoeff;
	return(NC_NOERR);
}

int WASP::OpenVarWrite(string name, int lod) {
	vector <size_t> bs;
	vector <size_t> cratios;
	vector <size_t> udims;
	vector <size_t> dims;
	string wname;

    if (_ncdfcptrs.size() < 1) {
        SetErrMsg("Invalid state");
        return(-1);
    }
	if (_open) {
		int rc = CloseVar();
		if (rc < 0) return (rc);
	}

	_open = false;

	int rc = _get_compression_params(name, bs, cratios, udims, dims, wname);
	if (rc<0) return(rc);

	if (lod < 0)  lod = cratios.size() - 1;

    if (lod >= cratios.size()) {
        SetErrMsg("Invalid level-of-detail : (%d)", lod);
        return(-1);
    }

	// Create one compressor for each execution thread
	//
	for (int i=0; i<_nthreads; i++) {
		_open_compressors[i] = new Compressor(bs, wname);
	}

	_open_bs = bs;
	_open_cratios = cratios;
	_open_udims = udims;
	_open_dims = dims;
	_open_lod = lod;
	_open_level = 0;
	_open_write = true;
	_open_slice_count = 0;
	_open_varname = name;
	_open = true;

	return(NC_NOERR);
}

int WASP::OpenVarRead(string name, int lod, int level) {
	vector <size_t> bs;
	vector <size_t> cratios;
	vector <size_t> udims;
	vector <size_t> dims;
	string wname;

    if (_ncdfcptrs.size() < 1) {
        SetErrMsg("Invalid state");
        return(-1);
    }

	if (_open) {
		int rc = CloseVar();
		if (rc < 0) return (rc);
	}

	_open = false;

	int rc = _get_compression_params(name, bs, cratios, udims, dims, wname);
	if (rc<0) return(rc);

	if (lod < 0)  lod = cratios.size() - 1;

    if (lod >= cratios.size()) {
        SetErrMsg("Invalid level-of-detail : (%d)", lod);
        return(-1);
    }

	for (int i=0; i<_nthreads; i++) {
		_open_compressors[i] = new Compressor(bs, wname);
	}

	if (level < 0) level = _open_compressors[0]->GetNumLevels();

    if (level > _open_compressors[0]->GetNumLevels()) {
        SetErrMsg("Invalid refinement level: (%d)", level);
		for (int i=0; i<_nthreads; i++) {
			if (_open_compressors[i]) delete _open_compressors[i];
			_open_compressors[i] = NULL;
		}
        return(-1);
    }

	_open_bs = bs;
	_open_cratios = cratios;
	_open_udims = udims;
	_open_dims = dims;
	_open_lod = lod;
	_open_level = level;
	_open_write = false;
	_open_slice_count = 0;
	_open_varname = name;
	_open = true;

	return(NC_NOERR);
}

int WASP::CloseVar() {

	_open = false;
	_open_write = false;
	for (int i=0; i<_nthreads; i++) {
		if (_open_compressors[i]) delete _open_compressors[i];
		_open_compressors[i] = NULL;
	}

	return(0);
}

// Validate parameters to PutVara()
//
bool WASP::_validate_put_vara_compressed(
	vector <size_t> start, vector <size_t> count, 
	vector <size_t> bs, vector <size_t> udims, vector <size_t> cratios
) const {
	if (start.size() != udims.size() || count.size() != udims.size()) {
        return(false);
	}
	assert (bs.size() <= start.size());

	for (int i=0; i<count.size(); i++) {
		if (count[i] < 1 || count[i] > udims[i]) return(false);
	}

	for (int i=0; i<start.size(); i++) {
		if (start[i] >= udims[i]) return(false);
		if ((start[i] + count[i]) > udims[i]) return(false);
	}

	// Count must be block aligned, or, in the case where an array
	// dimension is not block aligned, count must align with the 
	// array dimension boundary
	//
	for (int i0 = bs.size()-1, i1 = count.size()-1; i0>=0 && i1>=0; i0--,i1--) {
		if (((count[i1] % bs[i0]) != 0) && (count[i1]+start[i1] !=udims[i1])) {
			return(false);
		}
	}

	// Start must *always* be block aligned
	//
	for (int i0 = bs.size()-1, i1 = start.size()-1; i0>=0 && i1>=0; i0--,i1--) {
		if ((start[i1] % bs[i0]) != 0) return(false);
	}

	return(true);
}

// Validate parameters to GetVara()
//
bool WASP::_validate_get_vara_compressed(
	vector <size_t> start, vector <size_t> count, 
	vector <size_t> bs, vector <size_t> udims, vector <size_t> cratios
) const {
	if (start.size() != udims.size() || count.size() != udims.size()) {
        return(false);
	}
	assert (bs.size() <= start.size());

	for (int i=0; i<count.size(); i++) {
		if (count[i] < 1 || count[i] > udims[i]) return(false);
	}

	for (int i=0; i<start.size(); i++) {
		if (start[i] >= udims[i]) return(false);
		if ((start[i] + count[i]) > udims[i]) return(false);
	}

	return(true);
}
	


	


int WASP::PutVara(
    vector <size_t> start, vector <size_t> count, const float *data
) {
	if (! _open || ! _open_write) {
		SetErrMsg("Invalid state");
        return(-1);
	}

	bool compressed = false;
	int rc = InqVarCompressed(_open_varname, compressed);
	if (rc<0) return(rc);

	if (! compressed) {
		return(NetCDFCpp::PutVara(_open_varname, start, count, data));
	}

	if (! _validate_put_vara_compressed(
		start, count, _open_bs, _open_udims, _open_cratios)
	) {
		SetErrMsg("Invalid parameter");
        return(-1);
	}

	vector <size_t> ncoeffs;
	vector <size_t> encoded_dims;
	_get_encoding_vectors(
		_open_bs, _open_cratios, NC_FLOAT, ncoeffs, encoded_dims
	);

	
	// Handle case where not all coefficients are wanted
	//
	while (_open_lod < (ncoeffs.size()-1)) {
		ncoeffs.pop_back();
		encoded_dims.pop_back();
	}

	size_t block_size = vproduct(_open_bs);
	float *block = (float *) _blockbuf.alloc(
		block_size * _nthreads *sizeof(float)
	);

	size_t coeffs_size = vsum(ncoeffs);
	float *coeffs = (float *) _coeffbuf.alloc(
		coeffs_size * _nthreads * sizeof(float)
	);

	size_t maps_size = vsum(encoded_dims) - vsum(ncoeffs);  
	unsigned char *maps = (unsigned char*) _sigbuf.alloc(
		maps_size * _nthreads * sizeof (float)
	);

	vector <void *> argvec;
	for (int i=0; i<_nthreads; i++) {

		argvec.push_back((void *) new thread_state(
			i, _et, _open_varname, _ncdfcptrs, start, count, _open_bs, 
			_open_udims, ncoeffs,
			encoded_dims, _open_compressors, (float *) data,
			block + i*block_size, coeffs + i*coeffs_size, 
			maps + i*maps_size*sizeof(float), 0
		));
	}

	rc = _et->ParRun(_RunWriteThread, argvec);
	if (rc < 0) {
		SetErrMsg("Error spawning threads");
		return(-1);
	}

	return(((thread_state *) (argvec[0]))->_status);

}

int WASP::GetVara(
    vector <size_t> start, vector <size_t> count, float *data
) {
	if (! _open || _open_write) {
		SetErrMsg("Invalid state");
        return(-1);
	}

	bool compressed = false;
	int rc = InqVarCompressed(_open_varname, compressed);
	if (rc<0) return(rc);

	if (! compressed) {
		return(NetCDFCpp::GetVara(_open_varname, start, count, data));
	}

	if (! _validate_get_vara_compressed(
		start, count, _open_bs, _open_udims, _open_cratios)
	) {
		SetErrMsg("Invalid parameter");
        return(-1);
	}

	vector <size_t> ncoeffs;
	vector <size_t> encoded_dims;
	_get_encoding_vectors(
		_open_bs, _open_cratios, NC_FLOAT, ncoeffs, encoded_dims
	);


	// Compute the dimension and block size at the grid hierarchy
	// level indicated by _open_level
	//
	vector <size_t> dims_at_level;
	vector <size_t> bs_at_level;
	rc = _dims_at_level(
		_open_udims, _open_bs, _open_level, dims_at_level, bs_at_level
	);
	if (rc<0) return(rc);

	
	// Handle case where not all coefficients are wanted
	//
	while (_open_lod < (ncoeffs.size()-1)) {
		ncoeffs.pop_back();
		encoded_dims.pop_back();
	}

	size_t block_size = vproduct(_open_bs);
	float *block = (float *) _blockbuf.alloc(
		block_size * _nthreads *sizeof(float)
	);

	size_t coeffs_size = vsum(ncoeffs);
	float *coeffs = (float *) _coeffbuf.alloc(
		coeffs_size * _nthreads * sizeof(float)
	);

	size_t maps_size = vsum(encoded_dims) - vsum(ncoeffs);  
	unsigned char *maps = (unsigned char*) _sigbuf.alloc(
		maps_size * _nthreads * sizeof (float)
	);

	vector <void *> argvec;
	for (int i=0; i<_nthreads; i++) {

		argvec.push_back((void *) new thread_state(
			i, _et, _open_varname, _ncdfcptrs, start, count, bs_at_level, 
			dims_at_level, ncoeffs,
			encoded_dims, _open_compressors, data, 
			block + i*block_size, coeffs + i*coeffs_size, 
			maps + i*maps_size*sizeof(float), _open_level
		));
	}

	rc = _et->ParRun(_RunReadThread, argvec);
	if (rc < 0) {
		SetErrMsg("Error spawning threads");
		return(-1);
	}

	return(((thread_state *) (argvec[0]))->_status);
}


// For an array with dimensions 'dimnames' and compression ratio
// vector, cratios, compute the new names and lengths of the dimensions
// needed to store a compressed version of a variable with dimensions,
// 'dimnames'
//
int WASP::_GetCompressedDims(
	vector <string> dimnames, 
	vector <size_t> cratios,
	int xtype,
	vector <string> &cdimnames, 
	vector <size_t> &cdims,
	vector <string> &encoded_dim_names, 
	vector <size_t> &encoded_dims
) const {
	cdimnames.clear();
	cdims.clear();
	encoded_dim_names.clear();
	encoded_dims.clear();

	//
	// Look up dimlens associated with dimnames
	//
	vector <size_t> dims;
	for (int i=0; i<dimnames.size(); i++) {
		size_t dimlen;
		int rc = InqDimlen(dimnames[i], dimlen);
		if (rc<0) return(rc);
		dims.push_back(dimlen);
	}

	//
	// Get appropriate block sizes for given number of dimensions
	//
	vector <size_t> bs = _get_block_sizes(dims.size());
	assert(dims.size() >= bs.size());

	// 
	// Compute dimensions of blocked array in terms of blocks. Not all
	// dimensions are blocked - only n fastest varying, where 'n' is
	// rank of 'bs'.
	//
	cdims = dims;
	cdimnames = dimnames;
	for (int i=bs.size()-1, j=dimnames.size()-1; i>=0 && j>=0; i--, j--) {
		size_t bdim = (size_t) ceil ((double) dims[j] / (double) bs[i]);
		string bdimname = "Blk" + dimnames[j];

		cdims[j] = bdim;
		cdimnames[j] = bdimname;
	}

	//
	// Now get coefficient dimensions. Coefficient dimensions are
	// num coefficients + size of significance map needed to address
	// coefficients. There is one coefficient dimension for each LOD
	//
	vector <size_t> ncoeffs;
	_get_encoding_vectors(bs, cratios, xtype, ncoeffs, encoded_dims);

	for (int i=0; i<encoded_dims.size(); i++) {
		ostringstream oss;
		oss << "EncodedDim" << bs.size() << "D" << i;
		encoded_dim_names.push_back(oss.str());
	}

	return(NC_NOERR);
}

	
// Generate the path names for a multipath NetCDF data set
// containing 'n' paths.
//
vector <string> WASP::mkmultipaths(string path, int n) const {
	vector <string> paths;

	string basename = path;
	size_t p = basename.rfind(".nc");
	if (p != std::string::npos) basename = basename.substr(0, p); 

	for (int i=0; i<n; i++) {
		ostringstream oss;
		oss << basename << ".nc" << i;
		paths.push_back(oss.str());
	}
	return(paths);
}

// For each compression level (LOD) compute the number of coefficients,
// ncoeffs, and the dimension of array that will contain both the
// coefficients and the significance map
//
// bs : dimensions of compression block
// cratio : vector of compression ratios
// xtype : NetCDF external storage type
// ncoeffs : number of wavelet coefficients for each compression level
// encoded_dims : dimension of encoded block for each compression
// level.  The dimension is ncoeffs + size of encoded sig map
//
void WASP::_get_encoding_vectors(
	vector <size_t> bs, vector <size_t> cratios, int xtype,
	vector <size_t> &ncoeffs, 
	vector <size_t> &encoded_dims
) const {
	ncoeffs.clear();
	encoded_dims.clear();
	
    Compressor compressor(bs, _wname);

	// Total number of wavelet coefficients generated by a forward transform
	//
	size_t ntotal = compressor.GetNumWaveCoeffs();

	long naccum = 0;
	for (int i=0; i<cratios.size(); i++) {

		size_t n = ntotal / cratios[i];

		// There is a minumum number of coefficients that must be
		// used in reconstruction that places a lower bound on 
		// the compression rate
		//
		if (n < compressor.GetMinCompression()) {
			n = compressor.GetMinCompression();
		}

		n -= naccum; 	// only account for new coefficients

		if (n<1) n = 1;
		naccum += n;

		ncoeffs.push_back(n);

		// Signifance map is encoded with the wavelet coefficients. 
		// Size of sigmap returned by GetSigMapSize() is in bytes. Need to
		// convert bytes to word size of POD
		//
		if (cratios[i] != 1) {
			size_t s = compressor.GetSigMapSize(n);

			s = (s + SizeOf(xtype)-1) / SizeOf(xtype);

			encoded_dims.push_back(n+s);
		}
		else {
			assert (naccum = ntotal);

			// Special case. Don't need to explicitly store sigmap
			//
			encoded_dims.push_back(n+0);
		}
	}
}

// Generate a block dimension vector for the given number of
// dimensions. The rank of the returned vector will be <= to
// ndims
//
vector <size_t> WASP::_get_block_sizes(int ndims) const {
	if (_bs.size() <= ndims) return (_bs);

	vector <size_t> bs = _bs;
	bs.erase(bs.begin(), bs.begin() + bs.size() - ndims);
	return(bs);
}

	
// Make sure compression ratios vector is valid: monotonic with
// unique values, no zero
//
bool WASP::_validate_cratios(vector <size_t> cratios) const {

	if (cratios.size() != _ncratios) return(false);
	
	// Monotonic
	//
	for (int i=0; i<cratios.size()-1; i++) {
		if (cratios[i] == cratios[i+1]) return (false);
	}

	sort(cratios.begin(), cratios.end()); 
	for (int i=0; i<cratios.size(); i++) {
		if (cratios[i] == 0) return(false);
	}

	return(true);
}

int WASP::_get_compression_params(
	string name, vector <size_t> &bs, vector <size_t> &cratios, 
	vector <size_t> &udims, vector <size_t> &dims, string &wname
) const {
	bs.clear();
	cratios.clear();
	udims.clear();
	dims.clear();

	int rc = GetAtt("", AttNameBlockSize(), bs);
	if (rc<0) return(rc);

	rc = GetAtt(name, AttNameCRatios(), cratios);
	if (rc<0) return(rc);

	if (! _validate_cratios(cratios)) {
		SetErrMsg("Invalid cratios specification");
		return(-1);
	}

	vector <string> udimnames;
	rc = WASP::InqVarDims(name, udimnames, udims);
	if (rc<0) return(rc);

	vector <string> dimnames;
	rc = NetCDFCpp::InqVarDims(name, dimnames, dims);
	if (rc<0) return(rc);

	rc = GetAtt("", AttNameWavelet(), wname);
	if (rc<0) return(rc);
	
	return(0);

}



void *WASP::smartbuf::alloc(size_t size) {
	if (size <= _buf_sz) return(_buf);

	if (_buf) delete [] _buf; 

	_buf = new unsigned char [size];
	_buf_sz = size;
	return(_buf);
}
