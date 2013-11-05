#include <cstdio>
#include <cstdlib>
#include <vapor/Wrf2vdf.h>

using namespace VAPoR;
using namespace VetsUtil;

int main(int argc, char **argv) {

	MyBase::SetErrMsgFilePtr(stderr);
	Wrf2vdf w2v;
	int rc = w2v.launchWrf2Vdf(argc, argv);

	if (rc == 0) exit(0);
	else exit(1);

}
