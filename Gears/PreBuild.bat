set ProjDir=%1
copy /b %ProjDir%main.cpp +,,
copy /b/v/y %ProjDir%GitInfo.h.template %ProjDir%GitInfo.h

FOR /F "tokens=* USEBACKQ" %%F IN (`git rev-parse --short HEAD`) DO (
  SET GitInfo=%%F
)

git diff --quiet

SET GitDirty=
if errorlevel 1 (
  SET GitDirty=-dirty
)

IF "%CI%"=="true" (
 SET GitDirty=%GitDirty%-CI
)

ECHO #define GIT_HASH "%GitInfo%" >>"%ProjDir%GitInfo.h"
ECHO #define GIT_DIFF "%GitDirty%" >>"%ProjDir%GitInfo.h"
