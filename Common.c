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

#define ENUM2STR(X) case (X): return #X 

static char* port_type_2_name(mach_port_type_t type){
    
    switch (type) {
            ENUM2STR(MACH_PORT_TYPE_NONE);
            ENUM2STR(MACH_PORT_TYPE_SEND);
            ENUM2STR(MACH_PORT_TYPE_RECEIVE);
            ENUM2STR(MACH_PORT_TYPE_SEND_ONCE);
            ENUM2STR(MACH_PORT_TYPE_PORT_SET);
            ENUM2STR(MACH_PORT_TYPE_DEAD_NAME);
            ENUM2STR(MACH_PORT_TYPE_LABELH);
            ENUM2STR(MACH_PORT_TYPE_SEND_RECEIVE);
            ENUM2STR(MACH_PORT_TYPE_SEND_RIGHTS);
            ENUM2STR(MACH_PORT_TYPE_PORT_RIGHTS);
            ENUM2STR(MACH_PORT_TYPE_PORT_OR_DEAD);
            ENUM2STR(MACH_PORT_TYPE_ALL_RIGHTS);
            ENUM2STR(MACH_PORT_TYPE_DNREQUEST);
            ENUM2STR(MACH_PORT_TYPE_SPREQUEST);
            ENUM2STR(MACH_PORT_TYPE_SPREQUEST_DELAYED);
    }
    return "Unknown Type";
}

void port_introspect(FILE* io, mach_port_name_t port_desc){
    mach_port_type_t ptype;
    kern_return_t result;
    
    result = mach_port_type(mach_task_self(), 
                            port_desc, 
                            &ptype);
    if (result == KERN_SUCCESS) {
        fprintf(io, "port 0x%.4x > port_type: %s\n", 
                ptype, 
                port_type_2_name(ptype));
    }
}

