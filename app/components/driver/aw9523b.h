#ifndef AW9523_H_
#define AW9523_H_

#include "csro_common.h"

typedef struct
{
    uint8_t key_status[4];
    int holdtime[4];
} key_struct;

void csro_aw9523b_init(void);

void csro_aw9523b_test(void);

#endif