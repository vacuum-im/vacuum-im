@echo off

set LOPTIONS=-no-obsolete -locations none -source-language en

lupdate %LOPTIONS% utils/utils.pro
lupdate %LOPTIONS% loader/loader.pro
FOR /D %%d IN (plugins/*) DO FOR %%p IN (plugins/%%d/*.pro) DO lupdate %LOPTIONS% plugins/%%d/%%p