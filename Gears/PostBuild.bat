robocopy %1 %2 %3 /r:10 /w:2
if %errorlevel% leq 1 exit 0 else exit %errorlevel%
