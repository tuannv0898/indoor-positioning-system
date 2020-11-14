#include <rtls_mesh/ble_mesh/mesh_msg.h>

void msg_prepr_onoff(struct os_mbuf *os_mbuf, msg_onoff_t *msg){
    net_buf_simple_add_u8(os_mbuf, msg->value);
}

void msg_parse_onoff(struct os_mbuf *os_mbuf, msg_onoff_t *msg){
    msg->value = net_buf_simple_pull_u8(os_mbuf);
}

void msg_prepr_rtls(struct os_mbuf *mbuf, msg_rtls_t *msg){
    net_buf_simple_add_u8(mbuf, msg->type);
    net_buf_simple_add_be16(mbuf, msg->dstsrc);
    switch (msg->type)
    {
    case MAVLINK_MSG_ID_LOCATION:
        net_buf_simple_add_be32(mbuf, *((uint32_t *)(&msg->location_x)));
        net_buf_simple_add_be32(mbuf, *((uint32_t *)(&msg->location_y)));
        net_buf_simple_add_be32(mbuf, *((uint32_t *)(&msg->location_z)));
        break;
    case MAVLINK_MSG_ID_ONOFF:
        break;
    default:
        break;
    }
}

void msg_parse_rtls(struct os_mbuf *mbuf, msg_rtls_t *msg){
    msg->type = net_buf_simple_pull_u8(mbuf);
    msg->dstsrc = net_buf_simple_pull_be16(mbuf);

    switch (msg->type)
    {
    case MAVLINK_MSG_ID_LOCATION:
        *((uint32_t *)(&msg->location_x)) = net_buf_simple_pull_be32(mbuf);
        *((uint32_t *)(&msg->location_y)) = net_buf_simple_pull_be32(mbuf);
        *((uint32_t *)(&msg->location_z)) = net_buf_simple_pull_be32(mbuf);
        break;
    case MAVLINK_MSG_ID_ONOFF:
        break;
    default:
        break;
    }
}