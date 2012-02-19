//
//  main.c
//  mach_ipc_server
//
//  Created by Yogesh Swami on 2/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#include <stdio.h>
#include "Common.h"
#include <servers/bootstrap.h>
#include <mach/mach_traps.h>
#include <mach/mach_init.h>
#include <mach/task_special_ports.h>
#include <mach/mach_error.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#define ERR_EXIT(result, args...) do{      \
    if(result != KERN_SUCCESS){            \
        fprintf(stderr, args);             \
        fprintf(stderr, " Error: %s\n", mach_error_string(result)); \
        exit(-result);                          \
    }                                           \
}while(0)

static void reverse(const char* const source, char* dest){
    size_t len=strlen(source);
    for (size_t i = 0; i<len>>1; i++) {
        dest[len-i] = source[i];
    }
    dest[len] = '\0';
}


int main(){
    mach_port_t boot_port;
    mach_port_t server_port;
    mach_port_t task_self_port = task_self_trap();
    kern_return_t result;
    receive_message_t received_msg;
    send_message_t    sent_msg;
    mach_msg_header_t* mach_hdr;
    char                print_buffer[1024];

    str_mach_port_name(task_self_port, print_buffer, 1024);
    
    printf("Task self port: %s\n", print_buffer);
    
    result = task_get_special_port(task_self_port,
                                   TASK_NAME_PORT,
                                   &boot_port);
    ERR_EXIT(result, "task_get_special_port failed.");
    
    str_mach_port_name(boot_port, print_buffer, 1024);
    printf("Name port: %s\n", print_buffer);
    
    /* Get the bootstrap port for the current login context. */
    result = task_get_bootstrap_port(task_self_port, &boot_port);
    ERR_EXIT(result, "task_get_bootstrap_port failed.");
    str_mach_port_name(boot_port, print_buffer, 1024);
    printf("Boot strap port: %s\n", print_buffer);

    /* check in our service directly. No need for resgister etc. */
    result = bootstrap_check_in(boot_port, 
                                SERVER_NAME, 
                                &server_port);
    ERR_EXIT(result, "Bootstrap Checkin Failed.\n");
    str_mach_port_name(server_port, print_buffer, 1024);
    printf("Server port: %s\n", print_buffer);

	/* prepare for doing the real work. */
    
    do {
        
        mach_hdr = &(received_msg.msg_header);
        
        mach_hdr->msgh_id           = MULTIPLEX_ID;
        mach_hdr->msgh_local_port   = MACH_PORT_NULL;
        mach_hdr->msgh_remote_port  = MACH_PORT_NULL;
        mach_hdr->msgh_size         = sizeof(received_msg);
        mach_hdr->msgh_bits         = 0;
        
        result = mach_msg(mach_hdr, 
                          MACH_RCV_MSG, 
                          0, 
                          mach_hdr->msgh_size, 
                          server_port, 
                          MACH_MSG_TIMEOUT_NONE, 
                          MACH_PORT_NULL);
        
        if(result != KERN_SUCCESS){
            fprintf(stderr, "mach_msg(receive) failed. Error: %s\n",
                    mach_error_string(result));
			continue;
        }
		
        fprintf(stdout, 
                "Received message with ID: %d\n",
                received_msg.msg_header.msgh_id
                );

        
        reverse(received_msg.response, sent_msg.request);
        
        mach_hdr=&(sent_msg.msg_header);
        mach_hdr->msgh_id               = MULTIPLEX_ID;
        mach_hdr->msgh_remote_port      = received_msg.msg_header.msgh_remote_port;
        mach_hdr->msgh_local_port       = MACH_PORT_NULL;
        mach_hdr->msgh_bits             = received_msg.msg_header.msgh_bits;
        mach_hdr->msgh_size             = sizeof(sent_msg);
        
        result = mach_msg(mach_hdr,
                          MACH_SEND_MSG, 
                          mach_hdr->msgh_size, 
                          0, 
                          MACH_PORT_NULL, 
                          MACH_MSG_TIMEOUT_NONE, 
                          MACH_PORT_NULL);

        if(result != KERN_SUCCESS){
            fprintf(stderr, "mach_msg(send) failed. Error: %s\n",
                    mach_error_string(result));
			continue;
        }
		
        fprintf(stdout, "Successfully sent the message.\n");
    } while (1);
    return 0;
}