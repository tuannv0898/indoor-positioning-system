#ifndef _UWB_CCP_H_
#define _UWB_CCP_H_

#include <stdlib.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <dpl/dpl.h>
#include <hal/hal_timer.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>

#if MYNEWT_VAL(UWB_CCP_STATS)
#include <stats/stats.h>
STATS_SECT_START(uwb_ccp_stat_section)
    STATS_SECT_ENTRY(master_cnt)
    STATS_SECT_ENTRY(slave_cnt)
    STATS_SECT_ENTRY(send)
    STATS_SECT_ENTRY(listen)
    STATS_SECT_ENTRY(tx_complete)
    STATS_SECT_ENTRY(rx_complete)
    STATS_SECT_ENTRY(rx_relayed)
    STATS_SECT_ENTRY(rx_start_error)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(rx_other_frame)
    STATS_SECT_ENTRY(txrx_error)
#if MYNEWT_VAL(UWB_CCP_TOLERATE_MISSED_FRAMES) > 0
    STATS_SECT_ENTRY(err_tolerated)
#endif
    STATS_SECT_ENTRY(tx_start_error)
    STATS_SECT_ENTRY(tx_relay_error)
    STATS_SECT_ENTRY(tx_relay_ok)
    STATS_SECT_ENTRY(irq_latency)
    STATS_SECT_ENTRY(os_lat_behind)
    STATS_SECT_ENTRY(os_lat_margin)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(sem_timeout)
    STATS_SECT_ENTRY(reset)
    STATS_SECT_ENTRY(see_another_master)
STATS_SECT_END
#endif

/** @ingroup api_services
 *  This is the dev base class which utilises the functions to perform initialization and necessary configurations on device.
 *  @{
 */

// XXX This needs to be made bitfield-safe. Not sure the ifdefs below are enough
typedef union _uwb_ccp_timestamp_t{
    struct {
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
        uint64_t lo:40;
        uint64_t hi:23;
        uint64_t halfperiod:1;
#endif
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
        uint64_t halfperiod:1;
        uint64_t hi:23;
        uint64_t lo:40;
#endif
    };
    uint64_t timestamp;
}uwb_ccp_timestamp_t;

//! Timestamps and blink frame format  of uwb_ccp frame.
typedef union {
    //! Frame format of uwb_ccp blink frame.
    struct _uwb_ccp_blink_frame_t{
        struct _ieee_blink_frame_t;
        uint16_t short_address;                 //!< Short Address
        union {
            struct _transmission_interval_struct{
                uint64_t transmission_interval:40; //!< Transmission interval. Also used to correct transmission timestamp in relays
            };
            uint8_t ti_array[sizeof(struct _transmission_interval_struct)];
        }__attribute__((__packed__, aligned(1)));
        uwb_ccp_timestamp_t transmission_timestamp; //!< Transmission timestamp
        uint8_t rpt_count;                      //!< Repeat level
        uint8_t rpt_max;                        //!< Repeat max level
        uint16_t epoch_to_rm_us;                //!< How many uus before the rmarker the epoch is
    }__attribute__((__packed__, aligned(1)));
    uint8_t array[sizeof(struct _uwb_ccp_blink_frame_t)];
}uwb_ccp_blink_frame_t;

//! Timestamps and blink frame format  of uwb_ccp frame.
typedef union {
//! Frame format of uwb_ccp frame.
    struct _uwb_ccp_frame_t{
        struct _uwb_ccp_blink_frame_t;
        uint64_t reception_timestamp;       //!< Reception timestamp
        int32_t carrier_integrator;         //!< Receiver carrier_integrator
        int32_t rxttcko;                    //!< Receiver time tracking offset
    }__attribute__((__packed__, aligned(1)));
    uint8_t array[sizeof(struct _uwb_ccp_frame_t)];
}uwb_ccp_frame_t;

//! Status parameters of uwb_ccp.
struct uwb_ccp_status {
    uint16_t selfmalloc:1;            //!< Internal flag for memory garbage collection
    uint16_t initialized:1;           //!< Instance allocated
    uint16_t valid:1;                 //!< Set for valid parameters
    uint16_t valid_count:3;           //!< Set for valid count
    uint16_t start_tx_error:1;        //!< Set for start transmit error
    uint16_t start_rx_error:1;        //!< Set for start request error
    uint16_t rx_error:1;              //!< Set from error_cb
    uint16_t rx_timeout_error:1;      //!< Receive timeout error
    uint16_t timer_enabled:1;         //!< Indicates timer is enabled
    uint16_t timer_restarted:1;       //!< Indicates timer has been restarted
    uint16_t enabled:1;               //!< Current state of ccp
};

//! Extension ids for services.
typedef enum _uwb_ccp_role_t{
    CCP_ROLE_MASTER = 0x01,                        //!< Clock calibration packet master mode
    CCP_ROLE_SLAVE  = 0x02,                         //!< Clock calibration packet slave mode
    CCP_ROLE_RELAY  = 0x04                         //!< Clock calibration packet master replay mode
}uwb_ccp_role_t;

//! Callback for fetching clock source tof compensation
typedef uint32_t (*uwb_ccp_tof_compensation_cb_t)(uint16_t short_addr);

//! uwb_ccp config parameters.
struct uwb_ccp_config {
    uint16_t postprocess:1;           //!< CCP postprocess
    uint16_t role:4;                  //!< ccp_role_t
    uint16_t tx_holdoff_dly;          //!< Relay nodes holdoff
};


typedef enum{
    CCP_SYNC_LOST,
    CCP_SYNC_SYED
}ccp_sync_t;

typedef void (*uwb_ccp_sync_cb_t)(ccp_sync_t ccp_sync, void *arg);

//! uwb_ccp instance parameters.
struct uwb_ccp_instance {
    struct uwb_dev * dev_inst;                      //!< Pointer to struct uwb_dev
#if MYNEWT_VAL(UWB_CCP_STATS)
    STATS_SECT_DECL(uwb_ccp_stat_section) stat;     //!< Stats instance
#endif
    struct uwb_mac_interface cbs;                   //!< MAC Layer Callbacks
    uint64_t master_euid;                           //!< Clock Master EUID, used to reset wcs if master changes
    struct dpl_sem sem;                             //!< Structure containing os semaphores
    struct dpl_event postprocess_event;             //!< Structure of callout_postprocess
    struct uwb_ccp_status status;                   //!< uwb_ccp status parameters
    struct uwb_ccp_config config;                   //!< uwb_ccp config parameters
    uwb_ccp_timestamp_t master_epoch;               //!< uwb_ccp event referenced to master systime
    uint64_t local_epoch;                           //!< uwb_ccp event referenced to local systime
    uint32_t os_epoch;                              //!< uwb_ccp event referenced to ostime
    uwb_ccp_tof_compensation_cb_t tof_comp_cb;      //!< tof compensation callback
    uint32_t period;                                //!< Pulse repetition period
    uint16_t nframes;                               //!< Number of buffers defined to store the data
    uint16_t idx;                                   //!< Circular buffer index pointer
    uint8_t seq_num;                                //!< Clock Master reported sequence number
    struct hal_timer master_slave_timer;                         //!< Timer structure
    struct dpl_eventq eventq;                       //!< Event queues
    struct dpl_event master_slave_event;                   //!< Event callback
    struct dpl_task task_str;                       //!< Task structure
    uint8_t task_prio;                              //!< Priority based task
    uint16_t blink_frame_duration;                  //!< CCP blink duration in uus
    dpl_stack_t task_stack[MYNEWT_VAL(UWB_CCP_TASK_STACK_SZ)]
        __attribute__((aligned(DPL_STACK_ALIGNMENT))); //!< Task stack size
    struct dpl_event change_role_event;                 //!< Event used to change role
    uint64_t my_master;                                 //!< Address of network master
    bool master_role_request;                            //!< Request for master role has been sent
    uwb_ccp_sync_cb_t uwb_ccp_sync_cb;                   //!< Ccp sync event callback
    void *uwb_ccp_sync_arg;                              //!< Ccp sync event callback argument
    uwb_ccp_role_t uwb_ccp_role;
    bool recv_ccp_reqt;
    uwb_ccp_frame_t * frames[];                          //!< Buffers to uwb_ccp frames
};

uint64_t uwb_ccp_local_to_master(struct uwb_ccp_instance *uwb_ccp, uint32_t timestamp_local);
struct uwb_ccp_instance * uwb_ccp_init(struct uwb_dev* dev,  uint16_t nframes);
void uwb_ccp_free(struct uwb_ccp_instance * inst);
void uwb_ccp_set_postprocess(struct uwb_ccp_instance * inst, dpl_event_fn * uwb_ccp_postprocess);
void ccp_encode(uint64_t epoch, uint64_t transmission_timestamp, uint64_t delta, uint8_t seq_num,  dpl_float64_t carrier_integrator);

void rtls_ccp_start_role(struct uwb_ccp_instance *ccp, uwb_ccp_role_t uwb_ccp_role);
void rtls_ccp_set_sync_cb(struct uwb_ccp_instance *ccp, uwb_ccp_sync_cb_t uwb_ccp_sync_cb, void *arg);

/**
 * @}
 *
 */

#ifdef __cplusplus
}
#endif
#endif /* _UWB_CCP_H_ */
