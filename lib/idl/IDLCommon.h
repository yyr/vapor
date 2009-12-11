#ifndef	_IDLCommon_h_
#define	_IDLCommon_h_

#include <vapor/MetadataVDC.h>

using namespace VAPoR;

void    errFatal(
    const char *format,
    ...
);

void    myBaseErrChk();

MetadataVDC *varGetMetadata(
	IDL_VPTR var
);

#endif
