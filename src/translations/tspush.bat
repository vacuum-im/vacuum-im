@echo off

echo Pushing source translation (ru) to transifex
tx.exe push -s -f --no-interactive -l ru

echo Pushing english auto translation to transifex
tx.exe push -t -f --no-interactive -l en
