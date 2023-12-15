@echo off

set wildcard=*h *c

echo STATICS FOUND:
findstr -s -n -i -l "static" %wildcard%

echo ------
echo ------

echo GLOBALS FOUND:
findstr -s -n -i -l "local_persist" %wildcard%
findstr -s -n -i -l "global_variable" %wildcard%

