
#include <cstring>

#ifndef _VAPOR_UTILS_H_
#define _VAPOR_UTILS_H_

namespace VetsUtil {

class SmartBuf {
public:
 SmartBuf() { _buf = NULL; _buf_sz = 0;};

 SmartBuf(size_t size) {
	_buf = new unsigned char[size]; _buf_sz = size;
 };

 SmartBuf( const SmartBuf& rhs ) {
	_buf = new unsigned char[rhs._buf_sz]; _buf_sz = rhs._buf_sz;
	memcpy(_buf, rhs._buf, rhs._buf_sz);
 }

 SmartBuf& operator=( const SmartBuf& rhs ) {
	_buf = new unsigned char[rhs._buf_sz]; _buf_sz = rhs._buf_sz;
	memcpy(_buf, rhs._buf, rhs._buf_sz);
	return *this;
  }

 ~SmartBuf() {if (_buf) delete [] _buf;};
 void *Alloc(size_t size);

private:
 unsigned char *_buf;
 size_t _buf_sz;
};

};

#endif
