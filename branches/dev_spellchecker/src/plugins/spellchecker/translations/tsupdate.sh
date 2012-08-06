#!/bin/sh

LOPTIONS="-no-obsolete -locations none -source-language en"

lupdate=lupdate
if which -s lupdate-qt4; then
	lupdate=lupdate-qt4
fi

lupdate="${lupdate} ${LOPTIONS}"

find .. -name '*.pro' -exec ${lupdate} {} \;
