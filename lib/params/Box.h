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

class PARAMS_API Box : public ParamsBase {
public:
	Box(const string& name) : ParamsBase(name) {}
	Box();
	static ParamsBase* CreateDefaultInstance() {return new Box();}
	virtual Box* deepCopy(ParamNode* newRoot = 0) {
		Box* box = new Box(*this);
		box->SetRootParamNode(newRoot);
		if(newRoot) newRoot->SetParamsBase(box);
		return box;
	}
	//! Get the box extents as a double array.  If timestep is >= 0, then get it just
	//! for the specified timestep
	//!
	//! \param[out] double extents[6] returned extents
	//! \param[in] int timestep  specific time step being retrieved, or -1 for generic time steps
	//! \retcode int zero if successful.
	int GetExtents(double extents[6], int timestep = -1);
	//! Get the box extents as a vector.  First 6 values are default; additional
	//! values are associated with non-default regions
	//! \sa GetTimes()
	//!
	//! \param[out] const vector<double>& returned extents
	const vector<double>&  GetExtents() {return GetRootNode()->GetElementDouble(_extentsTag);}
	//! Specify the extents.  If time step is -1, then set the generic extents.
	//! Otherwise set the extents for a specific timestep.
	//!
	//! param[in] vector<double>& extents 6 doubles that will be new extents
	//! param[in] int timestep specified time step, or -1 for generic times
	//! \retcode int zero if successful.
	
	int SetExtents(const vector<double>& extents, int timestep = -1);
	//! Specify the extents as a double array.  If time step is -1, then set the generic extents.
	//! Otherwise set the extents for a specific timestep.
	//!
	//! param[in] vector<double>& extents 6 doubles that will be new extents
	//! param[in] int timestep specified time step, or -1 for generic times
	//! \retcode int zero if successful.
	int SetExtents(const double extents[6], int timestep = -1);
	//! Get the three orientation angles (theta, phi, psi)
	//! Defaults to empty vector if no angles are set.
	//! \retval const vector<double> vector of length 3 of angles.
	const vector<double>& GetAngles(){
		return GetRootNode()->GetElementDouble(Box::_anglesTag);
	}
	//! Get the angles as a double array
	//! \param [out] double angles[3] array of three doubles for theta, phi, psi
	int GetAngles(double ang[3]){
		const vector<double>& angles = GetRootNode()->GetElementDouble(Box::_anglesTag);
		if (angles.size() != 3) return -1;
		for (int i = 0; i<3;i++) ang[i]=angles[i];
		return 0;
	}
	//! Set the angles from a double array
	//! \param [in] angles array of three doubles for theta, phi, psi
	//! \retval int zero on success
	int SetAngles(const double ang[3]){
		vector<double> angles;
		for (int i = 0; i<3;i++) angles.push_back(ang[i]);
		return GetRootNode()->SetElementDouble(_anglesTag, angles);
	}
	//! Set the three orientation angles (theta, phi, psi) from a vector of doubles
	//! \param[in] const vector<double> vals& vector of length 3 of angles.
	void SetAngles(const vector<double>& vals){
		GetRootNode()->SetElementDouble(_anglesTag, vals);
	}
	//! Get the time(s) as a long vector.
	//! The first one should be negative, marking the default extents.
	//! Subsequent times are nonnegative integers indicating times for nondefault extents.
	//! Number of times should be 1/6 of the number of extents values
	//! \sa GetExtents()
	//! \retval vector<long>& vector of longs
	const vector<long>& GetTimes() { return( GetRootNode()->GetElementLong(Box::_timesTag));}
	//! Set the time(s) as a long vector.
	//! The first one should be negative, marking the default extents.
	//! Subsequent times are nonnegative integers indicating times for nondefault extents.
	//! This vector should be the same size as the extents vector.
	//! \param [out] const vector<long>& vector of times
	void SetTimes(const vector<long>& times) { GetRootNode()->SetElementLong(Box::_timesTag, times);}
	//! Trim both the times and extents vectors to same length.
	//! Default is to length 1
	//! \param[in] int numTimes  resulting length of times and extentss.
	void Trim(int numTimes = 1){
		if (numTimes > GetTimes().size()) return;
		vector<long> times = GetTimes();
		times.resize(numTimes);
		GetRootNode()->SetElementLong(Box::_timesTag,times);
		vector<double> exts; 
		exts = GetRootNode()->GetElementDouble(Box::_extentsTag);
		GetRootNode()->SetElementDouble(Box::_extentsTag, exts);

	}
	static const string _boxTag;
	static const string _anglesTag;
	static const string _extentsTag;
	static const string _timesTag;

	
};
};
#endif