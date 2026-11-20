#!/bin/sh
# usage: tests/snapshot.sh save|check LABEL [--audio] [app ...]
set -e
cd "$(dirname "$0")/.."
MODE=$1; LABEL=$2; shift 2
AUDIO=0; [ "$1" = "--audio" ] && { AUDIO=1; shift; }
APPS=${*:-"dub house hvypsy jazz idm snowcrash hiphop pop funk techno triphop bass rytm ctrl deck dinghy"}
OUT=build/snapshot/run; DB=build/snapshot/$LABEL.sha
rm -rf "$OUT"; mkdir -p "$OUT"
for a in $APPS; do
  src=firmware/src/fm_app.c; def=""; scr=""; lay=""
  case $a in
    dub)       scr=tests/dub_test.txt ;;
    house)     def=-DGENRE_HOUSE;       scr=tests/house_test.txt ;;
    hvypsy)    def=-DGENRE_HEAVY_PSYCH; scr=tests/hvypsy_test.txt ;;
    jazz)      def=-DGENRE_JAZZ;        scr=tests/jazz_test.txt ;;
    idm)       def=-DGENRE_IDM;         scr=tests/idm_test.txt ;;
    snowcrash) def=-DGENRE_SNOWCRASH;   scr=tests/house_test.txt ;;
    hiphop)    def=-DGENRE_HIPHOP;      scr=tests/hiphop_test.txt ;;
    pop)       def=-DGENRE_POP;         scr=tests/pop_test.txt ;;
    funk)      def=-DGENRE_FUNK;        scr=tests/funk_test.txt ;;
    techno)    def=-DGENRE_TECHNO;      scr=tests/techno_test.txt ;;
    triphop)   def=-DGENRE_TRIPHOP;     scr=tests/triphop_test.txt ;;
    bass)      def=-DGENRE_BASS;        scr=tests/bass_test.txt ;;
    rytm|ctrl) src=firmware/src/$a.c;   scr=tests/${a}_test.txt; lay=-DLAYOUT_APP_$(echo $a | tr a-z A-Z) ;;
    deck)      src=firmware/src/deck.c; lay=-DLAYOUT_APP_DECK ;;
    dinghy)    src=firmware/src/dinghy.c ;;
  esac
  [ -n "$scr" ] && [ ! -f "$scr" ] && scr=""
  cc -O2 -w -DDINGHY_HOST -Ifirmware/src -Ifirmware/gen -DAPP_VER='"dev"' $def \
     -o "$OUT/$a.bin" "$src"
  "$OUT/$a.bin" "$OUT/$a" $scr > /dev/null 2>&1
  if [ "$a" != dinghy ]; then
    cc -O0 -w -DDINGHY_HOST -Ifirmware/src -Ifirmware/gen -DAPP_VER='"dev"' $def $lay \
       -o "$OUT/$a.lay" tests/cfg_layout.c
    "$OUT/$a.lay" | sed "s/^/layout $a /" >> "$OUT/layout.txt"
  fi
done
( cd "$OUT" && shasum -a 256 *.wav *.ppm 2>/dev/null; cat layout.txt ) > "$OUT.sha"
if [ "$MODE" = save ]; then cp "$OUT.sha" "$DB"; echo "saved $DB"; exit 0; fi
if [ $AUDIO = 1 ]; then
  grep -v '\.ppm$' "$DB" > "$OUT.want"; grep -v '\.ppm$' "$OUT.sha" > "$OUT.got"
else
  cp "$DB" "$OUT.want"; cp "$OUT.sha" "$OUT.got"
fi
[ $# -gt 0 ] && { pat=$(echo "$APPS" | sed 's/ /|/g'); grep -E " ($pat)([._-]| |$)|layout ($pat) " "$OUT.want" > "$OUT.w2" || true; mv "$OUT.w2" "$OUT.want"; }
diff -u "$OUT.want" "$OUT.got" && echo "snapshot OK"
