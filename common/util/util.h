// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _UTIL_H_
#define _UTIL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define SET_BIT0(value, bit) ((value) &= ~(1 << (bit)))
#define SET_BIT1(value, bit) ((value) |= (1 << (bit)))

typedef enum
{
	crc16_ccitt = 1,
	crc16_ibm,
	crc16_modbus,
	crc16_usb
} crc16_type_e;

void set_root_caps(void);

int read_file(char *buffer, int buffer_len, char *path);

void hex_print(char *name, char *buffer, int buffer_len);

void char_to_hex(char *out, char *in, int in_len);

void hex_to_char(char *out, char *in, int in_len);

void short_to_net_byte(char *out, unsigned short in);

void int_to_net_byte(char *out, unsigned int in);

void longlong_to_net_byte(char *out, unsigned long long in);

void net_byte_to_short(unsigned short *out, char *in, int in_len);

void net_byte_to_int(unsigned int *out, char *in, int in_len);

void net_byte_to_longlong(unsigned long long *out, char *in, int in_len);

int base64_encode(char *out, char *in, int in_len);

int base64_decode(char *out, char *in, int in_len);

void xor(char *out, char *in, int in_len);

int nema_check(char *buffer);

unsigned short crc16(char *in, int in_len, crc16_type_e crc16_type);

#ifdef __cplusplus
}
#endif

#endif
