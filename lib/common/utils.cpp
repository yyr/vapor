
#include <vapor/utils.h>

using namespace VetsUtil;

void *SmartBuf::Alloc(size_t size) {
    if (size <= _buf_sz) return(_buf);

    if (_buf) delete [] _buf;

    _buf = new unsigned char [size];
    _buf_sz = size;
    return(_buf);
}

