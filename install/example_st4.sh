#!/bin/sh

# このファイルから送金情報を作る。
# まだ自動的にルートを決めることができないので、手動で行う。
# 3行目が自ノードからnode_3333へ、4行目がnode_3333からnode_5555への送金を表している。
#   CSVの列は、node_id, short_channel_id, amount_msat, cltv、の順に並んでいる。
# 1行目は送金先からのpreimage_hash、2行目はルートの行数となっている。
#
#   +------------+             +-----------+
#   | node_4444  +------------>| node_3333 |
#   |            |  0.5mBTC    |           |
#   +------------+             |      |    |
#                              |------|----|
#   +------------+             |      v    |
#   | node_5555  |<------------| node_3333 |
#   |            |  0.5mBTC    |           |
#   +------------+             +-----------+

./routing node_4444/dbucoin node_4444/node.conf `./ucoind ./node_5555/node.conf id` 50000000 30
if [ $? -ne 0 ]; then
	echo no routing
	return
fi

# pay設定ファイル出力
#		1: hash=(node_5555から取得したpayment_hash)
#		2: hop_num=3(以下の行数)
#		3: (node_4444 ID),(node_4444--node_3333間short_channel_id),0.5mBTC(msat),CLTV
#		4: (node_3333 ID),(node_3333--node_5555間short_channel_id),0.5mBTC(msat),CLTV
#		5: (node_5555 ID),0,0.5mBTC(msat),CLTV
# (最終行のmsatとCLTVは、その前と同じ値にしておく)。最終行のshort_channel_idは使っていない。
echo `./ucoincli -c conf/peer3333.conf -i 5555` > pay4444_3333_5555.conf
./routing node_4444/dbucoin node_4444/node.conf `./ucoind ./node_5555/node.conf id` 50000000 30 >> pay4444_3333_5555.conf

# 送金実施
./ucoincli -c conf/peer3333.conf -p pay4444_3333_5555.conf 4444

# 3秒以内には終わるはず
sleep 3

# 結果
./ucoincli -l 3333 > node_3333_after.cnl
./ucoincli -l 4444 > node_4444_after.cnl
./ucoincli -l 5555 > node_5555_after.cnl
