SCRIPT=$(readlink -f "$0")
SCRIPTPATH=$(dirname "$SCRIPT")

exec $SCRIPTPATH/build/dhl_manager --log-level=8 -l 1,2,3,4 -n 4 -- -p 2,3 -d 4
