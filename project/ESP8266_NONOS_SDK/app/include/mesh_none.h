#ifndef __MESH_NONE_H__
#define __MESH_NONE_H__

#include "c_types.h"

/******************************************************************************
 * FunctionName : mesh_http_proto_parser
 * Description  : this function is used to parse mesh topology response
*******************************************************************************/
void mesh_none_proto_parser(const void *mesh_header, uint8_t *pdata, uint16_t len);

#endif
