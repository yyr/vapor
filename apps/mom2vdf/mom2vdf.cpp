#include <cstdio>
#include <cstdlib>
//#include "2vdf.h"
#include <vapor/2vdf.h>

int main(int argc, char **argv) {
        string command = "mom";
        launch2vdf(argc, argv, command);
        exit(0);    
}
