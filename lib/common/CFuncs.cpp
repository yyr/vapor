#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#include <cerrno>
#include "vapor/CFuncs.h"

#include <iostream>

using namespace VetsUtil;

const char	*VetsUtil::Basename(const char *path) {
    const char *last;
    last = strrchr(path, '/');
    if ( !last ) return path;
    else return last + 1;
}

char   *VetsUtil::Dirname(const char *pathname, char *directory)
{
	char	*s;

	if (!pathname || (strlen(pathname) == 0))  return(NULL);

	strcpy(directory, pathname);

	s = directory + strlen(directory) - 1;

	// remove trailing slashes
	//
	while (*s == '/' && s != directory) {
		*s = '\0'; 
		s--; 
	}

	s = strrchr(directory, '/');
	if (s) {
		
		if (s != directory) { 
			*s = '\0';
		}
	}
	else {
		strcpy(directory, ".");
	}
	return (directory);
}
