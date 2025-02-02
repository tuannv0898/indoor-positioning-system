#include <assert.h>
#include <dpl/dpl.h>
#include <hal/hal_gpio.h>
#include <bsp/bsp.h>

#if MYNEWT_VAL(BLE_MESH)

/* BLE */
#include "mesh/mesh.h"
#include "nimble/ble.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "mesh/glue.h"

#include <message/mesh_msg.h>
#include <rtls/ble_mesh/mesh_define.h>
#include <rtls/rtls/rtls.h>

#include <stats/stats.h>
#include <rtls_tdma/rtls_tdma.h>

#define LED_DELAY_MS    200
#define LIGHT_BULB_0    9
#define LIGHT_BULB_1    10

STATS_SECT_START(model_root_stat_t)
    STATS_SECT_ENTRY(publish_succed)
    STATS_SECT_ENTRY(publish_failed)
    STATS_SECT_ENTRY(recv_setmsg)
    STATS_SECT_ENTRY(recv_sttmsg)
STATS_SECT_END

STATS_SECT_DECL(model_root_stat_t) g_model_root_stat;

STATS_NAME_START(model_root_stat_t)
    STATS_NAME(model_root_stat_t, publish_succed)
    STATS_NAME(model_root_stat_t, publish_failed)
    STATS_NAME(model_root_stat_t, recv_setmsg)
    STATS_NAME(model_root_stat_t, recv_sttmsg)
STATS_NAME_END(model_root_stat_t)

static struct os_task g_task_blink;
static os_stack_t g_task_blink_stack[MYNEWT_VAL(TASK_BLINK_STACK_SIZE)];

static struct os_task g_task_location;
static os_stack_t g_task_location_stack[MYNEWT_VAL(TASK_LOCATION_STACK_SIZE)];

struct os_mutex g_model_mutex;

static struct os_task g_led_task;
static os_stack_t g_task_led_stack[MYNEWT_VAL(TASK_LED_STACK_SIZE)];
static bool g_led_running = false;

static void
rtls_model_set(struct bt_mesh_model *model,
              struct bt_mesh_msg_ctx *ctx,
              struct os_mbuf *buf)
{
    STATS_INC(g_model_root_stat, recv_setmsg);
    msg_rtls_t msg_rtls;
    uint16_t uwb_address;
    msg_parse_rtls(buf, &msg_rtls);
    rtls_get_address(&uwb_address);
    if(msg_rtls.uwb_address != uwb_address) return;
    msg_rtls.opcode = BT_MESH_MODEL_OP_SET;

    switch (msg_rtls.msg_id)
    {
    case MAVLINK_MSG_ID_LOCATION:
    {
        rtls_set_location(msg_rtls.location_x, msg_rtls.location_y, msg_rtls.location_z);
        msg_print_rtls(&msg_rtls);
    }
        break;
    case MAVLINK_MSG_ID_ONOFF:
        if(msg_rtls.value & 0x01)
        {
            g_led_running = true;
            hal_gpio_write(LED_1, 0);
        }else{
            g_led_running = false;
            hal_gpio_write(LED_1, 1);
        }

        if(msg_rtls.value & 0x02){
            hal_gpio_write(LIGHT_BULB_0, 0);
        }
        else{
            hal_gpio_write(LIGHT_BULB_0, 1);
        }

        if(msg_rtls.value & 0x04){
            hal_gpio_write(LIGHT_BULB_1, 0);
        }
        else{
            hal_gpio_write(LIGHT_BULB_1, 1);
        }
        break;
    case MAVLINK_MSG_ID_BLINK:
        rtls_set_ntype(msg_rtls.role);
        msg_print_rtls(&msg_rtls);
        break;
    default:
        break;
    }
}

static const struct bt_mesh_model_op rtls_op[] = {
    { BT_MESH_MODEL_OP_SET, 0, rtls_model_set},
    BT_MESH_MODEL_OP_END,
};

static struct bt_mesh_cfg_srv cfg_srv = {
    .relay = BT_MESH_RELAY_DISABLED,
    .beacon = BT_MESH_BEACON_ENABLED,
    .frnd = BT_MESH_FRIEND_NOT_SUPPORTED,
    .gatt_proxy = BT_MESH_GATT_PROXY_ENABLED,
    .default_ttl = 7,
    .net_transmit = BT_MESH_TRANSMIT(0, 10),
    .relay_retransmit = BT_MESH_TRANSMIT(0, 10),
};
static struct bt_mesh_model_pub model_pub_rtls;

struct bt_mesh_model model_root[] = {
    BT_MESH_MODEL_CFG_SRV(&cfg_srv),
    BT_MESH_MODEL(BT_MESH_MODEL_ID_RTLS, rtls_op, &model_pub_rtls, NULL),
};

static struct bt_mesh_model *model;
static struct bt_mesh_model_pub *pub;
static msg_rtls_t msg_rtls;

static void
task_blink_func(void *arg){
    uint8_t cnt = 0;
    while (1) {
        dpl_time_delay(dpl_time_ms_to_ticks32(1000));
        if (pub->addr == BT_MESH_ADDR_UNASSIGNED) continue;
        os_mutex_pend(&g_model_mutex, OS_TIMEOUT_NEVER);

        cnt++;
        switch (cnt%3)
        {
        case 0:{
                msg_rtls.opcode = BT_MESH_MODEL_OP_STATUS;
                msg_rtls.msg_id = MAVLINK_MSG_ID_BLINK;
                rtls_get_address(&msg_rtls.uwb_address);

                rtls_get_ntype(&msg_rtls.role);

                msg_prepr_rtls(&pub->msg, &msg_rtls);
            }
            break;
        case 1:
            {
                msg_rtls.opcode = BT_MESH_MODEL_OP_STATUS;
                msg_rtls.msg_id = MAVLINK_MSG_ID_SLOT;
                rtls_get_address(&msg_rtls.uwb_address);

                rtls_get_slot(&msg_rtls.slot);

                msg_prepr_rtls(&pub->msg, &msg_rtls);
            }
            break;
        case 2:
            {
                msg_rtls.opcode = BT_MESH_MODEL_OP_STATUS;
                msg_rtls.msg_id = MAVLINK_MSG_ID_ONOFF;
                rtls_get_address(&msg_rtls.uwb_address);
                
                if(g_led_running){
                    msg_rtls.value = g_led_running;
                }
                else{
                    msg_rtls.value = (!hal_gpio_read(LIGHT_BULB_0) << 1) | (!hal_gpio_read(LIGHT_BULB_1) << 2);
                }
                msg_prepr_rtls(&pub->msg, &msg_rtls);
            }
            break;
        }

        if (bt_mesh_model_publish(model)) {
            STATS_INC(g_model_root_stat, publish_failed);
        }
        else{
            STATS_INC(g_model_root_stat, publish_succed);
        }
        os_mbuf_free(pub->msg);

        os_mutex_release(&g_model_mutex);
    }
}

static void
task_location_func(void *arg){

    uint8_t node_type;
    int cnt = 0;
    while(1){
        rtls_get_ntype(&node_type);
        if(node_type == RTR_ANCHOR){
            dpl_time_delay(dpl_time_ms_to_ticks32(10000));
            if (pub->addr == BT_MESH_ADDR_UNASSIGNED) continue;

            cnt++;
            if(cnt%2){
                os_mutex_pend(&g_model_mutex, OS_TIMEOUT_NEVER);
                msg_rtls.opcode = BT_MESH_MODEL_OP_STATUS;
                msg_rtls.msg_id = MAVLINK_MSG_ID_LOCATION_REDUCED;
                rtls_get_address(&msg_rtls.uwb_address);
            }else{
                os_mutex_pend(&g_model_mutex, OS_TIMEOUT_NEVER);
                msg_rtls.opcode = BT_MESH_MODEL_OP_STATUS;
                msg_rtls.msg_id = MAVLINK_MSG_ID_LOCATION;
                rtls_get_address(&msg_rtls.uwb_address);
            }
        }
        else{
            dpl_time_delay(dpl_time_ms_to_ticks32(MYNEWT_VAL(UWB_CCP_PERIOD)/1000));
            if (pub->addr == BT_MESH_ADDR_UNASSIGNED) continue;

            os_mutex_pend(&g_model_mutex, OS_TIMEOUT_NEVER);
            msg_rtls.opcode = BT_MESH_MODEL_OP_STATUS;
            msg_rtls.msg_id = MAVLINK_MSG_ID_LOCATION_REDUCED;
            rtls_get_address(&msg_rtls.uwb_address);
        }

        if(rtls_get_location(&msg_rtls.location_x, &msg_rtls.location_y, &msg_rtls.location_z)){
            msg_prepr_rtls(&pub->msg, &msg_rtls);

            if (bt_mesh_model_publish(model)) {
                STATS_INC(g_model_root_stat, publish_failed);
            }
            else{
                STATS_INC(g_model_root_stat, publish_succed);
            }
            os_mbuf_free(pub->msg);
        }

        os_mutex_release(&g_model_mutex);
    }
}

static void
task_led_func(void *arg){

    while (1) {
        if(!g_led_running) {
            dpl_time_delay(dpl_time_ms_to_ticks32(500));
            continue;
        }

        hal_gpio_write(12, 1);
        hal_gpio_write(9, 0);
        hal_gpio_write(10, 0);

        dpl_time_delay(dpl_time_ms_to_ticks32(LED_DELAY_MS));
        hal_gpio_write(13, 0);
        dpl_time_delay(dpl_time_ms_to_ticks32(LED_DELAY_MS));
        hal_gpio_write(23, 0);

        dpl_time_delay(dpl_time_ms_to_ticks32(LED_DELAY_MS));
        hal_gpio_write(13, 1);
        dpl_time_delay(dpl_time_ms_to_ticks32(LED_DELAY_MS));
        hal_gpio_write(23, 1);

        hal_gpio_write(12, 0);
        hal_gpio_write(9, 1);
        hal_gpio_write(10, 1);
        dpl_time_delay(dpl_time_ms_to_ticks32(1000));

    }
}

void model_gateway_init(){
    int rc;

    model = &model_root[1];
    pub =  model->pub;

    /* RTLS LED */
    hal_gpio_init_out(23, 1);
    hal_gpio_init_out(13, 1);

    /* RTLS Buzzer */
    hal_gpio_init_out(12, 0);

    /* DWM1001-DEV LED */
    hal_gpio_init_out(LED_1, 1);

    /* RTLS Light Bulb */
    hal_gpio_init_out(9, 1);
    hal_gpio_init_out(10, 1);

    os_mutex_init(&g_model_mutex);

    os_task_init(&g_led_task, "led",
                task_led_func,
                NULL,
                MYNEWT_VAL(TASK_LED_PRIORITY), 
                OS_WAIT_FOREVER,
                g_task_led_stack,
                MYNEWT_VAL(TASK_LED_STACK_SIZE));
    
    os_task_init(&g_task_blink, "blink",
                task_blink_func,
                NULL,
                MYNEWT_VAL(TASK_BLINK_PRIORITY), 
                OS_WAIT_FOREVER,
                g_task_blink_stack,
                MYNEWT_VAL(TASK_BLINK_STACK_SIZE));

    os_task_init(&g_task_location, "location",
                task_location_func,
                NULL,
                MYNEWT_VAL(TASK_LOCATION_PRIORITY), 
                OS_WAIT_FOREVER,
                g_task_location_stack,
                MYNEWT_VAL(TASK_LOCATION_STACK_SIZE));

    rc = stats_init(
        STATS_HDR(g_model_root_stat),
        STATS_SIZE_INIT_PARMS(g_model_root_stat, STATS_SIZE_32),
        STATS_NAME_INIT_PARMS(model_root_stat_t));
    assert(rc == 0);

    rc = stats_register("mrtls", STATS_HDR(g_model_root_stat));
    assert(rc == 0);
}

#endif