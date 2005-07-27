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
#ifdef FLOW_EXPORTS
#define FLOW_API __declspec(dllexport)
#else
#define FLOW_API __declspec(dllimport)
#endif
#ifdef JPEG_EXPORTS
//Slightly different definitions for jpeg project:
#     define JPEG_GLOBAL(type) __declspec(dllexport) type
#     define JPEG_EXTERN(type) extern __declspec(dllexport) type
#else
#     define JPEG_GLOBAL(type) __declspec(dllimport) type
#ifdef __cplusplus
#     define JPEG_EXTERN(type) extern "C" __declspec(dllimport) type
#else 
#     define JPEG_EXTERN(type) extern __declspec(dllimport) type
#endif
#endif //JPEG_EXPORTS

#else //not WIN32
#define COMMON_API
#define VDF_API
#define JPEG_GLOBAL(type) type
//Assume all outside projects depending on JPEG are C++
#ifdef JPEG_EXPORTS
#define JPEG_EXTERN(type) extern type
#else
#define JPEG_EXTERN(type) extern "C" type
#endif //ifeq JPEG_EXPORTS
#endif //end !Win32
