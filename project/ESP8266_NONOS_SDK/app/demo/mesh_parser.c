#include "mesh_parser.h"
#include "mesh.h"

// mesh_usr_proto_type in mesh_header
static struct mesh_general_parser_type g_packet_parser[] = {
    {M_PROTO_NONE, mesh_none_proto_parser},
    {M_PROTO_HTTP, mesh_http_proto_parser},
    {M_PROTO_JSON, mesh_json_proto_parser},
    {M_PROTO_MQTT, mesh_mqtt_proto_parser},
    {M_PROTO_BIN,  mesh_bin_proto_parser}
};

void ICACHE_FLASH_ATTR
mesh_packet_parser(void *arg, uint8_t *pdata, uint16_t len){
    // this funcion is called by "esp_recv_entrance()" in "mesh_demo.c"
    uint16_t i = 0;
    uint8_t *usr_data = NULL;
    uint16_t usr_data_len = 0;
    enum mesh_usr_proto_type proto;

    struct mesh_header_format *header = (struct mesh_header_format *)pdata;
    uint16_t proto_count = sizeof(g_packet_parser) / sizeof(g_packet_parser[0]);

    if (!espconn_mesh_get_usr_data_proto(header, &proto))
        return;
    if (!espconn_mesh_get_usr_data(header, &usr_data, &usr_data_len)) {
        usr_data = pdata;
        usr_data_len = len;
    }

    for (i = 0; i < proto_count; i ++) {
        if (g_packet_parser[i].proto == proto) {
            if (g_packet_parser[i].handler == NULL)
                break;
            // going to the corresponding parser protocol function
            g_packet_parser[i].handler(header, usr_data, usr_data_len);
            break;
        }
    }
}
