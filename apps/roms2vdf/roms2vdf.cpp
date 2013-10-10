#include <cstdio>
#include <cstdlib>
#include <vapor/Copy2VDF.h>

using namespace VAPoR;

int main(int argc, char **argv) {
		
	Copy2VDF copy2vdf;
	string command = "roms";
	copy2vdf.launch2vdf(argc, argv, command);
	exit(0);    
}
