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
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <jansson.h>
#include "utl_opts.h"
#include "utl_str.h"
#include "utl_jsonrpc.h"


/**************************************************************************
 * macros
 **************************************************************************/

#define BUFFER_SIZE     (256 * 1024)


/**************************************************************************
 * static variables
 **************************************************************************/

//static char         mPeerAddr[INET6_ADDRSTRLEN];
//static uint16_t     mPeerPort;
//static char         mPeerNodeId[BTC_SZ_PUBKEY * 2 + 1];
//static char         mBuf[BUFFER_SIZE];
//static bool         mTcpSend;
//static char         mAddr[256];
//static char         mErrStr[256];


/********************************************************************
 * prototypes
 ********************************************************************/

static int msg_send(char *pRecv, const char *pSend, const char *pAddr, uint16_t Port, bool bSend);


/********************************************************************
 * public functions
 ********************************************************************/

int main(int argc, char *argv[])
{
    utl_opt_t opts[] = {
        {"-?", NULL, NULL, "This help message", NULL, false},
        //{"-datadir", "dir", NULL, "Specify data directory", NULL, false},
        {"-rpcport", "port", NULL, "Connect to JSON-RPC on <port> (default: 9736)", NULL, false},
        {"-recon", NULL, NULL, "Don't actually send commands; just print them", NULL, false},
        {NULL, NULL, NULL, NULL, NULL, false}, //watchdog
    };

    if (!utl_opts_parse(opts, argc, (const char ** const)argv)) goto LABEL_EXIT;

    if (utl_opts_is_set(opts, "-?")) goto LABEL_EXIT;

    //if (utl_opts_is_set(opts, "-datadir")) {
    //    //Change working directory.
    //    // It is done at the beginning of this process.
    //    const char* dir = utl_opts_get_string(opts, "-datadir");
    //    if (!dir || chdir(dir) != 0) {
    //        fprintf(stderr, "fail: change the working directory\n");
    //        utl_opts_free(opts);
    //        return -1;
    //    }
    //}

    uint16_t rpcport = 9736;
    if (utl_opts_is_set(opts, "-rpcport")) {
        if (!utl_opts_get_u16(opts, &rpcport, "-rpcport")) goto LABEL_EXIT;
    }

    bool recon = false;
    if (utl_opts_is_set(opts, "-recon")) {
        recon = true;
    }

    //count <params>
    // skip program name and options
    int i;
    for (i = 0; i < argc; i++) {
        if (i == 0) continue;
        if (argv[i][0] != '-') break;
    }
    int paramc = argc - i;
    if (paramc == 0) goto LABEL_EXIT;
    paramc--;
    const char *method = argv[i];
    const char **paramv = (const char**)&argv[i + 1];
    
    //create request
    utl_jsonrpc_param_t non_string_params[] = {
        {"foo", 1}, //XXX
        {"setfeerate", 0},
        {"dev-debug", 0},
        {"addinvoice", 0},
        {"addinvoice", 1},
        {"connectpeer", 2},
        {"dev-disableautoconnect", 0},
        {"openchannel", 1},
        {"openchannel", 2},
        {"openchannel", 3},
        {"openchannel", 5},
        {NULL, 0}, //watchdog
    };
    utl_str_t body;
    utl_str_init(&body);
    if (!utl_jsonrpc_create_request(&body, method, paramc, paramv, non_string_params)) goto LABEL_EXIT;

    //send
    char recv_buf[BUFFER_SIZE];
    int ret = msg_send(recv_buf, utl_str_get(&body), "", rpcport, !recon);
    utl_str_free(&body);

    utl_opts_free(opts);
    return ret;

LABEL_EXIT:
    fprintf(stderr, "Ptarmigan RPC Client version vX.XX.X\n");
    fprintf(stderr, "\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  ptarmcli [options] <command> [params]        Send command to Ptarmigan Daemon\n");
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

    utl_opts_free(opts);
    return -1;
}


/********************************************************************
 * private functions
 ********************************************************************/

static int msg_send(char *pRecv, const char *pSend, const char *pAddr, uint16_t Port, bool bSend)
{
    int retval = -1;

    if (bSend) {
        struct sockaddr_in sv_addr;

        int sock = socket(PF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            fprintf(stderr, "fail socket: %s\n", strerror(errno));
            return retval;
        }

        memset(&sv_addr, 0, sizeof(sv_addr));
        sv_addr.sin_family = AF_INET;
        if (strlen(pAddr) == 0) {
            sv_addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        } else {
            sv_addr.sin_addr.s_addr = inet_addr(pAddr);
        }
        sv_addr.sin_port = htons(Port);

        retval = connect(sock, (struct sockaddr *)&sv_addr, sizeof(sv_addr));
        if (retval < 0) {
            fprintf(stderr, "fail connect: %s\n", strerror(errno));
            close(sock);
            return retval;
        }

        write(sock, pSend, strlen(pSend));

        ssize_t len = read(sock, pRecv, BUFFER_SIZE - 1);
        if (len > 0) {
            retval = -1;
            pRecv[len] = '\0';
            printf("%s\n", pRecv);

            json_t *p_root;
            json_error_t error;
            p_root = json_loads(pRecv, 0, &error);
            if (p_root) {
                json_t *p_result;
                p_result = json_object_get(p_root, "result");
                if (p_result) {
                    //success
                    retval = 0;
                }
            }
        } else if (len <= 0) {
            fprintf(stderr, "fail read: %s\n", strerror(errno));
        }

        close(sock);
    } else {
        printf("sendto: %s:%" PRIu16 "\n", (strlen(pAddr) != 0) ? pAddr : "localhost", Port);
        printf("%s\n", pSend);
        retval = 0;
    }

    return retval;
}
