#include <stdlib.h>
#include "mem.h"
#include "mesh.h"
#include "osapi.h"
#include "mesh_parser.h"
#include "esp_touch.h"

#define MESH_DEMO_PRINT  ets_printf // some mesh definitions
#define MESH_DEMO_STRLEN ets_strlen
#define MESH_DEMO_MEMCPY ets_memcpy
#define MESH_DEMO_MEMSET ets_memset
#define MESH_DEMO_FREE   os_free
#define MESH_DEMO_ZALLOC os_zalloc
#define MESH_DEMO_MALLOC os_malloc

static esp_tcp ser_tcp; // TCP socket info
struct espconn g_ser_conn; // socket struct

void user_init(void); // some "mesh_demo.c" headers
void esp_mesh_demo_test();
void mesh_enable_cb(int8_t res);
void esp_mesh_demo_con_cb(void *);
void esp_recv_entrance(void *, char *, uint16_t);

void user_rf_pre_init(void){} // don't delete this two
void user_pre_init(void){}

void ICACHE_FLASH_ATTR esp_recv_entrance(void *arg, char *pdata, uint16_t len){
    uint8_t *src = NULL, *dst = NULL;
    uint8_t *resp = "{\"received, thanks\"}";
    struct mesh_header_format *header = (struct mesh_header_format *)pdata;

    MESH_DEMO_PRINT("\npacket received\n");

    if (!pdata) // return if there isn't data
        return;

    mesh_packet_parser(arg, pdata, len);

    // then build response packet, the following code is response demo
    // *************************************************************************
    // ****************** Here's for application code **************************
    // *************************************************************************
//#if 1
#if 0
    if (!espconn_mesh_get_src_addr(header, &src) || !espconn_mesh_get_dst_addr(header, &dst)) {
        MESH_DEMO_PRINT("get addr fail\n");
        return;
    }

    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            src,   // destiny address
                            dst,   // source address
                            false, // not p2p packet
                            true,  // piggyback congest request
                            M_PROTO_JSON,  // packe with JSON format
                            MESH_DEMO_STRLEN(resp),  // data length
                            false, // no option
                            0,     // option len
                            false, // no frag
                            0,     // frag type, this packet doesn't use frag
                            false, // more frag
                            0,     // frag index
                            0);    // frag length
    if (!header) {
        MESH_DEMO_PRINT("create packet fail\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, resp, MESH_DEMO_STRLEN(resp))) {
        MESH_DEMO_PRINT("set user data fail\n");
        MESH_DEMO_FREE(header);
        return;
    }

    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
        MESH_DEMO_PRINT("mesh resp is busy\n");
        MESH_DEMO_FREE(header);
        return;
    }

    MESH_DEMO_FREE(header);
#endif
}

void ICACHE_FLASH_ATTR esp_mesh_demo_con_cb(void *arg)
{
    static os_timer_t tst_timer;
    struct espconn *server = (struct espconn *)arg;

    MESH_DEMO_PRINT("server_connection function callback is running\n");

    if (server != &g_ser_conn) {
        MESH_DEMO_PRINT("server_connection error, return\n");
        return;
    }

    os_timer_disarm(&tst_timer);
    os_timer_setfn(&tst_timer, (os_timer_func_t *)esp_mesh_demo_test, NULL);
    // cada application_time=XXXXXms se llama a "esp_mesh_demo_test()"
    os_timer_arm(&tst_timer, application_time, true); // take a look at "user_config.h"
}

void ICACHE_FLASH_ATTR mesh_enable_cb(int8_t res)
{
	MESH_DEMO_PRINT("mesh is enabling right now\n");

    if (res == MESH_OP_FAILURE) {
        MESH_DEMO_PRINT("enable mesh have failed, re-enabling\n");
        espconn_mesh_enable(mesh_enable_cb, MESH_ONLINE); // trying again
        return;
    }

    if (espconn_mesh_get_usr_context() &&
        espconn_mesh_is_root() && // if current node is ROOT_NODE and MESH_LOCAL is enable
        res == MESH_LOCAL_SUC)
        goto TEST_SCENARIO; // <-- goto this part of code

    // if you want to sent packet to one node in mesh, please build p2p packet
    // if you want to sent packet to server/mobile, please build normal packet (uincast packet)
    // if you want to sent bcast/mcast packet, please build bcast/mcast packet

    MESH_DEMO_MEMSET(&g_ser_conn, 0 ,sizeof(g_ser_conn));
    MESH_DEMO_MEMSET(&ser_tcp, 0, sizeof(ser_tcp));

    MESH_DEMO_MEMCPY(ser_tcp.remote_ip, server_ip, sizeof(server_ip));
    ser_tcp.remote_port = server_port; // take a look at "user_config.h"
    ser_tcp.local_port = espconn_port(); // find an available port
    g_ser_conn.proto.tcp = &ser_tcp;

    if (espconn_regist_connectcb(&g_ser_conn, esp_mesh_demo_con_cb)) {
        MESH_DEMO_PRINT("set up connection_callback error\n");
        espconn_mesh_disable(NULL);
        return;
    }

    if (espconn_regist_recvcb(&g_ser_conn, esp_recv_entrance)) {
        MESH_DEMO_PRINT("set up received_callback error\n");
        espconn_mesh_disable(NULL);
        return;
    }

    // regist the other callback: sent_cb, reconnect_cb, disconnect_cb
    // if you donn't need the above cb, you donn't need to register then

    // *************************************************************************
    // ****************** Here's for application code **************************
    // *************************************************************************

    if (espconn_mesh_connect(&g_ser_conn)) {
        MESH_DEMO_PRINT("connection error, switching ROOT_NODE to MESH_LOCAL\n");
        if (espconn_mesh_is_root())
            espconn_mesh_enable(mesh_enable_cb, MESH_LOCAL);
        else
            espconn_mesh_enable(mesh_enable_cb, MESH_ONLINE);
        return;
    }

TEST_SCENARIO:
    mesh_device_list_init(); // inicializar MAC_ADDRESS_LIST
    mesh_topo_test_init(); // topo request timer
    //mesh_json_mcast_test_init(); // json mcast init
    //mesh_json_bcast_test_init(); // json bcast init
    //mesh_json_p2p_test_init(); // json ucast init
}

void ICACHE_FLASH_ATTR esp_mesh_demo_test(){
    // *************************************************************************
    // ****************** Here's for application code **************************
    // *************************************************************************
    char tst_data[64];
    uint8_t src[6];
    uint8_t dst[6];
    struct ip_info ipconfig;
    struct mesh_header_format *header = NULL;

    wifi_get_ip_info(STATION_IF, &ipconfig);
    wifi_get_macaddr(STATION_IF, src);

    // this is ucast test case:
    // the mesh data can be any content, it can be string(json/http), or binary(MQTT)
    MESH_DEMO_MEMSET(tst_data, 0, sizeof(tst_data));
    uint8_t random_number = rand() % 10 + 20;
    os_sprintf(tst_data, "%s", "The temperature is ");
    os_sprintf(tst_data + MESH_DEMO_STRLEN(tst_data), "%d", random_number);
    os_sprintf(tst_data + MESH_DEMO_STRLEN(tst_data), "%s", "°C\r\n");
    MESH_DEMO_PRINT(tst_data);

    // *********************** MESSAGE IN DEMO *********************************
    /*os_sprintf(tst_data, "%s", "Hello World ! I'm ");
    os_sprintf(tst_data + MESH_DEMO_STRLEN(tst_data), MACSTR, MAC2STR(src));
    os_sprintf(tst_data + MESH_DEMO_STRLEN(tst_data), "%s", " with ip address ");
    os_sprintf(tst_data + MESH_DEMO_STRLEN(tst_data), IPSTR, IP2STR(&ipconfig.ip));
    os_sprintf(tst_data + MESH_DEMO_STRLEN(tst_data), "%s", "\r\n");*/
    MESH_DEMO_PRINT("\nEspacio libre en el stack de memoria: %u \n", system_get_free_heap_size());

    /*if (!wifi_get_macaddr(STATION_IF, src)) {
        MESH_DEMO_PRINT("get station MAC_ADDRESS have failed\n");
        return;
    }*/
    MESH_DEMO_MEMCPY(dst, server_ip, sizeof(server_ip));
    MESH_DEMO_MEMCPY(dst + sizeof(server_ip), &server_port, sizeof(server_port));

    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,   // destiny address
                            src,   // source address
                            false, // not p2p packet
                            true,  // piggyback congest request
                            M_PROTO_JSON,  // packe with JSON format
                            MESH_DEMO_STRLEN(tst_data),  // data length
                            false, // no option
                            0,     // option len
                            false, // no frag
                            0,     // frag type, this packet doesn't use frag
                            false, // more frag
                            0,     // frag index
                            0);    // frag length
    if (!header) {
        MESH_DEMO_PRINT("create packet have failed\n");
        return;
    }

    if (!espconn_mesh_set_usr_data(header, tst_data, MESH_DEMO_STRLEN(tst_data))) {
        MESH_DEMO_PRINT("set user data have failed\n");
        MESH_DEMO_FREE(header);
        return;
    }

    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
        MESH_DEMO_PRINT("ucast mesh is busy\n");
        MESH_DEMO_FREE(header);
        espconn_mesh_connect(&g_ser_conn); // if fail, we re-connect mesh
        return;
    }

    MESH_DEMO_FREE(header); // free mesh_header's memory space
}

void ICACHE_FLASH_ATTR esp_mesh_new_child_notify(void *mac){
    if (!mac)
        return;
    MESH_DEMO_PRINT("new child node:" MACSTR "\n", MAC2STR(((uint8_t *)mac)));
}

bool ICACHE_FLASH_ATTR esp_mesh_demo_init()
{
    espconn_mesh_print_ver(); // print mesh version

    // set the softAP password of MESH_NODE
    // please take a look at "user_config.h"
    if (!espconn_mesh_encrypt_init(MESH_AUTH, MESH_PASSWD, MESH_DEMO_STRLEN(MESH_PASSWD))) {
        MESH_DEMO_PRINT("set pw fail\n");
        return false;
    }

    // if you want set max_hop > 4, please make sure that memory heap is available
    // mac_route_table_size = (4^max_hop - 1)/3 * 6
    if (!espconn_mesh_set_max_hops(MESH_MAX_HOP)) {
        MESH_DEMO_PRINT("fail, max_hop:%d\n", espconn_mesh_get_max_hops());
        return false;
    }


    // mesh_group_id and mesh_ssid_prefix represent mesh network
    if (!espconn_mesh_set_ssid_prefix(MESH_SSID_PREFIX, MESH_DEMO_STRLEN(MESH_SSID_PREFIX))) {
        MESH_DEMO_PRINT("set mesh_ssid_prefix have failed\n");
        return false;
    }

    if (!espconn_mesh_group_id_init((uint8_t *)MESH_GROUP_ID, sizeof(MESH_GROUP_ID))) {
        MESH_DEMO_PRINT("set group_id  have failed\n");
        return false;
    }

    // set cloud server ip and port for mesh node
    if (!espconn_mesh_server_init((struct ip_addr *)server_ip, server_port)) {
        MESH_DEMO_PRINT("server_init have failed\n");
        return false;
    }

    espconn_mesh_regist_usr_cb(esp_mesh_new_child_notify); // when new child joins current AP
    return true;
}

/******************************************************************************
 * FunctionName : router_init
 * Description  : Initialize router related settings & Connecting router
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
static bool ICACHE_FLASH_ATTR router_init(){
    struct station_config config;

#if 0
//#if 1
    // in case of use "esp_touch"
    if (!espconn_mesh_is_root_candidate())
        goto INIT_SMARTCONFIG;

    MESH_DEMO_MEMSET(&config, 0, sizeof(config));
    espconn_mesh_get_router(&config);
    if (config.ssid[0] == 0xff ||
        config.ssid[0] == 0x00) {
#endif
        // please change MESH_ROUTER_SSID and MESH_ROUTER_PASSWD in "user_config.h"
        MESH_DEMO_MEMSET(&config, 0, sizeof(config));
        MESH_DEMO_MEMCPY(config.ssid, MESH_ROUTER_SSID, MESH_DEMO_STRLEN(MESH_ROUTER_SSID));
        MESH_DEMO_MEMCPY(config.password, MESH_ROUTER_PASSWD, MESH_DEMO_STRLEN(MESH_ROUTER_PASSWD));
        // if you use router with hide ssid, you MUST set bssid in config,
        // otherwise, node will fail to connect router.
        // if you use normal router, please pay no attention to the bssid,
        // and you don't need to modify the bssid, mesh will ignore the bssid.

        config.bssid_set = 1;
        MESH_DEMO_MEMCPY(config.bssid, MESH_ROUTER_BSSID, sizeof(config.bssid));
    //}

    if (!espconn_mesh_set_router(&config)) {
        MESH_DEMO_PRINT("set_routerAP have failed\n");
        return false;
    }

INIT_SMARTCONFIG:
    // use esp-touch(smart configure) to sent information about router AP to mesh node
    esptouch_init();
    MESH_DEMO_PRINT("flush ssid:%s pwd:%s\n", config.ssid, config.password);
    return true;
}

/******************************************************************************
 * FunctionName : wait_esptouch_over
 * Description  : wait for esptouch to run over and then enable mesh
 * Parameters   :
 * Returns      : none
*******************************************************************************/
static void ICACHE_FLASH_ATTR wait_esptouch_over(os_timer_t *timer)
{
    if (esptouch_is_running()) return;
    if (!timer) return;
    os_timer_disarm(timer);
    os_free(timer);

    // enable mesh, you should wait for the mesh_enable_cb to be triggered.
    espconn_mesh_enable(mesh_enable_cb, MESH_ONLINE);
    // if you want to setup softAP mesh, please set mesh type "MESH_SOFTAP"
    // espconn_mesh_enable(mesh_enable_cb, MESH_SOFTAP);
}

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
    // set uart baut ratio,
    // you can change the value in user_config.h
    uart_div_modify(0, UART_CLK_FREQ / UART_BAUT_RATIO); //uart0 in esp8266
    uart_div_modify(1, UART_CLK_FREQ / UART_BAUT_RATIO); //uart1 in esp8266

    if (!router_init())
        return;

    if (!esp_mesh_demo_init())
        return;

    user_devicefind_init();

    // wait for esptouch to run over and then execute espconn_mesh_enable
    os_timer_t *wait_timer = (os_timer_t *)os_zalloc(sizeof(os_timer_t));
    if (NULL == wait_timer) return;
    os_timer_disarm(wait_timer);
    os_timer_setfn(wait_timer, (os_timer_func_t *)wait_esptouch_over, wait_timer);
    os_timer_arm(wait_timer, 1000, true);
}
