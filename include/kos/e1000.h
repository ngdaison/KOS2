#ifndef KOS_E1000_H
#define KOS_E1000_H

#include <stdbool.h>
#include <stdint.h>

/* Intel 82540EM (the NIC exposed by QEMU's "-device e1000"). */
bool e1000_initialize(void);
bool e1000_is_initialized(void);
bool e1000_get_mac_address(uint8_t address[6]);
bool e1000_send_frame(const uint8_t *frame, uint64_t size);
bool e1000_wait_for_transmit(uint64_t timeout_ticks);
bool e1000_receive_frame(uint8_t *frame, uint64_t capacity, uint64_t *size);
uint64_t e1000_transmitted_frame_count(void);
uint64_t e1000_received_frame_count(void);
uint64_t e1000_transmit_timeout_count(void);

#endif
