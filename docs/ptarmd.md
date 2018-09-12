# ptarmd

## NAME

`ptarmd` - Ptarmigan Lightning Daemon

## SYNOPSIS

```bash
ptarmd [options]
```

### options

* -?
  * This help message

* -datadir=&lt;dir&gt;
  * Specify data directory

* -port=&lt;port&gt;
  * Listen for Lightning Network connections on TCP &lt;port&gt; (default: 9735, if DB does't have the data)
  * _NOTICE_: This value is saved to DB for the first time

* -externalip=&lt;ip&gt;
  * Specify external IPv4 address (default: None, if DB does't have the data)
  * _NOTICE_: This value is saved to DB for the first time

* -alias=&lt;name&gt;
  * Specify alias &lt;name&gt; up to 32 bytes (default: "node_"+`node_id first 6bytes`, if DB does't have the data)
  * _NOTICE_: This value is saved to DB for the first time

* -rpcport=&lt;port&gt;
  * Listen for JSON-RPC connections on TCP &lt;port&gt; (default: &lt;LN port&gt;+1)

* -bitcoin-conffile=&lt;file&gt;
  * Specify bitcoind conf file (default: &lt;HOME&gt;/.bitcoin/bitcoin.conf)

* -dev-removedb
  * Remove current DB (without my `node_id`, for developers)

* -dev-removenodeannodb
  * Remove `node_announcement` DB (for developers)

## DESCRIPTION

Start Ptarmigan Lightning Daemon.

### related config file

* announcement config file(`anno.conf`) format

```text
cltv_expiry_delta=[(channel_update) cltv_expiry_delta]
htlc_minimum_msat=[(channel_update) htlc_minimum_msat]
fee_base_msat=[(channel_update) fee_base_msat]
fee_prop_millionths=[(channel_update) fee_prop_millionths]
```

* channel config file(`channel.conf`) format

```text
dust_limit_sat=[dust_lmit_satoshis]
max_htlc_value_in_flight_msat=[max_htlc_value_in_flight_msat]
channel_reserve_sat=[channel_reserve_satothis]
htlc_minimum_msat=[htlc_minimum_msat]
to_self_delay=[to_self_delay]
max_accepted_htlcs=[max_accepted_htlcs]
min_depth=[minimum_depth]
```

## SEE ALSO

## AUTHOR

Nayuta Inc.
