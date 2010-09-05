@echo off

set TS_DIR=translations
set QM_DIR=../translations

FOR /D %%d IN (%TS_DIR%/??_??) DO FOR %%t IN (%TS_DIR%/%%d/*.ts) DO lrelease -compress %TS_DIR%/%%d/%%t -qm %QM_DIR%/%%d/%%~nt.qm
