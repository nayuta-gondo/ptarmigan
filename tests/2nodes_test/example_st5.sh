#!/bin/sh

# チャネルをクローズする。
# 今のところ、Mutual Closeのみ実装している。
# P2PKHアドレスに送金しているため、bitcoindで検出可能なはずである。
#
# チャネルを閉じてもノードとして機能するため、ptarmdは起動したままになる。
# DBからのチャネル情報削除は、gettxout によって funding_txが unspentではなくなったことを確認してから行っている。
. conf/peer3333.conf
./ptarmcli -rpcport=4445 closechannel $node_id

# mining
sleep 3
./generate.sh 1
