#include <cstdio>
#include <cstdlib>
#include <vapor\vdfcreate.h>

int main(int argc, char **argv) {
	string command = "mom";
	launchVdfCreate(argc, argv, command);
	exit(0);	
}
