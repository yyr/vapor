//
//      $Id$
//

#ifndef	_Lifting1D_h_
#define	_Lifting1D_h_

#include <iostream>
#include <vapor/EasyThreads.h>
#include <vapor/MyBase.h>

namespace VAPoR {

//
//! \class Lifting1D
//! \brief Wrapper for Wim Swelden's Liftpack wavelet transform interface
//! \author John Clyne
//! \version $Revision$
//! \date    $Date$
//!
//! This class provides an interface to the Liftpack wavelet transformation
//! library.
//
class VDF_API Lifting1D : public VetsUtil::MyBase {

public:
 typedef	double	Flt;
 typedef	float	DataType_T;

 //! maximum number of filter coefficients
 static const int	MAX_FILTER_COEFF = 32;

 //! \param[in] n Number of wavelet filter coefficients. Valid values are
 //! from 1 to Lifting1D::MAX_FILTER_COEFF
 //! \param[in] ntilde Number of wavelet lifting coefficients. Valid values are
 //! from 1 to Lifting1D::MAX_FILTER_COEFF
 //! \param[in] width Number of samples to be transformed
 //
 Lifting1D(
	unsigned int n,		// # wavelet filter coefficents
	unsigned int ntilde,	// # wavelet lifting coefficients
	unsigned int width	// # of samples to transform
 );

 ~Lifting1D();

 //! Apply forward lifting transform
 //!
 //! Apply forward lifting transform to \p width samples from \p data. The 
 //! transform is performed in place. 
 //! \param[in,out] data Data to transform
 //! \sa InverseTransform(), Lifting1D()
 //
 void	ForwardTransform(DataType_T *data);

 //! Apply inverse lifting transform
 //!
 //! Apply inverse lifting transform to \p width samples from \p data. The 
 //! transform is performed in place. 
 //! \param[in,out] data Data to transform
 //! \sa ForwardTransform(), Lifting1D()
 //
 void	InverseTransform(DataType_T *data);

private:
 int	_objInitialized;	// has the obj successfully been initialized?

 unsigned int	n_c;		// # wavelet filter coefficients
 unsigned int	ntilde_c;	// # wavelet lifting coefficients
 unsigned int	max_width_c;
 unsigned int	width_c;
 Flt	*fwd_filter_c;		// forward transform filter coefficients
 Flt	*inv_filter_c;		// inverse transform filter coefficients
 Flt	*fwd_lifting_c;		// forward transform lifting coefficients
 Flt	*inv_lifting_c;		// inverse transform lifting coefficients

};

};

#endif	//	_Lifting1D_h_
