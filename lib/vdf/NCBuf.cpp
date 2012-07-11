

#include <iostream>
#include <cassert>
#include <vapor/NCBuf.h>

using namespace VetsUtil;
using namespace VAPoR;


NCBuf::NCBuf(
	     int ncid, int varid, nc_type xtype, vector <size_t> dims, bool useCollective, int rank,size_t bufsize
) {

	_ncid = ncid;
	_varid = varid;
	_dims = dims;
	_rank = rank;
	_bufsize = bufsize;
	_bufcount = 0;
	_xtype = xtype;
	_collective = useCollective;
	for (int i=0; i<NC_MAX_DIMS; i++) {
		_start[i] = 0;
		_count[i] = 0;
	}
	_buf = new unsigned char[bufsize];
	switch (_xtype) {
    case NC_BYTE:
        _elem_size = 1;
    break;
    case NC_CHAR:
        _elem_size = 1;
    break;
    case NC_SHORT:
        _elem_size = 2;
    break;
    case NC_INT:
        _elem_size = 4;
    break;
    case NC_FLOAT:
        _elem_size = 4;
    break;
    case NC_DOUBLE:
        _elem_size = 8;
    break;
    default:
#ifdef PNETCDF
		SetErrMsg("Invalid variable type : %s", ncmpi_strerror(NC_EBADTYPE));
#else
		SetErrMsg("Invalid variable type : %s", nc_strerror(NC_EBADTYPE));
#endif
    break;
	}
}

void print_put_var(
		   string msg, int ncid, int varid, const size_t start[], const size_t count[], int n, int _rank
) {
#ifdef	PIOVDC_DEBUG
	size_t total = 1;
	for (int i=0; i<n; i++) total *= count[i];
	cerr << msg << ":rank ( "<< _rank << "): ncid(" << ncid << ") varid(" << varid << ")";
	cerr << "	start: ";
	for (int i=0; i<n; i++) cerr << start[i] << " " ;
	//	cerr <<;
	cerr << "	count: ";
	for (int i=0; i<n; i++) cerr << count[i] << " " ;
	cerr << "(" << total << " total elements)";
	cerr << endl;
#endif
}


int NCBuf::PutVara(
		   const size_t start[], const size_t count[], const void *data
) {
	size_t lstart, lcount;  // Start and count as a linear offset
	size_t mylstart, mylcount;  // Start and count as a linear offset

	linearize(start, count, &lstart, &lcount);
	linearize(_start, _count, &mylstart, &mylcount);

	//
	// if the region of data descibed by start+count is not contiguous
	// on disk we can not buffer the data and simply write it out. Ditto 
	// if the region size is larger than the buffer.
	//
	// Should we flush the buffer as well?
	//
	if ((! contiguous(count)) || (lcount*_elem_size) > _bufsize) {

	  if(!contiguous(count))
		print_put_var("non-contiguous Non-Buffered Write", _ncid, _varid, start, count, _dims.size(), _rank);
	  else if((lcount*_elem_size) > _bufsize)
	    {
	    std::stringstream ss (std::stringstream::in | std::stringstream::out);
	  ss << "_bufsize Non-Buffered Write buffsize(" << _bufsize << ") " << "lcount(" << lcount << ") " << "elem_size( " << _elem_size << ")";
		print_put_var(ss.str(), _ncid, _varid, start, count, _dims.size(), _rank);
	    }
	    
#ifdef PNETCDF

		MPI_Offset mp_start[4];
		MPI_Offset mp_count[4];
		mp_start[0] = start[0];
		mp_start[1] = start[1];
		mp_start[2] = start[2];
		mp_start[3] = start[3];
		mp_count[0] = count[0];
		mp_count[1] = count[1];
		mp_count[2] = count[2];
		mp_count[3] = count[3];
		MPI_Offset len = count[0] * count[1] * count[2] * count[3];
#endif 

#ifdef PNETCDF
		if(_collective)
		  return(ncmpi_put_vara_all(_ncid, _varid, mp_start, mp_count, data, len, MPI_UNSIGNED_CHAR));
		else
		  return(ncmpi_put_vara(_ncid, _varid, mp_start, mp_count, data, len, MPI_UNSIGNED_CHAR));
#else
		return(nc_put_vara(_ncid, _varid, start, count, data));
#endif
	}



	
	//
	// Flush the buffer if either the current chunk of data is not
	// adjacent to what is in the buffer, or if there is not enough
	// room in the buffer;
	//
	if ((mylstart * _elem_size + _bufcount != lstart*_elem_size) || 
		(_bufcount + (lcount * _elem_size) > _bufsize) ||
		(mylcount+lcount > max_reg(_start))) {

		int rc = Flush();
		if (rc<0) return(rc);
	}

	if (_bufcount == 0) {
		for (int i=0; i<_dims.size(); i++) {
			_count[i] = 0;
			_start[i] = start[i];
		}
	}
			
	memcpy(_buf + _bufcount, data, lcount*_elem_size);
	_bufcount += lcount * _elem_size;

	inc_count(lcount);

	return(0);

}

int NCBuf::Flush(
) {

	if (_bufcount < 1) return(0);

	delinearize(_count[_dims.size()-1], _count);

#ifdef PNETCDF
	MPI_Offset mp_start[4];
	MPI_Offset mp_count[4];
	mp_start[0] = _start[0];
	mp_start[1] = _start[1];
	mp_start[2] = _start[2];
	mp_start[3] = _start[3];
	mp_count[0] = _count[0];
	mp_count[1] = _count[1];
	mp_count[2] = _count[2];
	mp_count[3] = _count[3];
	MPI_Offset len = _count[0] * _count[1] * _count[2] * _count[3];
#endif

#ifdef PNETCDF
	int rc;
	if(_collective)
	  {
	    print_put_var("Collective Buffered write", _ncid, _varid, _start, _count, _dims.size(), _rank);
	    rc = ncmpi_put_vara_all(_ncid, _varid, mp_start, mp_count, _buf, len, MPI_FLOAT);
	  }
	else
	  {
	    print_put_var("Independent Buffered write", _ncid, _varid, _start, _count, _dims.size(), _rank);
	    rc = ncmpi_put_vara(_ncid, _varid, mp_start, mp_count, _buf, len, MPI_FLOAT);
	  }

#else

	print_put_var("Buffered write", _ncid, _varid, _start, _count, _dims.size(), _rank);
	int rc = (nc_put_vara(_ncid, _varid, _start, _count, _buf));
#endif

	_bufcount = 0;
	for (int i=0; i<_dims.size(); i++) {
		_count[i] = 0;
		_start[i] = 0;
	}

	return(rc);
}

//
// Returns true if the dimension lenths of the region described by
// 'count' define an area that is contiguous in memory. I.e. true
// if all adjacent elements have a stride between them of one.
//
bool NCBuf::contiguous(
	const size_t count[]
) {
	int dimidx = 0;

	//
	// find the length of the fastest varying dimension that is not
	// the same length as the variable
	//
	for (int i = _dims.size()-1; i>=0; i--) {
		if (count[i] != _dims[i]) {
			dimidx = i;
			break;
		}
	}

	//
	// The remaining, slower varying dimensions must all be of length one
	// for the region to be contiguous
	//
	for (int i = 0; i<dimidx; i++) {
		if (count[i] != 1) return(false);
	}

	return(true);
}
			
//
// Given a netCDF vector representation of a region (start and count),
// transform the vector description into a scalar offset and number of
// elements.
// 
void NCBuf::linearize(
	const size_t start[], const size_t count[], size_t *lstart, size_t *lcount
) {

	*lstart = 0;
	*lcount = 1;
	for (int i=0; i<_dims.size(); i++) {
		size_t l = 1;
		for (int j=i+1; j<_dims.size(); j++) l *= _dims[j];
		*lstart += l*start[i];

		*lcount *= count[i];
	}
}

//
// Transform a linear description of the number of elements contained
// in a contiguous region to a netCDF vector "count" representation
//
void NCBuf::delinearize(
	size_t lcount, size_t count[]
) {

	// assumes lcount defined a region that can be described by a 
	// count vector 
	//
	for (int i = _dims.size()-1; i>=0; i--) {
		if (lcount > _dims[i]) {
			assert((lcount % _dims[i]) == 0);
			count[i] = _dims[i];
			lcount = lcount / _dims[i];
		}
		else if (lcount > 0) {
			count[i] = lcount;
			lcount = 0;
		}
		else {
			count[i] = 1;
		}
	}
}

//
// Calculate the size of the largest contiguous region that can be described
// by a 'count' vector with the given 'start' location, subject to the 
// constraint that we don't exceed the buffer size
//
size_t NCBuf::max_reg(
	const size_t start[]
) {
	size_t lcount = 1;
	for (int i = _dims.size()-1; i>=0; i--) {
		size_t d = _dims[i] - start[i];

		// lcount can't be larger than the buffer
		//
		while ((lcount * d * _elem_size > _bufsize) && d > 1) d--; 

		lcount *= d;

		if (start[i] != 0) {
			break;	// We're done as soon as we process a non-zero start offset
		}

	}
	return(lcount);
}


//
// Increment the '_count' vector counter by lcount. Note, lcount is a
// scalar quantity. 
//
// The resulting '_count' vector may not be valid for the current (4.1.x)
// netcdf API, as its fastest varying dimension may exceed the dimension
// length of the variable.  It needs to be "delinearized" before it is 
// suitable for netCDF. N.B. We should probably delinearize _count in this
// method...
//
void NCBuf::inc_count(size_t lcount)
{
	//
	// Deal with initilization case when _count is zero
	//
	size_t mylstart, mylcount;
	linearize(_start, _count, &mylstart, &mylcount);
	if (mylcount == 0) {
		for (int i=0; i<_dims.size()-1; i++) _count[i] = 1;
		_count[_dims.size()-1] = 0;
	}
		
	_count[_dims.size()-1] += lcount;
	assert(contiguous(_count) == true);
}
