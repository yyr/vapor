//
//      $Id$
//


#ifndef __Transpose__
#define __Transpose__

#define Minimum(a,b) ((a<b)?a:b)
#define BlockSize 32


namespace VAPoR {

  //
  // blocked submatrix Transpose suitable for multithreading
  //   *a : pointer to input matrix
  //   *b : pointer to output matrix
  //    p1,p2: starting index of submatrix (row,col)
  //    m1,m2: size of submatrix (row,col)
  //    s1,s2: size of entire matrix (row,col)
  //
  
  void Transpose(float *a,float *b,int p1,int m1,int s1,int p2,int m2,int s2)
  {
    int I1,I2;
    int i1,i2;
    int q,r;
    register float c0;
    const int block=BlockSize;
    for(I2=p2;I2<p2+m2;I2+=block)
      for(I1=p1;I1<p1+m1;I1+=block)
	for(i2=I2;i2<Minimum(I2+block,p2+m2);i2++)
	  for(i1=I1;i1<Minimum(I1+block,p1+m1);i1++)
	    {
	      q=i2*s1+i1;
	      r=i1*s2+i2;
	      c0=a[q];
	      b[r]=c0;
	    }
  }

  // specialization for Real -> Complex
  // note the S1 matrix dimension is for the Real matrix
  // and the size of the Complex output is then s2 x (S1/2+1)

  
  //
  // blocked matrix Transpose single threaded
  //   *a : pointer to input matrix
  //   *b : pointer to output matrix
  //    s1,s2: size of entire matrix (row,col)
  //
  
  void Transpose(float *a,float *b,int s1,int s2)
  {
    Transpose(a,b,0,s1,s1,0,s2,s2);
  }

    
  
};

#endif

// Local Variables:
// mode:C++
// End:
