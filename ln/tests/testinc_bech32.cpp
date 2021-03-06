////////////////////////////////////////////////////////////////////////
//FAKE関数

//FAKE_VALUE_FUNC(int, external_function, int);

////////////////////////////////////////////////////////////////////////

class bech32: public testing::Test {
protected:
    virtual void SetUp() {
        //RESET_FAKE(external_function)
        utl_dbg_malloc_cnt_reset();
        btc_init(BTC_TESTNET, false);
    }

    virtual void TearDown() {
        ASSERT_EQ(0, utl_dbg_malloc_cnt());
        btc_term();
    }

public:
    static void DumpBin(const uint8_t *pData, uint16_t Len)
    {
        for (uint16_t lp = 0; lp < Len; lp++) {
            printf("%02x", pData[lp]);
        }
        printf("\n");
    }
};

////////////////////////////////////////////////////////////////////////

// https://github.com/lightningnetwork/lightning-rfc/blob/master/11-payment-encoding.md#examples
struct valid_invoice_data {
    const char* invoice;
    const uint8_t privkey[BTC_SZ_PRIVKEY];
    const uint8_t pubkey[BTC_SZ_PUBKEY];
    const uint8_t payment_hash[BTC_SZ_HASH256];
};
static struct valid_invoice_data ln_valid_invoice[] = {
    {
        "lnbc1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdpl2pkx2ctnv5sxxmmwwd5kgetjypeh2ursdae8g6twvus8g6rfwvs8qun0dfjkxaq8rkx3yf5tcsyz3d73gafnh3cax9rn449d9p5uxz9ezhhypd0elx87sjle52x86fux2ypatgddc6k63n7erqz25le42c4u4ecky03ylcqca784w",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lnbc2500u1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqdq5xysxxatsyp3k7enxv4jsxqzpuaztrnwngzn3kdzw5hydlzf03qdgm2hdq27cqv3agm2awhz5se903vruatfhq77w3ls4evs3ch9zw97j25emudupq63nyw24cg27h2rspfj9srp",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lnbc20m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqscc6gd6ql3jrc5yzme8v4ntcewwz5cnw92tz0pc8qcuufvq7khhr8wpald05e92xw006sq94mg8v2ndf4sefvf9sygkshp5zfem29trqq2yxxz7",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lntb20m1pvjluezhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqspp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqfpp3x9et2e20v6pu37c5d9vax37wxq72un98kmzzhznpurw9sgl2v0nklu2g4d0keph5t7tj9tcqd8rexnd07ux4uv2cjvcqwaxgj7v4uwn5wmypjd5n69z2xm3xgksg28nwht7f6zspwp3f9t",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lnbc20m1pvjluezpp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqsfpp3qjmp7lwpagxun9pygexvgpjdc4jdj85fr9yq20q82gphp2nflc7jtzrcazrra7wwgzxqc8u7754cdlpfrmccae92qgzqvzq2ps8pqqqqqqpqqqqq9qqqvpeuqafqxu92d8lr6fvg0r5gv0heeeqgcrqlnm6jhphu9y00rrhy4grqszsvpcgpy9qqqqqqgqqqqq7qqzqj9n4evl6mr5aj9f58zp6fyjzup6ywn3x6sk8akg5v4tgn2q8g4fhx05wf6juaxu9760yp46454gpg5mtzgerlzezqcqvjnhjh8z3g2qqdhhwkj",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lnbc20m1pvjluezhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqspp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqfppj3a24vwu6r8ejrss3axul8rxldph2q7z9kmrgvr7xlaqm47apw3d48zm203kzcq357a4ls9al2ea73r8jcceyjtya6fu5wzzpe50zrge6ulk4nvjcpxlekvmxl6qcs9j3tz0469gq5g658y",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lnbc20m1pvjluezhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqspp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqfppqw508d6qejxtdg4y5r3zarvary0c5xw7kepvrhrm9s57hejg0p662ur5j5cr03890fa7k2pypgttmh4897d3raaq85a293e9jpuqwl0rnfuwzam7yr8e690nd2ypcq9hlkdwdvycqa0qza8",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
    {
        "lnbc20m1pvjluezhp58yjmdan79s6qqdhdzgynm4zwqd5d7xmw5fk98klysy043l2ahrqspp5qqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqqqsyqcyq5rqwzqfqypqfp4qrp33g0q5c5txsp9arysrx4k6zdkfs4nce4xj0gdcccefvpysxf3q28j0v3rwgy9pvjnd48ee2pl8xrpxysd5g44td63g6xcjcu003j3qe8878hluqlvl3km8rm92f5stamd3jw763n3hck0ct7p8wwj463cql26ava",
        { 0xe1, 0x26, 0xf6, 0x8f, 0x7e, 0xaf, 0xcc, 0x8b, 0x74, 0xf5, 0x4d, 0x26, 0x9f, 0xe2, 0x06, 0xbe, 0x71, 0x50, 0x00, 0xf9, 0x4d, 0xac, 0x06, 0x7d, 0x1c, 0x04, 0xa8, 0xca, 0x3b, 0x2d, 0xb7, 0x34 },
        { 0x03, 0xe7, 0x15, 0x6a, 0xe3, 0x3b, 0x0a, 0x20, 0x8d, 0x07, 0x44, 0x19, 0x91, 0x63, 0x17, 0x7e, 0x90, 0x9e, 0x80, 0x17, 0x6e, 0x55, 0xd9, 0x7a, 0x2f, 0x22, 0x1e, 0xde, 0x0f, 0x93, 0x4d, 0xd9, 0xad },
        { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x01, 0x02 },
    },
};

// static void print_invoice(const ln_invoice_t *p_invoice_data) {
//     fprintf(stderr, "-----------------------------------------\n");
//     switch (p_invoice_data->hrp_type) {
//     case LN_INVOICE_MAINNET:
//         fprintf(stderr, "for mainnet\n");
//         break;
//     case LN_INVOICE_TESTNET:
//         fprintf(stderr, "for testnet\n");
//         break;
//     default:
//         fprintf(stderr, "unknown hrp_type\n");
//     }
//     fprintf(stderr, "amount_msat=%" PRIu64 "\n", p_invoice_data->amount_msat);
//     char time[UTL_SZ_TIME_FMT_STR + 1];
//     fprintf(stderr, "timestamp= %" PRIu64 " : %s\n", (uint64_t)p_invoice_data->timestamp, utl_time_fmt(time, p_invoice_data->timestamp));
//     fprintf(stderr, "min_final_cltv_expiry=%u\n", p_invoice_data->min_final_cltv_expiry);
//     fprintf(stderr, "pubkey=");
//     for (int lp = 0; lp < BTC_SZ_PUBKEY; lp++) {
//         fprintf(stderr, "%02x", p_invoice_data->pubkey[lp]);
//     }
//     fprintf(stderr, "\n");
//     fprintf(stderr, "payment_hash=");
//     for (int lp = 0; lp < BTC_SZ_HASH256; lp++) {
//         fprintf(stderr, "%02x", p_invoice_data->payment_hash[lp]);
//     }
//     fprintf(stderr, "\n");
//     fprintf(stderr, "-----------------------------------------\n");
// }



TEST_F(bech32, invoice_valid)
{
    size_t i;

    for (i = 0; i < sizeof(ln_valid_invoice) / sizeof(ln_valid_invoice[0]); ++i) {
        //printf("\n\n=[%d]=============================\n", (int)i);
        ln_invoice_t *p_invoice_data = NULL;
        bool ret;

        ln_node_setkey(ln_valid_invoice[i].privkey);
        ret = ln_invoice_decode(&p_invoice_data, ln_valid_invoice[i].invoice);
        ASSERT_TRUE(ret);
        ASSERT_EQ(0, memcmp(p_invoice_data->pubkey, ln_valid_invoice[i].pubkey, BTC_SZ_PUBKEY));

        char *p_invoice = NULL;
        ret = ln_invoice_encode(&p_invoice, p_invoice_data);
        ASSERT_TRUE(ret);

        ln_invoice_t *p_invoice_data2 = NULL;
        ret = ln_invoice_decode(&p_invoice_data2, p_invoice);
        ASSERT_TRUE(ret);

        //print_invoice(p_invoice_data2);
        ASSERT_EQ(p_invoice_data->hrp_type, p_invoice_data2->hrp_type);
        ASSERT_EQ(p_invoice_data->amount_msat, p_invoice_data2->amount_msat);
        ASSERT_NE(p_invoice_data->timestamp, p_invoice_data2->timestamp);       //時刻は更新するので、変化する
        ASSERT_EQ(p_invoice_data->min_final_cltv_expiry, p_invoice_data2->min_final_cltv_expiry);
        ASSERT_EQ(0, memcmp(p_invoice_data->pubkey, p_invoice_data2->pubkey, BTC_SZ_PUBKEY));
        ASSERT_EQ(0, memcmp(p_invoice_data->payment_hash, p_invoice_data2->payment_hash, BTC_SZ_HASH256));
        ASSERT_EQ(p_invoice_data->expiry, p_invoice_data2->expiry);

        free(p_invoice_data2);
        free(p_invoice_data);
        free(p_invoice);
    }
}
