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
/** @file   ptarmd_main.c
 *  @brief  ptarmd entry point
 */
#include <stdio.h>
#include <pthread.h>
#include <signal.h>

#include "btcrpc.h"
#include "conf.h"
#include "ln_db.h"
#include "utl_log.h"
#include "utl_opts.h"
#include "utl_addr.h"


/**************************************************************************
 * macros
 **************************************************************************/


/********************************************************************
 * prototypes
 ********************************************************************/

static void sig_set_catch_sigs(sigset_t *pSigSet);
static void *sig_handler_start(void *pArg);


/********************************************************************
 * entry point
 ********************************************************************/

int main(int argc, char *argv[])
{
    bool bret;

    utl_opt_t opts[] = {
        {"-?", NULL, NULL, "This help message", NULL, false},
        {"-datadir", "dir", NULL, "Specify data directory", NULL, false},
        {"-port", "port", NULL, "Listen for Lightning Network connections on TCP <port> (default: 9735)", NULL, false},
        {"-externalip", "ip", NULL, "Specify external IPv4 address (default: None)", NULL, false},
        {"-alias", "name", NULL, "Specify alias up to 32 bytes<name> (default: \"node_\"+`node_id first 6bytes`)", NULL, false},
        {"-rpcport", "port", NULL, "Listen for JSON-RPC connections on TCP <port> (default: <LN port>+1)", NULL, false},
        {"-bitcoin-conffile", "file", NULL, "Specify bitcoind conf file (default: <HOME>/.bitcoin/bitcoin.conf)", NULL, false},
        {"-dev-removedb", NULL, NULL, "Remove current DB (without my `node_id`, for developers)", NULL, false},
        {"-dev-removenodeannodb", NULL, NULL, "Remove `node_announcement` DB (for developers)", NULL, false},
        {NULL, NULL, NULL, NULL, NULL, false}, //watchdog
    };

    if (!utl_opts_parse(opts, argc, (const char ** const)argv)) goto LABEL_EXIT;

    if (utl_opts_is_set(opts, "-?")) goto LABEL_EXIT;

    if (utl_opts_is_set(opts, "-datadir")) {
        //Change working directory.
        // It is done at the beginning of this process.
        const char* dir = utl_opts_get_string(opts, "-datadir");
        if (!dir || chdir(dir) != 0) {
            fprintf(stderr, "fail: change the working directory\n");
            utl_opts_free(opts);
            return -1;
        }
    }

#ifdef ENABLE_PLOG_TO_STDOUT
    utl_log_init_stdout();
#else
    utl_log_init();
#endif

#ifndef NETKIND
#error not define NETKIND
#endif
#if NETKIND==0
    bret = btc_init(BTC_MAINNET, true);
#elif NETKIND==1
    bret = btc_init(BTC_TESTNET, true);
#endif
    if (!bret) {
        fprintf(stderr, "fail: btc_init()\n");
        utl_opts_free(opts);
        return -1;
    }

    const char *param;

    ln_nodeaddr_t *p_addr;
    if (utl_opts_is_set(opts, "-port")) {
        p_addr = ln_node_addr();
        if (!utl_opts_get_u16(opts, &p_addr->port, "-port")) goto LABEL_EXIT;
    }

    if (utl_opts_is_set(opts, "-externalip")) {
        param = utl_opts_get_string(opts, "-externalip");
        if (!param) goto LABEL_EXIT;
        p_addr = ln_node_addr();
        p_addr->type = LN_NODEDESC_IPV4;
        if (!utl_addr_ipv4_str2bin(p_addr->addrinfo.addr, param)) goto LABEL_EXIT;
    }

    if (utl_opts_is_set(opts, "-alias")) {
        //node name(alias)
        param = utl_opts_get_string(opts, "-alias");
        if (!param) goto LABEL_EXIT;
        if (strlen(param) > LN_SZ_ALIAS) goto LABEL_EXIT;
        char *p_alias = ln_node_alias();
        strncpy(p_alias, param, LN_SZ_ALIAS);
        p_alias[LN_SZ_ALIAS] = '\0';
    }

    uint16_t rpcport = 0;
    if (utl_opts_is_set(opts, "-rpcport")) {
        if (!utl_opts_get_u16(opts, &rpcport, "-rpcport")) goto LABEL_EXIT;
    }

    //load btcconf file
    rpc_conf_t rpc_conf;
    conf_btcrpc_init(&rpc_conf);
    if (utl_opts_is_set(opts, "-bitcoin-conffile")) {
        param = utl_opts_get_string(opts, "-bitcoin-conffile");
        if (!param) goto LABEL_EXIT;
        if (!conf_btcrpc_load(param, &rpc_conf)) goto LABEL_EXIT;
    }
    if ((strlen(rpc_conf.rpcuser) == 0) || (strlen(rpc_conf.rpcpasswd) == 0)) {
        //bitcoin.confから読込む
        bret = conf_btcrpc_load_default(&rpc_conf);
        if (!bret) goto LABEL_EXIT;
    }

    if (utl_opts_is_set(opts, "-dev-removedb")) {
        //ノード情報を残してすべて削除
        bret = ln_db_reset();
        fprintf(stderr, "db_reset: %d\n", bret);
        utl_opts_free(opts);
        return 0;
    }

    if (utl_opts_is_set(opts, "-dev-removenodeannodb")) {
        //node_announcementを全削除
        bret = ln_db_annonod_drop_startup();
        fprintf(stderr, "db_annonod_drop: %d\n", bret);
        utl_opts_free(opts);
        return 0;
    }

    utl_opts_free(opts);

    //O'REILLY Japan: BINARY HACKS #52
    sigset_t ss;
    pthread_t th_sig;
    sig_set_catch_sigs(&ss);
    sigprocmask(SIG_BLOCK, &ss, NULL);
    signal(SIGPIPE , SIG_IGN);   //ignore SIGPIPE
    pthread_create(&th_sig, NULL, &sig_handler_start, NULL);

    //bitcoind起動確認
    uint8_t genesis[LN_SZ_HASH];
    bret = btcrpc_init(&rpc_conf);
    if (!bret) {
        fprintf(stderr, "fail: initialize btcrpc\n");
        return -1;
    }
    bret = btcrpc_getgenesisblock(genesis);
    if (!bret) {
        fprintf(stderr, "fail: bitcoin getblockhash\n");
        return -1;
    }

    //https://github.com/lightningnetwork/lightning-rfc/issues/237
    for (int lp = 0; lp < LN_SZ_HASH / 2; lp++) {
        uint8_t tmp = genesis[lp];
        genesis[lp] = genesis[LN_SZ_HASH - lp - 1];
        genesis[LN_SZ_HASH - lp - 1] = tmp;
    }
    ln_set_genesishash(genesis);

#if NETKIND==0
    LOGD("start bitcoin mainnet\n");
#elif NETKIND==1
    LOGD("start bitcoin testnet/regtest\n");
#endif

    ptarmd_start(rpcport);

    return 0;

LABEL_EXIT:
    fprintf(stderr, "Ptarmigan Lightning Daemon version vX.XX.X\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  ptarmd [options]                     Start Ptarmigan Lightning Daemon\n");
    fprintf(stderr, "\n");

    utl_str_t messages;
    utl_str_init(&messages);
    if (utl_opts_get_help_messages(opts, &messages)) {
        fprintf(stderr, "Options:\n");
        fprintf(stderr, "\n");
        fprintf(stderr, "%s", utl_str_get(&messages));
        utl_str_free(&messages);
    } else {
        fprintf(stderr, "fatal\n");
    }

    utl_opts_free(opts); //double free is permitted
    return -1;
}


/********************************************************************
 * private functions
 ********************************************************************/

//捕捉するsignal設定
static void sig_set_catch_sigs(sigset_t *pSigSet)
{
    sigemptyset(pSigSet);
    sigaddset(pSigSet, SIGHUP);
    sigaddset(pSigSet, SIGINT);
    sigaddset(pSigSet, SIGQUIT);
    sigaddset(pSigSet, SIGTERM);
    sigaddset(pSigSet, SIGABRT);
    sigaddset(pSigSet, SIGSEGV);
}


//signal捕捉スレッド
static void *sig_handler_start(void *pArg)
{
    (void)pArg;

    LOGD("signal handler\n");
    pthread_detach(pthread_self());

    sigset_t ss;
    siginfo_t info;
    sig_set_catch_sigs(&ss);
    while (1) {
        if (sigwaitinfo(&ss, &info) > 0) {
            LOGD("!!! SIGNAL DETECT: %d !!!\n", info.si_signo);
            exit(-1);
        }
    }
    return NULL;
}
