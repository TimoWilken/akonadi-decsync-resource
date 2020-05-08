#!/usr/bin/bash
msg() {
    printf '\n\n' >&2
    printf ' ==> %s\n' "$@" >&2
    printf '\n\n' >&2
}

msg 'REPLACING TEST DECSYNC DATA'
rm -rf xdgdata/decsync &&
    cp -a ~/sync/decsync xdgdata/decsync ||
        exit 1

msg 'RUNNING AKONADITEST'
rm -f testenv.sh
akonaditest -c config.xml --testenv testenv.sh &

msg 'WAITING...'
until [ -f testenv.sh ]; do sleep 1s; done

msg 'SOURCING TESTENV.SH'
. ./testenv.sh || exit 2

msg 'RUNNING AKONADICONSOLE'
akonadiconsole

msg 'SHUTTING DOWN'
shutdown-testenvironment
wait
