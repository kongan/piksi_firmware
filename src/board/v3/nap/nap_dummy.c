
#include <string.h>

#include "board/nap/track_channel.h"

u8 nap_acq_fft_index_bits;
u8 nap_acq_downsample_stages;

u32 nap_error_rd_blocking(void)
{
  return 0;
}

u32 nap_rw_ext_event(u8 *event_pin, ext_event_trigger_t *event_trig,
		     ext_event_trigger_t next_trig)
{
  (void)event_pin;
  (void)event_trig;
  (void)next_trig;
  return 0;
}

void nap_pps(u64 rising_edge_count_8x)
{
  (void)rising_edge_count_8x;
}

void nap_pps_width(u32 falling_edge_count)
{
  (void)falling_edge_count;
}

bool nap_timing_strobe_wait(u32 timeout)
{
  (void)timeout;
  return true;
}

