#include <iostream>
#include <string>
#include <vector>
#include <sstream>

#include <vapor/Metadata.h>
#include <vapor/CFuncs.h>

using namespace VetsUtil;
using namespace VAPoR;

const char  *ProgName;

int	main(int argc, char **argv) {

	ProgName = Basename(argv[0]);

	if (argc != 4) {
		cerr << "Usage: src1 src2 dst" << argv[0] << endl;
		exit(1);
	}

	Metadata *m = new Metadata(argv[1]);
    if (Metadata::GetErrCode() != 0) {
        cerr << "Metadata::Metadata() : " << Metadata::GetErrMsg() << endl;
        exit (1);
    }

	m->Merge(argv[2]);
    if (Metadata::GetErrCode() != 0) {
        cerr << "Metadata::Merge() : " << Metadata::GetErrMsg() << endl;
        exit (1);
    }

	m->Write(argv[3], 0);
    if (Metadata::GetErrCode() != 0) {
        cerr << "Metadata::Write() : " << Metadata::GetErrMsg() << endl;
        exit (1);
    }


	delete m;
	exit(0);
}
