#!/bin/bash

DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
source $DIR/.env

POSITIONAL_ARGS=()
ENV=
while [[ $# -gt 0 ]]; do
    case $1 in
        --env)
            ENV="$ENV export $2 ;"
            shift
            shift
        ;;

        *)
            POSITIONAL_ARGS+=("$1") # save positional arg
            shift
        ;;
    esac
done
set -- "${POSITIONAL_ARGS[@]}" # restore positional parameters

EXEC=$DIR/../$1
TEST_DIR=$2

#remove empty lines from sbin/workers
sed -i '/^$/d' $DIR/workers

for m in $(cat $DIR/workers) ; do
    ssh -A $CLUSTER_USER@$m "sh -c 'mkdir -p $DTEST_HOME/tmp'" &
done
wait

for m in $(cat $DIR/workers) ; do
    scp -r $TEST_DIR $CLUSTER_USER@$m:$DTEST_HOME/tmp > /dev/null &
done
wait

$ENV
$EXEC --port $PORT --workers $(wc -l $DIR/workers | awk '{ print $1 }') $TEST_DIR &
DRIVER=$!

id=0
WORKERS=()
for m in $(cat $DIR/workers) ; do
    ssh -A $CLUSTER_USER@$m "sh -c '\
        cd $DTEST_HOME ; \
        $ENV \
        nohup $EXEC --driver $(hostname):$PORT --worker-id $id tmp > /dev/null 2>&1 & \
        echo \$! > tmp/pid \
    '" &
    let id=id+1
done

wait $DRIVER

for m in $(cat $DIR/workers) ; do
    ssh -A $CLUSTER_USER@$m "sh -c '\
        kill \$(cat $DTEST_HOME/tmp/pid) ; \
        rm -rf $DTEST_HOME/tmp \
    '" &
done
wait
