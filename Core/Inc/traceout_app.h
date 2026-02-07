/**
 * @file traceout_app.h
 * @brief Application-level TraceX output integration.
 */

#ifndef TRACEOUT_APP_H
#define TRACEOUT_APP_H

#ifndef ENABLE_TRACEX
#define ENABLE_TRACEX 1
#endif /* ENABLE_TRACEX */

#if (ENABLE_TRACEX)

/**
 * @brief Enable CPU cycle counter for TraceX timestamps.
 */
void tracex_enable_cycle_counter(void);

/**
 * @brief Initialize TraceX output for the application.
 *
 * Sets up the trace buffer, transport, and required MCU features.
 */
void TraceOut_AppInit(void);

#endif /* ENABLE_TRACEX */

#endif /* TRACEOUT_APP_H */