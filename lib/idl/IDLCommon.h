#ifndef	_IDLCommon_h_
#define	_IDLCommon_h_

#include <vapor/Metadata.h>

using namespace VAPoR;

void    errFatal(
    const char *format,
    ...
);

Metadata *varGetMetadata(
	IDL_VPTR var
);

#endif
