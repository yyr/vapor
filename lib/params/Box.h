//************************************************************************
//																	*
//		     Copyright (C)  2011									*
//     University Corporation for Atmospheric Research				*
//		     All Rights Reserved									*
//																	*
//************************************************************************/
//
//	File:		Box.h
//
//	Author:		Alan Norton
//			National Center for Atmospheric Research
//			PO 3000, Boulder, Colorado
//
//	Date:		April 2011
//
//	Description:	Defines the Box class
//		This supports control of a 2D or 3D box-shaped region that can be
//		rotated and changed over time.
//
#ifndef BOX_H
#define BOX_H

namespace VAPoR {
#include <vapor/ParamsBase.h>
//! \class Box
//! \brief 3D or 2D box with options for orientation angles and extents changing in time.  
//! Intended to be used in any Params class
//! \author Alan Norton
//! \version $Revision$
//! \date    $Date$
//!

class PARAMS_API Box : public ParamsBase {
public:
	
	Box();
	static ParamsBase* CreateDefaultInstance() {return new Box();}
	
	//! Get the box user extents as a double array at a specified time step >= 0
	//! \param[out] extents[6] double Returned extents
	//! \param[in] timestep size_t Specific time step being retrieved
	//! \retval int zero if successful.
	int GetUserExtents(double extents[6], size_t timestep);
	//! Get the user box extents as a float array at specified time step
	//! for the specified timestep
	//! \param[out] extents[6] float Returned extents
	//! \param[in] timestep size_t Specific time step being retrieved
	//! \retcode int zero if successful.
	int GetUserExtents(float extents[6], size_t timestep);
	//! Get the local box extents as a double array.  If timestep is >= 0, then get it just
	//! for the specified timestep
	//! \param[out] extents[6] double Returned extents
	//! \param[in] timestep int Specific time step being retrieved, or -1 for generic time steps
	//! \retval int zero if successful.

	int GetLocalExtents(double extents[6], int timestep = -1);
	//! Get the local box extents as a float array.  If timestep is >= 0, then get it just
	//! for the specified timestep
	//! \param[out] extents[6] float Returned extents
	//! \param[in] timestep int Specific time step being retrieved, or -1 for generic time steps
	//! \retcode int zero if successful.
	int GetLocalExtents(float extents[6], int timestep = -1);
	//! Get the local box extents as a vector.  First 6 values are default; additional
	//! values are associated with non-default regions
	//! \sa GetTimes()
	//!
	//! \param[out] extents const vector<double>& returned extents
	const vector<double>&  GetLocalExtents() {
		const vector<double> localExtents(6,0.);
		return GetRootNode()->GetElementDouble(_extentsTag,localExtents);
	}
	//! Specify the local extents.  If time step is -1, then set the generic extents.
	//! Otherwise set the extents for a specific timestep.
	//! param[in] extents vector<double>& Six doubles that will be new extents
	//! param[in] timestep int Specified time step, or -1 for generic times
	//! \retval int zero if successful.
	int SetLocalExtents(const vector<double>& extents, int timestep = -1);
	//! Specify the local extents as a double array.  If time step is -1, then set the generic extents.
	//! Otherwise set the extents for a specific timestep.
	//! param[in] double extents6] 6 doubles that will be new extents
	//! param[in] int timestep specified time step, or -1 for generic times
	//! \retval int zero if successful.
	int SetLocalExtents(const double extents[6], int timestep = -1);
	//! Specify the local extents as a float array.  If time step is -1, then set the generic extents.
	//! Otherwise set the extents for a specific timestep.
	//! param[in] float extents[6]
	//! param[in] int timestep specified time step, or -1 for generic times
	//! \retval int zero if successful.
	int SetLocalExtents(const float extents[6], int timestep = -1);
	
	//! Get the three orientation angles (theta, phi, psi)
	//! Defaults to empty vector if no angles are set.
	//! \retval const vector<double> vector of length 3 of angles.
	const vector<double>& GetAngles(){
		const vector<double> defaultAngles(3,0.);
		return GetRootNode()->GetElementDouble(Box::_anglesTag,defaultAngles);
	}
	//! Get the angles as a double array
	//! \param [out] double angles[3] array of three doubles for theta, phi, psi
	//! \retval int zero if successful
	int GetAngles(double ang[3]){
		const vector<double> defaultAngles(3,0.);
		const vector<double>& angles = GetRootNode()->GetElementDouble(Box::_anglesTag,defaultAngles);
		if (angles.size() != 3) return -1;
		for (int i = 0; i<3;i++) ang[i]=angles[i];
		return 0;
	}
	//! Get the angles as a float array
	//! \param [out] angles[3] float array of three floats for theta, phi, psi
	//! \retval zero if successful
	int GetAngles(float ang[3]){
		const vector<double> defaultAngles(3,0.);
		const vector<double>& angles = GetRootNode()->GetElementDouble(Box::_anglesTag,defaultAngles);
		if (angles.size() != 3) return -1;
		for (int i = 0; i<3;i++) ang[i]=(float)angles[i];
		return 0;
	}
	//! Set the angles from a double array
	//! \param [in] ang double[3] array of three doubles for theta, phi, psi
	//! \retval int zero on success
	int SetAngles(const double angles[3]){
		vector<double> ang;
		for (int i = 0; i<3;i++) ang.push_back(angles[i]);
		return GetRootNode()->SetElementDouble(_anglesTag, ang);
	}
	//! Set the angles from a float array
	//! \param [in] angles float[3] array of three floats for theta, phi, psi
	//! \retval int zero on success
	int SetAngles(const float angles[3]){
		vector<double> angl;
		for (int i = 0; i<3;i++) angl.push_back((double)angles[i]);
		return GetRootNode()->SetElementDouble(_anglesTag, angl);
	}
	//! Set the three orientation angles (theta, phi, psi) from a vector of doubles
	//! \param[in] vals const vector<double>& vector of length 3 of angles.
	void SetAngles(const vector<double>& vals){
		GetRootNode()->SetElementDouble(_anglesTag, vals);
	}
	//! Get the time(s) as a long vector.
	//! The first one should be negative, marking the default extents.
	//! Subsequent times are nonnegative integers indicating times for nondefault extents.
	//! Number of times should be 1/6 of the number of extents values
	//! \sa GetExtents()
	//! \retval vector<long>& vector of longs
	const vector<long>& GetTimes() { 
		const vector<long> defaultTimes(1,0);
		return( GetRootNode()->GetElementLong(Box::_timesTag,defaultTimes));
	}
	//! Set the time(s) as a long vector.
	//! The first one should be negative, marking the default extents.
	//! Subsequent times are nonnegative integers indicating times for nondefault extents.
	//! This vector should be the same size as the extents vector.
	//! \param [out] const vector<long>& vector of times
	void SetTimes(const vector<long>& times) { GetRootNode()->SetElementLong(Box::_timesTag, times);}
	//! Trim both the times and extents vectors to same length.
	//! Default is to length 1
	//! \param[in] numTimes int resulting length of times and extentss.
	void Trim(int numTimes = 1){
		if (numTimes > GetTimes().size()) return;
		vector<long> times = GetTimes();
		times.resize(numTimes);
		GetRootNode()->SetElementLong(Box::_timesTag,times);
		vector<double> exts; 
		vector<double>defaultExts(6,0.);
		exts = GetRootNode()->GetElementDouble(Box::_extentsTag,defaultExts);
		GetRootNode()->SetElementDouble(Box::_extentsTag, exts);
	}
	static const string _boxTag;
	static const string _anglesTag;
	static const string _extentsTag;
	static const string _timesTag;

	
};
};
#endif


