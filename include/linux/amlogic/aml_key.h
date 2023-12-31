/*
 * include/linux/amlogic/aml_key.h
 *
 * Copyright (C) 2017 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#ifndef _AML_KEY_H_
#define _AML_KEY_H_

enum user_id {
	DSC_LOC_DEC,
	DSC_NETWORK,
	DSC_LOC_ENC,

	CRYPTO_T0 = 0x100,
};

enum key_algo {
	KEY_ALGO_AES,
	KEY_ALGO_TDES,
	KEY_ALGO_DES,
	KEY_ALGO_CSA2,
	KEY_ALGO_CSA3,
	KEY_ALGO_NDL,
	KEY_ALGO_ND
};

struct key_descr {
	unsigned int key_index;
	unsigned int key_len;
	unsigned char key[32];
};

struct key_config {
	unsigned int key_index;
	int key_userid;
	int key_algo;
};

struct key_alloc {
	int is_iv;
	unsigned int key_index;
};

#define KEY_ALLOC         _IOWR('o', 64, struct key_alloc)
#define KEY_FREE          _IO('o', 65)
#define KEY_SET           _IOR('o', 66, struct key_descr)
#define KEY_CONFIG        _IOR('o', 67, struct key_config)
#define KEY_GET_FLAG      _IOWR('o', 68, struct key_descr)

//int dmx_key_init(void);
//void dmx_key_exit(void);

#endif

