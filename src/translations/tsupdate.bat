@echo off

set TS_DIR=.
set UTILS_DIR=../utils
set LOADER_DIR=../loader
set PLUGINS_DIR=../plugins

set LOPTIONS=-no-obsolete -locations none -source-language en

IF '%1' == '' (
  set UTILS_TS_FILE=
  set LOADER_TS_FILE=
  set PLUGINS_TS_FILE=
) ELSE (
  set UTILS_TS_FILE=-ts %TS_DIR%/%1/vacuumutils.ts
  set LOADER_TS_FILE=-ts %TS_DIR%/%1/vacuumutils.ts
  set PLUGINS_TS_FILE=-ts %TS_DIR%/%1/%%d.ts
)

lupdate %LOPTIONS% %UTILS_DIR%/utils.pro %UTILS_TS_FILE%
lupdate %LOPTIONS% %LOADER_DIR%/loader.pro %LOADER_TS_FILE%
FOR /D %%d IN (%PLUGINS_DIR%/*) DO FOR %%p IN (%PLUGINS_DIR%/%%d/*.pro) DO lupdate %LOPTIONS% %PLUGINS_DIR%/%%d/%%p %PLUGINS_TS_FILE%
