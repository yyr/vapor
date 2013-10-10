#include <cstdio>
#include <cstdlib>
#include <vapor/vdfcreate.h>

using namespace VAPoR;

int main(int argc, char **argv) {
	vdfcreate vdfc;
	string command = "mom";
	int rc = vdfc.launchVdfCreate(argc, argv, command);

    if (rc == 0) exit(0);
    else exit(1);

}
