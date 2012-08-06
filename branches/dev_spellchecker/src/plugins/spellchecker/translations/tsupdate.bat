@echo off
set LOPTIONS=-no-obsolete -locations none -source-language en
FOR %%p IN (..\*.pro) DO lupdate %LOPTIONS% %%p
