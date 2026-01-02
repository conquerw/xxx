/* SPDX-License-Identifier: GPL-2.0-or-later */

#include <fcntl.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <openssl/pem.h>
#include <openssl/bio.h>
#include <sys/capability.h>
#include "log.h"
#include "util.h"

void set_root_caps(void)
{
    cap_value_t cap_list[] = {CAP_SYS_NICE};

    cap_t caps = cap_get_proc();
    if(!caps)
	{
        perror("cap_get_proc failed");
		return;
	}
	
	int ret = cap_set_flag(caps, CAP_EFFECTIVE, 1, cap_list, CAP_SET);
	if(ret == -1)
	{
        perror("cap_set_flag effective failed");
		goto exit;
    }

	ret = cap_set_flag(caps, CAP_PERMITTED, 1, cap_list, CAP_SET);
	if(ret == -1)
	{
        perror("cap_set_flag permitted failed");
		goto exit;
    }

	ret = cap_set_proc(caps);
	if(ret == -1)
	{
        perror("cap_set_proc failed");
		goto exit;
    }

exit:
    cap_free(caps);
}

int read_file(char *buffer, int buffer_len, char *path)
{
	if(!buffer || buffer_len == 0 || !path)
		return -1;
	
	int fd = open(path, O_RDONLY);
	if(fd < 0)
	{
		LOG_ERROR("open %s is failed\n", path);
		return -1;
	}
	
	int ret = read(fd, buffer, buffer_len);
	if(ret > buffer_len || ret < 0)
	{
		LOG_ERROR("read %s is failed\n", path);
		close(fd);
		return -1;
	}
	
	close(fd);
	
	return ret;
}

void hex_print(char *name, char *buffer, int buffer_len)
{
	if(!name || !buffer)
		return;
	
    printf("%s, len: %d, ", name, buffer_len);
    for (int i = 0; i < buffer_len; i++)
        printf("%02X", (unsigned char)buffer[i]);
    printf("\n");
}

void char_to_hex(char *out, char *in, int in_len)
{
	if(!out || !in)
		return;
	
    char high = 0, low = 0;
    for(int i = 0; i < in_len; i++)
    {
        if(*in >= '0' && *in <= '9')
            high = *in - '0';
        else if(*in >= 'A' && *in <= 'Z')
            high = *in - 'A' + 0x0A;
        else if(*in >= 'a' && *in <= 'z')
            high = *in - 'a' + 0x0A;
		
        in++;
		
        if(*in >= '0' && *in <= '9')
            low = *in - '0';
        else if(*in >= 'A' && *in <= 'Z')
            low = *in - 'A' + 0x0A;
        else if(*in >= 'a' && *in <= 'z')
            low = *in - 'a' + 0x0A;
		
        *out = high << 4 | low;
        out++;
        in++;
    }
}

void hex_to_char(char *out, char *in, int in_len)
{
	if(!out || !in)
		return;
	
    for(int i = 0; i < in_len; i++)
    {
        char high = *in >> 4 & 0x0F;
        char low = *in & 0x0F;
        if(high > 9)
            high = high + 'A' - 0x0A;
        else
            high = high + '0';
        if(low > 9)
            low = low + 'A' - 0x0A;
        else
            low = low + '0';
        *out = high;
        out++;
        *out = low;
        out++;
        in++;
    }
}

void short_to_net_byte(char *out, unsigned short in)
{
	if(!out)
		return;
	
	short bytes = 0x1234;
	if(*(char *)&bytes == 0x34)
	{
		int out_len = sizeof(unsigned short);
		for(int i = 0; i < out_len; i++)
			*(out + i) = *((char *)&in + out_len - 1 - i);
	}
}

void int_to_net_byte(char *out, unsigned int in)
{
	if(!out)
		return;
	
	short bytes = 0x1234;
	if(*(char *)&bytes == 0x34)
	{
		int out_len = sizeof(unsigned int);
		for(int i = 0; i < out_len; i++)
			*(out + i) = *((char *)&in + out_len - 1 - i);
	}
}

void longlong_to_net_byte(char *out, unsigned long long in)
{
	if(!out)
		return;
	
	short bytes = 0x1234;
	if(*(char *)&bytes == 0x34)
	{
		int out_len = sizeof(unsigned long long);
		for(int i = 0; i < out_len; i++)
			*(out + i) = *((char *)&in + out_len - 1 - i);
	}
}

void net_byte_to_short(unsigned short *out, char *in, int in_len)
{
	if(!out || !in || in_len != 2)
		return;
	
	short bytes = 0x1234;
	if(*(char *)&bytes == 0x34)
	{
		for(int i = 0; i < in_len; i++)
			*((char *)out + i) = *(in + in_len - 1 - i);
	}
}

void net_byte_to_int(unsigned int *out, char *in, int in_len)
{
	if(!out || !in || in_len != 4)
		return;
	
	short bytes = 0x1234;
	if(*(char *)&bytes == 0x34)
	{
		for(int i = 0; i < in_len; i++)
			*((char *)out + i) = *(in + in_len - 1 - i);
	}
}

void net_byte_to_longlong(unsigned long long *out, char *in, int in_len)
{
	if(!out || !in || in_len != 8)
		return;
	
	short bytes = 0x1234;
	if(*(char *)&bytes == 0x34)
	{
		for(int i = 0; i < in_len; i++)
			*((char *)out + i) = *(in + in_len - 1 - i);
	}
}

int base64_encode(char *out, char *in, int in_len)
{
	if(!out || !in || in_len == 0)
		return -1;
	
	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
	
	BIO_write(bio, in, in_len);
	BIO_flush(bio);
	
	BUF_MEM *bptr = NULL;
	BIO_get_mem_ptr(bio, &bptr);
	int len = bptr->length;
	memcpy(out, bptr->data, len);
	
	len = strlen(out);
	
	BIO_free_all(bio);
	
	return len;
}

int base64_decode(char *out, char *in, int in_len)
{
	if(!out || !in || in_len == 0)
		return -1;
	
	BIO *b64 = BIO_new(BIO_f_base64());
	BIO *bio = BIO_new_mem_buf(in, in_len);
	bio = BIO_push(b64, bio);
	
	BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
	
	int len = BIO_read(bio, out, in_len);
	
	BIO_free_all(bio);
	
	return len;
}

void xor(char *out, char *in, int in_len)
{
	if(!out || !in || in_len == 0)
		return;
	
	*out = *(in + 1);
	for(int i = 2; i < in_len; i++)
		*out ^= *(in + i);
}

int nema_check(char *buffer)
{
	if(!buffer)
		return -1;
	
	// char *asterisk = strchr(buffer, '*');
	// if(asterisk)
	// {
	// 	static int len = 0;
	// 	len = asterisk - buffer;
		
	// 	static char check_sum = 0;
	// 	xor(&check_sum, buffer, len);
		
	// 	char check_sum2 = 0;
	// 	string_to_hex(&check_sum2, asterisk + 1, 2);
	// 	if(check_sum == check_sum2)
	// 		return len + 3;
	// 	else
	// 		LOG_WARN("checksum value is not equal, check_sum-check_sum2: %02X-%02X\n", check_sum, check_sum2);
	// }
	
	return -1;
}

static int invert_short(unsigned short *out, unsigned short *in)
{
	if(!out || !in)
		return -1;
	
	unsigned short temp = 0;
	for(unsigned int i = 0; i < 16; i++)
	{
		if(*in & (1 << i))
			temp |= 1 << (15 - i);
	}
	*out = temp;
	
	return 1;
}

static unsigned short crc16_invert(unsigned char *in, int in_len, crc16_type_e crc16_type)
{
	unsigned short crc_in = 0x0000;
	unsigned short crc_poly = 0x1021;
	unsigned short crc_xor = 0x0000;
	
	if(crc16_type == crc16_modbus || crc16_type == crc16_usb)
		crc_in = 0xFFFF;
	if(crc16_type == crc16_ibm || crc16_type == crc16_modbus || crc16_type == crc16_usb)
		crc_poly = 0x8005;
	if(crc16_type == crc16_usb)
		crc_xor = 0xFFFF;
	
	invert_short(&crc_poly, &crc_poly);
	
    while(in_len--)
    {
        crc_in ^= *in++;
        for(int i = 0; i < 8; i++)
        {
            if(crc_in & 0x0001)
                crc_in = (crc_in >> 1) ^ crc_poly;
            else
                crc_in = (crc_in >> 1);
        }
    }
	
    return crc_in ^ crc_xor;
}

unsigned short crc16(char *in, int in_len, crc16_type_e crc16_type)
{
	unsigned short ret = 0;
	if(crc16_type == crc16_ccitt || crc16_type == crc16_ibm || crc16_type == crc16_modbus || crc16_type == crc16_usb)
		ret = crc16_invert((unsigned char *)in, in_len, crc16_type);
	
	return ret;
}
