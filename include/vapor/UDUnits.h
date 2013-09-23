//
// $Id$
//


#ifndef	_UDUnits_h_
#define	_UDUnits_h_

#include <string>
#include <vector>
#include <map>
#include <algorithm>

union ut_unit;
struct ut_system;

namespace VAPoR {


class UDUnits {
 public:
  UDUnits();
  ~UDUnits();
  int Initialize();
  
  bool IsPressureUnit(std::string unitstr) const;
  bool IsTimeUnit(std::string unitstr) const;
  bool IsLatUnit(std::string unitstr) const;
  bool IsLonUnit(std::string unitstr) const;
  bool IsLengthUnit(std::string unitstr) const;
  bool AreUnitsConvertible(const ut_unit *unit, std::string unitstr) const;
  bool ValidUnit(std::string unitstr) const;
  bool Convert(
    const std::string from,
    const std::string to,
    const float *src,
    float *dst,
    size_t n
  ) const;
  void DecodeTime(
	double seconds, int* year, int* month, int* day,
	int* hour, int* minute, int* second
  ) const;

  double EncodeTime(
    int year, int month, int day, int hour, int minute, int second
  ) const;

  std::string GetErrMsg() const;

 private:
  std::map <int, std::string> _statmsg;
  int _status;
  ut_unit *_pressureUnit;
  ut_unit *_timeUnit;
  ut_unit *_latUnit;
  ut_unit *_lonUnit;
  ut_unit *_lengthUnit;
  ut_system *_unitSystem;
};

};
#endif
