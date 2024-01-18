@echo off

set common_compiler_flags= -MTd -nologo -Gm- -GR- -EHa- -O2 -Oi -WX -W4 -FC -Z7 -wd4201 -wd4100 -wd4189 -wd4505 -DGAME_SLOW=1 -DDEBUG_BOUNDING_BOX=1 -DDEBUG_VERTICES=1 -DDEBUG_DRAW_PLAYER_POS=0
set common_linker_flags= -incremental:no -opt:ref user32.lib gdi32.lib winmm.lib opengl32.lib

IF NOT EXIST ..\build mkdir ..\build
pushd ..\build
REM 64-bit build
del *.pdb > NUL 2> NUL
REM cl %common_compiler_flags% ..\src\game.cpp -Fgame.map /LD /link -incremental:no -opt:ref /PDB:game%random%.pdb /EXPORT:update_and_render
cl %common_compiler_flags% ..\src\win32_game.cpp -Fmwin32_game.map /link %common_linker_flags%


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
REM ----------------------Optimization Switches--------------------
REM  /O2 /oi /fp:fast
