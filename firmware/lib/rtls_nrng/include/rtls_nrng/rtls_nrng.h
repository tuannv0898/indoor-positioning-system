/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef _NRNG_H_
#define _NRNG_H_

#include <stdlib.h>
#include <stdint.h>

#include <hal/hal_spi.h>
#include <uwb/uwb.h>
#include <uwb/uwb_ftypes.h>
#include <euclid/triad.h>
#include <stats/stats.h>
#include <rtls_nrng/slots.h>

#if MYNEWT_VAL(UWB_RNG_ENABLED)
#include <uwb_rng/uwb_rng.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

#if MYNEWT_VAL(NRNG_STATS)
    STATS_SECT_START(nrng_stat_section)
    STATS_SECT_ENTRY(nrng_request)
    STATS_SECT_ENTRY(nrng_listen)
    STATS_SECT_ENTRY(rx_complete)
    STATS_SECT_ENTRY(rx_timeout)
    STATS_SECT_ENTRY(complete)
    STATS_SECT_ENTRY(rx_error)
    STATS_SECT_ENTRY(start_rx_error)
    STATS_SECT_ENTRY(rx_unsolicited)
    STATS_SECT_ENTRY(tx_error)
    STATS_SECT_ENTRY(start_tx_error)
    STATS_SECT_ENTRY(reset)
STATS_SECT_END

#define NRNG_STATS_INC(__X) STATS_INC(nrng->stat, __X)
#else
#define NRNG_STATS_INC(__X) {}
#endif

//! Range configuration parameters.
struct uwb_rng_config{
   uint32_t rx_holdoff_delay;        //!< Delay between frames, in UWB usec.
   uint32_t tx_holdoff_delay;        //!< Delay between frames, in UWB usec.
   uint32_t tx_guard_delay;          //!< Delay between frames from subsequent nodes, in UWB sec.
   uint16_t rx_timeout_delay;        //!< Receive response timeout, in UWB usec.
   uint16_t bias_correction:1;       //!< Enable range bias correction polynomial
   uint16_t fctrl_req_ack:1;         //!< Enable ack request
};

//! Range control parameters.
typedef struct _uwb_rng_control_t{
    uint16_t delay_start_enabled:1;  //!< Set for enabling delayed start
    uint16_t complete_after_tx:1;    //!< Set by ranging state machine to say that exchange is complete after next tx
}uwb_rng_control_t;

//! Range status parameters
typedef struct _uwb_rng_status_t{
    uint16_t selfmalloc:1;           //!< Internal flag for memory garbage collection
    uint16_t initialized:1;          //!< Instance allocated
    uint16_t mac_error:1;            //!< Error caused due to frame filtering
    uint16_t invalid_code_error:1;   //!< Error due to invalid code
    uint16_t tx_ack_expected:1;      //!< Acknowledge expected
    uint16_t rx_ack_expected:1;      //!< Acknowledge expected
}uwb_rng_status_t;

//! List of range types available
struct rng_config_list {
    uint16_t rng_code;
    const char* name;
    struct uwb_rng_config *config;
    SLIST_ENTRY(rng_config_list) next;
};
typedef enum _nrng_device_type_t{
    DWT_NRNG_INITIATOR,
    DWT_NRNG_RESPONDER
}nrng_device_type_t;

//! N-Ranges request frame
typedef union {
    struct _nrng_request_frame_t{
        struct _ieee_rng_request_frame_t;
        struct _slot_payload_t; //!< slot bitfields for request
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _nrng_request_frame_t)]; //!< Array of size nrng request frame
} nrng_request_frame_t;

//! N-Ranges response frame
typedef union {
    struct _nrng_response_frame_t{
       struct _ieee_rng_response_frame_t;
       uint8_t slot_id;  //!< slot_idx of transmitting anchor
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _nrng_response_frame_t)]; //!< Array of size nrng response frame
} nrng_response_frame_t;

//! N-Ranges response frame
typedef union {
    struct  _nrng_final_frame_t{
        struct _nrng_response_frame_t;
        uint32_t request_timestamp;
        uint32_t response_timestamp;
        int32_t carrier_integrator;
    }__attribute__((__packed__,aligned(1)));
    uint8_t array[sizeof(struct _nrng_final_frame_t)]; //!< Array of size range final frame
} nrng_final_frame_t;

//! N-Ranges ext response frame format
typedef union {
    struct _nrng_frame_t{
        struct _nrng_final_frame_t;
        union {
            struct uwb_dev_rxdiag diag;
            uint8_t diag_storage[MYNEWT_VAL(UWB_DEV_RXDIAG_MAXLEN)];
        };
#if MYNEWT_VAL(TWR_DS_EXT_NRNG_ENABLED)
        union {
            struct _twr_data_t;                            //!< Structure of twr_data
            uint8_t payload[sizeof(struct _twr_data_t)];   //!< Payload of size twr_data
        };
#endif
    } __attribute__((__packed__, aligned(1)));
    uint8_t array[sizeof(struct _nrng_frame_t)];        //!< Array of size twr_frame
} nrng_frame_t;

struct nrng_instance{
    struct uwb_dev * dev_inst;
#if MYNEWT_VAL(NRNG_STATS)
    STATS_SECT_DECL(nrng_stat_section) stat; //!< Stats instance
#endif
    uint16_t nframes;
    uint16_t nnodes;
    uint16_t slot_mask;
    uint16_t valid_mask;
    uint16_t cell_id;
    uint16_t resp_count;
    uint16_t t1_final_flag;
    uint64_t delay;
    uint8_t seq_num;
    struct dpl_sem sem;                          //!< Structure of semaphores
    struct uwb_mac_interface cbs;                //!< MAC Layer Callbacks
    nrng_device_type_t device_type;
    uwb_rng_status_t status;
    uwb_rng_control_t control;
    struct uwb_rng_config config;
    uint16_t idx;
    SLIST_HEAD(, rng_config_list) rng_configs;
    nrng_frame_t * frames[];
};

struct nrng_instance * nrng_init(struct uwb_dev * inst, struct uwb_rng_config * config, nrng_device_type_t type, uint16_t nframes, uint16_t nnodes);
struct uwb_dev_status nrng_request_delay_start(struct nrng_instance * nrng, uint16_t dst_address, uint64_t delay, uwb_dataframe_code_t code, uint16_t start_slot_id, uint16_t end_slot_id);
struct uwb_dev_status nrng_request(struct nrng_instance * nrng, uint16_t dst_address, uwb_dataframe_code_t code, uint16_t start_slot_id, uint16_t end_slot_id);
float nrng_twr_to_tof_frames(struct uwb_dev * inst, nrng_frame_t *first_frame, nrng_frame_t *final_frame);
void nrng_set_frames(struct nrng_instance * nrng, uint16_t nframes);
struct uwb_dev_status nrng_config(struct nrng_instance * nrng, struct uwb_rng_config * config);
struct uwb_rng_config * nrng_get_config(struct nrng_instance * nrng, uwb_dataframe_code_t code);
struct uwb_dev_status nrng_listen(struct nrng_instance * nrng, uwb_dev_modes_t mode);
uint32_t nrng_get_uids(struct nrng_instance * nrng, uint16_t uids[], uint16_t nranges, uint16_t base);
uint32_t nrng_get_ranges(struct nrng_instance * nrng, dpl_float32_t ranges[], uint16_t nranges, uint16_t base);
uint32_t nrng_get_ranges_addresses(struct nrng_instance * nrng, float ranges[], uint16_t addresses[], bool updated[], uint16_t nranges, uint16_t base);
uint32_t nrng_get_tofs_addresses(struct nrng_instance * nrng, uint32_t tofs[], uint16_t addresses[], uint8_t updated[], uint16_t nranges, uint16_t base);
uint32_t usecs_to_response(struct uwb_dev * inst, uint16_t nslots, struct uwb_rng_config * config, uint32_t duration);

void nrng_append_config(struct nrng_instance * nrng, struct rng_config_list *cfgs);
void nrng_remove_config(struct nrng_instance * nrng, uwb_dataframe_code_t code);


#ifdef __cplusplus
}
#endif
#endif /* _NRNG_H_ */
