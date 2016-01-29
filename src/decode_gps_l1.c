/*
 * Copyright (C) 2011-2015 Swift Navigation Inc.
 * Contact: Jacob McNamee <jacob@swiftnav.com>
 *
 * This source is subject to the license found in the file 'LICENSE' which must
 * be be distributed together with this source. All other rights reserved.
 *
 * THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY KIND,
 * EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <libswiftnav/logging.h>
#include <libswiftnav/nav_msg.h>
#include <assert.h>
#include "ephemeris.h"
#include "track.h"
#include "sbp.h"
#include "sbp_utils.h"
#include "decode.h"
#include "decode_gps_l1.h"
#include "l2c_capability.h"

#include "cfs/cfs-coffee.h"
#include "cfs/cfs.h"

#define NUM_GPS_L1_DECODERS   12

typedef struct {
  nav_msg_t nav_msg;
} gps_l1_decoder_data_t;

static decoder_t gps_l1_decoders[NUM_GPS_L1_DECODERS];
static gps_l1_decoder_data_t gps_l1_decoder_data[NUM_GPS_L1_DECODERS];

static void decoder_gps_l1_init(const decoder_channel_info_t *channel_info,
                                decoder_data_t *decoder_data);
static void decoder_gps_l1_disable(const decoder_channel_info_t *channel_info,
                                   decoder_data_t *decoder_data);
static void decoder_gps_l1_process(const decoder_channel_info_t *channel_info,
                                   decoder_data_t *decoder_data);

static const decoder_interface_t decoder_interface_gps_l1 = {
  .sid =          {.constellation = CONSTELLATION_GPS, .band = BAND_L1},
  .init =         decoder_gps_l1_init,
  .disable =      decoder_gps_l1_disable,
  .process =      decoder_gps_l1_process,
  .decoders =     gps_l1_decoders,
  .num_decoders = NUM_GPS_L1_DECODERS
};

static decoder_interface_list_element_t list_element_gps_l1 = {
  .interface = &decoder_interface_gps_l1,
  .next = 0
};

void decode_gps_l1_register(void)
{
  for (u32 i=0; i<NUM_GPS_L1_DECODERS; i++) {
    gps_l1_decoders[i].active = false;
    gps_l1_decoders[i].data = &gps_l1_decoder_data[i];
  }

  decoder_interface_register(&list_element_gps_l1);
}

/* GPS L1 decoder */
static void decoder_gps_l1_init(const decoder_channel_info_t *channel_info,
                                decoder_data_t *decoder_data)
{
  (void)channel_info;
  gps_l1_decoder_data_t *data = decoder_data;
  nav_msg_init(&data->nav_msg);
}

static void decoder_gps_l1_disable(const decoder_channel_info_t *channel_info,
                                   decoder_data_t *decoder_data)
{
  (void)channel_info;
  (void)decoder_data;
}

static void decoder_gps_l1_process(const decoder_channel_info_t *channel_info,
                                   decoder_data_t *decoder_data)
{
  gps_l1_decoder_data_t *data = decoder_data;

  /* Process incoming nav bits */
  s8 soft_bit;
  while (tracking_channel_nav_bit_get(channel_info->tracking_channel,
                                      &soft_bit)) {
    /* Update TOW */
    bool bit_val = soft_bit >= 0;
    s32 TOW_ms = nav_msg_update(&data->nav_msg, bit_val);
    s8 bit_polarity = data->nav_msg.bit_polarity;
    if ((TOW_ms >= 0) && (bit_polarity != BIT_POLARITY_UNKNOWN)) {
      if (!tracking_channel_time_sync(channel_info->tracking_channel, TOW_ms,
                                      bit_polarity)) {
        char buf[SID_STR_LEN_MAX];
        sid_to_string(buf, sizeof(buf), channel_info->sid);
        log_warn("%s TOW set failed", buf);
      }
    }
  }

  /* Check if there is a new nav msg subframe to process.
   * TODO: move this into a function */
  tracking_channel_t *ch = &tracking_channel[channel_info->tracking_channel];
  if ((ch->state != TRACKING_RUNNING) ||
      !subframe_ready(&data->nav_msg))
    return;

  /* Decode ephemeris to temporary struct and get info regarding
   * L2C capability status if it's been changed or not */
  ephemeris_t e = {.sid = channel_info->sid};
  gps_l1ca_decoded_data_t dd = {
    .ephemeris = &e,
    .ephemeris_upd_flag = false,
    .gps_l2c_sv_capability = 0,
    .gps_l2c_sv_capability_upd_flag = false
  };
  s8 ret = process_subframe(&data->nav_msg, &dd);

  if (ret <= 0)
    return;

  /* update l2c capability with value we got after process_subframe */
  if (dd.gps_l2c_sv_capability_upd_flag)
    l2c_capability_update(dd.gps_l2c_sv_capability);


  if(dd.ephemeris_upd_flag) {
    /* Decoded a new ephemeris. */
    ephemeris_new(dd.ephemeris);

    ephemeris_t *eph = ephemeris_get(channel_info->sid);
    if (!eph->healthy) {
      char buf[SID_STR_LEN_MAX];
      sid_to_string(buf, sizeof(buf), channel_info->sid);
      log_info("%s unhealthy", buf);
    }
  }
}
