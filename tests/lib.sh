md5=$(which md5sum 2>/dev/null || which md5)
famas=../src/famas
test -x $famas || exit 1