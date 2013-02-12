source lib.sh || exit 1

pushd .. > /dev/null
make clean >/dev/null;
make CFLAGS='-pedantic -UNDEBUG -g -O0  -I/opt/local/include -L/opt/local/lib -DTEST' >/dev/null 2>&1 || exit 1
# add -DTRACE -pedantic to see even more stuff
popd > /dev/null
$famas || exit 1
pushd .. > /dev/null 
make clean >/dev/null; make > /dev/null
popd > /dev/null
exit 0
