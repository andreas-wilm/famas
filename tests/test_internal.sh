pushd .. > /dev/null && make >/dev/null 2>&1 && popd >/dev/null
# otherwise lib sourceing might no work
source lib.sh || exit 1

pushd .. > /dev/null
make clean >/dev/null;
make CFLAGS='-pedantic -UNDEBUG -g -O0 -I/opt/local/include -L/opt/local/lib -DTEST' 2>&1 >/dev/null || \
make CFLAGS='-pedantic -UNDEBUG -g -O0 -I/mnt/software/include -L/mnt/software/lib -DTEST' 2>&1 >/dev/null || exit 1 
# add -DTRACE -pedantic to see even more stuff
popd > /dev/null
if ! $famas -h; then
    echoerror "Calling help failed"
    exit 1
fi
pushd .. > /dev/null 
make clean >/dev/null; make > /dev/null
popd > /dev/null
exit 0
