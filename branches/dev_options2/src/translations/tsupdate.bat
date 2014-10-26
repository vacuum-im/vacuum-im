@echo off

if '%1'=='' (set LANG=??) ELSE (set LANG=%1)

set TS_DIR=.
set UTILS_DIR=../utils
set LOADER_DIR=../loader
set PLUGINS_DIR=../plugins

set LOPTIONS=-no-obsolete -locations none -source-language en -target-language %%~nt

FOR /D %%t IN (%TS_DIR%/%LANG%) DO lupdate %LOPTIONS% %UTILS_DIR%/utils.pro -ts %TS_DIR%/%%t/vacuumutils.ts
FOR /D %%t IN (%TS_DIR%/%LANG%) DO lupdate %LOPTIONS% %LOADER_DIR%/loader.pro -ts %TS_DIR%/%%t/vacuum.ts
FOR /D %%t IN (%TS_DIR%/%LANG%) DO FOR /D %%d IN (%PLUGINS_DIR%/*) DO FOR %%p IN (%PLUGINS_DIR%/%%d/*.pro) DO lupdate %LOPTIONS% %PLUGINS_DIR%/%%d/%%p -ts %TS_DIR%/%%t/%%d.ts
