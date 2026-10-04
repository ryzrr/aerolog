#pragma once
#include <stddef.h>

// Lives here so payload stays hardware independent.
struct Reading { float temp_c, humidity, pressure_hpa; };

// Writes JSON into buf (NUL-terminated); NaN fields become null.
// Returns length written, or 0 if n == 0 or the output would be truncated.
size_t formatPayload(char* buf, size_t n, const Reading& r);
