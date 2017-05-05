#ifndef __MESH_HTTP_H__
#define __MESH_HTTP_H__

#include "c_types.h"

typedef void (*mesh_http_url_handler)(void *);

struct mesh_http_url_table {
    const char *url;
    mesh_http_url_handler url_handler;
    const void *url_arg;
};

 /******************************************************************************
  * FunctionName : mesh_http_proto_parser
  * Description  : this function is used to parse packet formatted with HTTP
 *******************************************************************************/
void mesh_http_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len);

#endif
