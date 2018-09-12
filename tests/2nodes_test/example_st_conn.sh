#!/bin/sh

killall ptarmd
for i in 3333 4444
do
    ./ptarmd -datadir=./node_$i -bitcoin-conffile=../regtest.conf -port=$i &
done

sleep 1

# ノード接続
#
./ptarmcli -c conf/peer4444.conf 3334
