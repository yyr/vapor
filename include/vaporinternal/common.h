// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the COMMON_EXPORTS
// symbol defined on the command line. this symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// COMMON_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef WIN32
#ifdef COMMON_EXPORTS
#define COMMON_API __declspec(dllexport)
#else
#define COMMON_API __declspec(dllimport)
#endif
#ifdef VDF_EXPORTS
#define VDF_API __declspec(dllexport)
#else
#define VDF_API __declspec(dllimport)
#endif

#else //not WIN32
#define COMMON_API
#define VDF_API

// This class is exported from the common.dll
//class COMMON_API Ccommon {
//public:
//	Ccommon(void);
	// TODO: add your methods here.
//};

//extern COMMON_API int ncommon;

//COMMON_API int fncommon(void);

#endif
