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
/** @file   cmd_json.c
 *  @brief  ptarmd JSON-RPC process
 */
//#include <stdio.h>
#include <stddef.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdbool.h>
//#include <stdlib.h>
//#include <string.h>
//#include <unistd.h>
//#include <linux/limits.h>
//#include <assert.h>
//
#include "jsonrpc-c.h"
#include "ln_segwit_addr.h"

#include "cmd_json.h"
#include "ln_db.h"
//#include "ln_db_lmdb.h"
//#include "btcrpc.h"
//
#include "p2p_svr.h"
#include "p2p_cli.h"
//#include "lnapp.h"
#include "monitoring.h"
#include "ptarmd.h"


/********************************************************************
 * macros
 ********************************************************************/

//#define M_SZ_JSONSTR            (8192)
#define M_SZ_PAYERR             (128)

#define M_RETRY_CONN_CHK        (10)        ///< 接続チェック[sec]


/********************************************************************
 * typedefs
 ********************************************************************/

typedef struct {
    bool b_local;
    const uint8_t *p_nodeid;
    cJSON *result;
} listtransactions_t;

typedef struct {
    ln_fieldr_t     **pp_field;
    uint8_t         *p_fieldnum;
} rfield_prm_t;


/********************************************************************
 * static variables
 ********************************************************************/

static struct jrpc_server   mJrpc;
static char                 mLastPayErr[M_SZ_PAYERR];       //最後に送金エラーが発生した時刻
//static int                  mPayTryCount = 0;               //送金トライ回数
//
//static const char *kOK = "OK";


/********************************************************************
 * prototypes
 ********************************************************************/

static cJSON *cmd_stop(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_getinfo(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_setfeerate(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_debug(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_addinvoice(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_removeinvoice(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_removeallinvoices(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_listinvoices(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_decodeinvoice(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_connectpeer(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_disconnectpeer(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_getlasterror(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_disableautoconnect(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_listtransactions(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_openchannel(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_closechannel(jrpc_context *ctx, cJSON *params, cJSON *id);
static cJSON *cmd_removechannel(jrpc_context *ctx, cJSON *params, cJSON *id);

//static cJSON *cmd_pay(jrpc_context *ctx, cJSON *params, cJSON *id);
//static cJSON *cmd_routepay_first(jrpc_context *ctx, cJSON *params, cJSON *id);
//static cJSON *cmd_routepay(jrpc_context *ctx, cJSON *params, cJSON *id);
//
//static int cmd_connect_proc(const peer_conn_t *pConn, jrpc_context *ctx);
//static int cmd_disconnect_proc(const uint8_t *pNodeId);
//static int cmd_stop_proc(void);
//static int cmd_invoice_proc(uint8_t *pPayHash, uint64_t AmountMsat);
//static int cmd_eraseinvoice_proc(const uint8_t *pPayHash);
//static int cmd_routepay_proc1(
//                ln_invoice_t **ppInvoiceData,
//                ln_routing_result_t *pRouteResult,
//                const char *pInvoice, uint64_t AddAmountMsat);
//static int cmd_routepay_proc2(
//                const ln_invoice_t *pInvoiceData,
//                const ln_routing_result_t *pRouteResult,
//                const char *pInvoiceStr, uint64_t AddAmountMsat);
static int closechannel_mutual(const uint8_t *pNodeId);
static int closechannel_unilateral(const uint8_t *pNodeId);
//
//static bool json_connect(cJSON *params, int *pIndex, peer_conn_t *pConn);
static char *create_bolt11(const uint8_t *pPayHash, uint64_t Amount, uint32_t Expiry, const ln_fieldr_t *pFieldR, uint8_t FieldRNum, uint32_t MinFinalCltvExpiry);
static void create_bolt11_rfield(ln_fieldr_t **ppFieldR, uint8_t *pFieldRNum);
static bool comp_func_cnl(ln_self_t *self, void *p_db_param, void *p_param);
static lnapp_conf_t *search_connected_lnapp_node(const uint8_t *p_node_id);
//static int send_json(const char *pSend, const char *pAddr, uint16_t Port);


/********************************************************************
 * public functions
 ********************************************************************/

void cmd_json_start(uint16_t Port)
{
    jrpc_server_init(&mJrpc, Port);

    jrpc_register_procedure(&mJrpc, cmd_stop,               "stop",  NULL);
    jrpc_register_procedure(&mJrpc, cmd_getinfo,            "getinfo",  NULL);
    jrpc_register_procedure(&mJrpc, cmd_setfeerate,         "setfeerate", NULL);
    jrpc_register_procedure(&mJrpc, cmd_debug,              "dev-debug", NULL);
    jrpc_register_procedure(&mJrpc, cmd_addinvoice,         "addinvoice", NULL);
    jrpc_register_procedure(&mJrpc, cmd_removeinvoice,      "removeinvoice", NULL);
    jrpc_register_procedure(&mJrpc, cmd_removeallinvoices,  "removeallinvoices", NULL);
    jrpc_register_procedure(&mJrpc, cmd_listinvoices,       "listinvoices", NULL);
    jrpc_register_procedure(&mJrpc, cmd_decodeinvoice,      "decodeinvoice", NULL);
    jrpc_register_procedure(&mJrpc, cmd_connectpeer,        "connectpeer", NULL);
    jrpc_register_procedure(&mJrpc, cmd_disconnectpeer,     "disconnectpeer", NULL);
    jrpc_register_procedure(&mJrpc, cmd_getlasterror,       "getlasterror", NULL);
    jrpc_register_procedure(&mJrpc, cmd_disableautoconnect, "dev-disableautoconnect", NULL);
    jrpc_register_procedure(&mJrpc, cmd_listtransactions,   "dev-listtransactions", NULL);
    jrpc_register_procedure(&mJrpc, cmd_openchannel,        "openchannel", NULL);
    jrpc_register_procedure(&mJrpc, cmd_closechannel,       "closechannel", NULL);
    jrpc_register_procedure(&mJrpc, cmd_removechannel,      "dev-removechannel", NULL);
    //TODO
//    jrpc_register_procedure(&mJrpc, cmd_eraseinvoice,"eraseinvoice", NULL);
//    jrpc_register_procedure(&mJrpc, cmd_pay,         "PAY", NULL);
//    jrpc_register_procedure(&mJrpc, cmd_routepay_first, "routepay", NULL);
//    jrpc_register_procedure(&mJrpc, cmd_routepay,    "routepay_cont", NULL);

    jrpc_server_run(&mJrpc);
    jrpc_server_destroy(&mJrpc);
}

void cmd_json_stop(void)
{
    if (mJrpc.port_number != 0) {
        jrpc_server_stop(&mJrpc);
    }
}

int cmd_json_connect(const uint8_t *pNodeId, const char *pIpAddr, uint16_t Port)
{
    //TODO dummy
    (void)pNodeId; (void)pIpAddr; (void)Port;
    return -1;
}

//int cmd_json_connect(const uint8_t *pNodeId, const char *pIpAddr, uint16_t Port)
//{
//    char nodestr[BTC_SZ_PUBKEY * 2 + 1];
//    char json[256];
//
//    utl_misc_bin2str(nodestr, pNodeId, BTC_SZ_PUBKEY);
//    sprintf(json, "{\"method\":\"connect\",\"params\":[\"%s\",\"%s\",%d]}",
//                        nodestr, pIpAddr, Port);
//
//    int retval = send_json(json, "127.0.0.1", mJrpc.port_number);
//    LOGD("retval=%d\n", retval);
//
//    return retval;
//}
//
//
//int cmd_json_pay(const char *pInvoice, uint64_t AddAmountMsat)
//{
//    LOGD("invoice:%s\n", pInvoice);
//    char *json = (char *)UTL_DBG_MALLOC(M_SZ_JSONSTR);      //UTL_DBG_FREE: この中
//    snprintf(json, M_SZ_JSONSTR,
//        "{\"method\":\"routepay_cont\",\"params\":[\"%s\",%" PRIu64 "]}", pInvoice, AddAmountMsat);
//    int retval = send_json(json, "127.0.0.1", mJrpc.port_number);
//    LOGD("retval=%d\n", retval);
//    UTL_DBG_FREE(json);     //UTL_DBG_MALLOC: この中
//
//    return retval;
//}
//
//
//int cmd_json_pay_retry(const uint8_t *pPayHash)
//{
//    bool ret;
//    int retval = ENOENT;
//    char *p_invoice = NULL;
//    uint64_t add_amount_msat;
//
//    ret = ln_db_invoice_load(&p_invoice, &add_amount_msat, pPayHash);   //p_invoiceはmalloc()される
//    if (ret) {
//        retval = cmd_json_pay(p_invoice, add_amount_msat);
//    } else {
//        LOGD("fail: invoice not found\n");
//    }
//    free(p_invoice);
//
//    return retval;
//}

int cmd_json_pay_retry(const uint8_t *pPayHash) {
    //TODO dummy
    (void)pPayHash;
    return ENOENT;
}


/********************************************************************
 * private functions : JSON-RPC
 ********************************************************************/

#define json_start(ctx, params, id) (void)(ctx); (void)(params); (void)(id);

static cJSON *json_end(jrpc_context *ctx, int err, cJSON *res) {
    if (err == 0) {
        return res ? res : cJSON_CreateString("OK");
    } else {
        ctx->error_code = err;
        ctx->error_message = ptarmd_error_str(err); //XXX
        return NULL;
    }
}

bool get_string(cJSON *params, int item, const char **s)
{
    cJSON *json = cJSON_GetArrayItem(params, item);
    if (!json) return false;
    if (json->type != cJSON_String) return false;
    *s = json->valuestring;
    return true;
}

bool get_bool(cJSON *params, int item, bool *b)
{
    cJSON *json = cJSON_GetArrayItem(params, item);
    if (!json) return false;
    if (json->type == cJSON_True) {
        *b = true;
    } else if (json->type == cJSON_False) {
        *b = false;
    } else {
        return false;
    }
    return true;
}

bool get_u16(cJSON *params, int item, uint16_t *u16)
{
    cJSON *json = cJSON_GetArrayItem(params, item);
    if (!json) return false;
    if (json->type != cJSON_Number) return false;
    if (json->valueu64 > UINT16_MAX) return false;
    *u16 = (uint16_t)json->valueu64;
    return true;
}

bool get_u32(cJSON *params, int item, uint32_t *u32)
{
    cJSON *json = cJSON_GetArrayItem(params, item);
    if (!json) return false;
    if (json->type != cJSON_Number) return false;
    if (json->valueu64 > UINT32_MAX) return false;
    *u32 = (uint32_t)json->valueu64;
    return true;
}

bool get_u64(cJSON *params, int item, uint64_t *u64)
{
    cJSON *json = cJSON_GetArrayItem(params, item);
    if (!json) return false;
    if (json->type != cJSON_Number) return false;
    *u64 = json->valueu64;
    return true;
}

bool is_end_of_params(cJSON *params, int item)
{
    if (cJSON_GetArrayItem(params, item)) return false;
    return true;
}

static bool proc_stop(cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    monitor_disable_autoconn(true);
    LOGD("stop\n");
    ptarmd_stop();
    jrpc_server_stop(&mJrpc);
    return true;
}

static cJSON *cmd_stop(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_stop(&res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_getinfo(cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    cJSON *result = cJSON_CreateObject();
    cJSON *result_peer = cJSON_CreateArray();

    uint64_t amount = ln_node_total_msat();

    //basic info
    char node_id[BTC_SZ_PUBKEY * 2 + 1];
    utl_misc_bin2str(node_id, ln_node_getid(), BTC_SZ_PUBKEY);
    cJSON_AddItemToObject(result, "node_id", cJSON_CreateString(node_id));
    cJSON_AddItemToObject(result, "node_port", cJSON_CreateNumber(ln_node_addr()->port));
    cJSON_AddNumber64ToObject(result, "total_our_msat", amount);

#ifdef DEVELOPER_MODE
    //blockcount
    int32_t blockcnt = btcrpc_getblockcount();
    if (blockcnt < 0) {
        LOGD("fail btcrpc_getblockcount()\n");
    } else {
        cJSON_AddItemToObject(result, "block_count", cJSON_CreateNumber(blockcnt));
    }
#endif

    //peer info
    p2p_svr_show_self(result_peer);
    p2p_cli_show_self(result_peer);
    cJSON_AddItemToObject(result, "peers", result_peer);

    //payment info
    uint8_t *p_hash;
    int cnt = ln_db_invoice_get(&p_hash);
    if (cnt > 0) {
        cJSON *result_hash = cJSON_CreateArray();
        uint8_t *p = p_hash;
        for (int lp = 0; lp < cnt; lp++) {
            char hash_str[LN_SZ_HASH * 2 + 1];
            utl_misc_bin2str(hash_str, p, LN_SZ_HASH);
            p += LN_SZ_HASH;
            cJSON_AddItemToArray(result_hash, cJSON_CreateString(hash_str));
        }
        free(p_hash);       //ln_lmdbでmalloc/realloc()している
        cJSON_AddItemToObject(result, "paying_hash", result_hash);
    }
    cJSON_AddItemToObject(result, "last_errpay_date", cJSON_CreateString(mLastPayErr));

    *res = result;
    return true;
}

static cJSON *cmd_getinfo(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_getinfo(&res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_setfeerate(uint32_t feerate_per_kw, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("setfeerate\n");
    LOGD("feerate_per_kw=%" PRIu32 "\n", feerate_per_kw);
    monitor_set_feerate_per_kw(feerate_per_kw);
    return true;
}

static cJSON *cmd_setfeerate(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    uint32_t feerate_per_kw;

    if (!get_u32(params, index++, &feerate_per_kw)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_setfeerate(feerate_per_kw, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_debug(uint32_t mask, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    cJSON *result = cJSON_CreateObject();

    char str[10];
    sprintf(str, "%08x", ln_get_debug());
    cJSON_AddItemToObject(result, "old", cJSON_CreateString(str));

    uint32_t dbg = ln_get_debug() ^ mask;
    ln_set_debug(dbg);
    sprintf(str, "%08x", dbg);
    if (!LN_DBG_FULFILL()) {
        LOGD("no fulfill return\n");
    }
    if (!LN_DBG_CLOSING_TX()) {
        LOGD("no closing tx\n");
    }
    if (!LN_DBG_MATCH_PREIMAGE()) {
        LOGD("force preimage mismatch\n");
    }
    if (!LN_DBG_NODE_AUTO_CONNECT()) {
        LOGD("no node Auto connect\n");
    }
    if (!LN_DBG_ONION_CREATE_NORMAL_REALM()) {
        LOGD("create invalid realm onion\n");
    }
    if (!LN_DBG_ONION_CREATE_NORMAL_VERSION()) {
        LOGD("create invalid version onion\n");
    }
    cJSON_AddItemToObject(result, "new", cJSON_CreateString(str));

    *res = result;
    return true;
}

static cJSON *cmd_debug(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    uint32_t mask;

    if (!get_u32(params, index++, &mask)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_debug(mask, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

void create_preimage(uint8_t *pPayHash, uint64_t AmountMsat)
{
    ln_db_preimg_t preimg;

    btc_util_random(preimg.preimage, LN_SZ_PREIMAGE);

    ptarmd_preimage_lock();
    preimg.amount_msat = AmountMsat;
    preimg.expiry = LN_INVOICE_EXPIRY;
    preimg.creation_time = 0;
    ln_db_preimg_save(&preimg, NULL);
    ptarmd_preimage_unlock();

    ln_calc_preimage_hash(pPayHash, preimg.preimage);
}

static bool proc_addinvoice(uint64_t amount, uint32_t min_final_cltv_expiry, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("addinvoice\n");
    LOGD("amount=%" PRIu64 "\n", amount);
    LOGD("min_final_cltv_expiry=%" PRIu32 "\n", min_final_cltv_expiry);

    uint8_t preimage_hash[LN_SZ_HASH];
    create_preimage(preimage_hash, amount);

    ln_fieldr_t *p_rfield = NULL;
    uint8_t rfieldnum = 0;
    create_bolt11_rfield(&p_rfield, &rfieldnum);

    char *p_invoice = create_bolt11(preimage_hash, amount,
                        LN_INVOICE_EXPIRY, p_rfield, rfieldnum,
                        min_final_cltv_expiry);
    if (!p_invoice) {
        LOGD("fail: BOLT11 format\n");
        *err = RPCERR_PARSE;
        UTL_DBG_FREE(p_rfield);
        return false;
    }

    char str_hash[LN_SZ_HASH * 2 + 1];
    utl_misc_bin2str(str_hash, preimage_hash, LN_SZ_HASH);
    *res = cJSON_CreateObject();
    cJSON_AddItemToObject(*res, "payment_hash", cJSON_CreateString(str_hash));
    cJSON_AddItemToObject(*res, "amount", cJSON_CreateNumber64(amount));
    cJSON_AddItemToObject(*res, "min_final_cltv_expiry", cJSON_CreateNumber64(min_final_cltv_expiry));
    cJSON_AddItemToObject(*res, "bolt11", cJSON_CreateString(p_invoice));

    free(p_invoice);
    UTL_DBG_FREE(p_rfield);
    return true;
}

static cJSON *cmd_addinvoice(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    uint64_t amount;
    uint32_t min_final_cltv_expiry;

    if (!get_u64(params, index++, &amount)) goto LABEL_EXIT;
    if (is_end_of_params(params, index)) {
        min_final_cltv_expiry = LN_MIN_FINAL_CLTV_EXPIRY;
    } else {
        if (!get_u32(params, index++, &min_final_cltv_expiry)) goto LABEL_EXIT;
    }
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_addinvoice(amount, min_final_cltv_expiry, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_removeinvoice(const char *preimage_hash, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("erase hash: %s\n", preimage_hash);

    uint8_t hash[LN_SZ_HASH];
    utl_misc_str2bin(hash, sizeof(hash), preimage_hash);
    if (!ln_db_preimg_del_hash(hash)) {
        *err = RPCERR_INVOICE_ERASE;
        return false;
    }
    return true;
}

static cJSON *cmd_removeinvoice(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *preimage_hash;

    if (!get_string(params, index++, &preimage_hash)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_removeinvoice(preimage_hash, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_removeallinvoices(cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    if (!ln_db_preimg_del(NULL)) {
        *err = RPCERR_INVOICE_ERASE;
        return false;
    }
    return true;
}

static cJSON *cmd_removeallinvoices(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_removeallinvoices(&res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_listinvoices(cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    uint8_t preimage_hash[LN_SZ_HASH];
    ln_db_preimg_t preimg;
    void *p_cur;
    bool ret;

    *res = cJSON_CreateArray();
    ret = ln_db_preimg_cur_open(&p_cur);
    while (ret) {
        bool detect;
        ret = ln_db_preimg_cur_get(p_cur, &detect, &preimg);
        if (detect) {
            ln_calc_preimage_hash(preimage_hash, preimg.preimage);
            cJSON *json = cJSON_CreateObject();

            char str_hash[LN_SZ_HASH * 2 + 1];
            utl_misc_bin2str(str_hash, preimage_hash, LN_SZ_HASH);
            cJSON_AddItemToObject(json, "payment_hash", cJSON_CreateString(str_hash));
            cJSON_AddItemToObject(json, "amount_msat", cJSON_CreateNumber64(preimg.amount_msat));
            char dtstr[UTL_SZ_DTSTR];
            utl_misc_strftime(dtstr, preimg.creation_time);
            cJSON_AddItemToObject(json, "creation_time", cJSON_CreateString(dtstr));
            if (preimg.expiry != UINT32_MAX) {
                cJSON_AddItemToObject(json, "expiry", cJSON_CreateNumber(preimg.expiry));
                // ln_fieldr_t *p_rfield = NULL;
                // uint8_t rfieldnum = 0;
                // create_bolt11_rfield(&p_rfield, &rfieldnum);
                // char *p_invoice = create_bolt11(preimage_hash, preimg.amount_msat, preimg.expiry, p_rfield, rfieldnum, LN_MIN_FINAL_CLTV_EXPIRY);
                // if (p_invoice != NULL) {
                //     cJSON_AddItemToObject(json, "invoice", cJSON_CreateString(p_invoice));
                //     free(p_invoice);
                //     APP_FREE(p_rfield);
                // }
            } else {
                cJSON_AddItemToObject(json, "expiry", cJSON_CreateString("remove after close"));
            }
            cJSON_AddItemToArray(*res, json);
        }
    }
    ln_db_preimg_cur_close(p_cur);
    return true;
}

static cJSON *cmd_listinvoices(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_listinvoices(&res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_decodeinvoice(const char *bolt11, cJSON **res, int *err)
{
    /* test data
    $ ./ptarmcli decodeinvoice lnbcrt1234560p1pd6r3wvnp4qwwxvye6sn9swafnzmwdzs3av4463y9nc56agdnanxrp5s08wdjvypp5zmkds8hcn4uuxm58s0uc749xt07wljdg3nxapmlx7qxtyuq28pusdq0wp6xzundd9nkzmsr9yqdzf60m9qpxn7x3ufanwrzq6we8pg0fxgc9th50d8d6eej57pvp45qqpkyqqqpqqqqqqqqq2qqqqqeqqysp082eh2xv3qh8lplxzmp2apwrekre6rz2svk0duslyefqu8tuqnjgqqxcsqqqxqqqqqqqqpgqqqqryqqjqu48seumdx9wftvvkafnu95uq6fns96usq7z22eaqnptlw3pr225zrhs974rxxhg52gsp9xgzdvt8w539svau75wszwkth766u82lvusp3zafd4 | jq
    {
      "result": {
        "currency": "bitcoin regtest",
        "amount": 123456,
        "timestamp": 1537328588,
        "min_final_cltv_expiry": 9,
        "pubkey": "039c66133a84cb07753316dcd1423d656ba890b3c535d4367d99861a41e77364c2",
        "payment_hash": "16ecd81ef89d79c36e8783f98f54a65bfcefc9a88ccdd0efe6f00cb2700a3879",
        "extra_routing_information": [
          {
            "pubkey": "03449d3f65004d3f1a3c4f66e1881a764e143d26460abbd1ed3b759cca9e0b035a",
            "short_channel_id,": "0001b10000040000",
            "fee_base_msat": 10,
            "fee_proportional_millionths": 100,
            "cltv_expiry_delta": 36
          },
          {
            "pubkey": "02f3ab375199105cff0fcc2d855d0b879b0f3a18950659ede43e4ca41c3af809c9",
            "short_channel_id,": "0001b10000060000",
            "fee_base_msat": 10,
            "fee_proportional_millionths": 100,
            "cltv_expiry_delta": 36
          }
        ]
      }
    }
    */

    *res = NULL;
    *err = 0;

    ln_invoice_t *p_invoice_data = NULL;
    if (!ln_invoice_decode(&p_invoice_data, bolt11)) {
        LOGD("fail decode invoice");
        *err = RPCERR_PARSE;
        return false;
    }

    *res = cJSON_CreateObject();

    switch (p_invoice_data->hrp_type) {
    case LN_INVOICE_MAINNET:
        cJSON_AddItemToObject(*res, "currency", cJSON_CreateString("bitcoin mainnet"));
        break;
    case LN_INVOICE_TESTNET:
        cJSON_AddItemToObject(*res, "currency", cJSON_CreateString("bitcoin testnet"));
        break;
    case LN_INVOICE_REGTEST:
        cJSON_AddItemToObject(*res, "currency", cJSON_CreateString("bitcoin regtest"));
        break;
    default:
        cJSON_AddItemToObject(*res, "currency", cJSON_CreateString("unknown"));
    }

    cJSON_AddItemToObject(*res, "amount", cJSON_CreateNumber64(p_invoice_data->amount_msat));
    cJSON_AddItemToObject(*res, "timestamp", cJSON_CreateNumber64(p_invoice_data->timestamp));
    cJSON_AddItemToObject(*res, "min_final_cltv_expiry", cJSON_CreateNumber64(p_invoice_data->min_final_cltv_expiry));

    char pubkey[BTC_SZ_PUBKEY * 2 + 1];
    utl_misc_bin2str(pubkey, p_invoice_data->pubkey, BTC_SZ_PUBKEY);
    cJSON_AddItemToObject(*res, "pubkey", cJSON_CreateString(pubkey));

    char payment_hash[BTC_SZ_SHA256 * 2 + 1];
    utl_misc_bin2str(payment_hash, p_invoice_data->payment_hash, BTC_SZ_SHA256);
    cJSON_AddItemToObject(*res, "payment_hash", cJSON_CreateString(payment_hash));

    cJSON *extra_routing_information = NULL;
    for (int lp = 0; lp < p_invoice_data->r_field_num; lp++) {
        if (!extra_routing_information) {
            extra_routing_information = cJSON_CreateArray();
            cJSON_AddItemToObject(*res, "extra_routing_information", extra_routing_information);
        }
        cJSON *item = cJSON_CreateObject();
        cJSON_AddItemToArray(extra_routing_information, item);
        utl_misc_bin2str(pubkey, p_invoice_data->r_field[lp].node_id, BTC_SZ_PUBKEY);
        cJSON_AddItemToObject(item, "pubkey", cJSON_CreateString(pubkey));

        char short_channel_id[LN_SZ_SHORT_CHANNEL_ID * 2 + 1];
        sprintf(short_channel_id, "%016" PRIx64, p_invoice_data->r_field[lp].short_channel_id);
        cJSON_AddItemToObject(item, "short_channel_id,", cJSON_CreateString(short_channel_id));

        cJSON_AddItemToObject(item, "fee_base_msat",
            cJSON_CreateNumber(p_invoice_data->r_field[lp].fee_base_msat));
        cJSON_AddItemToObject(item, "fee_proportional_millionths",
            cJSON_CreateNumber(p_invoice_data->r_field[lp].fee_prop_millionths));
        cJSON_AddItemToObject(item, "cltv_expiry_delta",
            cJSON_CreateNumber(p_invoice_data->r_field[lp].cltv_expiry_delta));
    }
    return true;
}

static cJSON *cmd_decodeinvoice(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *bolt11;

    if (!get_string(params, index++, &bolt11)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_decodeinvoice(bolt11, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool parse_peer_node_id(uint8_t node_id_bin[BTC_SZ_PUBKEY], const char *node_id_str)
{
    if (!utl_misc_str2bin(node_id_bin, BTC_SZ_PUBKEY, node_id_str)) {
        LOGD("fail: invalid node_id=%s\n", node_id_str);
        return false;
    }
    if (!memcmp(ln_node_getid(), node_id_bin, BTC_SZ_PUBKEY)) {
        LOGD("fail: same own node_id=%s\n", node_id_str);
        return false;
    }
    return true;
}

static bool proc_connectpeer(const char *peer_nodeid, const char *addr, uint16_t port, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("connect\n");
    LOGD("peer_nodeid=%s\n", peer_nodeid);
    LOGD("addr=%s\n", addr);
    LOGD("port=%u\n", port);

    peer_conn_t conn;

    //create conn
    if (!parse_peer_node_id(conn.node_id, peer_nodeid)) {
        *err = RPCERR_PARSE;
        return false;
    }
    if (strlen(addr) > SZ_IPV4_LEN) {
        *err = RPCERR_PARSE;
        return false;
    }
    strcpy(conn.ipaddr, addr);
    conn.port = port;

    //check aleady connected
    if (search_connected_lnapp_node(conn.node_id)) {
        *err = RPCERR_ALCONN;
        return false;
    }

    //initiate connection and wait for connecting
    if (!p2p_cli_start(&conn, err)) {
        if (!*err) {
            *err = RPCERR_ERROR;
        }
        return false;
    }
    int retry = M_RETRY_CONN_CHK;
    while (retry--) {
        lnapp_conf_t *p_appconf = search_connected_lnapp_node(conn.node_id);
        if ((p_appconf != NULL) && lnapp_is_looping(p_appconf) && lnapp_is_inited(p_appconf)) {
            break;
        }
        sleep(1);
    }
    if (retry < 0) {
        *err = RPCERR_CONNECT;
        return false;
    }
    return true;
}

static cJSON *cmd_connectpeer(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *peer_nodeid;
    const char *addr;
    uint16_t port;

    if (!get_string(params, index++, &peer_nodeid)) goto LABEL_EXIT;
    if (!get_string(params, index++, &addr)) goto LABEL_EXIT;
    if (!get_u16(params, index++, &port)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_connectpeer(peer_nodeid, addr, port, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_disconnectpeer(const char *peer_nodeid, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("disconnect\n");
    LOGD("peer_nodeid=%s\n", peer_nodeid);

    //node_id
    uint8_t node_id[BTC_SZ_PUBKEY];
    if (!parse_peer_node_id(node_id, peer_nodeid)) {
        *err = RPCERR_PARSE;
        return false;
    }

    //disconnect
    lnapp_conf_t *p_appconf = search_connected_lnapp_node(node_id);
    if (!p_appconf) {
        *err = RPCERR_NOCONN;
        return false;
    }
    lnapp_stop(p_appconf);
    return true;
}

static cJSON *cmd_disconnectpeer(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *peer_nodeid;

    if (!get_string(params, index++, &peer_nodeid)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_disconnectpeer(peer_nodeid, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_getlasterror(const char *peer_nodeid, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("getlasterror\n");
    LOGD("peer_nodeid=%s\n", peer_nodeid);

    //node_id
    uint8_t node_id[BTC_SZ_PUBKEY];
    if (!parse_peer_node_id(node_id, peer_nodeid)) {
        *err = RPCERR_PARSE;
        return false;
    }

    //getlasterror
    lnapp_conf_t *p_appconf = search_connected_lnapp_node(node_id);
    if (!p_appconf) {
        *err = RPCERR_NOCONN;
        return false;
    }
    LOGD("error code: %d\n", p_appconf->err);
    *err = p_appconf->err;
    return *err ? false : true;
}

static cJSON *cmd_getlasterror(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *peer_nodeid;

    if (!get_string(params, index++, &peer_nodeid)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_getlasterror(peer_nodeid, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_disableautoconnect(bool disable, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    monitor_disable_autoconn(disable);
    if (disable) {
        *res = cJSON_CreateString("disable auto connect");
    } else {
        *res = cJSON_CreateString("enable auto connect");
    }
    return true;
}

static cJSON *cmd_disableautoconnect(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    bool disable;

    if (!get_bool(params, index++, &disable)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_disableautoconnect(disable, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool comp_func_listtransactions(ln_self_t *self, void *p_db_param, void *p_param)
{
    (void)p_db_param;

    listtransactions_t *param = (listtransactions_t *)p_param;
    if (!memcmp(param->p_nodeid, ln_their_node_id(self), BTC_SZ_PUBKEY)) {
        lnapp_conf_t appconf;
        appconf.p_self= self;
        lnapp_get_committx(&appconf, param->result, param->b_local);
    }
    return false;
}

static bool proc_listtransactions(const char *peer_nodeid, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("listtransactions\n");
    LOGD("peer_nodeid=%s\n", peer_nodeid);

    //node_id
    uint8_t node_id[BTC_SZ_PUBKEY];
    if (!parse_peer_node_id(node_id, peer_nodeid)) {
        *err = RPCERR_PARSE;
        return false;
    }

    //listtransactions
    listtransactions_t param;
    *res = cJSON_CreateObject();
    param.b_local = false;
    param.p_nodeid = node_id;
    param.result = *res;
    ln_db_self_search(comp_func_listtransactions, &param);
    return true;
}

static cJSON *cmd_listtransactions(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *peer_nodeid;

    if (!get_string(params, index++, &peer_nodeid)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_listtransactions(peer_nodeid, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_openchannel(const char *peer_nodeid, uint64_t funding_sat, uint64_t push_sat, uint32_t feerate_per_kw, const char *txid, uint32_t txindex, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("openchannel\n");
    LOGD("peer_nodeid=%s\n", peer_nodeid);
    LOGD("funding_sat=%" PRIu64 "\n", funding_sat);
    LOGD("push_sat=%" PRIu64 "\n", push_sat);
    LOGD("feerate_per_kw=%" PRIu32 "\n", feerate_per_kw);
    LOGD("txid=%s\n", txid);
    LOGD("txindex=%" PRIu32 "\n", txindex);

    //node_id
    uint8_t node_id[BTC_SZ_PUBKEY];
    if (!parse_peer_node_id(node_id, peer_nodeid)) {
        *err = RPCERR_PARSE;
        return false;
    }

    //fund
    funding_conf_t fund;
    fund.funding_sat = funding_sat;
    fund.push_sat = push_sat;
    fund.feerate_per_kw = feerate_per_kw;
    if (!utl_misc_str2bin_rev(fund.txid, BTC_SZ_TXID, txid)) {
        *err = RPCERR_PARSE;
        return false;
    }
    fund.txindex = txindex;

    lnapp_conf_t *p_appconf = search_connected_lnapp_node(node_id);
    if (p_appconf == NULL) {
        //not connected
        *err = RPCERR_NOCONN;
        return false;
    }

    if (ln_node_search_channel(NULL, node_id)) {
        //already opened
        *err = RPCERR_ALOPEN;
        return false;
    }

    if (ln_is_funding(p_appconf->p_self)) {
        //already opening
        *err = RPCERR_OPENING;
        return false;
    }

    if (!lnapp_is_inited(p_appconf)) {
        //BOLTメッセージとして初期化が完了していない(init/channel_reestablish交換できていない)
        *err = RPCERR_NOINIT;
        return false;
    }

    if (!lnapp_funding(p_appconf, &fund)) {
        *err = RPCERR_FUNDING;
        return false;
    }

    return true;
}

static cJSON *cmd_openchannel(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *peer_nodeid;
    uint64_t funding_sat;
    uint64_t push_sat;
    uint32_t feerate_per_kw;
    const char *txid;
    uint32_t txindex;

    if (!get_string(params, index++, &peer_nodeid)) goto LABEL_EXIT;
    if (!get_u64(params, index++, &funding_sat)) goto LABEL_EXIT;
    if (!get_u64(params, index++, &push_sat)) goto LABEL_EXIT;
    //XXX TODO optional params
    if (!get_u32(params, index++, &feerate_per_kw)) goto LABEL_EXIT;
    if (!get_string(params, index++, &txid)) goto LABEL_EXIT;
    if (!get_u32(params, index++, &txindex)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_openchannel(peer_nodeid, funding_sat, push_sat, feerate_per_kw, txid, txindex, &res, &err)) goto LABEL_EXIT;

    res = cJSON_CreateString("Progressing");

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_closechannel(const char *peer_nodeid, bool force, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("closechannel\n");
    LOGD("peer_nodeid=%s\n", peer_nodeid);
    LOGD("force=%s\n", force ? "true" : "false");

    //node_id
    uint8_t node_id[BTC_SZ_PUBKEY];
    if (!parse_peer_node_id(node_id, peer_nodeid)) {
        *err = RPCERR_PARSE;
        return false;
    }

    if (force) {
        *err = closechannel_unilateral(node_id);
    } else {
        *err = closechannel_mutual(node_id);
    }

    return true;
}

static cJSON *cmd_closechannel(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *peer_nodeid;
    bool force;

    if (!get_string(params, index++, &peer_nodeid)) goto LABEL_EXIT;
    if (is_end_of_params(params, index)) {
        force = false;
    } else {
        if (!get_bool(params, index++, &force)) goto LABEL_EXIT;
    }
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_closechannel(peer_nodeid, force, &res, &err)) goto LABEL_EXIT;

    if (force) {
        res = cJSON_CreateString("Start Unilateral Close");
    } else {
        res = cJSON_CreateString("Start Mutual Close");
    }

LABEL_EXIT:
    return json_end(ctx, err, res);
}

static bool proc_removechannel(const char *channelid, cJSON **res, int *err)
{
    *res = NULL;
    *err = 0;

    LOGD("removechannel\n");
    LOGD("channelid=%s\n", channelid);

    //channel_id
    uint8_t channel_id[LN_SZ_CHANNEL_ID];
    if (!utl_misc_str2bin(channel_id, LN_SZ_CHANNEL_ID, channelid)) {
        *err = RPCERR_PARSE;
        return false;
    }

    if (!ln_db_self_del(channel_id)) {
        *err = RPCERR_PARSE;
        return false;
    }

    return true;
}

static cJSON *cmd_removechannel(jrpc_context *ctx, cJSON *params, cJSON *id)
{
    json_start(ctx, params, id);

    int err = RPCERR_PARSE;
    cJSON *res = NULL;
    int index = 0;

    const char *channelid;

    if (!get_string(params, index++, &channelid)) goto LABEL_EXIT;
    if (!is_end_of_params(params, index++)) goto LABEL_EXIT;

    if (!proc_removechannel(channelid, &res, &err)) goto LABEL_EXIT;

LABEL_EXIT:
    return json_end(ctx, err, res);
}


/** 状態出力 : ptarmcli -l
 *
 */
//static cJSON *cmd_getinfo(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    (void)ctx; (void)params; (void)id;
//
//    cJSON *result = cJSON_CreateObject();
//    cJSON *result_peer = cJSON_CreateArray();
//
//    uint64_t amount = ln_node_total_msat();
//
//    //basic info
//    char node_id[BTC_SZ_PUBKEY * 2 + 1];
//    utl_misc_bin2str(node_id, ln_node_getid(), BTC_SZ_PUBKEY);
//    cJSON_AddItemToObject(result, "node_id", cJSON_CreateString(node_id));
//    cJSON_AddItemToObject(result, "node_port", cJSON_CreateNumber(ln_node_addr()->port));
//    cJSON_AddNumber64ToObject(result, "total_our_msat", amount);
//
//#ifdef DEVELOPER_MODE
//    //blockcount
//    int32_t blockcnt = btcrpc_getblockcount();
//    if (blockcnt < 0) {
//        LOGD("fail btcrpc_getblockcount()\n");
//    } else {
//        cJSON_AddItemToObject(result, "block_count", cJSON_CreateNumber(blockcnt));
//    }
//#endif
//
//    //peer info
//    p2p_svr_show_self(result_peer);
//    p2p_cli_show_self(result_peer);
//    cJSON_AddItemToObject(result, "peers", result_peer);
//
//    //payment info
//    uint8_t *p_hash;
//    int cnt = ln_db_invoice_get(&p_hash);
//    if (cnt > 0) {
//        cJSON *result_hash = cJSON_CreateArray();
//        uint8_t *p = p_hash;
//        for (int lp = 0; lp < cnt; lp++) {
//            char hash_str[LN_SZ_HASH * 2 + 1];
//            utl_misc_bin2str(hash_str, p, LN_SZ_HASH);
//            p += LN_SZ_HASH;
//            cJSON_AddItemToArray(result_hash, cJSON_CreateString(hash_str));
//        }
//        free(p_hash);       //ln_lmdbでmalloc/realloc()している
//        cJSON_AddItemToObject(result, "paying_hash", result_hash);
//    }
//    cJSON_AddItemToObject(result, "last_errpay_date", cJSON_CreateString(mLastPayErr));
//
//    return result;
//}


/** ノード終了 : ptarmcli -q
 *
 */
//static cJSON *cmd_stop(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    (void)ctx; (void)params; (void)id;
//
//    cJSON *result = NULL;
//
//    monitor_disable_autoconn(true);
//    int err = cmd_stop_proc();
//    if (err == 0) {
//        result = cJSON_CreateString("OK");
//    } else {
//        ctx->error_code = err;
//        ctx->error_message = ptarmd_error_str(err);
//    }
//    jrpc_server_stop(&mJrpc);
//
//    return result;
//}


/** 送金開始(テスト用) : "PAY"
 *
 */
//static cJSON *cmd_pay(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    (void)id;
//
//    cJSON *json;
//    payment_conf_t payconf;
//    cJSON *result = NULL;
//    int index = 0;
//
//    if (params == NULL) {
//        index = -1;
//        goto LABEL_EXIT;
//    }
//
//    //blockcount
//    int32_t blockcnt = btcrpc_getblockcount();
//    LOGD("blockcnt=%d\n", blockcnt);
//    if (blockcnt < 0) {
//        index = -1;
//        goto LABEL_EXIT;
//    }
//
//    //payment_hash, hop_num
//    json = cJSON_GetArrayItem(params, index++);
//    if (json && (json->type == cJSON_String)) {
//        utl_misc_str2bin(payconf.payment_hash, LN_SZ_HASH, json->valuestring);
//        LOGD("payment_hash=%s\n", json->valuestring);
//    } else {
//        index = -1;
//        goto LABEL_EXIT;
//    }
//    json = cJSON_GetArrayItem(params, index++);
//    if (json && (json->type == cJSON_Number)) {
//        payconf.hop_num = json->valueint;
//        LOGD("hop_num=%d\n", json->valueint);
//    } else {
//        index = -1;
//        goto LABEL_EXIT;
//    }
//    //array
//    json = cJSON_GetArrayItem(params, index++);
//    if (json && (json->type == cJSON_Array)) {
//        LOGD("trace array\n");
//    } else {
//        index = -1;
//        goto LABEL_EXIT;
//    }
//    //[ [...], [...], ..., [...] ]
//    for (int lp = 0; lp < payconf.hop_num; lp++) {
//        ln_hop_datain_t *p = &payconf.hop_datain[lp];
//
//        LOGD("loop=%d\n", lp);
//        cJSON *jarray = cJSON_GetArrayItem(json, lp);
//        if (jarray && (jarray->type == cJSON_Array)) {
//            //[node_id, short_channel_id, amt_to_forward, outgoing_cltv_value]
//
//            //node_id
//            cJSON *jprm = cJSON_GetArrayItem(jarray, 0);
//            LOGD("jprm=%p\n", jprm);
//            if (jprm && (jprm->type == cJSON_String)) {
//                utl_misc_str2bin(p->pubkey, BTC_SZ_PUBKEY, jprm->valuestring);
//                LOGD("  node_id=");
//                DUMPD(p->pubkey, BTC_SZ_PUBKEY);
//            } else {
//                LOGD("fail: p=%p\n", jprm);
//                index = -1;
//                goto LABEL_EXIT;
//            }
//            //short_channel_id
//            jprm = cJSON_GetArrayItem(jarray, 1);
//            if (jprm && (jprm->type == cJSON_String)) {
//                p->short_channel_id = strtoull(jprm->valuestring, NULL, 16);
//                LOGD("  short_channel_id=%016" PRIx64 "\n", p->short_channel_id);
//            } else {
//                LOGD("fail: p=%p\n", jprm);
//                index = -1;
//                goto LABEL_EXIT;
//            }
//            //amt_to_forward
//            jprm = cJSON_GetArrayItem(jarray, 2);
//            if (jprm && (jprm->type == cJSON_Number)) {
//                p->amt_to_forward = jprm->valueu64;
//                LOGD("  amt_to_forward=%" PRIu64 "\n", p->amt_to_forward);
//            } else {
//                LOGD("fail: p=%p\n", jprm);
//                index = -1;
//                goto LABEL_EXIT;
//            }
//            //outgoing_cltv_value
//            jprm = cJSON_GetArrayItem(jarray, 3);
//            if (jprm && (jprm->type == cJSON_Number)) {
//                p->outgoing_cltv_value = jprm->valueint + blockcnt;
//                LOGD("  outgoing_cltv_value=%u\n", p->outgoing_cltv_value);
//            } else {
//                LOGD("fail: p=%p\n", jprm);
//                index = -1;
//                goto LABEL_EXIT;
//            }
//        } else {
//            LOGD("fail: p=%p\n", jarray);
//            index = -1;
//            goto LABEL_EXIT;
//        }
//    }
//
//    LOGD("payment\n");
//
//    lnapp_conf_t *p_appconf = search_connected_lnapp_node(payconf.hop_datain[1].pubkey);
//    if (p_appconf != NULL) {
//
//        bool inited = lnapp_is_inited(p_appconf);
//        if (inited) {
//            bool ret;
//            ret = lnapp_payment(p_appconf, &payconf);
//            if (ret) {
//                result = cJSON_CreateString("Progressing");
//            } else {
//                ctx->error_code = RPCERR_PAY_STOP;
//                ctx->error_message = ptarmd_error_str(RPCERR_PAY_STOP);
//            }
//        } else {
//            //BOLTメッセージとして初期化が完了していない(init/channel_reestablish交換できていない)
//            ctx->error_code = RPCERR_NOINIT;
//            ctx->error_message = ptarmd_error_str(RPCERR_NOINIT);
//        }
//    } else {
//        ctx->error_code = RPCERR_NOCONN;
//        ctx->error_message = ptarmd_error_str(RPCERR_NOCONN);
//    }
//
//LABEL_EXIT:
//    if (index < 0) {
//        ctx->error_code = RPCERR_PARSE;
//        ctx->error_message = ptarmd_error_str(RPCERR_PARSE);
//    }
//    if (ctx->error_code != 0) {
//        ln_db_invoice_del(payconf.payment_hash);
//        //一時的なスキップは削除する
//        ln_db_routeskip_drop(true);
//    }
//
//    return result;
//}


/** 送金開始: ptarmcli -r
 *
 * 一時ルーティング除外リストをクリアしてから送金する
 */
//static cJSON *cmd_routepay_first(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    LOGD("routepay_first\n");
//    ln_db_routeskip_drop(true);
//    mPayTryCount = 0;
//    return cmd_routepay(ctx, params, id);
//}


/** 送金・再送金: ptarmcli -r / -R
 *
 */
//static cJSON *cmd_routepay(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    (void)id;
//
//    LOGD("routepay\n");
//
//    int err = RPCERR_PARSE;
//    cJSON *result = NULL;
//    bool retry = false;
//    cJSON *json;
//    int index = 0;
//
//    char *p_invoice = NULL;
//    uint64_t add_amount_msat = 0;
//
//    ln_invoice_t *p_invoice_data = NULL;
//    ln_routing_result_t rt_ret;
//
//    if (params == NULL) {
//        goto LABEL_EXIT;
//    }
//
//    json = cJSON_GetArrayItem(params, index++);
//    if (json && (json->type == cJSON_String)) {
//        p_invoice = strdup(json->valuestring);
//    } else {
//        LOGD("fail: invalid invoice string\n");
//        goto LABEL_EXIT;
//    }
//
//    json = cJSON_GetArrayItem(params, index++);
//    if (json && (json->type == cJSON_Number)) {
//        add_amount_msat = json->valueu64;
//    } else {
//        LOGD("fail: invalid add amount_msat\n");
//        goto LABEL_EXIT;
//    }
//
//    err = cmd_routepay_proc1(&p_invoice_data, &rt_ret,
//                    p_invoice, add_amount_msat);
//    if (err != 0) {
//        LOGD("fail: pay1\n");
//        goto LABEL_EXIT;
//    }
//
//    // 送金開始
//    //      ここまでで送金ルートは作成済み
//    //      これ以降は失敗してもリトライする
//    LOGD("routepay: pay1\n");
//    retry = true;
//
//    //再送のためにinvoice保存
//    err = cmd_routepay_proc2(p_invoice_data, &rt_ret, p_invoice, add_amount_msat);
//    if (err == RPCERR_PAY_RETRY) {
//        //送金
//        cmd_json_pay(p_invoice, add_amount_msat);
//        LOGD("retry: skip %016" PRIx64 "\n", rt_ret.hop_datain[0].short_channel_id);
//    }
//
//LABEL_EXIT:
//    if (err == 0) {
//        result = cJSON_CreateString("start payment");
//    } else if (!retry) {
//        //送金失敗
//        ln_db_invoice_del(p_invoice_data->payment_hash);
//
//        //最後に失敗した時間
//        char date[50];
//        utl_misc_datetime(date, sizeof(date));
//        char str_payhash[LN_SZ_HASH * 2 + 1];
//        utl_misc_bin2str(str_payhash, p_invoice_data->payment_hash, LN_SZ_HASH);
//
//        sprintf(mLastPayErr, "[%s]payment fail", date);
//        LOGD("%s\n", mLastPayErr);
//        lnapp_save_event(NULL, "payment fail: payment_hash=%s try=%d", str_payhash, mPayTryCount);
//
//        ctx->error_code = err;
//        ctx->error_message = ptarmd_error_str(err);
//    } else {
//        //already processed
//    }
//    free(p_invoice_data);
//    free(p_invoice);
//
//    return result;
//}


/** channel mutual close開始 : ptarmcli -x
 *
 */


/** 最後に発生したエラー出力 : ptarmcli -w
 *
 */
//static cJSON *cmd_getlasterror(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    (void)id;
//
//    peer_conn_t conn;
//    int index = 0;
//
//    //connect parameter
//    bool ret = json_connect(params, &index, &conn);
//    if (!ret) {
//        ctx->error_code = RPCERR_PARSE;
//        ctx->error_message = ptarmd_error_str(RPCERR_PARSE);
//        goto LABEL_EXIT;
//    }
//
//    LOGD("getlasterror\n");
//
//    lnapp_conf_t *p_appconf = search_connected_lnapp_node(conn.node_id);
//    if (p_appconf != NULL) {
//        //接続中
//        LOGD("error code: %d\n", p_appconf->err);
//        ctx->error_code = p_appconf->err;
//        if (p_appconf->p_errstr != NULL) {
//            LOGD("error msg: %s\n", p_appconf->p_errstr);
//            ctx->error_message = p_appconf->p_errstr;
//        }
//    } else {
//        ctx->error_code = RPCERR_NOCONN;
//        ctx->error_message = ptarmd_error_str(RPCERR_NOCONN);
//    }
//
//LABEL_EXIT:
//    return NULL;
//}


/** デバッグフラグのトグル : ptarmcli -d
 *
 */
//static cJSON *cmd_debug(jrpc_context *ctx, cJSON *params, cJSON *id)
//{
//    (void)ctx; (void)id;
//
//    cJSON *result = NULL;
//    char str[10];
//    cJSON *json;
//
//    if (params == NULL) {
//        ctx->error_code = RPCERR_PARSE;
//        ctx->error_message = ptarmd_error_str(RPCERR_PARSE);
//        goto LABEL_EXIT;
//    }
//
//    json = cJSON_GetArrayItem(params, 0);
//    if (json && (json->type == cJSON_Number)) {
//        result = cJSON_CreateObject();
//
//        sprintf(str, "%08lx", ln_get_debug());
//        cJSON_AddItemToObject(result, "old", cJSON_CreateString(str));
//
//        unsigned long dbg = ln_get_debug() ^ json->valueint;
//        ln_set_debug(dbg);
//        sprintf(str, "%08lx", dbg);
//        if (!LN_DBG_FULFILL()) {
//            LOGD("no fulfill return\n");
//        }
//        if (!LN_DBG_CLOSING_TX()) {
//            LOGD("no closing tx\n");
//        }
//        if (!LN_DBG_MATCH_PREIMAGE()) {
//            LOGD("force preimage mismatch\n");
//        }
//        if (!LN_DBG_NODE_AUTO_CONNECT()) {
//            LOGD("no node Auto connect\n");
//        }
//        if (!LN_DBG_ONION_CREATE_NORMAL_REALM()) {
//            LOGD("create invalid realm onion\n");
//        }
//        if (!LN_DBG_ONION_CREATE_NORMAL_VERSION()) {
//            LOGD("create invalid version onion\n");
//        }
//        cJSON_AddItemToObject(result, "new", cJSON_CreateString(str));
//    } else {
//        ctx->error_code = RPCERR_PARSE;
//        ctx->error_message = ptarmd_error_str(RPCERR_PARSE);
//    }
//
//LABEL_EXIT:
//    return result;
//}


/** チャネル情報削除 : ptarmcli -X
 *
 * DBから強制的にチャネル情報を削除する。
 */


/** feerate_per_kw手動設定 : ptarmcli --setfeerate
 *
 */


/********************************************************************
 * private functions : procedure
 ********************************************************************/

/** peer接続
 *
 * @param[in]       pConn
 * @param[in,out]   ctx
 * @retval  エラーコード
 */
//static int cmd_connect_proc(const peer_conn_t *pConn, jrpc_context *ctx)
//{
//    LOGD("connect\n");
//
//    lnapp_conf_t *p_appconf = search_connected_lnapp_node(pConn->node_id);
//    if (p_appconf != NULL) {
//        return RPCERR_ALCONN;
//    }
//
//    bool ret = p2p_cli_start(pConn, ctx);
//    if (!ret) {
//        return RPCERR_CONNECT;
//    }
//
//    //チェック
//    int retry = M_RETRY_CONN_CHK;
//    while (retry--) {
//        p_appconf = search_connected_lnapp_node(pConn->node_id);
//        if ((p_appconf != NULL) && lnapp_is_looping(p_appconf) && lnapp_is_inited(p_appconf)) {
//            break;
//        }
//        sleep(1);
//    }
//    if (retry < 0) {
//        return RPCERR_CONNECT;
//    }
//
//    return 0;
//}


/** peer切断
 *
 * @param[in]       pNodeId
 * @retval  エラーコード
 */
//static int cmd_disconnect_proc(const uint8_t *pNodeId)
//{
//    LOGD("disconnect\n");
//
//    int err;
//    lnapp_conf_t *p_appconf = search_connected_lnapp_node(pNodeId);
//    if (p_appconf != NULL) {
//        lnapp_stop(p_appconf);
//        err = 0;
//    } else {
//        err = RPCERR_NOCONN;
//    }
//
//    return err;
//}


/** node終了
 *
 * @retval  エラーコード
 */
//static int cmd_stop_proc(void)
//{
//    LOGD("stop\n");
//
//    ptarmd_stop();
//
//    return 0;
//}


/** channel establish開始
 *
 * @param[in]   pNodeId
 * @param[in]   pFund
 * @retval  エラーコード
 */


/** invoice作成
 *
 * @param[out]  pPayHash
 * @param[in]   AmountMsat
 * @retval  エラーコード
 */


/** invoice削除
 *
 * @param[in]   pPayHash
 * @retval  エラーコード
 */
//static int cmd_eraseinvoice_proc(const uint8_t *pPayHash)
//{
//    bool ret;
//
//    if (pPayHash != NULL) {
//        ret = ln_db_preimg_del_hash(pPayHash);
//    } else {
//        ret = ln_db_preimg_del(NULL);
//    }
//    if (!ret) {
//        return RPCERR_INVOICE_ERASE;
//    }
//    return 0;
//}


/** 送金開始1
 * 送金経路作成
 *
 * @param[out]      ppInvoiceData
 * @param[out]      pRouteResult
 * @param[in]       pInvoice
 * @param[in]       AddAmountMsat
 * @retval  エラーコード
 */
//static int cmd_routepay_proc1(
//                ln_invoice_t **ppInvoiceData,
//                ln_routing_result_t *pRouteResult,
//                const char *pInvoice, uint64_t AddAmountMsat)
//{
//    bool bret = ln_invoice_decode(ppInvoiceData, pInvoice);
//    if (!bret) {
//        return RPCERR_PARSE;
//    }
//
//    ln_invoice_t *p_invoice_data = *ppInvoiceData;
//    if ( (p_invoice_data->hrp_type != LN_INVOICE_TESTNET) &&
//        (p_invoice_data->hrp_type != LN_INVOICE_REGTEST) ) {
//        LOGD("fail: mismatch blockchain\n");
//        return RPCERR_INVOICE_FAIL;
//    }
//    time_t now = time(NULL);
//    if (p_invoice_data->timestamp + p_invoice_data->expiry < (uint64_t)now) {
//        LOGD("fail: invoice outdated\n");
//        return RPCERR_INVOICE_OUTDATE;
//    }
//    p_invoice_data->amount_msat += AddAmountMsat;
//
//    //blockcount
//    int32_t blockcnt = btcrpc_getblockcount();
//    LOGD("blockcnt=%d\n", blockcnt);
//    if (blockcnt < 0) {
//        return RPCERR_BLOCKCHAIN;
//    }
//
//    lnerr_route_t rerr = ln_routing_calculate(pRouteResult,
//                    ln_node_getid(),
//                    p_invoice_data->pubkey,
//                    blockcnt + p_invoice_data->min_final_cltv_expiry,
//                    p_invoice_data->amount_msat,
//                    p_invoice_data->r_field_num, p_invoice_data->r_field);
//    if (rerr != LNROUTE_NONE) {
//        LOGD("fail: routing\n");
//        switch (rerr) {
//        case LNROUTE_NOTFOUND:
//            return RPCERR_NOROUTE;
//        case LNROUTE_TOOMANYHOP:
//            return RPCERR_TOOMANYHOP;
//        default:
//            return RPCERR_PAYFAIL;
//        }
//    }
//
//    return 0;
//}


/** 送金開始2
 * 送金
 *
 * @param[in]       pInvoiceData
 * @param[in]       pRouteResult
 * @param[in]       pInvoiceStr
 * @param[in]       AddAmountMsat
 * @retval  エラーコード
 */
//static int cmd_routepay_proc2(
//                const ln_invoice_t *pInvoiceData,
//                const ln_routing_result_t *pRouteResult,
//                const char *pInvoiceStr, uint64_t AddAmountMsat)
//{
//    int err = RPCERR_PAY_RETRY;
//
//    //再送のためにinvoice保存
//    (void)ln_db_invoice_save(pInvoiceStr, AddAmountMsat, pInvoiceData->payment_hash);
//
//    LOGD("-----------------------------------\n");
//    for (int lp = 0; lp < pRouteResult->hop_num; lp++) {
//        LOGD("node_id[%d]: ", lp);
//        DUMPD(pRouteResult->hop_datain[lp].pubkey, BTC_SZ_PUBKEY);
//        LOGD("  amount_msat: %" PRIu64 "\n", pRouteResult->hop_datain[lp].amt_to_forward);
//        LOGD("  cltv_expiry: %" PRIu32 "\n", pRouteResult->hop_datain[lp].outgoing_cltv_value);
//        LOGD("  short_channel_id: %016" PRIx64 "\n", pRouteResult->hop_datain[lp].short_channel_id);
//    }
//    LOGD("-----------------------------------\n");
//
//    lnapp_conf_t *p_appconf = search_connected_lnapp_node(pRouteResult->hop_datain[1].pubkey);
//    if (p_appconf != NULL) {
//        bool inited = lnapp_is_inited(p_appconf);
//        if (inited) {
//            payment_conf_t payconf;
//
//            memcpy(payconf.payment_hash, pInvoiceData->payment_hash, LN_SZ_HASH);
//            payconf.hop_num = pRouteResult->hop_num;
//            memcpy(payconf.hop_datain, pRouteResult->hop_datain, sizeof(ln_hop_datain_t) * (1 + LN_HOP_MAX));
//
//            bool ret = lnapp_payment(p_appconf, &payconf);
//            if (ret) {
//                LOGD("start payment\n");
//                err = 0;
//            } else {
//                LOGD("fail: lnapp_payment\n");
//                ln_db_routeskip_save(pRouteResult->hop_datain[0].short_channel_id, true);
//            }
//        } else {
//            //BOLTメッセージとして初期化が完了していない(init/channel_reestablish交換できていない)
//            LOGD("fail: not inited\n");
//        }
//    } else {
//        LOGD("fail: not connect(%016" PRIx64 "): \n", pRouteResult->hop_datain[0].short_channel_id);
//        DUMPD(pRouteResult->hop_datain[1].pubkey, BTC_SZ_PUBKEY);
//        ln_db_routeskip_save(pRouteResult->hop_datain[0].short_channel_id, true);
//    }
//
//    mPayTryCount++;
//
//    if (mPayTryCount == 1) {
//        //初回ログ
//        uint64_t total_amount = ln_node_total_msat();
//        char str_payhash[LN_SZ_HASH * 2 + 1];
//        utl_misc_bin2str(str_payhash, pInvoiceData->payment_hash, LN_SZ_HASH);
//        char str_payee[BTC_SZ_PUBKEY * 2 + 1];
//        utl_misc_bin2str(str_payee, pInvoiceData->pubkey, BTC_SZ_PUBKEY);
//
//        lnapp_save_event(NULL, "payment: payment_hash=%s payee=%s total_msat=%" PRIu64" amount_msat=%" PRIu64,
//                    str_payhash, str_payee, total_amount, pInvoiceData->amount_msat);
//    }
//
//    return err;
//}


/** channel mutual close開始
 *
 * @param[in]       pNodeId
 * @retval  エラーコード
 */
static int closechannel_mutual(const uint8_t *pNodeId)
{
    LOGD("mutual close\n");

    int err;
    lnapp_conf_t *p_appconf = search_connected_lnapp_node(pNodeId);
    if ((p_appconf != NULL) && (ln_htlc_num(p_appconf->p_self) == 0)) {
        //接続中
        bool ret = lnapp_close_channel(p_appconf);
        if (ret) {
            err = 0;
        } else {
            LOGD("fail: mutual  close\n");
            err = RPCERR_CLOSE_START;
        }
    } else {
        err = RPCERR_NOCONN;
    }

    return err;
}


/** channel unilateral close開始
 *
 * @param[in]       pNodeId
 * @retval  エラーコード
 */
static int closechannel_unilateral(const uint8_t *pNodeId)
{
    LOGD("unilateral close\n");

    int err;
    bool haveCnl = ln_node_search_channel(NULL, pNodeId);
    if (haveCnl) {
        bool ret = lnapp_close_channel_force(pNodeId);
        if (ret) {
            err = 0;
        } else {
            LOGD("fail: unilateral close\n");
            err = RPCERR_CLOSE_FAIL;
        }
    } else {
        //チャネルなし
        err = RPCERR_NOCHANN;
    }

    return err;
}


/********************************************************************
 * private functions : others
 ********************************************************************/

/** ptarmcli -c解析
 *
 */
//static bool json_connect(cJSON *params, int *pIndex, peer_conn_t *pConn)
//{
//    cJSON *json;
//
//    if (params == NULL) {
//        return false;
//    }
//
//    //peer_nodeid, peer_addr, peer_port
//    json = cJSON_GetArrayItem(params, (*pIndex)++);
//    if (json && (json->type == cJSON_String)) {
//        bool ret = utl_misc_str2bin(pConn->node_id, BTC_SZ_PUBKEY, json->valuestring);
//        if (ret) {
//            LOGD("pConn->node_id=%s\n", json->valuestring);
//        } else {
//            LOGD("fail: invalid node_id string\n");
//            return false;
//        }
//    } else {
//        LOGD("fail: node_id\n");
//        return false;
//    }
//    if (memcmp(ln_node_getid(), pConn->node_id, BTC_SZ_PUBKEY) == 0) {
//        //node_idが自分と同じ
//        LOGD("fail: same own node_id\n");
//        return false;
//    }
//    json = cJSON_GetArrayItem(params, (*pIndex)++);
//    if (json && (json->type == cJSON_String)) {
//        strcpy(pConn->ipaddr, json->valuestring);
//        LOGD("pConn->ipaddr=%s\n", json->valuestring);
//    } else {
//        LOGD("fail: ipaddr\n");
//        return false;
//    }
//    json = cJSON_GetArrayItem(params, (*pIndex)++);
//    if (json && (json->type == cJSON_Number)) {
//        pConn->port = json->valueint;
//        LOGD("pConn->port=%d\n", json->valueint);
//    } else {
//        LOGD("fail: port\n");
//        return false;
//    }
//
//    return true;
//}


/** not public channel情報からr field情報を作成
 * 
 * channel_announcementする前や、channel_announcementしない場合、invoiceのr fieldに経路情報を追加することで、
 * announcementしていない部分の経路を知らせることができる。
 * ただ、その経路は自分へ向いているため、channelの相手が送信するchannel_updateの情報を追加することになる。
 * 現在接続していなくても、送金時には接続している可能性があるため、r fieldに追加する。
 */
static void create_bolt11_rfield(ln_fieldr_t **ppFieldR, uint8_t *pFieldRNum)
{
    rfield_prm_t prm;

    *ppFieldR = NULL;
    *pFieldRNum = 0;

    prm.pp_field = ppFieldR;
    prm.p_fieldnum = pFieldRNum;
    ln_db_self_search(comp_func_cnl, &prm);

    if (*pFieldRNum != 0) {
        LOGD("add r_field: %d\n", *pFieldRNum);
    } else {
        LOGD("no r_field\n");
    }
}


/** #ln_node_search_channel()処理関数
 *
 * @param[in,out]   self            DBから取得したself
 * @param[in,out]   p_db_param      DB情報(ln_dbで使用する)
 * @param[in,out]   p_param         rfield_prm_t構造体
 */
static bool comp_func_cnl(ln_self_t *self, void *p_db_param, void *p_param)
{
    (void)p_db_param;

    bool ret;
    rfield_prm_t *prm = (rfield_prm_t *)p_param;

    utl_buf_t buf_bolt = UTL_BUF_INIT;
    ln_cnl_update_t msg;
    ret = ln_get_channel_update_peer(self, &buf_bolt, &msg);
    if (ret && !ln_is_announced(self)) {
        size_t sz = (1 + *prm->p_fieldnum) * sizeof(ln_fieldr_t);
        *prm->pp_field = (ln_fieldr_t *)UTL_DBG_REALLOC(*prm->pp_field, sz);

        ln_fieldr_t *pfield = *prm->pp_field + *prm->p_fieldnum;
        memcpy(pfield->node_id, ln_their_node_id(self), BTC_SZ_PUBKEY);
        pfield->short_channel_id = ln_short_channel_id(self);
        pfield->fee_base_msat = msg.fee_base_msat;
        pfield->fee_prop_millionths = msg.fee_prop_millionths;
        pfield->cltv_expiry_delta = msg.cltv_expiry_delta;

        (*prm->p_fieldnum)++;
        LOGD("r_field num=%d\n", *prm->p_fieldnum);
    }
    utl_buf_free(&buf_bolt);

    return false;
}


/** BOLT11文字列生成
 *
 */
static char *create_bolt11(const uint8_t *pPayHash, uint64_t Amount, uint32_t Expiry, const ln_fieldr_t *pFieldR, uint8_t FieldRNum, uint32_t MinFinalCltvExpiry)
{
    uint8_t type;
    btc_genesis_t gtype = btc_util_get_genesis(ln_get_genesishash());
    switch (gtype) {
    case BTC_GENESIS_BTCMAIN:
        type = LN_INVOICE_MAINNET;
        break;
    case BTC_GENESIS_BTCTEST:
        type = LN_INVOICE_TESTNET;
        break;
    case BTC_GENESIS_BTCREGTEST:
        type = LN_INVOICE_REGTEST;
        break;
    default:
        type = BTC_GENESIS_UNKNOWN;
        break;
    }
    char *p_invoice = NULL;
    if (type != BTC_GENESIS_UNKNOWN) {
        ln_invoice_create(&p_invoice, type,
                pPayHash, Amount, Expiry, pFieldR, FieldRNum, MinFinalCltvExpiry);
    }
    return p_invoice;
}


/** 接続済みlnapp_conf_t取得
 *
 */
static lnapp_conf_t *search_connected_lnapp_node(const uint8_t *p_node_id)
{
    lnapp_conf_t *p_appconf;

    p_appconf = p2p_cli_search_node(p_node_id);
    if (p_appconf == NULL) {
        p_appconf = p2p_svr_search_node(p_node_id);
    }
    return p_appconf;
}


/** JSON-RPC送信
 *
 */
//static int send_json(const char *pSend, const char *pAddr, uint16_t Port)
//{
//    int retval = -1;
//    struct sockaddr_in sv_addr;
//
//    int sock = socket(PF_INET, SOCK_STREAM, 0);
//    if (sock < 0) {
//        return retval;
//    }
//    memset(&sv_addr, 0, sizeof(sv_addr));
//    sv_addr.sin_family = AF_INET;
//    sv_addr.sin_addr.s_addr = inet_addr(pAddr);
//    sv_addr.sin_port = htons(Port);
//    retval = connect(sock, (struct sockaddr *)&sv_addr, sizeof(sv_addr));
//    if (retval < 0) {
//        close(sock);
//        return retval;
//    }
//    write(sock, pSend, strlen(pSend));
//
//    //受信を待つとDBの都合でロックしてしまうため、すぐに閉じる
//
//    close(sock);
//
//    return 0;
//}


