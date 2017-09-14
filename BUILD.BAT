mkdir BDEBUG
brcc32.exe -IC:\BCC55\Include\ mainw.rc mainw.res
copy mainw.res BDEBUG\mainw.res
del mainw.res
bcc32.exe -c -nBDEBUG /VM /W /VF /w -Lc:\tc5\lib -IC:\BCC55\Include\ mainw.cpp
cd BDEBUG
ilink32.exe -aa -LC:\BCC55\Lib\ -LC:\BCC55\Lib\PSDK\ mainw.obj c0w32.obj, , , cw32mt.lib import32.lib kernel32.lib gdi32.lib glu32.lib opengl32.lib, , mainw.res
cd..
