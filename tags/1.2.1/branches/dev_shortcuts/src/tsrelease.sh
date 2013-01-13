#!/bin/sh

TS_DIR=translations
QM_DIR=../translations

lrelease=lrelease
if which -s lrelease-qt4; then
	lrelease=lrelease-qt4
fi

find "${TS_DIR}" -name '*.ts' | sed "s,^${TS_DIR}/,," | \
while read ts; do
	${lrelease} -compress ${TS_DIR}/${ts} -qm ${QM_DIR}/${ts%.ts}.qm 
done
