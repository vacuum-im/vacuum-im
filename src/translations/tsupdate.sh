#!/bin/sh

LOPTIONS="-no-obsolete -locations none -source-language en"

lupdate=lupdate
if which -s lupdate-qt5; then
	lupdate=lupdate-qt5
fi

lupdate="${lupdate} ${LOPTIONS}"

${lupdate} ../utils/utils.pro
${lupdate} ../loader/loader.pro
find ../plugins -name '*.pro' -exec ${lupdate} {} \;
