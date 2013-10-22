#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>

using namespace VAPoR;
using namespace VetsUtil;

int main(int argc, char **argv) {
	MyBase::SetErrMsgFilePtr(stderr);
	vdfcreate vdfc;
	string command = "mom";
	int rc = vdfc.launchVdfCreate(argc, argv, command);

    if (rc == 0) exit(0);
    else exit(1);

}
