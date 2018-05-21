SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

exec $SCRIPTPATH/build/ipsec_gateway -l 6 -n 4 --proc-type=secondary
