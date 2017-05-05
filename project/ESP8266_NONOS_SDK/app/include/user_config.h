#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "c_types.h"

#define MESH_DEMO_PRINT  ets_printf
#define MESH_DEMO_STRLEN ets_strlen
#define MESH_DEMO_MEMCPY ets_memcpy
#define MESH_DEMO_MEMSET ets_memset
#define MESH_DEMO_FREE   os_free

static const uint32_t json_p2p_test_time = 19000;
static const uint32_t json_bcast_test_time = 18000;
static const uint32_t json_mcast_test_time = 17000;
static const uint32_t topo_test_time = 7000;
static const uint32_t application_time = 60000; // 15000ms is the minimum with ThingSpeak Free Cloud Platform
static const uint16_t server_port = 7000; /*PORT USED BY USER IOT SERVER FOR MESH SERVICE*/
static const uint8_t server_ip[4] = {10, 42, 0, 1}; /*USER IOT SERVER IP ADDRESS*/
static const uint32_t UART_BAUT_RATIO = 115200; /*UART BAUT RATIO*/
static const uint8_t MESH_GROUP_ID[6] = {0x18,0xfe,0x34,0x00,0x00,0x50}; /*MESH_GROUP_ID & MESH_SSID_PREFIX REPRESENTS ONE MESH NETWORK*/
static const uint8_t MESH_ROUTER_BSSID[6] = {0xE0, 0x2A, 0x82, 0x3C, 0xF3, 0xAD}; /*MAC OF ROUTER*/
/*MAC ADDRESS - AP HOME CARACAS --> {0xF4, 0xEC, 0x38, 0xA8, 0xB9, 0x4E}*/
/*MAC ADDRESS - AP LAPTOP HP --> {0xE0, 0x2A, 0x82, 0x3C, 0xF3, 0xAD}*/

// please change MESH_ROUTER_SSID - MESH_ROUTER_PASSWD - MESH_ROUTER_BSSID according to your routerAP
#define MESH_ROUTER_SSID     "routerAP_Server"      /*THE ROUTER SSID*/
#define MESH_ROUTER_PASSWD   "12345678"    /*THE ROUTER PASSWORD*/
#define MESH_SSID_PREFIX     "MESH_DEMO"    /*SET THE DEFAULT MESH SSID PREFIX;THE FINAL SSID OF SOFTAP WOULD BE "MESH_SSID_PREFIX_X_YYYYYY"*/
#define MESH_AUTH            AUTH_WPA2_PSK  /*AUTH_MODE OF SOFTAP FOR EACH MESH NODE*/
#define MESH_PASSWD          "123123123"    /*SET PASSWORD OF SOFTAP FOR EACH MESH NODE*/
#define MESH_MAX_HOP         (4)            /*MAX_HOPS OF MESH NETWORK*/

#endif
