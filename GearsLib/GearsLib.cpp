// GearsLib.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "GearsLib.h"


// This is an example of an exported variable
GEARSLIB_API int nGearsLib=0;

// This is an example of an exported function.
GEARSLIB_API int fnGearsLib(void)
{
    return 42;
}

// This is the constructor of a class that has been exported.
// see GearsLib.h for the class definition
CGearsLib::CGearsLib()
{
    return;
}
