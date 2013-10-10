#include <cstdio>
#include <cstdlib>
#include <vapor/Copy2VDF.h>

using namespace VAPoR;

int main(int argc, char **argv) {

	Copy2VDF c2vdf;
	string command = "mom";
	int rc = c2vdf.launch2vdf(argc, argv, command);

	if (rc == 0) exit(0);
	else exit(1);

}
