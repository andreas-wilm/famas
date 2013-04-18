echoerror() {
    echo "ERROR: $@" 1>&2
}
echook() {
    echo "OK: $@" 1>&2
}
echowarn() {
    echo "WARN: $@" 1>&2
}
echoinfo() {
    echo "INFO: $@" 1>&2
}
echodebug() {
    echo "DEBUG: $@" 1>&2
}

md5=$(which md5sum 2>/dev/null || which md5)

famas=../src/famas
test -x $famas || exit 1
