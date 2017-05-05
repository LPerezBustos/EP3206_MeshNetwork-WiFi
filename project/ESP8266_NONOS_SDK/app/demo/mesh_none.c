#include "mem.h"
#include "mesh.h"
#include "osapi.h"
#include "mesh_parser.h"

extern struct espconn g_ser_conn; // TCP conection with sever

void ICACHE_FLASH_ATTR // print nodes below ROOT_NODE --> optional*
mesh_disp_sub_dev_mac(uint8_t *sub_mac, uint16_t sub_count){
    uint16_t i = 0;
    const uint8_t mac_len = 6;

    if (!sub_mac || sub_count == 0) { // solo existe root node
        MESH_PARSER_PRINT("sub_mac:%p, sub_count:%u\n", sub_mac, sub_count);
        return;
    }

    for (i = 0; i < sub_count; i ++) { // print CHILD_NODES y sus MAC_ADDRESS
        MESH_PARSER_PRINT("child node: %u, mac:" MACSTR "\n", i, MAC2STR(sub_mac));
        sub_mac += mac_len;
    }
}

void ICACHE_FLASH_ATTR // se llama desde "mesh_parser.c", mesh_header contiene M_O_TOPO_RESP
mesh_none_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len){
    uint16_t op_idx = 1; // se inicia el contador de opciones en 1
    uint16_t dev_count = 0;
    uint8_t *dev_mac = NULL;
    const uint8_t mac_len = 6; // longitud de MAC_ADDRESS
    struct mesh_header_format *header = NULL;
    struct mesh_header_option_format *option = NULL;

    MESH_PARSER_PRINT("%s is running\n", __func__); // se imprime funcion actual

    if (!pdata) // retornar si *pdata apunta a NULL
        return;

    header = (struct mesh_header_format *)pdata; // casting de *pdata a mesh_header_format
    // solo los MESH_NODES reciben M_O_TOPO_RESP --> arman su MAC_ADDRESS_LIST con M_O_TOPO_REQ
    //MESH_PARSER_PRINT("root's mac:" MACSTR "\n", MAC2STR(header->src_addr));
    // MESH_NODES reciben M_O_TOPO_RESP del ROOT_NODE, se configura esta MAC_ADDRESS como root
    mesh_device_set_root((struct mesh_device_mac_type *)header->src_addr);
    // "espconn_mesh_get_option()" busca entre las opciones dentro del mesh_header
    while (espconn_mesh_get_option(header, M_O_TOPO_RESP, op_idx++, &option)) {
        dev_count = option->olen / mac_len; // cantidad de MAC_ADDRESS recibidas desde el ROOT_NODE
        dev_mac = option->ovalue;
        // añadir MAC_ADDRESS que no estan en la MAC_ADDRESS_LIST --> se verifica dentro de "mesh_device_add()"
        mesh_device_add((struct mesh_device_mac_type *)dev_mac, dev_count);
        //mesh_disp_sub_dev_mac(dev_mac, dev_count);
    }

    mesh_device_disp_mac_list(); // se imprime la MAC_ADDRESS_LIST
}

// Esta funcion se llama segun el intervalo de tiempo definido por topo_test_time
void ICACHE_FLASH_ATTR mesh_topo_test(){
    uint8_t src[6]; // MAC_ADDRESS
    uint8_t dst[6];
    struct mesh_header_format *header = NULL;
    struct mesh_header_option_format *option = NULL;
    uint8_t ot_len = sizeof(struct mesh_header_option_header_type) + sizeof(*option) + sizeof(dst);

    if (!wifi_get_macaddr(STATION_IF, src)){ // se obtiene la STATION MAC_ADDRESS del nodo actual
        MESH_PARSER_PRINT("get station MAC_ADDRESS in topology parser have failed\n");
        return;
    }

    // root device uses "espconn_mesh_get_node_info()" to get mac address of all devices
    if (espconn_mesh_is_root()) {
        uint8_t *sub_dev_mac = NULL;
        uint16_t sub_dev_count = 0;
        // MESH_NODE_ALL pregunta a todos los nodos de la red
        // the first one is mac address of router
        if (!espconn_mesh_get_node_info(MESH_NODE_ALL, &sub_dev_mac, &sub_dev_count))
            return;
        //mesh_disp_sub_dev_mac(sub_dev_mac, sub_dev_count);
        mesh_device_set_root((struct mesh_device_mac_type *)src); // nodo actual es ROOT_NODE

        if (sub_dev_count > 1){ // si hay MESH_NODES, se completa MAC_ADDRESS_LIST
            struct mesh_device_mac_type *list = (struct mesh_device_mac_type *)sub_dev_mac;
            mesh_device_add(list + 1, sub_dev_count - 1);
        }
        mesh_device_disp_mac_list(); // mostrar ROOT_NODE'S MAC_ADDRESS_LIST

        espconn_mesh_get_node_info(MESH_NODE_ALL, NULL, NULL);
        return;
    }

    // non-root devices use M_O_TOPO_REQ with bcast to get mac address of all devices
    // bcast_address = 00:00:00:00:00:00
    os_memset(dst, 0, sizeof(dst));  // use bcast to get all the devices working in mesh from root.
    header = (struct mesh_header_format *)espconn_mesh_create_packet(
                            dst,     // destiny address (bcast)
                            src,     // source address
                            false,   // not p2p packet
                            true,    // piggyback congest request
                            M_PROTO_NONE,  // packe with NONE format
                            0,       // data length
                            true,    // no option
                            ot_len,  // option total len
                            false,   // no frag
                            0,       // frag type, this packet doesn't use frag
                            false,   // more frag
                            0,       // frag index
                            0);      // frag length
    if (!header) {
        MESH_PARSER_PRINT("create packet in topology parser have failed\n");
        return;
    }

    option = (struct mesh_header_option_format *)espconn_mesh_create_option(
            M_O_TOPO_REQ, dst, sizeof(dst));
    if (!option) {
        MESH_PARSER_PRINT("create option in topology parser have failed\n");
        goto TOPO_FAIL;
    }

    if (!espconn_mesh_add_option(header, option)) { // add option to mesh_header
        MESH_PARSER_PRINT("set option in topology parser have failed\n");
        goto TOPO_FAIL;
    }

    // se envoit le package au serveur pour ascendre autour les niveaux du reseau
    if (espconn_mesh_sent(&g_ser_conn, (uint8_t *)header, header->len)) {
        MESH_PARSER_PRINT("topology parser is busy right now\n");
        espconn_mesh_connect(&g_ser_conn);
    }
TOPO_FAIL: // liberer espace de la memoir si il y a des erreurs a la creation du package
    option ? MESH_DEMO_FREE(option) : 0;
    header ? MESH_DEMO_FREE(header) : 0;
}

void ICACHE_FLASH_ATTR mesh_topo_test_init(){
    static os_timer_t topo_timer; // se configura el temporizador
    os_timer_disarm(&topo_timer);
    os_timer_setfn(&topo_timer, (os_timer_func_t *)mesh_topo_test, NULL); // callback_function
    os_timer_arm(&topo_timer, topo_test_time, true); // cada topo_test_time=XXXXXms se llama a un "mesh_topo_test()"
}
