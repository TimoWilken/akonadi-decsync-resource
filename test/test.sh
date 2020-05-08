#!/bin/sh
rm -rf xdgdata/decsync &&
    cp -a ~/sync/decsync xdgdata/decsync ||
        exit 1

akonaditest -c config.xml --testenv testenv.sh &
sleep 1s
. ./testenv.sh || exit 2
akonadiconsole
shutdown-testenvironment
wait
