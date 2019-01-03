/*
 *  Copyright (C) 2017, Nayuta, Inc. All Rights Reserved
 *  SPDX-License-Identifier: Apache-2.0
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
/** @file   btc_sw.c
 *  @brief  bitcoin処理: Segwitトランザクション生成関連
 */
#include "utl_dbg.h"
#include "utl_int.h"

#include "btc_local.h"
#include "btc_util.h"
#include "btc_script.h"
#include "btc_sig.h"
#include "btc_sw.h"


/**************************************************************************
 * public functions
 **************************************************************************/

bool btc_sw_add_vout_p2wpkh_pub(btc_tx_t *pTx, uint64_t Value, const uint8_t *pPubKey)
{
    return btcl_util_add_vout_pub(pTx, Value, pPubKey, (mNativeSegwit) ? BTC_PREF_P2WPKH : BTC_PREF_P2SH);
}


bool btc_sw_add_vout_p2wpkh(btc_tx_t *pTx, uint64_t Value, const uint8_t *pPubKeyHash)
{
    return btcl_util_add_vout_pkh(pTx, Value, pPubKeyHash, (mNativeSegwit) ? BTC_PREF_P2WPKH : BTC_PREF_P2SH);
}


bool btc_sw_add_vout_p2wsh_wit(btc_tx_t *pTx, uint64_t Value, const utl_buf_t *pWitScript)
{
    if (!pWitScript->len) return false;

    uint8_t wit_prog[BTC_SZ_WITPROG_P2WSH];
    btc_sw_wit2prog_p2wsh(wit_prog, pWitScript);
    if (mNativeSegwit) {
        btc_vout_t *vout = btc_tx_add_vout(pTx, Value);
        if (!utl_buf_alloccopy(&vout->script, wit_prog, sizeof(wit_prog))) return false;
    } else {
        uint8_t sh[BTC_SZ_HASH_MAX];

        btc_util_hash160(sh, wit_prog, sizeof(wit_prog));
        if (!btc_tx_add_vout_p2sh(pTx, Value, sh)) return false;
    }
    return true;
}


bool btc_sw_scriptcode_p2wpkh_vin(utl_buf_t *pScriptCode, const btc_vin_t *pVin)
{
    //P2WPKH witness
    //      0: <signature>
    //      1: <pubkey>
    if (pVin->wit_item_cnt != 2) {
        return false;
    }

    return btc_script_code_p2wpkh(pScriptCode, pVin->witness[1].buf);
}


bool btc_sw_scriptcode_p2wsh_vin(utl_buf_t *pScriptCode, const btc_vin_t *pVin)
{
    //P2WSH witness
    //      ....
    //      wit_item_cnt - 1: witnessScript
    if (pVin->wit_item_cnt == 0) {
        return false;
    }

    return btc_script_code_p2wsh(pScriptCode, &pVin->witness[pVin->wit_item_cnt - 1]);
}


bool btc_sw_sighash(uint8_t *pTxHash, const btc_tx_t *pTx, uint32_t Index, uint64_t Value,
                const utl_buf_t *pScriptCode)
{
    // [transaction version : 4]
    // [hash_prevouts : 32]
    // [hash_sequence : 32]
    // [outpoint : 32 + 4]
    // [scriptcode : xx]
    // [amount : 8]
    // [sequence : 4]
    // [hash_outputs : 32]
    // [locktime : 4]
    // [hash_type : 4]

    bool ret = false;
    utl_buf_t buf = UTL_BUF_INIT;
    utl_buf_t buf_tmp = UTL_BUF_INIT;
    uint32_t lp;
    uint8_t *p;
    uint8_t *p_tmp;
    int len;

    btc_tx_valid_t txvld = btc_tx_is_valid(pTx);
    if (txvld != BTC_TXVALID_OK) {
        LOGD("fail: invalid tx\n");
        return false;
    }

    if (!utl_buf_alloc(&buf, 156 + pScriptCode->len)) goto LABEL_EXIT;
    p = buf.buf;

    //version
    utl_int_unpack_u32le(p, pTx->version);
    p += 4;

    //vin:
    // prev outs:

    //hash_prevouts: double-SHA256((txid(32) | index(4)) * n)
    if (!utl_buf_realloc(&buf_tmp, pTx->vin_cnt * (32 + 4))) goto LABEL_EXIT;
    p_tmp = buf_tmp.buf;
    for (lp = 0; lp < pTx->vin_cnt; lp++) {
        btc_vin_t *vin = &pTx->vin[lp];

        memcpy(p_tmp, vin->txid, BTC_SZ_TXID);
        p_tmp += BTC_SZ_TXID;
        utl_int_unpack_u32le(p_tmp, vin->index);
        p_tmp += 4;
    }
    btc_util_hash256(p, buf_tmp.buf, buf_tmp.len);
    p += BTC_SZ_HASH256;

    //hash_sequence: double-SHA256(sequence(4) * n)
    if (!utl_buf_realloc(&buf_tmp, pTx->vin_cnt * 4)) goto LABEL_EXIT;
    p_tmp = buf_tmp.buf;
    for (lp = 0; lp < pTx->vin_cnt; lp++) {
        btc_vin_t *vin = &pTx->vin[lp];

        utl_int_unpack_u32le(p_tmp, vin->sequence);
        p_tmp += 4;
    }
    btc_util_hash256(p, buf_tmp.buf, buf_tmp.len);
    p += BTC_SZ_HASH256;

    //outpoint: double-SHA256(txid(32) | Index(4))
    memcpy(p, pTx->vin[Index].txid, BTC_SZ_TXID);
    p += BTC_SZ_TXID;
    utl_int_unpack_u32le(p, pTx->vin[Index].index);
    p += 4;

    //scriptcode
    memcpy(p, pScriptCode->buf, pScriptCode->len);
    p += pScriptCode->len;

    //amount
    utl_int_unpack_u64le(p, Value);
    p += 8;

    //sequence
    utl_int_unpack_u32le(p, pTx->vin[Index].sequence);
    p += 4;

    //vout:
    // next vins:

    //hash_outputs: double-SHA256((value(8) | scriptPk) * n)
    len = 0;
    for (lp = 0; lp < pTx->vout_cnt; lp++) {
        len += 8;
        len += 1; //XXX:
        len += pTx->vout[lp].script.len;
    }
    if (!utl_buf_realloc(&buf_tmp, len)) goto LABEL_EXIT;
    p_tmp = buf_tmp.buf;
    for (lp = 0; lp < pTx->vout_cnt; lp++) {
        btc_vout_t *vout = &pTx->vout[lp];

        utl_int_unpack_u64le(p_tmp, vout->value);
        p_tmp += 8;
        *p_tmp = vout->script.len;
        p_tmp++;
        memcpy(p_tmp, vout->script.buf, vout->script.len);
        p_tmp += vout->script.len;
    }
    btc_util_hash256(p, buf_tmp.buf, buf_tmp.len);
    p += BTC_SZ_HASH256;

    //locktime
    utl_int_unpack_u32le(p, pTx->locktime);
    p += 4;

    //hashtype
    utl_int_unpack_u32le(p, SIGHASH_ALL);
    p += 4;

    btc_util_hash256(pTxHash, buf.buf, buf.len);

    ret = true;

LABEL_EXIT:
    utl_buf_free(&buf);
    utl_buf_free(&buf_tmp);

    return ret;
}


bool btc_sw_set_vin_p2wpkh(btc_tx_t *pTx, uint32_t Index, const utl_buf_t *pSig, const uint8_t *pPubKey)
{
    //P2WPKH:
    //witness
    //  item[0]=sig
    //  item[1]=pubkey

    btc_vin_t *vin = &(pTx->vin[Index]);
    utl_buf_t *p_buf = &vin->script;

    if (p_buf->len != 0) {
        utl_buf_free(p_buf);
    }

    if (mNativeSegwit) {
        //vin
        //  空
    } else {
        //vin
        //  len + <witness program>
        p_buf->len = 3 + BTC_SZ_HASH160;
        p_buf->buf = (uint8_t *)UTL_DBG_REALLOC(p_buf->buf, p_buf->len);
        p_buf->buf[0] = 0x16;
        //witness program
        p_buf->buf[1] = 0x00;
        p_buf->buf[2] = (uint8_t)BTC_SZ_HASH160;
        btc_util_hash160(&p_buf->buf[3], pPubKey, BTC_SZ_PUBKEY);
    }

    if (vin->wit_item_cnt != 0) {
        //一度解放する
        for (uint32_t lp = 0; lp < vin->wit_item_cnt; lp++) {
            utl_buf_free(&vin->witness[lp]);
        }
        vin->wit_item_cnt = 0;
    }
    //[0]signature
    utl_buf_t *p_sig = btc_tx_add_wit(vin);
    utl_buf_alloccopy(p_sig, pSig->buf, pSig->len);
    //[1]pubkey
    utl_buf_t *p_pub = btc_tx_add_wit(vin);
    utl_buf_alloccopy(p_pub, pPubKey, BTC_SZ_PUBKEY);
    return true;
}


bool btc_sw_set_vin_p2wsh(btc_tx_t *pTx, uint32_t Index, const utl_buf_t *pWits[], int Num)
{
    //P2WSH:
    //vin
    //  len + <witness program>
    //witness
    //  パターンが固定できないので、pWitsに作ってあるものをそのまま載せる。
    btc_vin_t *vin = &(pTx->vin[Index]);
    utl_buf_t *p_buf = &vin->script;

    if(mNativeSegwit) {
        //vin
        //  空
    } else {
        //vin
        //  len + <witness program>
        p_buf->len = 3 + BTC_SZ_HASH256;
        p_buf->buf = (uint8_t *)UTL_DBG_REALLOC(p_buf->buf, p_buf->len);
        p_buf->buf[0] = 0x22;
        //witness program
        p_buf->buf[1] = 0x00;
        p_buf->buf[2] = (uint8_t)BTC_SZ_HASH256;
        //witnessScriptのSHA256値
        //  witnessScriptは一番最後に置かれる
        btc_util_sha256(p_buf->buf + 3, pWits[Num - 1]->buf, pWits[Num - 1]->len);
    }

    if (vin->wit_item_cnt != 0) {
        //一度解放する
        for (uint32_t lp = 0; lp < vin->wit_item_cnt; lp++) {
            utl_buf_free(&vin->witness[lp]);
        }
        vin->wit_item_cnt = 0;
    }
    for (int lp = 0; lp < Num; lp++) {
        utl_buf_t *p = btc_tx_add_wit(vin);
        utl_buf_alloccopy(p, pWits[lp]->buf, pWits[lp]->len);
    }
    return true;
}


bool btc_sw_verify_p2wpkh(const btc_tx_t *pTx, uint32_t Index, uint64_t Value, const uint8_t *pPubKeyHash)
{
    btc_vin_t *vin = &(pTx->vin[Index]);
    if (vin->wit_item_cnt != 2) {
        //P2WPKHのwitness itemは2
        return false;
    }

    const utl_buf_t *p_sig = &vin->witness[0];
    const utl_buf_t *p_pub = &vin->witness[1];

    if (p_pub->len != BTC_SZ_PUBKEY) {
        return false;
    }

    utl_buf_t script_code = UTL_BUF_INIT;
    if (!btc_script_code_p2wpkh(&script_code, p_pub->buf)) {
        return false;
    }

    bool ret;
    uint8_t txhash[BTC_SZ_HASH256];
    ret = btc_sw_sighash(txhash, pTx, Index, Value, &script_code);
    if (ret) {
        ret = btc_sig_verify(p_sig, txhash, p_pub->buf);
    }
    if (ret) {
        //pubKeyHashチェック
        uint8_t pkh[BTC_SZ_HASH_MAX];

        btc_util_hash160(pkh, p_pub->buf, BTC_SZ_PUBKEY);
        if (!mNativeSegwit) {
            btc_util_create_pkh2wpkh(pkh, pkh);
        }
        ret = (memcmp(pkh, pPubKeyHash, BTC_SZ_HASH160) == 0);
    }

    utl_buf_free(&script_code);

    return ret;
}


bool btc_sw_verify_p2wpkh_addr(const btc_tx_t *pTx, uint32_t Index, uint64_t Value, const char *pAddr)
{
    bool ret;
    uint8_t hash[BTC_SZ_HASH_MAX];

    int pref;
    ret = btc_keys_addr2hash(hash, &pref, pAddr);
    if (mNativeSegwit) {
        if (ret && (pref == BTC_PREF_P2WPKH)) {
            ret = btc_sw_verify_p2wpkh(pTx, Index, Value, hash);
        } else {
            ret = false;
        }
    } else {
        if (ret && (pref == BTC_PREF_P2SH)) {
            ret = btc_sw_verify_p2wpkh(pTx, Index, Value, hash);
        } else {
            ret = false;
        }
    }

    return ret;
}


bool btc_sw_verify_2of2(const btc_tx_t *pTx, uint32_t Index, const uint8_t *pTxHash, const utl_buf_t *pVout)
{
    if (pTx->vin[Index].wit_item_cnt != 4) {
        //2-of-2は4項目
        LOGD("items not 4.n");
        return false;
    }

    utl_buf_t *wits = pTx->vin[Index].witness;
    utl_buf_t *wit;

    //このvinはP2SHの予定
    //      1. 前のvoutのpubKeyHashが、redeemScriptから計算したpubKeyHashと一致するか確認
    //      2. 署名チェック
    //
    //  none
    //  <署名1>
    //  <署名2>
    //  redeemScript

    //none
    wit = &wits[0];
    if (wit->len != 0) {
        LOGD("top isnot none\n");
        return false;
    }

    //署名
    const utl_buf_t *sig1 = &wits[1];
    if ((sig1->len == 0) || (sig1->buf[sig1->len - 1] != SIGHASH_ALL)) {
        //SIGHASH_ALLではない
        LOGD("SIG1: not SIGHASH_ALL\n");
        return false;
    }
    const utl_buf_t *sig2 = &wits[2];
    if ((sig2->len == 0) || (sig2->buf[sig2->len - 1] != SIGHASH_ALL)) {
        //SIGHASH_ALLではない
        LOGD("SIG2: not SIGHASH_ALL\n");
        return false;
    }

    //witnessScript
    wit = &wits[3];
    if (wit->len != 71) {
        //2-of-2 witnessScriptのサイズではない
        LOGD("witScript: invalid length: %u\n", wit->len);
        return false;
    }
    const uint8_t *p = wit->buf;
    if ( (*p != OP_2) || (*(p + 1) != BTC_SZ_PUBKEY) || (*(p + 35) != BTC_SZ_PUBKEY) ||
         (*(p + 69) != OP_2) || (*(p + 70) != OP_CHECKMULTISIG) ) {
        //2-of-2のredeemScriptではない
        LOGD("witScript: invalid script\n");
        LOGD("1: %d\n", (*p != OP_2));
        LOGD("2: %d\n", (*(p + 1) != BTC_SZ_PUBKEY));
        LOGD("3: %d\n", (*(p + 35) != BTC_SZ_PUBKEY));
        LOGD("4: %d\n", (*(p + 69) != OP_2));
        LOGD("5: %d\n", (*(p + 70) != OP_CHECKMULTISIG));
        return false;
    }
    const uint8_t *pub1 = p + 2;
    const uint8_t *pub2 = p + 36;

    //pubkeyhashチェック
    //  native segwit
    //      00 [len] [pubkeyHash/scriptHash]
    if (pVout->buf[0] != 0x00) {
        LOGD("invalid previous vout(not native segwit)\n");
        return false;
    }
    if (pVout->buf[1] == BTC_SZ_HASH256) {
        //native P2WSH
        uint8_t pkh[BTC_SZ_HASH256];
        btc_util_sha256(pkh, wit->buf, wit->len);
        bool ret = (memcmp(pkh, &pVout->buf[2], BTC_SZ_HASH256) == 0);
        if (!ret) {
            LOGD("pubkeyhash mismatch.\n");
            return false;
        }
    } else {
        LOGD("invalid previous vout length(not P2WSH)\n");
        return false;
    }

    //署名チェック
    //      2-of-2なので、順番通りに全一致
#if 1
    bool ret = btc_sig_verify(sig1, pTxHash, pub1);
    if (ret) {
        ret = btc_sig_verify(sig2, pTxHash, pub2);
        if (!ret) {
            LOGD("fail: btc_sig_verify(sig2)\n");
        }
    } else {
        LOGD("fail: btc_sig_verify(sig1)\n");
    }
#else
    bool ret1 = btc_sig_verify(sig1, pTxHash, pub1);
    bool ret2 = btc_sig_verify(sig2, pTxHash, pub2);
    bool ret3 = btc_sig_verify(sig1, pTxHash, pub2);
    bool ret4 = btc_sig_verify(sig2, pTxHash, pub1);
    bool ret = ret1 && ret2;
    printf("txhash=");
    DUMPD(pTxHash, BTC_SZ_HASH256);
    printf("ret1=%d\n", ret1);
    printf("ret2=%d\n", ret2);
    printf("ret3=%d\n", ret3);
    printf("ret4=%d\n", ret4);
#endif

    return ret;
}


#if 0   //今のところ使い道がない
/** WTXID計算
 *
 * @param[out]  pWTxId      計算結果(Little Endian)
 * @param[in]   pTx         対象トランザクション
 *
 * @note
 *      - pWTxIdにはLittleEndianで出力される
 */
bool btc_sw_wtxid(uint8_t *pWTxId, const btc_tx_t *pTx)
{
    utl_buf_t txbuf;

    if (!btc_sw_is_segwit(pTx)) {
        assert(0);
        return false;
    }

    bool ret = btcl_util_create_tx(&txbuf, pTx, true);
    if (!ret) {
        assert(0);
        goto LABEL_EXIT;
    }
    btc_util_hash256(pWTxId, txbuf.buf, txbuf.len);
    utl_buf_free(&txbuf);

LABEL_EXIT:
    return ret;
}


bool btc_sw_is_segwit(const btc_tx_t *pTx)
{
    bool ret = false;

    for (int lp = 0; lp < pTx->vin_cnt; lp++) {
        if (pTx->vin[lp].wit_item_cnt > 0) {
            ret = true;
            break;
        }
    }

    return ret;
}
#endif


void btc_sw_wit2prog_p2wsh(uint8_t *pWitProg, const utl_buf_t *pWitScript)
{
    pWitProg[0] = 0x00;
    pWitProg[1] = BTC_SZ_HASH256;
    btc_util_sha256(pWitProg + 2, pWitScript->buf, pWitScript->len);
}
