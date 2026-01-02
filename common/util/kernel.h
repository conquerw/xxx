// SPDX-License-Identifier: GPL-2.0-or-later

#ifndef _KERNEL_H_
#define _KERNEL_H_

#ifdef __cplusplus
extern "C"
{
#endif

#define LIST_POISON1 NULL
#define LIST_POISON2 NULL

#define offset(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/**
 * container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 *
 */
#define container_of(ptr, type, member) ({          \
	const typeof(((type *)0)->member)*__mptr = (ptr);    \
		     (type *)((char *)__mptr - offset(type, member)); })
			 
#define WRITE_ONCE(var, val) \
	(*((volatile typeof(val) *)(&(var))) = (val))
	
#define READ_ONCE(var) (*((volatile typeof(var) *)(&(var))))

#ifdef __cplusplus
}
#endif

#endif
