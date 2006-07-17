//----------------------------------------------------------------------------
// Simple cross-platform stopwatch timer
//
// Adapted from:
//   Wright, R.S., Jr., & Lipchak B. (2005). OpenGL SuperBible (3rd ed.).
//   Indianapolis: Sams Publishing.
//----------------------------------------------------------------------------

#ifndef Stopwatch_h
#define Stopwatch_h
#include "common.h"
#ifndef WIN32
#include <sys/time.h>
#endif

namespace VAPoR {

  class RENDER_API Stopwatch
  {
  public:
    
    Stopwatch();
    ~Stopwatch();
    
    float read();
    void reset();
    
  private:
    
#ifdef WIN32
    LARGE_INTEGER _lastCount;
#else
    struct timeval _last;
#endif
  };

};

#endif
