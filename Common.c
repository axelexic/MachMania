//
//  Common.c
//  MachMania
//
//  Created by Yogesh Swami on 2/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "Common.h"
#include <mach/port.h>
#include <mach/mach_port.h>
#include <mach/mach_init.h>
#include <mach/vm_map.h>
#include <assert.h>
#include <stdarg.h>
#include <unistd.h>
#include <mach/port.h>
#include <mach/message.h>

/* defined in ipc_object.h */
#define	IO_BITS_KOTYPE		0x00000fff
#define ENUM2STR(x) case (x): return #x
#define ENUM2PTR(x,y) case (x): (y) = #x;break


static char* str_local_remote(int range){
    char* msg;
    if (range == 0) {
        return "NONE";
    }

    switch (range) {
            ENUM2PTR(MACH_MSG_TYPE_MOVE_RECEIVE, msg);
            ENUM2PTR(MACH_MSG_TYPE_MOVE_SEND, msg);
            ENUM2PTR(MACH_MSG_TYPE_MOVE_SEND_ONCE, msg);
            ENUM2PTR(MACH_MSG_TYPE_COPY_SEND, msg);
            ENUM2PTR(MACH_MSG_TYPE_MAKE_SEND, msg);
            ENUM2PTR(MACH_MSG_TYPE_MAKE_SEND_ONCE, msg);
            ENUM2PTR(MACH_MSG_TYPE_COPY_RECEIVE, msg);
        default:
            msg = NULL;
            assert(FALSE);
    }
    return msg;
}

static int str_mach_msgh_bits(mach_msg_bits_t bits, char* buffer, int len){

    int used=0;
    int remote = MACH_MSGH_BITS_REMOTE(bits);
    int local = MACH_MSGH_BITS_LOCAL(bits);

    if (MACH_MSGH_BITS_ZERO == bits) {
        return snprintf(buffer, len, "%s", "MACH_MSGH_BITS_ZERO");
    }

    if (bits&MACH_MSGH_BITS_COMPLEX) {
        used+= snprintf(buffer, len, "%s | ", "MACH_MSGH_BITS_COMPLEX");
    }
    used += snprintf(buffer+used, len-used, "<%s> | ", str_local_remote(remote));
    used += snprintf(buffer+used, len-used, "<%s>", str_local_remote(local));
    return used;
}

static void print_aligned(FILE* io, int width, const char* fmt, ...){
    int consumed = 0;
    va_list ap;
    va_start(ap, fmt);
    consumed = vfprintf(io, fmt, ap);
    va_end(ap);
    consumed = width - consumed;
    while (consumed-- > 0) {
        fprintf(io, " ");
    }
}

static void print_types(FILE* io, int width, mach_port_type_t type){
    if(type & MACH_PORT_TYPE_RECEIVE){
        width -= fprintf(io, "r ");
    }

    if(type & MACH_PORT_TYPE_SEND){
        width -= fprintf(io, "s ");
    }

    if(type & MACH_PORT_TYPE_SEND_ONCE){
        width -= fprintf(io, "so ");
    }

    if(type & MACH_PORT_TYPE_DEAD_NAME){
        width -= fprintf(io, "dn ");
    }
    if(type & MACH_PORT_TYPE_LABELH){
        width -= fprintf(io, "l ");
    }
    while (width-- > 0) {
        fprintf(io, " ");
    }
}

static void print_uref(FILE* io,
                       int width,
                       mach_port_name_t name,
                       mach_port_right_t right){
    mach_port_urefs_t uref;
    kern_return_t     result;
    int consumed = 0;

    result = mach_port_get_refs(mach_task_self(),
                                name,
                                right,
                                &uref);
    if (result == KERN_SUCCESS) {
        consumed += fprintf(io, "%u", uref);
        width = width - consumed;
    }
    /* if we fail, we just print blank space. */

    while (width-- > 0 ) {
        fprintf(io, " ");
    }
}

static void print_kern_obj(FILE* io, int width, mach_port_name_t name){
    natural_t object_type; // This is the IPC object type
    mach_vm_address_t object_addr; // This is the addr of kernel object
    kern_return_t result;
    int consumed = 0;

    result = mach_port_kobject(mach_task_self(),
                               name,
                               &object_type,
                               &object_addr);

    if(result == KERN_SUCCESS){
        if (object_addr) {
            consumed += fprintf(io,
                                "0x%llx, 0x%.3x",
                                object_addr,
                                object_type & IO_BITS_KOTYPE);
        }else{
            consumed += fprintf(io, "NULL");
        }
        width = width - consumed;
    }

    while (width-- > 0) {
        fprintf(io, " ");
    }
}

void enumerate_ports_with_status(FILE* io){
    char buffer[128];
    mach_port_name_array_t names;
    mach_port_type_array_t types;
    mach_msg_type_number_t namesCount;
    mach_msg_type_number_t typesCount;

    kern_return_t result;

	if(io == NULL){
		io = stdout;
	}

    result = mach_port_names(mach_task_self(),
                             &names,
                             &namesCount,
                             &types,
                             &typesCount);
    assert(namesCount == typesCount);

    fprintf(io, "Number of ports for pid (%d): %d\n\n", getpid(), namesCount);
    fprintf(io, "PortIndex |  Port  |  Recv  |  Send  | SO     |    Kern Addr,      \n");
    fprintf(io, "   :gen   |  Type  |  Ref   |  Ref   | Ref    |    Obj Type        \n");
    fprintf(io, "----------+--------+--------+--------+--------+------------------------------\n");
    //              10       7         7         7      7          30


    for (int i = 0; i<namesCount; i++) {
        str_mach_port_name(names[i], buffer, 128);
        print_aligned(io, 10, "%s", buffer);
        fprintf(io, "| ");
        print_types(io, 7, types[i]);
        fprintf(io, "| ");
        print_uref(io, 7, names[i], MACH_PORT_RIGHT_RECEIVE);
        fprintf(io, "| ");
        print_uref(io, 7, names[i], MACH_PORT_RIGHT_SEND);
        fprintf(io, "| ");
        print_uref(io, 7, names[i], MACH_PORT_RIGHT_SEND_ONCE);
        fprintf(io, "| ");
        print_kern_obj(io, 30, names[i]);
        fprintf(io, "\n");
    }

    fprintf(io, "\n\n");

    /* kernel does a vm_allocate in the current process address space. */
    vm_deallocate(mach_task_self(),
                  (vm_address_t)names,
                  namesCount*sizeof(mach_port_name_t));

    vm_deallocate(mach_task_self(),
                  (vm_address_t)types,
                  typesCount*sizeof(mach_port_type_t));
}

/*
 * Print the descriptive name, with port index and
 * generation number with it.
 */
int str_mach_port_name(mach_port_name_t name, char* buffer, int len){
    mach_port_name_t index, gen;
    index = MACH_PORT_INDEX(name);
    gen = MACH_PORT_GEN(name) >> 24;

    if (index==0) {
        return snprintf(buffer, len, "MACH_PORT_NULL");
    }

    if (gen != 0) {
        // This is a user defined port name.
        return snprintf(buffer, len, "%d:%d", index, gen);
    }else {
        return snprintf(buffer, len, "%d:usr",index);
    }
}

/* Print the mach msg header. */
void print_mach_msg_header(FILE* io, const mach_msg_header_t* const hdr){
    char msgh_bit[512];
    char msgh_remote_port[128];
    char msgh_local_port[128];

    if (io == NULL) {
        io = stdout;
    }
    str_mach_msgh_bits(hdr->msgh_bits, msgh_bit, 1024);
    str_mach_port_name(hdr->msgh_remote_port, msgh_remote_port, 128);
    str_mach_port_name(hdr->msgh_local_port, msgh_local_port, 128);

    fprintf(io, "Mach Message Header: %p\n{\n"
            "\tmsgh_bits        : %s\n"
            "\tmsgh_id          : %d\n"
            "\tmsgh_size        : %d\n"
            "\tmsgh_remote_port : %s\n"
            "\tmsgh_local_port  : %s\n"
            "}\n",
            hdr,
            msgh_bit,
            hdr->msgh_id,
            hdr->msgh_size,
            msgh_remote_port,
            msgh_local_port
            );
}
