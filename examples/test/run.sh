SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

exec $SCRIPTPATH/build/test -l 6 -n 4 --proc-type=secondary
