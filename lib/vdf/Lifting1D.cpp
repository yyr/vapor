/*
 *  -*- Mode: ANSI C -*-
 *  $Id$
 *  Author: Gabriel Fernandez, Senthil Periaswamy
 *
 *     >>> Fast Lifted Wavelet Transform on the Interval <<<
 *
 *                             .-. 
 *                            / | \
 *                     .-.   / -+- \   .-.
 *                        `-'   |   `-'
 *
 *  Using the "Lifting Scheme" and the "Square 2D" method, the coefficients
 *  of a "Biorthogonal Wavelet" or "Second Generation Wavelet" are found for
 *  a two-dimensional data set. The same scheme is used to apply the inverse
 *  operation.
 */
/* do not edit anything above this line */

/* System header files */
#include <cstdio>
#include <cstdlib>   
#include <cmath>
#include <cstring>   
#include <cerrno>   
#include <cassert>   
#include "vapor/Lifting1D.h"

using namespace VAPoR;
using namespace VetsUtil;

#ifndef	MAX
#define	MAX(A,B) ((A)>(B)?(A):(B))
#endif

typedef	Lifting1D::Flt	flt_t;
static const double TINY = 1.0e-20;    /* A small number */

static flt_t	*create_fwd_filter (unsigned int);
static flt_t	*create_inv_filter (unsigned int, const flt_t *);
static flt_t	*create_fwd_lifting(
	const flt_t*, const unsigned int, const unsigned int, 
	const unsigned int 
);
static flt_t	*create_inv_lifting(flt_t *, unsigned int, unsigned int);

static void FLWT1D_Predict(Lifting1D::DataType_T*, const long, const long, flt_t*);
static void FLWT1D_Update (Lifting1D::DataType_T *, const long, const long, const flt_t*);

static void	forward_transform1d_haar(
	float *data,
	int width
);

static void	inverse_transform1d_haar(
	float *data,
	int width
);

Lifting1D::Lifting1D(
	unsigned int	n,
	unsigned int	ntilde,
	unsigned int	width
) {
	_objInitialized = 0;
	n_c = n;
	ntilde_c = ntilde;
	width_c = width;
	fwd_filter_c = NULL;
	fwd_lifting_c = NULL;
	inv_filter_c = NULL;
	inv_lifting_c = NULL;

	SetClassName("Lifting1D");

	// Check for haar wavelets which are handled differently
	//
	if (n_c == 1 && ntilde_c == 1) {
		_objInitialized = 1;
		return;
	}

	if (IsOdd(n_c)) {
		SetErrMsg(
			"Invalid # lifting coeffs., n=%d, is odd", 
			n_c
		);
		return;
	}

	if (n_c > MAX_FILTER_COEFF) {
		SetErrMsg(
			"Invalid # of lifting coeffs., n=%d, exceeds max=%d",
			n_c, MAX_FILTER_COEFF
		);
		return;
	}
	if (ntilde_c > MAX_FILTER_COEFF) {
		SetErrMsg(
			"Invalid # of lifting coeffs., ntilde=%d, exceeds max=%d",
			ntilde_c, MAX_FILTER_COEFF
		);
		return;
	}

	int	maxN = MAX(n_c, ntilde_c) - 1;	// max vanishing moments 
	int	maxl = (width_c == 1) ? 0 : (int)LogBaseN ((double)(width_c-1)/maxN, (double) 2.0);

	if (maxl < 1) {
		SetErrMsg(
			"Invalid # of samples, width=%d, less than # moments",
			width
		);
		return;
		
	}
	

	if (! (fwd_filter_c = create_fwd_filter(n_c))) {
		SetErrMsg("malloc() : %s", strerror(errno));
		return;
	}
	if (! (inv_filter_c = create_inv_filter(n_c, fwd_filter_c))) {
		SetErrMsg("malloc() : %s", strerror(errno));
		return;
	}


#ifdef	DEBUGGING
	{
            int Nrows = (n_c>>1) + 1;
            filterPtr = filter_c;
            fprintf (stdout, "\nFilter Coefficients (N=%d)\n", n_c);
            for (row = 0 ; row < Nrows ; row++) {
                fprintf (stdout, "For %d in the left and %d in the right\n[ ",
                         row, N-row);
                for (col = 0 ; col < n_c ; col++)
                    fprintf (stdout, "%.18g ", *(filterPtr++));
                fprintf (stdout, "]\n");
            }
        }
#endif


	fwd_lifting_c = create_fwd_lifting(fwd_filter_c,n_c,ntilde_c,width_c);
	if (! fwd_lifting_c) {
		SetErrMsg("malloc() : %s", strerror(errno));
		return;
	}

	inv_lifting_c = create_inv_lifting(fwd_lifting_c, ntilde_c, width_c);
	if (! inv_lifting_c) {
		SetErrMsg("malloc() : %s", strerror(errno));
		return;
	}
	_objInitialized = 1;
}

Lifting1D::~Lifting1D()
{
	if (! _objInitialized) return;

	if (fwd_filter_c) delete [] fwd_filter_c;
	fwd_filter_c = NULL;

	if (inv_filter_c) delete [] inv_filter_c;
	inv_filter_c = NULL;

	if (fwd_lifting_c) delete [] fwd_lifting_c;
	fwd_lifting_c = NULL;

	if (inv_lifting_c) delete [] inv_lifting_c;
	inv_lifting_c = NULL;

	_objInitialized = 0;
}

/* CODE FOR THE IN-PLACE FLWT (INTERPOLATION CASE, N EVEN) */

//
// ForwardTransform function: This is a 1D Forward Fast Lifted Wavelet
//                  Trasform. Since the Lifting Scheme is used, the final
//                  coefficients are organized in-place in the original 
//                  vector Data[]. In file mallat.c there are
//                  functions provided to change this organization to
//                  the Isotropic format originally established by Mallat.
//
void   Lifting1D::ForwardTransform(
	DataType_T *data
) {
	if (n_c == 1 && ntilde_c == 1) {
		forward_transform1d_haar(data, width_c);
	}
	else {
		FLWT1D_Predict (data, width_c, n_c, fwd_filter_c);
		FLWT1D_Update (data, width_c, ntilde_c, fwd_lifting_c);
	}
}

//
// InverseTransform function: This is a 1D Inverse Fast Lifted Wavelet
//                  Trasform. Since the Lifting Scheme is used, the final
//                  coefficients are organized in-place in the original 
//                  vector Data[]. In file mallat.c there are
//                  functions provided to change this organization to
//                  the Isotropic format originally established by Mallat.
//
void   Lifting1D::InverseTransform(
	DataType_T *data
) {
	if (n_c == 1 && ntilde_c == 1) {
		inverse_transform1d_haar(data, width_c);
	}
	else {
		FLWT1D_Update (data, width_c, ntilde_c, inv_lifting_c);
		FLWT1D_Predict (data, width_c, n_c, inv_filter_c);
	}
}

static const flt_t	Flt_Epsilon = 	(flt_t) 6E-8;
static int zero( const flt_t x )
{
	const flt_t __negeps_f = -100 * Flt_Epsilon;
	const flt_t __poseps_f = 100 * Flt_Epsilon;

	return ( __negeps_f < x ) && ( x < __poseps_f );
}

/*
 *  -*- Mode: ANSI C -*-
 *  (Polynomial Interpolation)
 *  $Id$
 *  Author: Wim Sweldens, Gabriel Fernandez
 *
 *  Given n points of the form (x,f), this program uses the Neville algorithm
 *  to find the value at xx of the polynomial of degree (n-1) interpolating
 *  the points (x,f).
 *  Ref: Stoer and Bulirsch, Introduction to Numerical Analysis,
 *  Springer-Verlag.
 */

static	flt_t neville ( const flt_t *x, const flt_t *f, const int n, const flt_t xx )
{
	register int i,j;
	flt_t	vy[Lifting1D::MAX_FILTER_COEFF];
	flt_t y;

	for ( i=0; i<n; i++ ) {
		vy[i] = f[i];
		for ( j=i-1; j>=0; j--) {
			flt_t den = x[i] - x[j];
			assert(! zero(den));
			vy[j] = vy[j+1] + (vy[j+1] - vy[j]) * (xx - x[i]) / den;
		}
	}
	y = vy[0];

	return y;
}

//
// UpdateMoment function: calculates the integral-moment tuple for the current
//                        level of calculations.
//
static void update_moment(
	flt_t** moment,
	const flt_t *filter,
	const int step2,
	const int noGammas, 
	const int len,
	const int n,
	const int ntilde
) {
	int i, j,              /* counters */
	row, col,          /* indices of the matrices */
	idxL, idxG,        /* pointers to Lambda & Gamma coeffs, resp. */
	top1, top2, top3;  /* number of iterations for L<ntilde, L=ntilde, L>ntilde */
	const flt_t	*filterPtr;	// ptr to filter coefficients

	/***************************************/
	/* Update Integral-Moments information */
	/***************************************/

	/* Calculate number of iterations for each case */
	top1 = (n>>1) - 1;                 /* L < ntilde */
	top3 = (n>>1) - IsOdd(len);          /* L > ntilde */
	top2 = noGammas - (top1 + top3);   /* L = ntilde */

	/* Cases where nbr. left Lambdas < nbr. right Lambdas */
	filterPtr = filter;   /* initialize pointer to first row */
	idxG = step2>>1;      /* second coefficient is Gamma */
	for ( row = 1 ; row <= top1 ; row++ ) {
		idxL = 0;   /* first Lambda is always the first coefficient */
		filterPtr += n;   /* go to next filter row */
		for ( col = 0 ; col < n ; col++ ) {
			/* Update (int,mom_1,mom_2,...) */
			for ( j = 0 ; j < ntilde ; j++ ) {
				moment[idxL][j] += filterPtr[col]*moment[idxG][j];
			}
			/* Jump to next Lambda coefficient */
			idxL += step2;
		}
		idxG += step2;   /* go to next Gamma coefficient */
	}

	/* Cases where nbr. left Lambdas = nbr. right Lambdas */
	filterPtr += n;   /* go to last filter row */
	for ( i = 0 ; i < top2 ; i++ ) {
		idxL = i*step2;
		for ( col = 0 ; col < n ; col++ ) {
			/* Update (int,mom_1,mom_2,...) */
			for ( j = 0 ; j < ntilde ; j++ ) {
				moment[idxL][j] += filterPtr[col]*moment[idxG][j];
			}
			/* Jump to next Lambda coefficient */
			idxL += step2;
		}
		idxG += step2;   /* go to next Gamma coefficient and stay */
		/* in the same row of filter coefficients */
	}

	/* Cases where nbr. left Lambdas > nbr. right Lambdas */
	for ( row = top3 ; row >= 1 ; row-- ) {
		idxL = (top2-1)*step2;	// first Lambda is always in this place
		filterPtr -= n;   /* go to previous filter row */
		for ( col = n-1 ; col >= 0 ; col-- ) {
			/* Update (int,mom_1,mom_2,...) */
			for ( j = 0 ; j < ntilde ; j++ ) {
				moment[idxL][j] += filterPtr[col]*moment[idxG][j];
			}
			// Jump to next Lambda coefficient and next filter row
			idxL += step2;
		}
		idxG += step2;   /* go to next Gamma coefficient */
	}
}


//
// allocate a f.p. matrix with subscript range m[nrl..nrh][ncl..nch] 
//
flt_t **matrix( long nrl, long nrh, long ncl, long nch)
{
    long i;
    flt_t	**m;

    /* allocate pointers to rows */
    m = new flt_t* [nrh-nrl+1];
    if (!m) return NULL;
    m -= nrl;

    /* allocate rows and set pointers to them */
    for ( i=nrl ; i<=nrh ; i++ ) {
        m[i] = new flt_t[nch-ncl+1];
        if (!m[i]) return NULL;
        m[i] -= ncl;
    }
    /* return pointer to array of pointers to rows */
    return m;
}

/* free a f.p. matrix llocated by matrix() */
void free_matrix (flt_t **m, long nrl, long nrh, long ncl, long nch )
{
    long i;

	for ( i=nrh ; i>=nrl ; i-- ) {
		m[i] += ncl;
		delete [] m[i];
	}
    m += nrl;
    delete [] m;
}


/*
 *  -*- Mode: ANSI C -*-
 *  $Id$
 *  Author: Gabriel Fernandez
 *
 *  Definition of the functions used to perform the LU decomposition
 *  of matrix a. Function LUdcmp() performs the LU operations on a[][]
 *  and function LUbksb() performs the backsubstitution giving the
 *  solution in b[];
 */
/* do not edit anything above this line */


/* code */

/*
 * LUdcmp function: Given a matrix a [0..n-1][0..n-1], this routine
 *                  replaces it by the LU decomposition of a
 *                  rowwise permutation of itself. a and n are
 *                  input. a is output, arranged with L and U in
 *                  the same matrix; indx[0..n-1] is an output vector
 *                  that records the row permutation affected by
 *                  the partial pivoting; d is output as +/-1
 *                  depending on whether the number of row
 *                  interchanges was even or odd, respectively.
 *                  This routine is used in combination with LUbksb()
 *                  to solve linear equations or invert a matrix.
*/
static void LUdcmp(flt_t **a, int n, int *indx, flt_t *d)
{
    int i, imax, j, k;
    flt_t big, dum, sum, temp;
    flt_t *vv;   /* vv stores the implicit scaling of each row */

    vv=new flt_t[n];
    *d=(flt_t)1;               /* No row interchanges yet. */
    for (i=0;i<n;i++) {     /* Loop over rows to get the implicit scaling */
        big=(flt_t)0;          /* information. */
        for (j=0;j<n;j++)
            if ((temp=(flt_t)fabs((flt_t)a[i][j])) > big)
                big=temp;
	assert(big != (flt_t)0.0);	// singular matrix
        /* Nonzero largest element. */
        vv[i]=(flt_t)1/big;    /* Save the scaling. */
    }
    for (j=0;j<n;j++) {     /* This is the loop over columns of Crout's method. */
        for (i=0;i<j;i++) {  /* Sum form of a triangular matrix except for i=j. */
            sum=a[i][j];     
            for (k=0;k<i;k++) sum -= a[i][k]*a[k][j];
            a[i][j]=sum;
        }
        big=(flt_t)0;      /* Initialize for the search for largest pivot element. */
        imax = -1;        /* Set default value for imax */
        for (i=j;i<n;i++) {  /* This is i=j of previous sum and i=j+1...N */
            sum=a[i][j];      /* of the rest of the sum. */
            for (k=0;k<j;k++)
                sum -= a[i][k]*a[k][j];
            a[i][j]=sum;
            if ( (dum=vv[i]*(flt_t)fabs((double)sum)) >= big) {
            /* Is the figure of merit for the pivot better than the best so far? */
                big=dum;
                imax=i;
            }
        }
        if (j != imax) {          /* Do we need to interchange rows? */
            for (k=0;k<n;k++) {  /* Yes, do so... */
                dum=a[imax][k];
                a[imax][k]=a[j][k];
                a[j][k]=dum;
            }
            *d = -(*d);           /* ...and change the parity of d. */
            vv[imax]=vv[j];       /* Also interchange the scale factor. */
        }
        indx[j]=imax;
        if (a[j][j] == (flt_t)0.0)
            a[j][j]=(flt_t)TINY;
        /* If the pivot element is zero the matrix is singular (at least */
        /* to the precision of the algorithm). For some applications on */
        /* singular matrices, it is desiderable to substitute TINY for zero. */
        if (j != n-1) {           /* Now, finally divide by pivot element. */
            dum=(flt_t)1/(a[j][j]);
            for ( i=j+1 ; i<n ; i++ )
                a[i][j] *= dum;
        }
    }	/* Go back for the next column in the reduction. */
    delete [] vv;
}
    
    
/*
 *  LUbksb function: Solves the set of n linear equations A.X = B.
 *                   Here a[1..n][1..n] is input, not as the matrix A
 *                   but rather as its LU decomposition, determined
 *                   by the routine LUdcmp(). indx[1..n] is input as
 *                   the permutation vector returned by LUdcmp().
 *                   b[1..n] is input as the right hand side vector B,
 *                   and returns with the solution vector X. a, n, and
 *                   indx are not modified by this routine and can be
 *                   left in place for successive calls with different
 *                   right-hand sides b. This routine takes into account
 *                   the possibility that b will begin with many zero
 *                   elements, so it it efficient for use in matrix
 *                   inversion.
 */
void LUbksb (flt_t **a, int n, int *indx, flt_t *b )
{
    int i,ii=-1,ip,j;
    flt_t sum;

    for (i=0;i<n;i++) {   /* When ii is set to a positive value, it will */
        ip=indx[i];        /* become the index of the first nonvanishing */
        sum=b[ip];         /* element of b. We now do the forward substitution. */
        b[ip]=b[i];        /* The only new wrinkle is to unscramble the */
        if (ii>=0)            /* permutation as we go. */
            for (j=ii;j<=i-1;j++) sum -= a[i][j]*b[j];
        else if (sum)      /* A nonzero element was encountered, so from now on */
            ii=i;          /* we will have to do the sums in the loop above */
        b[i]=sum;
    }
    for (i=n-1;i>=0;i--) {   /* Now we do the backsubstitution. */
        sum=b[i];
        for (j=i+1;j<n;j++) sum -= a[i][j]*b[j];
        b[i]=sum/a[i][i];  /* Store a component of the solution vector X. */
        if (zero(b[i])) b[i] = (flt_t)0;   /* Verify small numbers. */
    }                      /* All done! */
}


/*
 * UpdateLifting function: calculates the lifting coefficients using the given
 *                         integral-moment tuple.
 */
static void update_lifting (
	flt_t *lifting,
	const flt_t** moment,
	const int len,
	const int step2,
	const int noGammas,
	const int ntilde
) {
	int lcIdx,             /* index of lifting vector */
	i, j,              /* counters */
	row, col,          /* indices of the matrices */
	idxL, idxG,        /* pointers to Lambda & Gamma coeffs, resp. */
	top1, top2, top3;  /* number of iterations for L<ntilde, L=ntilde, L>ntilde */
	flt_t** lift;	// used to find the lifting coefficients
	flt_t *b;	// used to find the lifting coefficients
	int *indx;             	// used by the LU routines
	flt_t d;	// used by the LU routines

	/**********************************/
	/* Calculate Lifting Coefficients */
	/**********************************/

	lcIdx = 0;         /* first element of lifting vector */

	/* Allocate space for the indx[] vector */
	indx = new int[ntilde];

	/* Allocate memory for the temporary lifting matrix and b */
	/* temporary matrix to be solved */
	lift = matrix(0, (long)ntilde-1, 0, (long)ntilde-1);   
	b = new flt_t[ntilde];                  /* temporary solution vector */

	/* Calculate number of iterations for each case */
	top1 = (ntilde>>1) - 1;                 /* L < ntilde */
	top3 = (ntilde>>1) - IsOdd(len);          /* L > ntilde */
	top2 = noGammas - (top1 + top3);   /* L = ntilde */

	/* Cases where nbr. left Lambdas < nbr. right Lambdas */
	idxG = step2>>1;   /* second coefficient is Gamma */
	for ( i=0 ; i<top1 ; i++ ) {
		idxL = 0;   /* first Lambda is always the first coefficient */
		for (col=0 ; col<ntilde ; col++ ) {
			/* Load temporary matrix to be solved */
			for ( row=0 ; row<ntilde ; row++ ) {
				lift[row][col] = moment[idxL][row];	//matrix
			}
			/* Jump to next Lambda coefficient */
			idxL += step2;
		}
		/* Apply LU decomposition to lift[][] */
		LUdcmp (lift, ntilde, indx, &d);
		/* Load independent vector */
		for ( j=0 ; j<ntilde ; j++) {
			b[j] = moment[idxG][j];   /* independent vector */
		}
		/* Apply back substitution to find lifting coefficients */
		LUbksb (lift, ntilde, indx, b);
		for (col=0; col<ntilde; col++) {	// save them in lifting vector 
			lifting[lcIdx++] = b[col];
		}

		idxG += step2;   /* go to next Gamma coefficient */
	}

	/* Cases where nbr. left Lambdas = nbr. right Lambdas */
	for ( i=0 ; i<top2 ; i++ ) {
		idxL = i*step2;
		for ( col=0 ; col<ntilde ; col++ ) {
		/* Load temporary matrix to be solved */
			for ( row=0 ; row<ntilde ; row++ )
				lift[row][col] = moment[idxL][row];      /* matrix */
			/* Jump to next Lambda coefficient */
			idxL += step2;
		}
		/* Apply LU decomposition to lift[][] */
		LUdcmp (lift, ntilde, indx, &d);
		/* Load independent vector */
		for ( j=0 ; j<ntilde ; j++)
			b[j] = moment[idxG][j];   /* independent vector */
		/* Apply back substitution to find lifting coefficients */
		LUbksb (lift, ntilde, indx, b);
		for ( col=0 ; col<ntilde ; col++ )    /* save them in lifting vector */
			lifting[lcIdx++] = b[col];

		idxG += step2;   /* go to next Gamma coefficient */
	}

	/* Cases where nbr. left Lambdas > nbr. right Lambdas */
	for ( i=0 ; i<top3 ; i++ ) {
		idxL = (top2-1)*step2;   /* first Lambda is always in this place */
		for ( col=0 ; col<ntilde ; col++ ) {
			/* Load temporary matrix to be solved */
			for ( row=0 ; row<ntilde ; row++ )
				lift[row][col] = moment[idxL][row];      /* matrix */
			/* Jump to next Lambda coefficient */
			idxL += step2;
		}
		/* Apply LU decomposition to lift[][] */
		LUdcmp (lift, ntilde, indx, &d);
		/* Load independent vector */
		for ( j=0 ; j<ntilde ; j++)
			b[j] = moment[idxG][j];   /* independent vector */
		/* Apply back substitution to find lifting coefficients */
		LUbksb (lift, ntilde, indx, b);
		for (col=0;col<ntilde;col++) {	// save them in lifting vector
			lifting[lcIdx++] = b[col];
		}

	idxG += step2;   /* go to next Gamma coefficient */
	}

	/* Free memory */
	free_matrix(lift, 0, (long)ntilde-1, 0, (long)ntilde-1);
	delete [] indx;
	delete [] b;
}

/*
 * create_fwd_filter function: finds the filter coefficients used in the
 *                     Gamma coefficients prediction routine. The
 *                     Neville polynomial interpolation algorithm
 *                     is used to find all the possible values for
 *                     the filter coefficients. Thanks to this
 *                     process, the boundaries are correctly treated
 *                     without including artifacts.
 *                     Results are ordered in matrix as follows:
 *                         0 in the left and N   in the right
 *                         1 in the left and N-1 in the right
 *                         2 in the left and N-2 in the right
 *                                        .
 *                                        .
 *                                        .
 *                         N/2 in the left and N/2 in the right
 *                     For symmetry, the cases from
 *                         N/2+1 in the left and N/2-1
 *                     to
 *                         N in the left and 0 in the right
 *                     are the same as the ones shown above, but with
 *                     switched sign.
 */
static flt_t	*create_fwd_filter (unsigned int n)
{
	flt_t xa[Lifting1D::MAX_FILTER_COEFF], ya[Lifting1D::MAX_FILTER_COEFF], x;
	int row, col, cc, Nrows;
	flt_t *filter, *ptr;


	/* Number of cases for filter calculations */
	Nrows = (n>>1) + 1;    /* n/2 + 1 */

	/* Allocate memory for filter matrix */
	filter = new flt_t[Nrows*n];
	if (! filter) return NULL;
	ptr = filter;

	/* Generate values of xa */
	xa[0] = (flt_t)0.5*(flt_t)(1 - (int) n);	// -n/2 + 0.5
	for (col = 1 ; col < (int)n ; col++) {
		xa[col] = xa[col-1] + (flt_t)1;
	}

	/* Find filter coefficient values */
	filter += ( (Nrows*n) - 1 ); // go to last position in filter matrix
	for (row = 0 ; row < Nrows ; row++) {
		x = (flt_t)row;
		for (col = 0 ; col < (int)n ; col++) {
			for (cc = 0 ; cc < (int)n ; cc++)
			ya[cc] = (flt_t)0;
			ya[col] = (flt_t)1;
			*(filter--) = neville (xa, ya, n, x);
		}
	}

	return ptr;
}

static flt_t	*create_inv_filter (unsigned int n, const flt_t *filter)
{
	flt_t	*inv_filter;
	int	Nrows;
	int	i;

	/* Number of cases for filter calculations */
	Nrows = (n>>1) + 1;    /* n/2 + 1 */

	/* Allocate memory for filter matrix */
	inv_filter = new flt_t[Nrows*n];
	if (! inv_filter) return NULL;

        for ( i=0 ; i<(int)(n*(1+(n>>1))) ; i++ ) inv_filter[i] = -filter[i];

	return(inv_filter);
}

//
// create_moment function: Initializes the values of the Integral-Moments
//                      matrices. The moments are equal to k^i, where
//                      k is the coefficient index and i is the moment
//                      number (0,1,2,...).
//
static flt_t** create_moment(
	const unsigned int ntilde, 
	const unsigned int width, 
	const int print
) {
	int row, col;

	flt_t**	moment;

	/* Allocate memory for the Integral-Moment matrices */
	moment = matrix( 0, (long)(width-1), 0, (long)(ntilde-1) );

	/* Initialize Integral and Moments */
	/* Integral is equal to one at the beginning since all the filter */
	/* coefficients must add to one for each gamma coefficient. */
	/* 1st moment = k, 2nd moment = k^3, 3rd moment = k^3, ... */
	for ( row=0 ; row<(int)width ; row++ )    /* for the rows */
		for ( col=0 ; col<(int)ntilde ; col++ ) {
	if ( row==0 && col==0 )
		moment[row][col] = (flt_t)1.0;   /* pow() domain error */
	else
		moment[row][col] = (flt_t)pow( (flt_t)row, (flt_t)col );
	}

	/* Print Moment Coefficients */
	if (print) {
		fprintf (stdout, "Integral-Moments Coefficients for X (ntilde=%2d)\n", ntilde);
		for ( row=0 ; row<(int)width ; row++ ) {
			fprintf (stdout, "Coeff[%d] (%.20g", row, moment[row][0]);
			for ( col=1 ; col<(int)ntilde ; col++ )
				fprintf (stdout, ",%.20g", moment[row][col]);
			fprintf (stdout, ")\n");
		}
	}

	return moment;
}

#define CEIL(num,den) ( ((num)+(den)-1)/(den) )

static flt_t	*create_inv_lifting(
	flt_t	*fwd_lifting,
	unsigned int	ntilde,
	unsigned int	width
) {
	int	noGammas;
	flt_t	*inv_lifting = NULL;
	int	col;

	/* Allocate lifting coefficient matrices */
	noGammas = width >> 1;    	// number of Gammas 
	if (noGammas > 0) {
		inv_lifting = new flt_t[noGammas*ntilde];
		for ( col=0 ; col<(int)(noGammas*ntilde) ; col++ ) {
			inv_lifting[col] = -fwd_lifting[col];
		}
	}
	return(inv_lifting);
}

//
// create_fwd_lifting : Updates corresponding moment matrix (X if dir is FALSE
//                      and Y if dir is TRUE) and calculates the lifting
//                      coefficients using the new moments.
//                      This function is used for the forward transform and
//                      uses the original filter coefficients.
//
static flt_t *create_fwd_lifting(
	const flt_t *filter, 
	const unsigned int n, 
	const unsigned int ntilde, 
	const unsigned int width
) {
	flt_t** moment;   /* pointer to the Integral-Moments matrix */
	int len;         /* number of coefficients in this level */
	int step2;      /* step size between same coefficients */
	int noGammas;    /* number of Gamma coeffs */
	flt_t *lifting;

	int col;

	moment = create_moment(ntilde, width, 0);
	if (! moment) return NULL;


	/**********************************/
	/* Initialize important constants */
	/**********************************/

	/* Calculate number of coefficients and step size */
	len = CEIL (width, 1);
	step2 = 1 << 1;

	/* Allocate lifting coefficient matrices */
	noGammas = width >> 1;    	// number of Gammas 
	if (noGammas > 0) {
		lifting = new flt_t[noGammas*ntilde];
		for ( col=0 ; col<(int)(noGammas*ntilde) ; col++ ) {
			lifting[col] = (flt_t)0;
		}
	}

	update_moment(moment, filter, step2, noGammas, len, n, ntilde );
	update_lifting(
		lifting, (const flt_t **) moment, len, step2, 
		noGammas, ntilde
	);

#ifdef	DEBUGING
	fprintf (stdout, "\nLifting Coefficients:\n");
	liftPtr = lifting;
	for ( x=0 ; x<(width>>1) ; x++ ) {
		sprintf (buf, "%.20g", *(liftPtr++));
		for ( k=1 ; k<nTilde ; k++ )
			sprintf (buf, "%s,%.20g", buf, *(liftPtr++));
		fprintf (stdout, "Gamma[%d] (%s)\n", x, buf);
	}
#endif

	/* Free memory allocated for the moment matrices */
	free_matrix(moment, 0, (long)(width-1), 0, (long)(ntilde-1) );

	return(lifting);
}




/*
 * FLWT1D_Predict function: The Gamma coefficients are found as an average
 *                          interpolation of their neighbors in order to find
 *                          the "failure to be linear" or "failure to be
 *                          cubic" or the failure to be the order given by
 *                          N-1. This process uses the filter coefficients
 *                          stored in the filter vector and predicts
 *                          the odd samples based on the even ones
 *                          storing the difference in the gammas.
 *                          By doing so, a Dual Wavelet is created.
 */
static void FLWT1D_Predict (
	Lifting1D::DataType_T* vect,
	const long width,
	const long N,
	flt_t *filter
) {
    register Lifting1D::DataType_T *lambdaPtr,	//pointer to Lambda coeffs
			*gammaPtr;		// pointer to Gamma coeffs
    register flt_t *fP, *filterPtr;   	// pointers to filter coeffs
    register long len,              	// number of coeffs at current level
                  j,                 /* counter for the filter cases */
                  stepIncr;          /* step size between coefficients of the same type */

    long stop1,                      /* number of cases when L < R */
         stop2,                      /* number of cases when L = R */
         stop3,                      /* number of cases when L > R */
         soi;                        /* increment for the middle cases */

    /************************************************/
    /* Calculate values of some important variables */
    /************************************************/
    len       = CEIL(width, 1);   /* number of coefficients at current level */
    stepIncr  = 1 << 1;   /* step size betweeen coefficients */

    /************************************************/
    /* Calculate number of iterations for each case */
    /************************************************/
    j     = IsOdd(len);
    stop1 = N >> 1;
    stop3 = stop1 - j;                /* L > R */
    stop2 = (len >> 1) - N + 1 + j;   /* L = R */
    stop1--;                          /* L < R */

    /***************************************************/
    /* Predict Gamma (wavelet) coefficients (odd guys) */
    /***************************************************/

    filterPtr = filter + N;   /* position filter pointer */

    /* Cases where nbr. left Lambdas < nbr. right Lambdas */
    gammaPtr = vect + (stepIncr >> 1);   /* second coefficient is Gamma */
    while(stop1--) {
        lambdaPtr = vect;   /* first coefficient is always first Lambda */
        j = N;
        do {   /* Gamma update (Gamma - predicted value) */
            *(gammaPtr) -= (*(lambdaPtr)*(*(filterPtr++)));
            lambdaPtr   += stepIncr;   /* jump to next Lambda coefficient */
        } while(--j);   /* use all N filter coefficients */
        /* Go to next Gamma coefficient */
        gammaPtr += stepIncr;
    }

    /* Cases where nbr. left Lambdas = nbr. right Lambdas */
    soi = 0;
    while(stop2--) {
        lambdaPtr = vect + soi;   /* first Lambda to be read */
        fP = filterPtr;   /* filter stays in same values for this cases */
        j = N;
        do {   /* Gamma update (Gamma - predicted value) */
            *(gammaPtr) -= (*(lambdaPtr)*(*(fP++)));
            lambdaPtr   += stepIncr;   /* jump to next Lambda coefficient */
        } while(--j);   /* use all N filter coefficients */
        /* Move start point for the Lambdas */
        soi += stepIncr;
        /* Go to next Gamma coefficient */
        gammaPtr += stepIncr;
    }

    /* Cases where nbr. left Lambdas > nbr. right Lambdas */
    fP = filterPtr;   /* start going backwards with the filter coefficients */
    vect += (soi-stepIncr);   /* first Lambda is always in this place */
    while (stop3--) {
        lambdaPtr = vect;   /* position Lambda pointer */
        j = N;
        do {   /* Gamma update (Gamma - predicted value) */
            *(gammaPtr) -= (*(lambdaPtr)*(*(--fP)));
            lambdaPtr   += stepIncr;   /* jump to next Lambda coefficient */
        } while(--j);   /* use all N filter coefficients */
        /* Go to next Gamma coefficient */
        gammaPtr += stepIncr;
    }
}

/*
 * FLWT1D_Update: the Lambda coefficients have to be "lifted" in order
 *                to find the real wavelet coefficients. The new Lambdas
 *                are obtained  by applying the lifting coeffients stored
 *                in the lifting vector together with the gammas found in
 *                the prediction stage. This process assures that the
 *                moments of the wavelet function are preserved.
 */
static void FLWT1D_Update (
	Lifting1D::DataType_T* vect,
	const long width,
	const long nTilde,
	const flt_t * lc
) {
    const register flt_t * lcPtr;   /* pointer to lifting coefficient values */
    register Lifting1D::DataType_T	*vL,	/* pointer to Lambda values */
					*vG;	/* pointer to Gamma values */
    register long j;         /* counter for the lifting cases */

    long len,                /* number of coefficietns at current level */
         stop1,              /* number of cases when L < R */
         stop2,              /* number of cases when L = R */
         stop3,              /* number of cases when L > R */
         noGammas,           /* number of Gamma coefficients at this level */
         stepIncr,           /* step size between coefficients of the same type */
         soi;                /* increment for the middle cases */


    /************************************************/
    /* Calculate values of some important variables */
    /************************************************/
    len      = CEIL(width, 1);   /* number of coefficients at current level */
    stepIncr = 1 << 1;   /* step size between coefficients */
    noGammas = len >> 1 ;          /* number of Gamma coefficients */

    /************************************************/
    /* Calculate number of iterations for each case */
    /************************************************/
    j	  = IsOdd(len);
    stop1 = nTilde >> 1;
    stop3 = stop1 - j;                   /* L > R */
    stop2 = noGammas - nTilde + 1 + j;   /* L = R */
    stop1--;                             /* L < R */

    /**********************************/
    /* Lift Lambda values (even guys) */
    /**********************************/

    lcPtr = lc;   /* position lifting pointer */

    /* Cases where nbr. left Lambdas < nbr. right Lambdas */
    vG = vect + (stepIncr >> 1);   /* second coefficient is Gamma */
    while(stop1--) {
        vL = vect;   /* lambda starts always in first coefficient */
        j = nTilde;
        do {
            *(vL) += (*(vG)*(*(lcPtr++)));   /* lift Lambda (Lambda + lifting value) */
            vL    += stepIncr;               /* jump to next Lambda coefficient */
        } while(--j);   /* use all nTilde lifting coefficients */
        /* Go to next Gamma coefficient */
        vG += stepIncr;
    }

    /* Cases where nbr. left Lambdas = nbr. right Lambdas */
    soi = 0;
    while(stop2--) {
        vL = vect + soi;   /* first Lambda to be read */
        j = nTilde;
        do {
            *(vL) += (*(vG)*(*(lcPtr++)));   /* lift Lambda (Lambda + lifting value) */
            vL    += stepIncr;               /* jump to next Lambda coefficient */
        } while(--j);   /* use all nTilde lifting coefficients */
        /* Go to next Gamma coefficient */
        vG  += stepIncr;
        /* Move start point for the Lambdas */
        soi += stepIncr;
    }

    /* Cases where nbr. left Lambdas = nbr. right Lambdas */
    vect += (soi - stepIncr);   /* first Lambda is always in this place */
    while(stop3--) {
        vL = vect;   /* position Lambda pointer */
        j = nTilde;
        do {
            *(vL) += (*(vG)*(*(lcPtr++)));   /* lift Lambda (Lambda + lifting value) */
            vL    += stepIncr;               /* jump to next Lambda coefficient */
        } while(--j);   /* use all nTilde lifting coefficients */
        /* Go to next Gamma coefficient */
        vG += stepIncr;
    }
}

static void	forward_transform1d_haar(
	float *data,
	int width
) {
	int	i;

	int	nG;	// # gamma coefficients
	int	nL;	// # lambda coefficients
	double	lsum = 0.0;	// sum of lambda values
	double	lave;	// average of lambda values

    nG = (width >> 1);
    nL = width - nG;

	//
	// Need to preserve average for odd sizes
	//
	if (IsOdd(width)) {
		double	t = 0.0;

		for(i=0;i<width;i++) {
			t += data[i];
		}
		lave = t / (double) width;
	}

	for (i=0; i<nG; i++) {
		data[1] = data[1] - data[0];	// gamma
		data[0] = (float)(data[0] + (data[1] /2.0)); // lambda
		lsum += data[0];

		data += 2;
	}

    // If IsOdd(width), then we have one additional case for */
    // the Lambda calculations. This is a boundary case  */
    // and, therefore, has different filter values.      */
	//
	if (IsOdd(width)) {
		data[0] = (float)((lave * (double) nL) - lsum);
	}
}



static void	inverse_transform1d_haar(
	float *data,
	int width
) {
	int	i;
	int	nG;	// # gamma coefficients
	int	nL;	// # lambda coefficients
	double	lsum = 0.0;	// sum of lambda values
	double	lave;	// average of lambda values

	nG = (width >> 1);
	nL = width - nG;

    // Odd # of coefficients require special handling at boundary
	// Calculate Lambda average 
	//
    if (IsOdd(width) ) {
        double  t = 0.0;

		for(i=0;i<nL;i++) {
            t += data[i];
        }
        lave = t/(double)nL;   // average we've to maintain
    }

	for (i=0; i<nG; i++) {
		data[0] = (float)(data[0] - (data[1] * 0.5));
		data[1] = data[1] + data[0];
		lsum += data[0] +  data[1];

		data += 2;
	}

    // If ODD(len), then we have one additional case for */
    // the Lambda calculations. This is a boundary case  */
    // and, therefore, has different filter values.      */
	//
    if (IsOdd(width)) {
        *data = (float)((lave * (double) width) - lsum);
    }
}
