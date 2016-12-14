// The following ifdef block is the standard way of creating macros which make exporting 
// from a DLL simpler. All files within this DLL are compiled with the GEARSLIB_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see 
// GEARSLIB_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#ifdef GEARSLIB_EXPORTS
#define GEARSLIB_API __declspec(dllexport)
#else
#define GEARSLIB_API __declspec(dllimport)
#endif

// This class is exported from the GearsLib.dll
class GEARSLIB_API CGearsLib {
public:
	CGearsLib(void);
	// TODO: add your methods here.
};

extern GEARSLIB_API int nGearsLib;

GEARSLIB_API int fnGearsLib(void);
