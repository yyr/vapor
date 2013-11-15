#include <cstdio>
#include <cstdlib>
#include <vapor/WrfVDFcreator.h>
#include <vapor/MyBase.h>

using namespace VAPoR;
using namespace VetsUtil;


int main(int argc, char **argv) {
	MyBase::SetErrMsgFilePtr(stderr);
	wrfvdfcreate launcher;

	int rc = launcher.launchVdfCreate(argc,argv);
	if (rc == 0) exit(0);
	exit(1);
}
