''' The vapor_utils module contains:
	mag3d - calculate magnitude of 3-vector
	mag2d - calculate magnitude of 2-vector
	deriv_findiff - calculate derivative using 6th order finite differences
	curl_findiff - calculate curl using 6th order finite differences
	div_findiff - calculate divergence using 6 order finite differences
	grad_findiff - calculate gradient using 6th order finite differences.
	deriv_vert - calculate a vertical derivative of one 3D variable with 
		respect to another.	
'''

def mag3d(a1,a2,a3): 
	'''Calculate the magnitude of a 3-vector.
	Calling sequence: MAG = mag3d(A,B,C)
	Where:  A, B, and C are float32 numpy arrays.
	Result MAG is a float32 numpy array containing the square root
	of the sum of the squares of A, B, and C.'''
	from numpy import sqrt
	return sqrt(a1*a1 + a2*a2 + a3*a3)

def mag2d(a1,a2): 
	'''Calculate the magnitude of a 2-vector.
	Calling sequence: MAG = mag2d(A,B)
	Where:  A, and B are float32 numpy arrays.
	Result MAG is a float32 numpy array containing the square root
	of the sum of the squares of A and B.'''
	from numpy import sqrt
	return sqrt(a1*a1 + a2*a2)

def deriv_findiff(a,dir,dx):
	''' Function that calculates first-order derivatives 
	using sixth order finite differences in regular Cartesian grids.
	Calling sequence: deriv = deriv_findiff(ary,dir,delta)
	ary is a 3-dimensional float32 numpy array
	dir is 1, 2, or 3 (for X, Y, or Z directions)
	delta is the grid spacing in the direction dir.
	Returned array 'deriv' is the derivative of ary in the 
	specified direction dir. '''
	import numpy 
	s = numpy.shape(a)	#size of the input array
	aprime = numpy.array(a)	#output has the same size than input

	#
	# derivative for dir=1
	#
	if dir == 1: 

		#forward differences near the first boundary
		for i in range(3):
			aprime[:,:,i] = (-11*a[:,:,i]+18*a[:,:,i+1]-9*a[:,:,i+2]+2*a[:,:,i+3]) / (6*dx)     

		#centered differences
		for i in range(3,s[2]-3):
			aprime[:,:,i] = (-a[:,:,i-3]+9*a[:,:,i-2]-45*a[:,:,i-1]+45*a[:,:,i+1] -9*a[:,:,i+2]+a[:,:,i+3])/(60*dx) 

		#backward differences near the second boundary
		for i in range(s[2]-3,s[2]):
			aprime[:,:,i] = (-2*a[:,:,i-3]+9*a[:,:,i-2]-18*a[:,:,i-1]+11*a[:,:,i]) /(6*dx)     

	#
	# derivative for dir=2
	#
	if dir == 2: 
		for i in range(3):
			aprime[:,i,:] = (-11*a[:,i,:]+18*a[:,i+1,:]-9*a[:,i+2,:]+2*a[:,i+3,:]) /(6*dx)     #forward differences near the first boundary

		for i in range(3,s[1]-3):
			aprime[:,i,:] = (-a[:,i-3,:]+9*a[:,i-2,:]-45*a[:,i-1,:]+45*a[:,i+1,:] -9*a[:,i+2,:]+a[:,i+3,:])/(60*dx) #centered differences

		for i in range(s[1]-3,s[1]):
			aprime[:,i,:] = (-2*a[:,i-3,:]+9*a[:,i-2,:]-18*a[:,i-1,:]+11*a[:,i,:]) /(6*dx)     #backward differences near the second boundary

	#
	# derivative for dir=3
	#
	if dir == 3:
		for i in range(3):
			aprime[i,:,:] = (-11*a[i,:,:]+18*a[i+1,:,:]-9*a[i+2,:,:]+2*a[i+3,:,:]) /(6*dx)     #forward differences near the first boundary

		for i in range(3,s[0]-3):
			aprime[i,:,:] = (-a[i-3,:,:]+9*a[i-2,:,:]-45*a[i-1,:,:]+45*a[i+1,:,:] -9*a[i+2,:,:]+a[i+3,:,:])/(60*dx) #centered differences

		for i in range(s[0]-3,s[0]):
			aprime[i,:,:] = (-2*a[i-3,:,:]+9*a[i-2,:,:]-18*a[i-1,:,:]+11*a[i,:,:]) /(6*dx)     #backward differences near the second boundary

	return aprime

def curl_findiff(A,B,C):
	''' Operator that calculates the curl of a vector field 
	using 6th order finite differences.
	Calling sequence:  curlfield = curl_findiff(A,B,C)
	Where:
	A,B,C are three 3-dimensional float32 arrays that define a
	vector field.  
	curlfield is a 3-tuple of 3-dimensional float32 arrays that is 
	returned by this operator.'''
	import vapor
	ext = (vapor.BOUNDS[3]-vapor.BOUNDS[0], vapor.BOUNDS[4]-vapor.BOUNDS[1],vapor.BOUNDS[5]-vapor.BOUNDS[2])
	usrmax = vapor.MapVoxToUser([vapor.BOUNDS[3],vapor.BOUNDS[4],vapor.BOUNDS[5]],vapor.REFINEMENT)
	usrmin = vapor.MapVoxToUser([vapor.BOUNDS[0],vapor.BOUNDS[1],vapor.BOUNDS[2]],vapor.REFINEMENT)
	dx = (usrmax[2]-usrmin[2])/ext[2]	# grid delta is dx in user coord system
	dy =  (usrmax[1]-usrmin[1])/ext[1]
	dz =  (usrmax[0]-usrmin[0])/ext[0]
	
	aux1 = deriv_findiff(C,2,dy)       #x component of the curl
	aux2 = deriv_findiff(B,3,dz)     
	outx = aux1-aux2						

	aux1 = deriv_findiff(A,3,dz)       #y component of the curl
	aux2 = deriv_findiff(C,1,dx)
	outy = aux1-aux2

	aux1 = deriv_findiff(B,1,dx)       #z component of the curl
	aux2 = deriv_findiff(A,2,dy)
	outz = aux1-aux2

	return outx, outy, outz     	#return results in user coordinate order


# Calculate divergence
def div_findiff(A,B,C):
	''' Operator that calculates the divergence of a vector field
	using 6th order finite differences.
	Calling sequence:  DIV = div_findiff(A,B,C)
	Where:
	A, B, and C are 3-dimensional float32 arrays defining a vector field.
	Resulting DIV is a 3-dimensional float3d array consisting of
	the divergence of the triple (A,B,C).'''
	import vapor
	ext = (vapor.BOUNDS[3]-vapor.BOUNDS[0], vapor.BOUNDS[4]-vapor.BOUNDS[1],vapor.BOUNDS[5]-vapor.BOUNDS[2])
        usrmax = vapor.MapVoxToUser([vapor.BOUNDS[3],vapor.BOUNDS[4],vapor.BOUNDS[5]],vapor.REFINEMENT)
        usrmin = vapor.MapVoxToUser([vapor.BOUNDS[0],vapor.BOUNDS[1],vapor.BOUNDS[2]],vapor.REFINEMENT)
        dx = (usrmax[2]-usrmin[2])/ext[2]       # grid delta in user coord system
        dy =  (usrmax[1]-usrmin[1])/ext[1]
        dz =  (usrmax[0]-usrmin[0])/ext[0]
# in User coords, A,B,C are x,y,z components
        return deriv_findiff(C,3,dz)+deriv_findiff(B,2,dy)+deriv_findiff(A,1,dx)


def grad_findiff(A):
	''' Operator that calculates the gradiant of a scalar field 
	using 6th order finite differences.
	Calling sequence:  GRD = grad_findiff(A)
	Where:
	A is a float32 array defining a scalar field.
	Result GRD is a triple of 3 3-dimensional float3d arrays consisting of
	the gradient of A.'''
	import vapor
	ext = (vapor.BOUNDS[3]-vapor.BOUNDS[0], vapor.BOUNDS[4]-vapor.BOUNDS[1],vapor.BOUNDS[5]-vapor.BOUNDS[2])
	usrmax = vapor.MapVoxToUser([vapor.BOUNDS[3],vapor.BOUNDS[4],vapor.BOUNDS[5]],vapor.REFINEMENT)
	usrmin = vapor.MapVoxToUser([vapor.BOUNDS[0],vapor.BOUNDS[1],vapor.BOUNDS[2]],vapor.REFINEMENT)
	dx = (usrmax[2]-usrmin[2])/ext[2]
	dy =  (usrmax[1]-usrmin[1])/ext[1]
	dz =  (usrmax[0]-usrmin[0])/ext[0]

	aux1 = deriv_findiff(A,1,dx)       #x component of the gradient (in python system)
	aux2 = deriv_findiff(A,2,dy)
	aux3 = deriv_findiff(A,3,dz)
	

	return aux1,aux2,aux3 # return in user coordinate (x,y,z) order

def deriv_vert(a,deltap):
        ''' Calculates vertical derivative of a 3D variable 
        with respect to another 3D variable,
        using 6th order finite differences. 
        Calling sequence: DERV = deriv_vert(A,DELTAP) where:
        A is an array that is being differentiated
        DELTAP is the array of differences of the second variable'''
#  calculates derivative of a variable with respect to another var (P)
#  based on deriv_findiff code
#  finds da/dp where deltap is a 3d differences array for p
	import numpy
        s = numpy.shape(a)      #size of the input array
        aprime = numpy.array(a) #output has the same size than input
        for i in range(3):
                aprime[i,:,:] = (-11*a[i,:,:]+18*a[i+1,:,:]-9*a[i+2,:,:]+2*a[i+3,:,:]) /(6*deltap[i,:,:])     #forward differences near the first boundary
        for i in range(3,s[0]-3):
                aprime[i,:,:] = (-a[i-3,:,:]+9*a[i-2,:,:]-45*a[i-1,:,:]+45*a[i+1,:,:] -9*a[i+2,:,:]+a[i+3,:,:])/(60*deltap[i,:,:]) #centered differences
        for i in range(s[0]-3,s[0]):
                aprime[i,:,:] = (-2*a[i-3,:,:]+9*a[i-2,:,:]-18*a[i-1,:,:]+11*a[i,:,:]) /(6*deltap[i,:,:])     #backward differences near the second boundary
        return aprime

