TARGET = spellchecker
include(spellchecker.pri)
include(../plugins.inc)

USE_ENCHANT {
  include(enchantchecker.inc)
} else:USE_ASPELL {
  include(aspellchecker.inc)
} else:USE_MACSPELL {
  include(macspellchecker.inc)
} else:USE_NUSPELL {
  include(nuspellchecker.inc)
} else {
  include(hunspellchecker.inc)
}

