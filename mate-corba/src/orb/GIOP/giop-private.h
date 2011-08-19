#ifndef GIOP_PRIVATE_H
#define GIOP_PRIVATE_H 1

#include "config.h"
#include <matecorba/matecorba-config.h>
#include <matecorba/GIOP/giop.h>
#include <linc/linc.h>

void giop_send_buffer_init     (gboolean wipe);
void giop_recv_buffer_init     (void);

#endif
