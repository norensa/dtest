#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
cd $DIR
source ./.env
cd ..

EXEC=$1
TEST_DIR=$2

#remove empty lines from sbin/workers
sed -i '/^$/d' sbin/workers

for m in $(cat sbin/workers) ; do
    ssh -A $CLUSTER_USER@$m "sh -c 'mkdir -p $HOME_DIR/tmp'" &
done
wait

for m in $(cat sbin/workers) ; do
    scp -r $TEST_DIR $CLUSTER_USER@$m:$HOME_DIR/tmp > /dev/null &
done
wait

$EXEC --port $PORT --workers $(wc -l sbin/workers | awk '{ print $1 }') $TEST_DIR &
DRIVER=$!

id=0
WORKERS=()
for m in $(cat sbin/workers) ; do
    ssh -A $CLUSTER_USER@$m "sh -c 'nohup $HOME_DIR/$EXEC --driver $(hostname):$PORT --worker-id $id $HOME_DIR/tmp > /dev/null 2>&1 & echo \$! > $HOME_DIR/tmp/pid'" &
    let id=id+1
done

wait $DRIVER

for m in $(cat sbin/workers) ; do
    ssh -A $CLUSTER_USER@$m "sh -c 'kill \$(cat $HOME_DIR/tmp/pid) ; rm -rf $HOME_DIR/tmp'" &
done
wait
