@echo off

set common_compiler_flags= -DGAME_SLOW=1 -MTd -nologo -Od -Oi -TC -W4 -FC -Zi -wd4201 -wd4100 -wd4204 -wd4459 -DDEBUG_BOUNDING_BOX=1 -DDEBUG_VERTICES=1 -DDEBUG_TILE_MAP=1
set common_linker_flags= -incremental:no -opt:ref user32.lib Gdi32.lib Winmm.lib Kernel32.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
REM 64-bit build
del *.pdb > NUL 2> NUL
REM cl %common_compiler_flags% ..\src\game.c -Fgame.map /LD /link -incremental:no -opt:ref /PDB:game%random%.pdb /EXPORT:update_and_render
cl %common_compiler_flags% ..\src\win32_game.c -Fmwin32_game.map /link %common_linker_flags%


REM ---------------------------COMPILER FLAGS------------------------------
REM
REM -Mtd	creates debug multithreaded exe file
REM -nolog	suppresses display of sign-on banner
REM -Od		disables optimization
REM -GR		uses the __fastcall calling convention x86 only
REM -EHa	enable c++ exception handling
REM -Oi		generates intrinsinc functions
REM -wd		disables specific warning e.g. -wd4189 disables warning 4189
REM -W4		sets ouput warning lvl to 4
REM -D		defines constants and macros
REM -FC		displays full path of src files passed to cl.exe in diagnostic test??
REM -Zi		generates complete debugging info
REM
REM Note to self cant have space
REM		WRONG:		set CommonCompilerFlags =
REM		CORRECT:	set CommonCompilerFlags=
REM
REM ---------------------LINKER FLAGS------------------------------
REM
