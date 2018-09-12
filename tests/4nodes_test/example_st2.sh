#!/bin/sh

# ノードの起動
#
# ここでは連続して起動させているが、動作を見る場合にはコンソールをそれぞれ開き、
# 各コンソールで起動させた方がログを見やすい。
for i in 3333 4444 5555 6666
do
    rm -rf ./node_$i/dbptarm
    ./ptarmd -datadir=./node_$i -bitcoin-conffile=../regtest.conf -port=$i &
done
