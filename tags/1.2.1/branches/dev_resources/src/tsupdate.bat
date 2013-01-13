@echo off

lupdate client/client.pro
lupdate utils/utils.pro
FOR /D %%d IN (plugins/*) DO FOR %%p IN (plugins/%%d/*.pro) DO lupdate plugins/%%d/%%p