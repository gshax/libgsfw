#pragma once

#define PACKED_STRUCT(name, body) typedef struct name body __attribute__ ((packed)) name##_t
