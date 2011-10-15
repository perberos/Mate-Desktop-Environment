/*
 * linc-source.h: This file is part of the linc library.
 *
 * Author:
 *    Michael Meeks  (michael@ximian.com)
 *
 * Copyright 2002, Ximian, Inc.,
 */
#ifndef _LINK_SOURCE_H_
#define _LINK_SOURCE_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * A dead simple, raw source - with fast conditon change.
 */
GSource *link_source_create_watch  (GMainContext *context,
				    int           fd,
				    GIOChannel   *opt_channel,
				    GIOCondition  condition,
				    GIOFunc       func,
				    gpointer      user_data);
void     link_source_set_condition (GSource      *source,
				    GIOCondition  condition);

#ifdef __cplusplus
}
#endif

#endif /* _LINK_SOURCE_H_ */
