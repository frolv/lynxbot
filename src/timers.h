/*
 * timers.h: functions to check bot and stream uptimes
 */

#ifndef TIMERS_H
#define TIMERS_H

#include <time.h>

/* init_timers: initialize bot and channel start timers */
void init_timers(const char *channel, const char *token, const char *cid);

/* check_channel: update stream start timer */
void check_channel(const char *channel, const char *token, const char *cid);

/* bot_uptime: return how long bot has been running */
time_t bot_uptime();

/* channel_uptime: return how long stream has been live */
time_t channel_uptime();

#endif /* TIMERS_H */
