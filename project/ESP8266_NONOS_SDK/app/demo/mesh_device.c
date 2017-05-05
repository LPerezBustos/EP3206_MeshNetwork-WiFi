#include "mem.h"
#include "mesh.h"
#include "osapi.h"
#include "c_types.h"
#include "espconn.h"
#include "user_config.h"
#include "mesh_parser.h"

#define MESH_DEV_MEMSET os_memset
#define MESH_DEV_MEMCPY os_memcpy
#define MESH_DEV_MALLOC os_malloc
#define MESH_DEV_ZALLOC os_zalloc
#define MESH_DEV_MEMCMP os_memcmp
#define MESH_DEV_FREE   os_free

static bool g_mesh_device_init = false;
static struct mesh_device_list_type g_node_list;

void ICACHE_FLASH_ATTR
mesh_device_disp_mac_list(){
    // display MAC_ADDRESS_LIST of current node
    uint16_t idx = 0;
    sint8_t rssi_info = 0;
    uint8_t children_nodes = 0;
    uint8_t *sub_dev_mac = NULL;
    uint16_t sub_dev_count = 0;
    uint8_t hwaddr[6];
    struct softap_config softap_config;
    struct ip_info ipconfig;
    if (g_node_list.scale < 1)
        return; // return if there's not ROOT_NODE

    (void)wifi_softap_get_config(&softap_config);
    wifi_get_ip_info(STATION_IF, &ipconfig);
    wifi_get_macaddr(STATION_IF, hwaddr);

    if (!espconn_mesh_get_node_info(MESH_NODE_PARENT, &sub_dev_mac, &sub_dev_count))
        // MESH_NODE_PARENT ask to SoftAP that current node is connected
        // sub_dev_count == 1 (all the nodes have just one parent)
        return;

    rssi_info = wifi_station_get_rssi();
    children_nodes = wifi_softap_get_station_num();

    MESH_DEMO_PRINT("\nHi ! I'm " MACSTR " with ip_address " IPSTR "\n", MAC2STR(hwaddr), IP2STR(&ipconfig.ip));
    MESH_DEMO_PRINT("My parent node is " MACSTR "\n", MAC2STR((uint8_t *)sub_dev_mac));
    MESH_DEMO_PRINT("with RSSI = %d dBm\n", rssi_info);
    MESH_DEMO_PRINT("My SSID is %s\n", &softap_config.ssid);
    MESH_DEMO_PRINT("I've %d children nodes\n", children_nodes);
    MESH_DEMO_PRINT("I'm using WiFi channel %d\n", softap_config.channel);
    MESH_DEMO_PRINT("with a beacon interval of %d ms\n", softap_config.beacon_interval);
    MESH_DEMO_PRINT("cantidad de nodos en la red %d\n",g_node_list.scale);
    MESH_DEMO_PRINT("===== network mac list info =====\n"); // header
    // always first ROOT_NODE'S MAC_ADDRESS
    MESH_DEMO_PRINT("root -> " MACSTR "\n", MAC2STR(g_node_list.root.mac));
    if (g_node_list.scale < 2) // if there's not MESH_NODES
         goto MAC_ADDRESS_LIST_END;
    for (idx = 0; idx < g_node_list.scale - 1; idx ++)
        MESH_DEMO_PRINT("child node %d -> " MACSTR "\n", idx+1, MAC2STR(g_node_list.list[idx].mac));
MAC_ADDRESS_LIST_END:
    MESH_DEMO_PRINT("====== network mac list end =====\n"); // footer
    MESH_DEMO_PRINT("\n***** take the correct path ! *****\n"); // mac routing table header
    espconn_mesh_disp_route_table();
    MESH_DEMO_PRINT("\n******* Have a nice trip :) *******\n"); // mac routing table footer
}

bool ICACHE_FLASH_ATTR
mesh_device_get_mac_list(const struct mesh_device_mac_type **list, uint16_t *count){
   // get MAC_ADDRESS_LIST for proto parser
    if (!g_mesh_device_init) {
        MESH_DEMO_PRINT("please init mesh device list firstly\n");
        return false;
    }

    if (!list || !count)
        return false;

    if (g_node_list.scale < 2) {
        *list = NULL;
        *count = 0;
        return true;
    }

    *list = g_node_list.list;
    *count = g_node_list.scale - 1; // no deberia ser g_node_list.size?
    return true;
}

bool ICACHE_FLASH_ATTR
mesh_device_get_root(const struct mesh_device_mac_type **root){
    // get ROOT_NODE for proto parser
    if (!g_mesh_device_init) {
        MESH_DEMO_PRINT("please init mesh device list firstly\n");
        return false;
    }

    if (g_node_list.scale == 0) {
        MESH_DEMO_PRINT("no mac in current mac list\n");
        return false;
    }

    if (!root)
        return false; // return if *root == NULL

    *root = &g_node_list.root;
    return true;
}

void ICACHE_FLASH_ATTR mesh_device_list_release(){
// se libera espacio de memoria ocupado por MAC_ADDRESS_LIST
    if (!g_mesh_device_init)
        return;

    if (g_node_list.list != NULL) {
        MESH_DEV_FREE(g_node_list.list);
        g_node_list.list = NULL;
    }
    MESH_DEV_MEMSET(&g_node_list, 0, sizeof(g_node_list));
}

void ICACHE_FLASH_ATTR mesh_device_list_init() // se inicializa MAC_ADDRESS_LIST
{
    if (g_mesh_device_init)
        return;

    MESH_DEV_MEMSET(&g_node_list, 0, sizeof(g_node_list));
    g_mesh_device_init = true; // flag
}

void ICACHE_FLASH_ATTR
mesh_device_set_root(struct mesh_device_mac_type *root){
   // se configura el ROOT_NODE respectivo
    if (!g_mesh_device_init)
        mesh_device_list_init();

    // si no existe ROOT_NODE --> _node_list.scale == 0
    if (g_node_list.scale == 0){
        MESH_DEMO_PRINT("new root:" MACSTR "\n", MAC2STR((uint8_t *)root));
        MESH_DEV_MEMCPY(&g_node_list.root, root, sizeof(*root));
        g_node_list.scale = 1; // root's flag
        return;
    }

    // ROOT_NODE is the same to the current node, we don't need to modify anything
    if (!MESH_DEV_MEMCMP(&g_node_list.root, root, sizeof(*root)))
        return;

    // switch ROOT_NODE, we need to free MAC_ADDRESS_LIST
    MESH_DEMO_PRINT("switch root: " MACSTR " to root: " MACSTR "\n",
            MAC2STR((uint8_t *)&g_node_list.root), MAC2STR((uint8_t *)root));
    mesh_device_list_release();
    MESH_DEV_MEMCPY(&g_node_list.root, root, sizeof(*root));
    g_node_list.scale = 1; // root's flag
}

bool ICACHE_FLASH_ATTR
mesh_search_device(const struct mesh_device_mac_type *node){
    // look for a node in MAC_ADDRESS_LIST
    uint16_t idx = 0;
    uint16_t scale = 0, i = 0;
    struct mesh_device_mac_type *list = NULL;

    if (g_node_list.scale == 0)
        return false; // there isn't ROOT_NODE
    if (!MESH_DEV_MEMCMP(&g_node_list.root, node, sizeof(*node)))
        return true; // c'est le << Noeud initial >>
    if (g_node_list.list == NULL)
        return false;

    scale = g_node_list.scale - 1;
    list = g_node_list.list;

    for (i = 0; i < scale; i ++){ // trying to find current node in g_node_list.list
        if (!MESH_DEV_MEMCMP(list, node, sizeof(*node)))
            return true; // current node is in the MAC_ADDRESS_LIST
        list ++;
    }
    return false; // LE NODE ACTUEL N'EST PAS A MAC_ADDRESS_LIST
}

bool ICACHE_FLASH_ATTR
mesh_device_add(struct mesh_device_mac_type *nodes, uint16_t count){
    // adding some nodes to MAC_ADDRESS_LIST
#define MESH_DEV_STEP (10)
    uint16_t idx = 0; // para moverse a traves de posiciones de memoria de 16bits
    uint16_t rest = g_node_list.size + 1 - g_node_list.scale;

    if (!g_mesh_device_init)
        mesh_device_list_init();

    if (g_node_list.size == 0) // there isn't ROOT_NODE
        rest = 0;

    if (rest < count){ // si nueva cantidad de nodos es mayor que el tamaño de MAC_ADDRESS_LIST
        uint16_t size = g_node_list.size + rest + MESH_DEV_STEP;
        uint8_t *buf = (uint8_t *)MESH_DEV_ZALLOC(size * sizeof(*nodes));
        if (!buf) {
            MESH_DEMO_PRINT("mesh reallocation list buffer have failed\n");
            return false;
        }

        // move the current list to new buffer
        if (g_node_list.list && g_node_list.scale > 1)
            MESH_DEV_MEMCPY(buf, g_node_list.list,
                    (g_node_list.scale - 1) * sizeof(*nodes));
        if (g_node_list.list) // se libera espacio de "g_node_list.list"
            MESH_DEV_FREE(g_node_list.list);
        g_node_list.list = (struct mesh_device_mac_type *)buf;
        g_node_list.size = size; // new MAC_ADDRESS_LIST is bigger than before
    }

    while (idx < count){ // se agregan nuevos nodos a la MAC_ADDRESS_LIST
        if (!mesh_search_device(nodes + idx)){ // not in list, add it into list
            MESH_DEV_MEMCPY(g_node_list.list + g_node_list.scale - 1,
                    nodes + idx, sizeof(*nodes));
            g_node_list.scale ++;
        }
        idx ++;
    }
    return true;
}

bool ICACHE_FLASH_ATTR
mesh_device_del(struct mesh_device_mac_type *nodes, uint16_t count){
    // delete some nodes from MAC_ADDRESS_LIST
    uint16_t idx = 0, i = 0;
    uint16_t sub_count = g_node_list.scale - 1;

    if (!nodes || count == 0)
        return true; // nothing to do

    if (!g_mesh_device_init)
        mesh_device_list_init(); // <-- go there first

    if (g_node_list.scale == 0)
        return false; // there isn't ROOT_NODE

    while (idx < count) {
        // node is not in list, do nothing
        if (!mesh_search_device(nodes + idx)) {
            idx ++;
            continue;
        }

        // root will be delete, so current mac list is stale
        if (!MESH_DEV_MEMCMP(nodes + idx, &g_node_list.root, sizeof(*nodes))) {
            mesh_device_list_release();
            return true;
        }

        // delete node from mac list
        for (i = 0; i < sub_count; i ++) { // se recorre la MAC_ADDRESS_LIST
            if (!MESH_DEV_MEMCMP(nodes + idx, &g_node_list.list[i], sizeof(*nodes))) {
                // MESH_DEV_MEMCMP --> os_memcmp --> ets_memcmp (take a look at "osapi.h")
                // int os_memcmp(const void *s1, const void *s2, size_t n)
                // s1: first buffer ; s2: second buffer ; n: max number of bytes to compare
                // ** return: int<0 if s1<s2; int==0 if s1=s2; int>0 if s1>s2
                if (sub_count - i  > 1) // the node is in the list
                // get into this code if there is more than just the ROOT_NODE
                    MESH_DEV_MEMCPY(&g_node_list.list[i], &g_node_list.list[i + 1],
                            (sub_count - i - 1) * sizeof(*nodes)); // reallocate the list
                sub_count --; // one node less to compare
                g_node_list.scale --; // one node less in MAC_ADDRESS_LIST
                i --;
                MESH_DEV_MEMSET(&g_node_list.list[g_node_list.scale], 0, sizeof(*nodes));
                break;
            }
        }
        idx ++;
    }
    return true;
}
