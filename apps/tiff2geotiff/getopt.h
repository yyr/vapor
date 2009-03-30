#ifndef GETOPT_H

//externs in tiff2geotiff
int getopt(int nargc, char** nargv, char* ostr);
/*
 * get option letter from argument vector
 */
static int	opterr=1;		/* if error message should be printed */
static int	optind=1;		/* index into parent argv vector */
static int	optopt;			/* character checked for validity */
static char	*optarg;		/* argument associated with option */

#endif