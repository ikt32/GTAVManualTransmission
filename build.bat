@call "%VS140COMNTOOLS%\..\..\VC\vcvarsall.bat" x64
@echo checking for nuget
@if exist nuget.exe nuget.exe restore
set ExternalCompilerOptions=
msbuild /p:Platform=x64 /p:Configuration=Release /clp:Summary /nologo
