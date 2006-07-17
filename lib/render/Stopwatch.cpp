//----------------------------------------------------------------------------
// Simple cross-platform stopwatch timer
//
// Adapted from:
//   Wright, R.S., Jr., & Lipchak B. (2005). OpenGL SuperBible (3rd ed.).
//   Indianapolis: Sams Publishing.
//----------------------------------------------------------------------------

#ifdef WIN32
#include <windows.h>
#endif
#include "Stopwatch.h"

using namespace VAPoR;

//----------------------------------------------------------------------------
// Constructor
//----------------------------------------------------------------------------
Stopwatch::Stopwatch()
{
  reset();
}

//----------------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------------
Stopwatch::~Stopwatch()
{
}

//----------------------------------------------------------------------------
// Reset stopwatch
//----------------------------------------------------------------------------
void Stopwatch::reset()
{
#ifdef WIN32
  QueryPerformanceCounter(&_lastCount);
#else
  gettimeofday(&_last, 0);
#endif
}

//----------------------------------------------------------------------------
// Get the number of seconds since the last reset
//----------------------------------------------------------------------------
float Stopwatch::read()
{
#ifdef WIN32
  LARGE_INTEGER counterFrequency;
  LARGE_INTEGER current;
  
  QueryPerformanceFrequency(&counterFrequency);
  QueryPerformanceCounter(&current);
  
  return (float)((current.QuadPart - _lastCount.QuadPart) /
                 (float)(counterFrequency.QuadPart));    
#else

  struct timeval current;    // Current timer
  float fSeconds, fFraction; // Number of seconds & fraction (microseconds)
    
  // Get current time
  gettimeofday(&current, 0);
        
  // Subtract the last time from the current time. This is tricky because
  // we have seconds and microseconds. Both must be subtracted and added 
  // together to get the final difference.
  fSeconds = (float)(current.tv_sec - _last.tv_sec);
  fFraction = (float)(current.tv_usec - _last.tv_usec) * 0.000001f;
  return fSeconds + fFraction;
#endif
}

