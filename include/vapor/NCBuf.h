

#ifndef	_NCBuf_h_
#define	_NCBuf_h_

#include <vector>

#ifdef PNETCDF
#include <pnetcdf.h>
#else
#include <netcdf.h>
#endif

#include <sstream>
#include <vapor/MyBase.h>

namespace VAPoR {

class VDF_API NCBuf : public VetsUtil::MyBase {
public:

 NCBuf(
       int ncid, int varid, nc_type xtype, vector <size_t> dims,  bool useCollective, int rank,	size_t bufsize = 4*1024*1024
 );

 ~NCBuf() { Flush(); };

 int PutVara(
	const size_t start[], const size_t count[], 
	const void *data
 );

 int Flush();

private:
 int _ncid;
 int _varid;
 unsigned char *_buf;
 size_t _bufsize;	// size of _buf in bytes
 size_t _bufcount; // num bytes in _buf
 size_t _start[NC_MAX_DIMS];
 size_t _count[NC_MAX_DIMS];
 std::vector <size_t> _dims;
 nc_type _xtype;
 size_t _elem_size;
 bool _collective;
 int _rank;

 bool contiguous(const size_t count[]);
 void linearize(
	const size_t start[], const size_t count[], size_t *lstart, size_t *lcount
 ); 
 void inc_count(size_t lcount);

 void delinearize(size_t lcount, size_t count[]);

 size_t max_reg( const size_t start[]);

 
};

};
#endif
