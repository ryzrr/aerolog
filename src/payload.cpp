#include "payload.h"
#include <math.h>
#include <stdio.h>

static void fmtField(char* out, size_t n, float v) {
  if (isnan(v)) snprintf(out, n, "null");
  else snprintf(out, n, "%.1f", v);
}

size_t formatPayload(char* buf, size_t n, const Reading& r) {
  if (n == 0) return 0;
  char t[16], h[16], p[16];
  fmtField(t, sizeof t, r.temp_c);
  fmtField(h, sizeof h, r.humidity);
  fmtField(p, sizeof p, r.pressure_hpa);
  int len = snprintf(buf, n, "{\"temp_c\":%s,\"humidity\":%s,\"pressure_hpa\":%s}", t, h, p);
  if (len < 0 || (size_t)len >= n) { buf[0] = '\0'; return 0; }
  return (size_t)len;
}
