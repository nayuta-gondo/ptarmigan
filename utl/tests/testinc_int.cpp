////////////////////////////////////////////////////////////////////////
//FAKE関数

//FAKE_VALUE_FUNC(int, external_function, int);

////////////////////////////////////////////////////////////////////////

class int_: public testing::Test {
protected:
    virtual void SetUp() {
        //RESET_FAKE(external_function)
        utl_dbg_malloc_cnt_reset();
    }

    virtual void TearDown() {
        ASSERT_EQ(0, utl_dbg_malloc_cnt());
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

TEST_F(int_, pack)
{
    uint8_t data0[] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    };
    uint8_t data1[] = {
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    };

    ASSERT_EQ(utl_int_pack_u16be(data0), 0x0123);
    ASSERT_EQ(utl_int_pack_u16be(data1), 0xfedc);
    ASSERT_EQ(utl_int_pack_u32be(data0), 0x01234567);
    ASSERT_EQ(utl_int_pack_u32be(data1), 0xfedcba98);
    ASSERT_EQ(utl_int_pack_u64be(data0), 0x0123456789abcdefUL);
    ASSERT_EQ(utl_int_pack_u64be(data1), 0xfedcba9876543210UL);

    ASSERT_EQ(utl_int_pack_u16le(data0), 0x2301);
    ASSERT_EQ(utl_int_pack_u16le(data1), 0xdcfe);
    ASSERT_EQ(utl_int_pack_u32le(data0), 0x67452301);
    ASSERT_EQ(utl_int_pack_u32le(data1), 0x98badcfe);
    ASSERT_EQ(utl_int_pack_u64le(data0), 0xefcdab8967452301UL);
    ASSERT_EQ(utl_int_pack_u64le(data1), 0x1032547698badcfeUL);
}


TEST_F(int_, unpack)
{
    uint8_t data0[] = {
        0x01, 0x23, 0x45, 0x67, 0x89, 0xab, 0xcd, 0xef,
    };
    uint8_t data1[] = {
        0xfe, 0xdc, 0xba, 0x98, 0x76, 0x54, 0x32, 0x10,
    };
    uint8_t buf[8];

    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u16be(buf, 0x0123);
    ASSERT_EQ(memcmp(buf, data0, 2), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u16be(buf, 0xfedc);
    ASSERT_EQ(memcmp(buf, data1, 2), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u32be(buf, 0x01234567);
    ASSERT_EQ(memcmp(buf, data0, 4), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u32be(buf, 0xfedcba98);
    ASSERT_EQ(memcmp(buf, data1, 4), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u64be(buf, 0x0123456789abcdefUL);
    ASSERT_EQ(memcmp(buf, data0, 8), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u64be(buf, 0xfedcba9876543210UL);
    ASSERT_EQ(memcmp(buf, data1, 8), 0);

    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u16le(buf, 0x2301);
    ASSERT_EQ(memcmp(buf, data0, 2), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u16le(buf, 0xdcfe);
    ASSERT_EQ(memcmp(buf, data1, 2), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u32le(buf, 0x67452301);
    ASSERT_EQ(memcmp(buf, data0, 4), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u32le(buf, 0x98badcfe);
    ASSERT_EQ(memcmp(buf, data1, 4), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u64le(buf, 0xefcdab8967452301UL);
    ASSERT_EQ(memcmp(buf, data0, 8), 0);
    memset(buf, 0x00, sizeof(buf));
    utl_int_unpack_u64le(buf, 0x1032547698badcfeUL);
    ASSERT_EQ(memcmp(buf, data1, 8), 0);
}

