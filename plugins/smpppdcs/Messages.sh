#!bin/sh
$EXTRACTRC `find . -name \*.rc -o -name \*.ui -o -name \*.kcfg` >> rc.cpp
$XGETTEXT `find . -name \*.cpp -o -name \*.h | grep -v '/unittest/'` -o $podir/kopete_smpppdcs.pot
