#!/bin/sh

## Author: Mikhail Borisov <borisov.mikhail@gmail.com>
## A utility script to convert between per-file and combined translation files


# Immediately fail on errors
set -e

# Set up lupdate
LOPTIONS="-locations absolute -source-language en -target-language $locale"
lupdate=lupdate
if which lupdate-qt4 >/dev/null 2>&1; then
  lupdate=lupdate-qt4
fi
lupdate="${lupdate} ${LOPTIONS}"

action="$1"
locale="$2"

combined="$2_combined.ts"

check_locale() {
  if [ -z $locale ]; then
    echo "You need to specify a locale to work with"
    if [ ! -z $LANG ]; then
      echo "Your locale is $(echo $LANG | sed -e 's/\..*//')"
    fi
    return 1
  elif [ -d $locale ]; then
    return 0
  else
    echo "Locale $locale not existing"
    return 1
  fi
}

pack_locale() {
  echo "Packing locale..."

  head -n3 $locale/vacuum.ts > $combined
  for i in $locale/*.ts; do
    sed -e '1,3d' -e '$d' $i >> $combined
  done
  tail -n1 $locale/vacuum.ts >> $combined

  echo "Invoking lupdate on the combined locale..."
  (cd ../.. && $lupdate -recursive src/ -ts src/translations/$combined)
  echo "Done."
}

unpack_locale() {
  if [ ! -f $combined ]; then
    echo "Combined locale file not found"
    return 1
  fi

  echo "Unpacking locale..."

  for i in $locale/*.ts; do 
    cp $combined $i 
  done

  echo "Stripping locale files with tsupdate.sh"
  sh tsupdate.sh 2>/dev/null

  echo "Done"
}

if [ ! -f tsupdate.sh ]; then
  echo "Sanity check failed: this script must only be run from src/translations subdirectory"
  exit 1
fi

case $action in
  pack)
    check_locale 
    pack_locale
    ;;
  unpack)
    check_locale 
    unpack_locale
    ;;
  *)
    echo "Usage:"
    echo "  $0 pack <locale> --- Create a combined locale .ts file as <locale>_combined.ts"
    echo "  $0 unpack <locale> --- Applies <locale>_combined.ts to <locale>"
    ;;
esac
