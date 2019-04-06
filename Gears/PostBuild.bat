robocopy %1 %2 %3
if %errorlevel% leq 1 exit 0 else exit %errorlevel%
