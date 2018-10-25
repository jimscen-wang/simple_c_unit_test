
#if !defined(MBEDTLS_CONFIG_FILE)
#include "mbedtls/config.h"
#else
#include MBEDTLS_CONFIG_FILE
#endif


#include "test_atiny.h"
#include <string.h>
#include <stdio.h>
#include "fota/fota_package_storage_device.h"
#include "fota_port.h"
#include "hal_spi_flash.h"
#include "los_hwi.h"
//#include "cm_backtrace.h"
#include <board.h>
//#include "../components/ota/fota/ota_sha256.h"
#include "dtls_interface.h"
#include "flag_manager.h"
#include "upgrade_flag.h"
#include "mbedtls/sha256.h"






#define BLOCK_SIZE  4096

#if 0
#undef MALLOC
#define MALLOC malloc

#undef FREE
#define FREE free
#endif



typedef struct
{
     uint8_t m_data0[BLOCK_SIZE];
     ota_opt_s ota_opt;
     //int init_flag;
     atiny_fota_storage_device_s *storage_device;
}test_fota_s;

test_fota_s g_test_fota;





extern void HardFault_Handler_bt(void);
static int haha_flag = 0;
void haha(void)
{
    haha_flag++;
}



static void testFlashRead(void)
{
    //#define UPDATE_INFO_ADDR              0x00008000

    uint8_t write_data0[16];
    uint32_t offset = 0;
    test_fota_s *test_fota = &g_test_fota;

    TEST_LOG("testFlashRead begin ");

    memset(write_data0, 0x1, sizeof(write_data0));
    uint8_t *tmp = (uint8_t *)atiny_malloc(BLOCK_SIZE);
    EXPECT_TRUE(tmp != NULL);
    int ret = hal_spi_flash_read(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR);
    EXPECT_TRUE(TEST_OK == ret);
    memcpy(tmp, write_data0, sizeof(write_data0));
    ret = hal_spi_flash_erase_write(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR);
    EXPECT_TRUE(TEST_OK == ret);

    memset(test_fota->m_data0, 0x2, sizeof(test_fota->m_data0));
    ret = hal_spi_flash_read(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR);
    EXPECT_TRUE(TEST_OK == ret);
    memcpy(tmp + sizeof(write_data0), test_fota->m_data0, BLOCK_SIZE - sizeof(write_data0));
    ret = hal_spi_flash_erase_write(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR);
    EXPECT_TRUE(TEST_OK == ret);

    ret = hal_spi_flash_read(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR + BLOCK_SIZE);
    EXPECT_TRUE(TEST_OK == ret);
    memcpy(tmp, test_fota->m_data0 + (sizeof(test_fota->m_data0) - sizeof(write_data0)), sizeof(write_data0));
    ret = hal_spi_flash_erase_write(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR + BLOCK_SIZE);
    EXPECT_TRUE(TEST_OK == ret);

    uint8_t write_data1[17];
    memset(write_data1, 0x3, sizeof(write_data1));
    ret = hal_spi_flash_read(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR + BLOCK_SIZE);
    EXPECT_TRUE(TEST_OK == ret);
    memcpy(tmp + sizeof(write_data0), write_data1, sizeof(write_data1));
    ret = hal_spi_flash_erase_write(tmp, BLOCK_SIZE, UPDATE_INFO_ADDR + BLOCK_SIZE);
    EXPECT_TRUE(TEST_OK == ret);
    atiny_free(tmp);
    tmp = NULL;

    uint32_t read_size = 16 + BLOCK_SIZE + 17;
    uint8_t *read_data = (uint8_t *)atiny_malloc(read_size);
    offset = 0;
    EXPECT_TRUE(read_data != NULL);
    ret = hal_spi_flash_read(read_data, 15, UPDATE_INFO_ADDR + offset);
    EXPECT_TRUE(TEST_OK == ret);
    offset += 15;

    ret = hal_spi_flash_read(read_data + offset, BLOCK_SIZE + 1, UPDATE_INFO_ADDR + offset);
    EXPECT_TRUE(TEST_OK == ret);
    offset += BLOCK_SIZE + 1;

    ret = hal_spi_flash_read(read_data + offset, 17, UPDATE_INFO_ADDR + offset);
    EXPECT_TRUE(TEST_OK == ret);
    atiny_free(read_data);
    TEST_LOG("testFlashRead end ");
}


static uint8_t * test_fota_create_data(uint32_t len, uint8_t value)
{
    uint8_t *buffer = MALLOC(len);
    EXPECT_TRUE(buffer != NULL);
    memset(buffer, value, len);
    return buffer;
}


static void testWriteAndReadUpdataInfo()
{
#if 0
    atiny_fota_storage_device_s *device = fota_get_pack_device();


    test_fota_s *test_fota = &g_test_fota;

    EXPECT_TRUE(device != NULL);

    memset(test_fota->m_data0, 0, sizeof(test_fota->m_data0));
    int ret;
    for(int i = 0; i < 2; i++)
    {
            ret = device->write_update_info(device, i * sizeof(test_fota->m_data0), test_fota->m_data0, sizeof(test_fota->m_data0));
            EXPECT_TRUE(TEST_OK == ret);
    }

    uint8_t write_data0[16];
    memset(write_data0, 0x1, sizeof(write_data0));


    ret = device->write_update_info(device, 0, write_data0, sizeof(write_data0));
    EXPECT_TRUE(TEST_OK == ret);

    memset(test_fota->m_data0, 0x2, sizeof(test_fota->m_data0));
    device->write_update_info(device, sizeof(write_data0), test_fota->m_data0, sizeof(test_fota->m_data0));
    EXPECT_TRUE(TEST_OK == ret);

    uint8_t write_data1[17];
    memset(write_data1, 0x3, sizeof(write_data1));
    device->write_update_info(device, sizeof(write_data0) + sizeof(test_fota->m_data0), write_data1, sizeof(write_data1));
    EXPECT_TRUE(TEST_OK == ret);

    const uint32_t read_size = sizeof(write_data0) + sizeof(test_fota->m_data0) + sizeof(write_data1);
    uint8_t *read_data = (uint8_t *)MALLOC(read_size);
    EXPECT_TRUE(read_data != NULL);
    memset(read_data, 0, read_size);

    ret = device->read_update_info(device, 0, read_data, sizeof(write_data0) - 1);
    EXPECT_TRUE(TEST_OK == ret);

    //set_spi_debug(true);

    ret = device->read_update_info(device, sizeof(write_data0) - 1,
                                    read_data + sizeof(write_data0) - 1, sizeof(test_fota->m_data0) + 1);
    EXPECT_TRUE(TEST_OK == ret);
    ret = device->read_update_info(device, sizeof(write_data0) + sizeof(test_fota->m_data0) ,
                                    read_data + sizeof(write_data0) + sizeof(test_fota->m_data0), sizeof(write_data1));
    EXPECT_TRUE(TEST_OK == ret);


    ret = memcmp(read_data, write_data0, sizeof(write_data0));
    EXPECT_TRUE(0 == ret);
    ret = memcmp(read_data + sizeof(write_data0), test_fota->m_data0, sizeof(test_fota->m_data0));
    EXPECT_TRUE(0 == ret);
    ret = memcmp(read_data + sizeof(write_data0) + sizeof(test_fota->m_data0), write_data1, sizeof(write_data1));
    EXPECT_TRUE(0 == ret);
    FREE(read_data);
#endif
}


static void testWriteAndReadUpdataInfoMultipleBlocks()
{
#if 0
    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    const uint32_t len = 2 * BLOCK_SIZE + 1111;
    uint8_t *write_data =test_fota_create_data(len, 0);
    int ret = device->write_update_info(device, 0, write_data, len);
    EXPECT_TRUE(TEST_OK == ret);

    memset(write_data, 0xef, len);
    ret = device->write_update_info(device, 0, write_data, len);
    EXPECT_TRUE(TEST_OK == ret);

    uint8_t *read_data = test_fota_create_data(len, 0);
    ret = device->read_update_info(device, 0, read_data, len);
    EXPECT_TRUE(TEST_OK == ret);

    ret = memcmp(read_data, write_data, len);
    EXPECT_TRUE(0 == ret);
#endif
}


static const uint8_t g_head_data[] = {
    0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x53,0x00,0x00,0x00,0x5a,0x00,0x64,0x00,0x15,
    0x4c,0x69,0x74,0x65,0x4f,0x53,0x56,0x31,0x52,0x32,0x43,0x35,0x31,0x30,0x53,0x50,
    0x43,0x30,0x30,0xff,0xee,0x00,0x65,0x00,0x06,0x64,0x9c,0x12,0x34,0x56,0x78,0x00,
    0x01,0x00,0x20,0xc7,0x4c,0xd9,0xd7,0x93,0x27,0x86,0xe3,0x56,0xc1,0xa5,0x74,0x6a,
    0x2c,0xb1,0x79,0xae,0x18,0xff,0x86,0xc6,0x29,0x3c,0xbb,0xa8,0xda,0x43,0x18,0x47,
    0x8d,0x0b,0x39
};

enum {
    TEST_HEAD_LEN = sizeof(g_head_data),
    TEST_TOTAL_POS = 8,
    TEST_CHECKSUM_POS = 0x33,
    TEST_SHA32_LEN = 32,
    TEST_DATA_LEN = 15241/*145111*/,
    TEST_TOTAL_LEN = (TEST_DATA_LEN + TEST_HEAD_LEN),
    TEST_FRAME_SIZE = 1024
};



typedef struct
{
    uint8_t data[TEST_HEAD_LEN];
}test_head_s;


static void test_head_write_bytes(test_head_s *head, uint8_t *data, uint32_t value, uint32_t len)
{
    uint32_t mask;
    uint32_t shift_num;
    if(len == 4)
    {
        mask = 0xff000000;
        shift_num = 24;

    }
    else
    {
        mask = 0x0000ff00;
        shift_num = 16;
    }

    for(int i = 0; i < len; i++)
    {
        data[i] = ((mask & value) >> shift_num);
        mask = (mask >> 8);
        shift_num -= 8;
    }

}


void test_head_init(test_head_s *head, uint32_t total_len)
{
    memcpy(head->data, g_head_data, TEST_HEAD_LEN);
    test_head_write_bytes(head, head->data + TEST_TOTAL_POS, total_len, sizeof(total_len));
    memset(head->data + TEST_CHECKSUM_POS, 0, TEST_SHA32_LEN);
}

uint8_t * test_head_get(test_head_s *head)
{
    return head->data;
}

void test_head_write_checksum(test_head_s *head, const uint8_t *buff, uint32_t len)
{
    memcpy(head->data + TEST_CHECKSUM_POS, buff, len);
}




/*  write part head then break*/
static void test_software_write_part_head_break()
{
    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    test_head_s head;
    test_head_init(&head, TEST_TOTAL_LEN);

    const uint32_t WRITE_LEN = 20 + TEST_HEAD_LEN;
    uint8_t write_data[WRITE_LEN];

    memset(write_data, 0xfe, sizeof(write_data));
    memcpy(write_data, test_head_get(&head), TEST_HEAD_LEN);
    uint32_t tmp_len = 15;
    int ret = device->write_software(device,  0, write_data, tmp_len);
    EXPECT_TRUE(TEST_OK == ret);

    ret = device->write_software(device,  tmp_len, write_data + tmp_len, sizeof(write_data) - tmp_len);
    EXPECT_TRUE(TEST_OK == ret);

    ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_FAIL, 0);
     EXPECT_TRUE(TEST_OK == ret);

    uint8_t read_data[WRITE_LEN - TEST_HEAD_LEN];
    memset(read_data, 0, sizeof(read_data));

    ret = hal_spi_flash_read(read_data, sizeof(read_data), OTA_IMAGE_DOWNLOAD_ADDR);
    EXPECT_TRUE(TEST_OK == ret);
    ret = memcmp(read_data, write_data + TEST_HEAD_LEN, sizeof(read_data));
    EXPECT_TRUE(0 == ret);


}

/* write head not continuous */

static void test_software_write_head_not_continuous(void)
{
    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    test_head_s head;
    test_head_init(&head, TEST_TOTAL_LEN);

    int ret;
    uint32_t len = 10;
    ret = device->write_software(device,  0, test_head_get(&head), len);
    EXPECT_TRUE(TEST_OK == ret);

    ret = device->write_software(device,  len + 1, test_head_get(&head) + len + 1, 1);
    EXPECT_TRUE(TEST_OK != ret);
}




typedef struct
{
    uint8_t *buff;
    test_head_s head;
}test_pack_s;


void test_pack_destroy(test_pack_s *pack)
{
    if(pack->buff)
    {
        FREE(pack->buff);
    }
}

uint32_t test_pack_get_frame_num(const test_pack_s *pack)
{
    return (TEST_TOTAL_LEN / TEST_FRAME_SIZE) + 1;
}

void test_pack_get_frame(test_pack_s *pack, uint32_t idx, uint8_t **buff, uint32_t *len)
{
    uint32_t pos = 0;

    EXPECT_TRUE(idx < test_pack_get_frame_num(pack));

    if(0 == idx)
    {
        memcpy(pack->buff, test_head_get(&pack->head), TEST_HEAD_LEN);
        pos = TEST_HEAD_LEN;
    }
    memset(pack->buff + pos, (uint8_t)(idx + 1), TEST_FRAME_SIZE - pos);
    *buff = pack->buff;
    if((test_pack_get_frame_num(pack) - 1) == idx)
    {
        *len = TEST_TOTAL_LEN - ((TEST_TOTAL_LEN / TEST_FRAME_SIZE) * TEST_FRAME_SIZE);
    }
    else
    {
        *len = TEST_FRAME_SIZE;
    }



}

void test_pack_calc_checksum(test_pack_s *pack)
{
#if (FOTA_PACK_CHECKSUM == FOTA_PACK_SHA256)
    mbedtls_sha256_context sha256_context;


    mbedtls_sha256_init(&sha256_context);
    mbedtls_sha256_starts(&sha256_context, false);

    uint32_t frame_num = test_pack_get_frame_num(pack);
    for(uint32_t i = 0; i < frame_num; i++)
    {
        uint8_t *buff = NULL;
        uint32_t len = 0;

        test_pack_get_frame(pack, i, &buff, &len);
        mbedtls_sha256_update(&sha256_context, buff, len);
    }

    mbedtls_sha256_finish(&sha256_context, test_head_get(&pack->head) + TEST_CHECKSUM_POS);
    mbedtls_sha256_free(&sha256_context);
#endif
}
void test_pack_init(test_pack_s *pack)
{
    memset(pack, 0, sizeof(*pack));
    test_head_init(&pack->head, TEST_TOTAL_LEN);
    pack->buff = MALLOC(TEST_FRAME_SIZE);
    EXPECT_TRUE(pack->buff != NULL);
    test_pack_calc_checksum(pack);
}


static void test_fota_clear_pack_flash(uint32_t frame_num)
{
    uint8_t *tmp_data = test_fota_create_data(g_test_fota.ota_opt.flash_block_size, 0);

    uint32_t len = frame_num * TEST_FRAME_SIZE;
    uint32_t offset = 0;

    while(len > 0)
    {
        int ret;
        ret = g_test_fota.ota_opt.write_flash(OTA_FULL_SOFTWARE, tmp_data, g_test_fota.ota_opt.flash_block_size,
                offset);
        EXPECT_TRUE(TEST_OK == ret);
        offset += g_test_fota.ota_opt.flash_block_size;
        if (len > g_test_fota.ota_opt.flash_block_size)
        {
            len -= g_test_fota.ota_opt.flash_block_size;
        }
        else
        {
            len = 0;
        }
    }
     FREE(tmp_data);
}


static void test_fota_check_pack(test_pack_s *pack)
{
    uint32_t read_len;
    uint32_t frame_num = test_pack_get_frame_num(pack);
    int ret;
    uint8_t *tmp_data = test_fota_create_data(TEST_FRAME_SIZE, 0);

    EXPECT_TRUE(tmp_data != NULL);

    for(uint32_t i = 0; i < frame_num; i++)
    {
        uint8_t *buff = NULL;
        uint32_t len = 0;
        uint32_t offset;

        test_pack_get_frame(pack, i, &buff, &len);
        if(i == 0)
        {
            offset = 0;
            read_len = TEST_FRAME_SIZE - TEST_HEAD_LEN;
        }
        else if(i != (frame_num -1))
        {
            offset =  i *TEST_FRAME_SIZE - TEST_HEAD_LEN;
            read_len = TEST_FRAME_SIZE;
        }
        else
        {
            offset =  i *TEST_FRAME_SIZE - TEST_HEAD_LEN;
            read_len = TEST_DATA_LEN - offset;
        }
        ret = hal_spi_flash_read(tmp_data, read_len, offset + OTA_IMAGE_DOWNLOAD_ADDR);
        EXPECT_TRUE(TEST_OK == ret);
        if(i == 0)
        {
            ret = memcmp(buff + TEST_HEAD_LEN, tmp_data, read_len);
        }
        else
        {
            ret = memcmp(buff, tmp_data, read_len);
        }
     /*   TEST_LOG("i %d", i);
        if(i == 5)
        {
            TEST_LOG("ssssa");
        }*/
        EXPECT_TRUE(0 == ret);

    }
    FREE(tmp_data);
}

static void test_software_write_success_with_flag(bool break_flag)
{
    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    test_pack_s pack;
    test_pack_init(&pack);

    uint32_t frame_num = test_pack_get_frame_num(&pack);
    test_fota_clear_pack_flash(frame_num);

    int ret;
    uint32_t offset = 0;
    for(uint32_t i = 0; i < frame_num; i++)
    {
        uint8_t *buff = NULL;
        uint32_t len = 0;
        test_pack_get_frame(&pack, i, &buff, &len);
        ret = device->write_software(device,  offset, buff, len);
        offset += len;
        EXPECT_TRUE(TEST_OK == ret);

        if(break_flag && i == 4)
        {
            ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_FAIL, 0);
            EXPECT_TRUE(TEST_OK == ret);
        }
    }

    ret =device->write_software_end(device, ATINY_FOTA_DOWNLOAD_OK, offset);
    EXPECT_TRUE(TEST_OK == ret);

    test_fota_check_pack(&pack);

    test_pack_destroy(&pack);
}


/* write head and data finish */
static void test_software_write_success()
{
    test_software_write_success_with_flag(false);
}


/* write head and part data then break and resume success*/
static void test_software_write_break_and_success()
{
    test_software_write_success_with_flag(true);
}



/* write data not continuous */
static void test_software_write_data_not_continuous()
{
    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    test_pack_s pack;
    test_pack_init(&pack);

    uint8_t *buff = NULL;
    uint32_t len = 0;
    test_pack_get_frame(&pack, 0, &buff, &len);

    int ret = device->write_software(device,  0, buff, 60);
    EXPECT_TRUE(TEST_OK == ret);
    ret = device->write_software(device,  50, buff + 50, len);
    EXPECT_TRUE(TEST_OK == ret);

    uint8_t *tmp_data = test_fota_create_data(TEST_FRAME_SIZE, 0);
    ret = hal_spi_flash_read(tmp_data, TEST_FRAME_SIZE - TEST_HEAD_LEN,  OTA_IMAGE_DOWNLOAD_ADDR);

    ret = memcmp(buff + TEST_HEAD_LEN, tmp_data, TEST_FRAME_SIZE - TEST_HEAD_LEN);
    EXPECT_TRUE(0 == ret);
}


static void test_software_check_sha256()
{
   const uint8_t buff[] = { 0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x53,0x00,0x00,0x00,0x5a,0x00,0x64,0x00,0x15,
        0x4c,0x69,0x74,0x65,0x4f,0x53,0x56,0x31,0x52,0x32,0x43,0x35,0x31,0x30,0x53,0x50,
        0x43,0x30,0x30,0xff,0xee,0x00,0x65,0x00,0x06,0x64,0x9c,0x12,0x34,0x56,0x78,0x00,
        0x01,0x00,0x20,0xc7,0x4c,0xd9,0xd7,0x93,0x27,0x86,0xe3,0x56,0xc1,0xa5,0x74,0x6a,
        0x2c,0xb1,0x79,0xae,0x18,0xff,0x86,0xc6,0x29,0x3c,0xbb,0xa8,0xda,0x43,0x18,0x47,
        0x8d,0x0b,0x39,0x61,0x31,0x32,0x00,0x01,0x73,0x64
    };

    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);


    int ret = device->write_software(device,  0, buff, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);
    ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_OK, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);
}


/* write software big data */
static void test_software_write_big_data_success()
{
    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    test_pack_s pack;
    test_pack_init(&pack);

    uint32_t frame_num = test_pack_get_frame_num(&pack);
    test_fota_clear_pack_flash(frame_num);

    int ret;
    uint32_t offset = 0;
    uint8_t *big_data = test_fota_create_data(9 * TEST_FRAME_SIZE, 0);
    uint32_t big_len = 0;
    for(uint32_t i = 0; i < frame_num; i++)
    {
        uint32_t idx = (i % 10);
        uint8_t *buff = NULL;
        uint32_t len = 0;
        test_pack_get_frame(&pack, i, &buff, &len);

        if(0 == idx)
        {
            ret = device->write_software(device,  offset, buff, len);
            EXPECT_TRUE(TEST_OK == ret);
            offset += len;
        }
        else
        {
            EXPECT_TRUE(big_len + len <= 9 * TEST_FRAME_SIZE);
            memcpy(big_data + big_len, buff, len);
            big_len += len;
            if(idx == 9 || i == (frame_num - 1))
            {
                ret = device->write_software(device,  offset, big_data, big_len);
                EXPECT_TRUE(TEST_OK == ret);
                offset += big_len;
                big_len = 0;
            }
        }

    }
    FREE(big_data);

    ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_OK, offset);
    EXPECT_TRUE(TEST_OK == ret);

    test_fota_check_pack(&pack);

    test_pack_destroy(&pack);
}


/* write software success mutiple times */
static void test_software_mutiple_big_data_success(void)
{
    for(int i = 0 ; i < 10; i++)
    {
        TEST_LOG("multiple software %d begin", i);
        test_software_write_big_data_success();
        TEST_LOG("multiple software %d end", i);
    }
}
int mbedtls_rsa_self_test( int verbose );

static void test_software_check_sha256_rsa2048()
{
#if 0

    mbedtls_rsa_self_test(1);
#else
   static uint8_t buff[] = {
        0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x33,0x00,0x00,0x01,0x3b,0x00,0x64,0x00,0x15,
        0x4c,0x69,0x74,0x65,0x4f,0x53,0x56,0x31,0x52,0x32,0x43,0x35,0x31,0x30,0x53,0x50,
        0x43,0x30,0x30,0xff,0xee,0x00,0x65,0x00,0x06,0x64,0x9c,0x12,0x34,0x56,0x78,0x00,
        0x03,0x01,0x00,0x7d,0x95,0xa0,0xf4,0x6f,0x08,0xfc,0xb6,0x66,0xa6,0xe2,0xeb,0xee,
        0xc0,0x54,0x43,0xff,0x77,0x0b,0x35,0xef,0x2b,0x3e,0x26,0xa3,0x70,0x59,0x3d,0x22,
        0x6a,0xfc,0xa7,0x1b,0x92,0x8b,0x85,0x9b,0x77,0xa9,0x69,0x89,0xea,0x9f,0x0d,0x7d,
        0xab,0x4c,0x53,0x69,0xeb,0xc0,0xeb,0x2f,0xac,0x71,0x06,0x73,0x8d,0x3a,0xe9,0x6c,
        0x4a,0x5c,0x26,0xc7,0x5f,0x7b,0xd1,0x71,0x92,0x43,0x86,0xc8,0x19,0x51,0xe0,0xbd,
        0xb4,0xf7,0x20,0xc9,0x42,0x3a,0xdd,0x6b,0x8b,0xb9,0x32,0xb1,0xaf,0x95,0x5f,0xde,
        0x1e,0x5a,0x43,0xf8,0x0e,0x13,0x61,0x12,0xc7,0x1e,0xf0,0xf3,0x6f,0x70,0x6c,0x91,
        0xd4,0x05,0xa1,0xa0,0xc4,0x3b,0xa9,0x14,0x18,0x44,0x84,0x84,0x59,0x32,0x11,0x05,
        0xab,0xe5,0x61,0x7e,0x40,0x49,0x5a,0x36,0x0d,0x77,0x66,0xa4,0xfb,0x53,0xb7,0xde,
        0x3e,0x71,0xbd,0xee,0x7c,0x7e,0x46,0xd8,0x4b,0x6b,0x93,0x49,0xd0,0x74,0xb7,0x49,
        0x82,0x63,0xb3,0x32,0xbd,0x49,0x09,0xa8,0xea,0xf6,0x91,0x0a,0xa2,0xbc,0x18,0x8b,
        0x54,0x72,0x29,0xf2,0xd5,0x31,0x94,0x3a,0x1f,0x72,0x73,0xe2,0x4c,0x78,0x72,0x75,
        0xf1,0x5c,0xb7,0xa6,0x1c,0x36,0x2b,0xbd,0x7a,0xbb,0x9d,0x00,0x2f,0x67,0x0c,0x31,
        0x3a,0x27,0x95,0xa7,0x85,0xde,0x51,0xf2,0xa6,0x13,0x20,0xc2,0xe6,0x95,0x56,0x89,
        0xe9,0xc2,0x8b,0xe1,0x38,0xfd,0x6f,0x54,0x23,0xac,0x1a,0x95,0x57,0xf7,0xf4,0xc7,
        0xf1,0xc3,0xfa,0x45,0x3a,0x90,0x35,0xe7,0x8f,0x8b,0x63,0xc3,0xbe,0xf6,0x4f,0x6a,
        0x48,0x29,0xa1,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38
    };

    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    int ret = device->write_software(device,  0, buff, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);
    ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_OK, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);

    uint8_t tmp = buff[sizeof(buff) - 1];
    buff[sizeof(buff) - 1] = 0xff;
    ret = device->write_software(device,  0, buff, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);
    ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_OK, sizeof(buff));
    EXPECT_TRUE(TEST_OK != ret);
    buff[sizeof(buff) - 1] = tmp;

    ret = device->write_software(device,  0, buff, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);
    ret = device->write_software_end(device, ATINY_FOTA_DOWNLOAD_OK, sizeof(buff));
    EXPECT_TRUE(TEST_OK == ret);
#endif
}

const char *g_rsa_N = "C94BECB7BCBFF459B9A71F12C3CC0603B11F0D3A366A226FD3E73D453F96EFBBCD4DFED6D9F77FD78C3AB1805E1BD3858131ACB5303F61AF524F43971B4D429CB847905E68935C1748D0096C1A09DD539CE74857F9FDF0B0EA61574C5D76BD9A67681AC6A9DB1BB22F17120B1DBF3E32633DCE34F5446F52DD7335671AC3A1F21DC557FA4CE9A4E0E3E99FED33A0BAA1C6F6EE53EDD742284D6582B51E4BF019787B8C33C2F2A095BEED11D6FE68611BD00825AF97DB985C62C3AE0DC69BD7D0118E6D620B52AFD514AD5BFA8BAB998332213D7DBF5C98DC86CB8D4F98A416802B892B8D6BEE5D55B7E688334B281E4BEDDB11BD7B374355C5919BA5A9A1C91F";
const char *g_rsa_E = "10001";

static void test_mbedtsl_rsa()
{

    mbedtls_rsa_context dtls_rsa;
    uint8_t real_sha256[32];


    mbedtls_rsa_init(&dtls_rsa, MBEDTLS_RSA_PKCS_V21, 0);
    dtls_rsa.len = 256;
    EXPECT_TRUE(mbedtls_mpi_read_string(&dtls_rsa.N, 16, g_rsa_N) == 0);

    EXPECT_TRUE(mbedtls_mpi_read_string(&dtls_rsa.E, 16, g_rsa_E) == 0);

    EXPECT_TRUE(mbedtls_rsa_check_pubkey(&dtls_rsa) == 0);

    mbedtls_sha256_context sha256_context;
    mbedtls_sha256_init(&sha256_context);
    mbedtls_sha256_starts(&sha256_context, false);
#if 0

    uint8_t buff[] = {'1','2','3','4','5','6','7','8'};


    const uint8_t checksum[] = {
        0x01,0x5d,0x60,0xfe,0xdf,0x7b,0x36,0xce,0xfe,0x18,0x5d,0x1d,0x23,0x07,0x4a,0x3a,
        0x2b,0x0d,0x01,0x90,0x75,0x0b,0xcb,0xa0,0x11,0x25,0x13,0x16,0x43,0x9f,0x94,0x82,
        0x67,0x5a,0xa3,0xef,0x05,0xc2,0x87,0xf9,0x9a,0x28,0x4a,0x75,0x82,0x9f,0xd2,0x18,
        0x0b,0x84,0xf7,0xa7,0x87,0x86,0xf6,0xc5,0x34,0xc1,0x73,0x77,0x16,0x93,0x90,0x9c,
        0x25,0x7f,0x7f,0xca,0x1e,0xea,0x90,0x77,0x67,0x3a,0x4d,0xe2,0xf4,0x31,0xea,0x19,
        0x42,0x3c,0xf0,0x75,0x8a,0x67,0xa3,0x62,0xbf,0x85,0x27,0x16,0xc0,0xe5,0xc4,0x9c,
        0x32,0x27,0xf0,0x9f,0xd4,0xee,0x63,0xef,0x3f,0x7c,0xff,0xcc,0x8f,0xdb,0xc5,0xa2,
        0xed,0x14,0xab,0x8f,0x2e,0x1c,0xca,0x36,0xf1,0x6c,0x10,0x4c,0x8f,0x99,0x7a,0xa1,
        0x5f,0x2e,0xc7,0x18,0x87,0xaf,0x1c,0xb2,0x9f,0x06,0xb2,0x00,0xc7,0x0a,0x94,0xf8,
        0x95,0xc1,0x89,0x4a,0xf6,0x48,0x92,0xc5,0xfc,0x32,0x5a,0x40,0x32,0x54,0x67,0x02,
        0x1e,0xd0,0xa6,0x10,0xb1,0x67,0x99,0x76,0xd9,0x20,0xf3,0xae,0x1a,0x26,0xf3,0x2d,
        0xa1,0x18,0x57,0xfa,0xdd,0xd2,0xfd,0x84,0x56,0x69,0x97,0x05,0xfe,0x18,0x44,0x3f,
        0xd9,0x2a,0xf6,0x6f,0xb8,0xf9,0xb5,0x77,0xe3,0xcb,0xd1,0x65,0x4c,0x5a,0x21,0xcf,
        0x7e,0x33,0x57,0xdf,0xdd,0xb3,0xec,0x3c,0x4b,0xc8,0x64,0xb3,0x3e,0xb0,0x67,0x00,
        0x5a,0xa9,0x6d,0xe8,0xfd,0x2d,0xc4,0x1a,0xce,0xe2,0x76,0x04,0x17,0x91,0x88,0x07,
        0x1f,0x6f,0x00,0xdb,0x10,0x0d,0x07,0x41,0x6f,0xdc,0x40,0xa0,0xca,0xee,0x5e,0xe2
    };



#else
    const uint8_t checksum[] = {0x7d,0x95,0xa0,0xf4,0x6f,0x08,0xfc,0xb6,0x66,0xa6,0xe2,0xeb,0xee,
    0xc0,0x54,0x43,0xff,0x77,0x0b,0x35,0xef,0x2b,0x3e,0x26,0xa3,0x70,0x59,0x3d,0x22,
    0x6a,0xfc,0xa7,0x1b,0x92,0x8b,0x85,0x9b,0x77,0xa9,0x69,0x89,0xea,0x9f,0x0d,0x7d,
    0xab,0x4c,0x53,0x69,0xeb,0xc0,0xeb,0x2f,0xac,0x71,0x06,0x73,0x8d,0x3a,0xe9,0x6c,
    0x4a,0x5c,0x26,0xc7,0x5f,0x7b,0xd1,0x71,0x92,0x43,0x86,0xc8,0x19,0x51,0xe0,0xbd,
    0xb4,0xf7,0x20,0xc9,0x42,0x3a,0xdd,0x6b,0x8b,0xb9,0x32,0xb1,0xaf,0x95,0x5f,0xde,
    0x1e,0x5a,0x43,0xf8,0x0e,0x13,0x61,0x12,0xc7,0x1e,0xf0,0xf3,0x6f,0x70,0x6c,0x91,
    0xd4,0x05,0xa1,0xa0,0xc4,0x3b,0xa9,0x14,0x18,0x44,0x84,0x84,0x59,0x32,0x11,0x05,
    0xab,0xe5,0x61,0x7e,0x40,0x49,0x5a,0x36,0x0d,0x77,0x66,0xa4,0xfb,0x53,0xb7,0xde,
    0x3e,0x71,0xbd,0xee,0x7c,0x7e,0x46,0xd8,0x4b,0x6b,0x93,0x49,0xd0,0x74,0xb7,0x49,
    0x82,0x63,0xb3,0x32,0xbd,0x49,0x09,0xa8,0xea,0xf6,0x91,0x0a,0xa2,0xbc,0x18,0x8b,
    0x54,0x72,0x29,0xf2,0xd5,0x31,0x94,0x3a,0x1f,0x72,0x73,0xe2,0x4c,0x78,0x72,0x75,
    0xf1,0x5c,0xb7,0xa6,0x1c,0x36,0x2b,0xbd,0x7a,0xbb,0x9d,0x00,0x2f,0x67,0x0c,0x31,
    0x3a,0x27,0x95,0xa7,0x85,0xde,0x51,0xf2,0xa6,0x13,0x20,0xc2,0xe6,0x95,0x56,0x89,
    0xe9,0xc2,0x8b,0xe1,0x38,0xfd,0x6f,0x54,0x23,0xac,0x1a,0x95,0x57,0xf7,0xf4,0xc7,
    0xf1,0xc3,0xfa,0x45,0x3a,0x90,0x35,0xe7,0x8f,0x8b,0x63,0xc3,0xbe,0xf6,0x4f,0x6a,
    0x48,0x29,0xa1};

    /*const uint8_t checksum[] = {
          0x20,0xa8,0xcc,0x39,0x38,0x3e,0xf8,0x11,0xbf,0x95,0x01,0x9e,0x60,
          0x4c,0x4f,0x88,0x79,0x5b,0x64,0x9f,0xfb,0xd6,0x50,0xd5,0xdd,0xee,0xf3,0x01,0x96,
          0x2d,0x05,0x43,0x8c,0x61,0xce,0xe5,0x35,0x47,0xec,0x8e,0x05,0xe8,0x78,0x3c,0xe0,
          0x89,0xa1,0x7a,0x4f,0x1e,0x86,0x88,0xb5,0x94,0x9c,0x35,0xf6,0x27,0x4f,0xaa,0xbb,
          0x5d,0xb5,0x3a,0x61,0x92,0x56,0x38,0x78,0x6e,0xa8,0xc7,0xdb,0xfe,0xdb,0x45,0x59,
          0x36,0x9a,0xac,0xc6,0x57,0x76,0xc5,0x03,0x2f,0x32,0x09,0x40,0x67,0x91,0xe7,0x12,
          0xa5,0x79,0x57,0x37,0xea,0xdc,0x53,0x22,0xf9,0xd9,0x09,0xc4,0xec,0x63,0x48,0x2a,
          0xdd,0xc0,0xfb,0x8c,0x2a,0x33,0x71,0x42,0x1a,0xc4,0x70,0xb1,0x09,0xa6,0x8e,0x5e,
          0x3a,0xbd,0x2a,0x6f,0x57,0xa7,0xf5,0xee,0x73,0xe7,0xef,0x7d,0x46,0xd6,0x86,0x47,
          0x3f,0xef,0x7a,0x87,0x69,0x9b,0x01,0xaf,0x0f,0x45,0x60,0xaa,0xc1,0xa5,0xef,0xd4,
          0x4c,0x58,0x3f,0x70,0x37,0xcf,0x4d,0xf7,0xec,0xef,0xaf,0xa9,0xd5,0x26,0x2b,0xc2,
          0xa4,0x21,0xd8,0xbe,0xc4,0x04,0xd1,0x6a,0x79,0xfd,0x98,0xb7,0x3e,0x15,0x02,0x3a,
          0x11,0x00,0xec,0x42,0x77,0x97,0xf8,0x89,0xec,0xf9,0x73,0xe5,0x13,0x93,0x19,0x13,
          0x54,0x59,0x43,0x73,0xda,0x6d,0x90,0x2d,0xb5,0x6e,0xd7,0x2c,0xff,0x8d,0x5f,0x1f,
          0x19,0x35,0x9a,0xde,0xb9,0x98,0xc7,0xfc,0x01,0x6f,0xe2,0xb0,0x41,0x47,0xbd,0x00,
          0xae,0x6d,0x34,0x6b,0x23,0x7c,0x4f,0x4b,0x6f,0xeb,0x8f,0xd2,0xa4,0xe3,0x3f,0xcd,
          0x8f,0x2c,0x8c};*/
     static uint8_t buff[] = {
          0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x33,0x00,0x00,0x01,0x3b,0x00,0x64,0x00,0x15,
          0x4c,0x69,0x74,0x65,0x4f,0x53,0x56,0x31,0x52,0x32,0x43,0x35,0x31,0x30,0x53,0x50,
          0x43,0x30,0x30,0xff,0xee,0x00,0x65,0x00,0x06,0x64,0x9c,0x12,0x34,0x56,0x78,0x00,
          0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
          0x00,0x00,0x00,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38
      };
#endif
    mbedtls_sha256_update(&sha256_context, buff, sizeof(buff));
    mbedtls_sha256_finish(&sha256_context, real_sha256);
    EXPECT_TRUE(mbedtls_rsa_pkcs1_verify(&dtls_rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, 0, real_sha256, checksum) == 0);

#if 1
    mbedtls_sha256_init(&sha256_context);
    mbedtls_sha256_starts(&sha256_context, false);
    uint8_t tmp = buff[0];
    buff[0] = 0xff;
    mbedtls_sha256_update(&sha256_context, buff, sizeof(buff));
    mbedtls_sha256_finish(&sha256_context, real_sha256);
    EXPECT_TRUE(mbedtls_rsa_pkcs1_verify(&dtls_rsa, NULL, NULL, MBEDTLS_RSA_PUBLIC, MBEDTLS_MD_SHA256, 0, real_sha256, checksum) != 0);
#endif
    buff[0] = tmp;
    mbedtls_rsa_free( &dtls_rsa );
}

#if 0
static void setup()
{

//    test_fota_s *test_fota = &g_test_fota;
    const char *rsa_N = g_rsa_N;
    const char *rsa_E = g_rsa_E;


    TEST_LOG("test fota ver09");
    //dtls_int();
#if 0
    cm_backtrace_init("test", "11", "22");
    UINTPTR uvIntSave = LOS_IntLock();
    osSetVector(-13, HardFault_Handler_bt);
    LOS_IntRestore(uvIntSave);
#endif

    TEST_LOG("haha_flag %d", haha_flag);

   // test_fota->init_flag = true;
/*
    void (*f)(void) = (void (*)(void))0x3;
    f();*/

    int ret = hal_init_fota();
    EXPECT_TRUE(TEST_OK == ret);

    atiny_fota_storage_device_s *device = fota_get_pack_device();
    EXPECT_TRUE(device != NULL);

    atiny_fota_storage_device_s *storage_device = NULL;
    fota_hardware_s *hardware = NULL;
    ret = hal_get_fota_device(&storage_device, &hardware);
    EXPECT_TRUE(TEST_OK == ret);
    EXPECT_TRUE(storage_device != NULL);
    EXPECT_TRUE(hardware != NULL);

    fota_pack_device_info_s device_info;
    device_info.storage_device = storage_device;
    device_info.hardware = hardware;
    device_info.head_info_notify = NULL;
    device_info.key.rsa_N = rsa_N;
    device_info.key.rsa_E = rsa_E;
    ret = fota_set_pack_device(device, &device_info);
    EXPECT_TRUE(TEST_OK == ret);
    g_test_fota.storage_device = storage_device;
}
#endif

static int test_fota_flag_read(void* buf, int32_t len)
{
    int (*read_flash)(ota_flash_type_e type, void *buf, int32_t len, uint32_t location) =
                g_test_fota.ota_opt.read_flash;
    if (read_flash)
    {
        return read_flash(OTA_UPDATE_INFO, buf, len, 0);
    }
    return -1;
}

static int test_fota_flag_write(const void* buf, int32_t len)
{
    int (*write_flash)(ota_flash_type_e type, const void *buf, int32_t len, uint32_t location) =
                g_test_fota.ota_opt.write_flash;

    if (write_flash)
    {
        return write_flash(OTA_UPDATE_INFO, buf, len, 0);
    }
    return -1;
}


static void setup()
{
//    test_fota_s *test_fota = &g_test_fota;
    const char *rsa_N = g_rsa_N;
    const char *rsa_E = g_rsa_E;

    hal_init_ota();

    hal_get_ota_opt(&g_test_fota.ota_opt);
    g_test_fota.ota_opt.key.rsa_N = rsa_N;
    g_test_fota.ota_opt.key.rsa_E = rsa_E;

    flag_op_s flag_op;
    flag_op.func_flag_read = test_fota_flag_read;
    flag_op.func_flag_write = test_fota_flag_write;
    (void)flag_init(&flag_op);

    int ret = flag_upgrade_init();
    EXPECT_TRUE(ret == 0);


    TEST_LOG("test fota ver09");
    //dtls_int();
#if 0
    cm_backtrace_init("test", "11", "22");
    UINTPTR uvIntSave = LOS_IntLock();
    osSetVector(-13, HardFault_Handler_bt);
    LOS_IntRestore(uvIntSave);
#endif

    TEST_LOG("setup");

   // test_fota->init_flag = true;
/*
    void (*f)(void) = (void (*)(void))0x3;
    f();*/




    ret = ota_init_pack_device(&g_test_fota.ota_opt);
    EXPECT_TRUE(TEST_OK == ret);

}




/* test big size */

void test_init(void)
{

    test_class_s * test_class;
    TEST_DECLARE_CLASS(test_class, "fota");
    test_class_set(test_class, setup, NULL, NULL, NULL);
#if (FOTA_PACK_CHECKSUM == FOTA_PACK_SHA256_RSA2048)
    TEST_ADD_CASE(test_class, test_mbedtsl_rsa);
    TEST_ADD_CASE(test_class, test_software_check_sha256_rsa2048);
#elif (FOTA_PACK_CHECKSUM == FOTA_PACK_SHA256)

    TEST_ADD_CASE(test_class, testFlashRead);
    //TEST_ADD_CASE(test_class, testMultipleFlashReadWrite);
    TEST_ADD_CASE(test_class, testWriteAndReadUpdataInfo);
    TEST_ADD_CASE(test_class, testWriteAndReadUpdataInfoMultipleBlocks);
    TEST_ADD_CASE(test_class, test_software_check_sha256);
    TEST_ADD_CASE(test_class, test_software_write_part_head_break);
    TEST_ADD_CASE(test_class, test_software_write_head_not_continuous);
    TEST_ADD_CASE(test_class, test_software_write_success);
    TEST_ADD_CASE(test_class, test_software_write_break_and_success);
    TEST_ADD_CASE(test_class, test_software_write_data_not_continuous);
    TEST_ADD_CASE(test_class, test_software_write_big_data_success);
    TEST_ADD_CASE(test_class, test_software_mutiple_big_data_success);
#endif
}


