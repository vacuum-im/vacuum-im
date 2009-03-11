@echo off

set LOPTIONS=-no-obsolete -source-language en

lupdate %LOPTIONS% client/client.pro
lupdate %LOPTIONS% utils/utils.pro
FOR /D %%d IN (plugins/*) DO FOR %%p IN (plugins/%%d/*.pro) DO lupdate %LOPTIONS% plugins/%%d/%%p