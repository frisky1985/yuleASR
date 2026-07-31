/*==================================================================================================
 * projdefs.h - FreeRTOS project definitions (yuleASR port)
 * Provides pdTRUE/pdFALSE/pdPASS/pdFAIL and task function type.
 *================================================================================================*/
#ifndef PROJDEFS_H
#define PROJDEFS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Type used by the task function. */
typedef void (*TaskFunction_t)( void * );

/* FreeRTOS boolean/result constants. */
#define pdTRUE                              ( ( BaseType_t ) 1 )
#define pdFALSE                             ( ( BaseType_t ) 0 )

#define pdPASS                              ( pdTRUE )
#define pdFAIL                              ( pdFALSE )

#define pdMS_TO_TICKS( xTimeInMs )          ( ( TickType_t ) ( ( ( TickType_t ) ( xTimeInMs ) * configTICK_RATE_HZ ) / ( TickType_t ) 1000 ) )

#ifdef __cplusplus
}
#endif

#endif /* PROJDEFS_H */
