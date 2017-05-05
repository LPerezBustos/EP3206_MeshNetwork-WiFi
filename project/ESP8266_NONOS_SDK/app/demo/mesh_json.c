#include "mem.h"
#include "mesh.h"
#include "osapi.h"
#include "mesh_parser.h"

extern struct espconn g_ser_conn; // TCP socket from "demo_mesh.c"

void ICACHE_FLASH_ATTR
mesh_json_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len){
    // parser for json mesh packages
    uint8_t *src = NULL, *dst = NULL;
    if (!espconn_mesh_get_src_addr((struct mesh_header_format *)mesh_header, &src) ||
            !espconn_mesh_get_dst_addr((struct mesh_header_format *)mesh_header, &dst)) {
        MESH_PARSER_PRINT("get address have failed\n");
    }
    MESH_PARSER_PRINT("%s is running\n", __func__);
    MESH_PARSER_PRINT("mesh package from " MACSTR " to " MACSTR "\n",
            MAC2STR((uint8_t *)src), MAC2STR((uint8_t *)dst));
    MESH_PARSER_PRINT("data: %s\n", pdata);
}

void ICACHE_FLASH_ATTR
mesh_json_bcast_test(){
    char buf[64];
    uint8_t src[6];
    uint8_t dst[6];
    struct mesh_header_format *header = NULL;

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PARSER_PRINT("get station_mac_address of current node have failed\n");
        return;
    }

    os_memset(buf, 0, sizeof(buf));
    os_sprintf(buf, "%s", "{\"using json bcast protocol\":\"");
    os_sprintf(buf + os_strlen(buf), MACSTR, MAC2STR(src));
    os_sprintf(buf + os_strlen(buf), "%s", "\"}\r\n");

    os_memset(dst, 0, sizeof(dst));
    // use bcast to get all the devices working in mesh from root.
    // bcast_address = 00:00:00:00:00:00
    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,     // destiny address (bcast)
                            src,     // source address
                            false,   // not p2p packet
                            true,    // piggyback congest request
                            M_PROTO_JSON,   // packe with JSON format
                            os_strlen(buf), // data length
                            false,   // no option
                            0,       // option total len
                            false,   // no frag
                            0,       // frag type, this packet doesn't use frag
                            false,   // more frag
                            0,       // frag index
                            0);      // frag length
    if (!header) {
        MESH_PARSER_PRINT("bcast create packet have failed\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, buf, os_strlen(buf))) {
        MESH_DEMO_PRINT("bcast set user data have failed\n");
        MESH_DEMO_FREE(header); // free header memory
        return;
    }

    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
        MESH_DEMO_PRINT("bcast_mesh is busy right now\n");
        MESH_DEMO_FREE(header); // free header memory
        espconn_mesh_connect(&g_ser_conn); // trying to connect again
        return;
    }

    MESH_DEMO_FREE(header); // free header memory
}

void ICACHE_FLASH_ATTR
mesh_json_p2p_test(){
    uint16_t idx;
    char buf[64];
    uint8_t src[6];
    uint8_t dst[6];
    uint16_t dev_count = 0;
    struct mesh_header_format *header = NULL;
    struct mesh_device_mac_type *list = NULL;

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PARSER_PRINT("get station_mac_address of current node have failed\n");
        return;
    }

    if (!mesh_device_get_mac_list((const struct mesh_device_mac_type **)&list, &dev_count)) {
        MESH_PARSER_PRINT("p2p get mac_address_list have failed\n");
        return;
    }

    // if device count in mac list is more than 1,
    // we select one device random as destination device, otherwise, we use root as destination.
    idx = dev_count > 1 ? (os_random() % (dev_count + 1)) : 0;
    if (!idx) { // idx == 0
        if (!mesh_device_get_root((const struct mesh_device_mac_type **)&list))
            return; // mesh_device_get_root have failed
        os_memcpy(dst, list, sizeof(dst)); // select ROOT_NODE
    } else { // idx != 0
        os_memcpy(dst, list + idx - 1, sizeof(dst)); // select random destiny
    }

    MESH_PARSER_PRINT("%s is running\n", __func__); // random json p2p destiny
    MESH_PARSER_PRINT("sending json p2p package to... " MACSTR "\n", MAC2STR(dst));

    os_memset(buf, 0, sizeof(buf));
    os_sprintf(buf, "%s", "{\"using json p2p protocol\":\"");
    os_sprintf(buf + os_strlen(buf), MACSTR, MAC2STR(src));
    os_sprintf(buf + os_strlen(buf), "%s", "\"}\r\n");

    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,     // destiny address
                            src,     // source address
                            true,    // not p2p packet
                            true,    // piggyback congest request
                            M_PROTO_JSON,        // packe with JSON format
                            os_strlen(buf),// data length
                            false,   // no option
                            0,       // option total len
                            false,   // no frag
                            0,       // frag type, this packet doesn't use frag
                            false,   // more frag
                            0,       // frag index
                            0);      // frag length
    if (!header) {
        MESH_PARSER_PRINT("p2p create packet have failed\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, buf, os_strlen(buf))) {
        MESH_DEMO_PRINT("p2p set user data have failed\n");
        MESH_DEMO_FREE(header); // free header memory
        return;
    }

    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
        MESH_DEMO_PRINT("p2p mesh is busy right now\n");
        MESH_DEMO_FREE(header); // free header memory
        espconn_mesh_connect(&g_ser_conn); // trying to connect again
        return;
    }

    MESH_DEMO_FREE(header); // free header memory
}

void ICACHE_FLASH_ATTR
mesh_json_mcast_test()
{
    char buf[32];
    uint8_t src[6];
    uint16_t i = 0;
    uint16_t ot_len = 0, op_count = 0;
    uint16_t dev_count = 0, max_count = 0;
    struct mesh_header_format *header = NULL;
    struct mesh_header_option_format *option = NULL;
    uint8_t dst[6] = {0x01, 0x00, 0x5E, 0x00, 0x00, 0x00}; // mcast address
    struct mesh_device_mac_type *list = NULL, *root = NULL;

    if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_PARSER_PRINT("get station_mac_address of current node have failed\n");
        return;
    }

    if (!mesh_device_get_mac_list((const struct mesh_device_mac_type **)&list, &dev_count)) {
        MESH_PARSER_PRINT("mcast get mac_address_list have failed\n");
        return;
    }

    if (!mesh_device_get_root((const struct mesh_device_mac_type **)&root)) {
        MESH_PARSER_PRINT("mcast get root have failed\n");
        return;
    }

    dev_count ++;
    // *************************************************************************
    // ****** if it is possible, we select all nodes as one mcast group ********
    // *************************************************************************
    os_memset(buf, 0, sizeof(buf));
    os_sprintf(buf, "%s", "{\"mcast\":\"");
    os_sprintf(buf + os_strlen(buf), MACSTR, MAC2STR(src));
    os_sprintf(buf + os_strlen(buf), "%s", "\"}\r\n");

    // rest len for mcast option:
    // +ESP_MESH_PKT_LEN_MAX = 1300
    // -ESP_MESH_HLEN = sizeof(struct mesh_header_format)
    // -buf = usr_data
    max_count = (ESP_MESH_PKT_LEN_MAX - ESP_MESH_HLEN - os_strlen(buf));
    // max mcast option count
    // ESP_MESH_OPTION_MAX_LEN = 255 Bytes
    max_count /= ESP_MESH_OPTION_MAX_LEN;
    // max device count for mcast in current packet
    // take a look at mesh.h:
    // #define ESP_MESH_DEV_MAX_PER_OP ((ESP_MESH_OPTION_MAX_LEN - ESP_MESH_OPTION_HLEN) / ESP_MESH_ADDR_LEN)
    // ESP_MESH_OPTION_HLEN = sizeof(struct mesh_header_option_format) --> size of mesh header option
    max_count *= ESP_MESH_DEV_MAX_PER_OP; // tous les directions mac qui peut etre ajouter dans le package mesh
    if (dev_count > max_count)
    // if MESH_NODES in MAC_ADDRESS_LIST is greater than max number of MAC_ADDRESS in mcast package
        dev_count = max_count;

    op_count = dev_count / ESP_MESH_DEV_MAX_PER_OP; // options that we need to send mcast package
    ot_len = ESP_MESH_OT_LEN_LEN // option total length --> to make mesh packages
             + op_count * (sizeof(*option) + sizeof(*root) * ESP_MESH_DEV_MAX_PER_OP)
             + sizeof(*option) + (dev_count - op_count * ESP_MESH_DEV_MAX_PER_OP) * sizeof(*root);

    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,     // destiny address --> 01:00:5E:00:00:00
                            src,     // source address
                            false,   // not p2p packet
                            true,    // piggyback congest request
                            M_PROTO_JSON,    // package with JSON format
                            os_strlen(buf),  // data length
                            true,    // option exist
                            ot_len,  // option total len
                            false,   // no frag
                            0,       // frag type, this packet doesn't use frag
                            false,   // more frag
                            0,       // frag index
                            0);      // frag length
    if (!header) {
        MESH_PARSER_PRINT("mcast create packet have failed\n");
        return;
    }

    while (i < op_count) {
    // se agregan MAC_ADDRESS en MAC_ADDRESS_LIST al json mcast package
        option = (struct mesh_header_option_format *)espconn_mesh_create_option(
                M_O_MCAST_GRP, (uint8_t *)(list + i * ESP_MESH_DEV_MAX_PER_OP),
                (uint8_t)(sizeof(*root) * ESP_MESH_DEV_MAX_PER_OP));
        if (!option) {
            MESH_PARSER_PRINT("mcast create option %d have failed\n", i);
            goto MCAST_FAIL; // <-- if there's an error, go to this code
        } else{
          MESH_PARSER_PRINT("mcast create option %d have been succesfull\n", i);
        }

        if (!espconn_mesh_add_option(header, option)) {
            MESH_PARSER_PRINT("mcast %d set option fail\n", i);
            goto MCAST_FAIL; // <-- if there's an error, go to this code
        }

        i ++; // if we need to fill another option
        MESH_DEMO_FREE(option); // free some space in memory
        option = NULL;
    }

    {
        char *rest_dev = NULL; // if there's other MAC_ADDRESS to add
        uint8_t rest = dev_count - op_count * ESP_MESH_DEV_MAX_PER_OP;
        if (rest > 0) { // <-- run this code
            rest_dev = (char *)os_zalloc(sizeof(*root) * rest);
            if (!rest_dev) {
                MESH_PARSER_PRINT("mcast alloc the last option buf have failed\n");
                goto MCAST_FAIL; // <-- if there's an error, go to this code
            }
            os_memcpy(rest_dev, root, sizeof(*root));
            if (list && rest > 1)
                os_memcpy(rest_dev + sizeof(*root),
                        list + i * ESP_MESH_DEV_MAX_PER_OP,
                        (rest - 1) * sizeof(*root));
            option = (struct mesh_header_option_format *)espconn_mesh_create_option(
                    M_O_MCAST_GRP, rest_dev, sizeof(*root) * rest);
            MESH_DEMO_FREE(rest_dev); // free some memory
            if (!option) {
                MESH_PARSER_PRINT("mcast create the last option have failed\n");
                goto MCAST_FAIL; // <-- if there's an error, go to this code
            }

            if (!espconn_mesh_add_option(header, option)) {
                MESH_PARSER_PRINT("mcast set the last option have failed\n");
                goto MCAST_FAIL; // <-- if there's an error, go to this code
            }
        }
    }

    if (!espconn_mesh_set_usr_data(header, buf, os_strlen(buf))) {
        MESH_DEMO_PRINT("mcast set_user data have failed\n");
        goto MCAST_FAIL; // <-- if there's an error, go to this code
    }

    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
        MESH_DEMO_PRINT("mcast mesh is busy right now\n");
        espconn_mesh_connect(&g_ser_conn); // trying to connect again
    }

MCAST_FAIL: // <-- if there's an error, go to this code
    option ? MESH_DEMO_FREE(option) : 0;
    header ? MESH_DEMO_FREE(header) : 0;
}

void ICACHE_FLASH_ATTR mesh_json_mcast_test_init(){
    // mcast_test timer configuration
    // take a look at "user_config.h"
    static os_timer_t mcast_timer;
    os_timer_disarm(&mcast_timer);
    os_timer_setfn(&mcast_timer, (os_timer_func_t *)mesh_json_mcast_test, NULL);
    // cada json_mcast_test_time=XXXXXms se llama a "mesh_json_mcast_test()"
    os_timer_arm(&mcast_timer, json_mcast_test_time, true);
}

void ICACHE_FLASH_ATTR mesh_json_bcast_test_init(){
    // bcast_test timer configuration
    // take a look at "user_config.h"
    static os_timer_t bcast_timer;
    os_timer_disarm(&bcast_timer);
    os_timer_setfn(&bcast_timer, (os_timer_func_t *)mesh_json_bcast_test, NULL);
    // cada json_bcast_test_time=XXXXXms se llama a "mesh_json_bcast_test()"
    os_timer_arm(&bcast_timer, json_bcast_test_time, true);
}

void ICACHE_FLASH_ATTR mesh_json_p2p_test_init(){
    // p2p_test timer configuration
    // take a look at "user_config.h"
    static os_timer_t p2p_timer;
    os_timer_disarm(&p2p_timer);
    os_timer_setfn(&p2p_timer, (os_timer_func_t *)mesh_json_p2p_test, NULL);
    // cada json_p2p_test_time=XXXXXms se llama a "mesh_json_p2p_test()"
    os_timer_arm(&p2p_timer, json_p2p_test_time, true);
}
