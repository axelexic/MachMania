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

/* defined in ipc_object.h */
#define	IO_BITS_KOTYPE		0x00000fff 


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
    mach_port_name_array_t names;
    mach_port_type_array_t types;
    mach_msg_type_number_t namesCount;
    mach_msg_type_number_t typesCount;
    
    kern_return_t result;
    
    result = mach_port_names(mach_task_self(), 
                             &names,
                             &namesCount, 
                             &types,
                             &typesCount);
    assert(namesCount == typesCount);
    
    fprintf(io, "   Port   |  Port  |  Recv  |  Send  | SO     |    Kern Addr,      \n");
    fprintf(io, "   Name   |  Type  |  Ref   |  Ref   | Ref    |    Obj Type        \n");
    fprintf(io, "----------+--------+--------+--------+--------+------------------------------\n");
    //              10       7         7         7      7          30
    
    
    for (int i = 0; i<namesCount; i++) {
        print_aligned(io, 10, "0x%.4x", names[i]);
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
    
    
    
    /* kernel does a vm_allocate in the current process address space. */
    vm_deallocate(mach_task_self(), 
                  (vm_address_t)names, 
                  namesCount*sizeof(mach_port_name_t));
    
    vm_deallocate(mach_task_self(), 
                  (vm_address_t)types, 
                  typesCount*sizeof(mach_port_type_t));
}
