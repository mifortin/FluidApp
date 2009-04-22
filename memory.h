/*
 *  memory.h
 *  FluidApp
 */

#ifndef MEMORY_H
#define MEMORY_H

//Provides a means to deal with the memory in the system.  That is, a simple
//way to handle malloc/free where multiple objects may need this one to live
//for a certain amount of time.
//
//Also, a generic way to free the data on a single thread (required for OpenGL)
//
//	All methods are thread-safe!

//Function pointer to deallocator.  Used to free up the memory once it's
//no longer needed.  Required as things remain in memory for a potentially
//longer amount of time.
typedef void(*x_dealloc)(void *o);

//Replacement allocator
void *x_malloc(int size, x_dealloc in_d) __attribute__ ((malloc));

//Replacment free function
void x_free(void *in_o);

//Retain (extension)
void x_retain(void *in_o);

#endif
