#!/bin/sh

if [ $# -ne 1 ]; then
	echo "Usage: $0 newpluginname"
	exit
fi

name=$1
project=${name}/${name}

mkdir ${name}
echo "SUBDIRS += ${name}" >> plugins.pro
echo "TARGET = ${name}" >> ${project}.pro
echo "include(${name}.pri)" >> ${project}.pro
echo "include(../plugins.inc)" >> ${project}.pro

echo "HEADERS = " >> ${project}.pri
echo "SOURCES = " >> ${project}.pri

lupdate=lupdate

if which -s lupdate-qt4; then
	lupdate=lupdate-qt4
fi

${lupdate} ${project}.pro

