/*
 *  error.h
 *  FluidApp
 */

//Errors are in a linked list (for multiple errors), and sub-errors.
//	Errors can be "handled" at some point.

#ifndef ERROR_H
#define ERROR_H

#define error_none		0		/* no error */
#define error_memory	1		/* out of memory */
#define error_flags		2		/* bad flags */
#define error_create	3		/* creation error (socket, file, etc.) */
#define error_net		4		/* error from network transmission */
#define error_duplicate	5		/* when adding something that already exists */
#define error_specify	6		/* Got an error - becoming more explicit */
#define error_thread	7		/* Error caused by threading issues. */
#define error_script	8		/* Error having to do with a scripting engine */
#define error_timeout	9		/* Elapsed waiting time exceeded. */

typedef struct error error;

error *errorCreate(error *in_prev, int in_code, const char *in_text, ...);
error *errorReply(error *in_error, int in_reply_code, const char *in_text, ...);

error *errorNext(error *in_error);

error *errorReplyTo(error *in_error);

//Frees an error and all sub-errors.
void errorFree(error *in_error);


int errorCode(error *in_error);
const char *errorMsg(error *in_error);
#endif
