//
//  main.c
//  mach_ipc_client
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
#include <mach/mach_port.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>

#define ERR_EXIT(result, args...) do{      \
    if(result != KERN_SUCCESS){            \
        fprintf(stderr, args);             \
        fprintf(stderr, " Error: %s\n", mach_error_string(result)); \
        exit(-result);                          \
    }                                           \
}while(0)

static void reverse(const char* const source, char* dest){
    size_t len=strlen(source);
    for (size_t i = 0; i<len; i++) {
        dest[len-i] = source[i];
    }
    dest[len] = '\0';
}

#define SHORTCUT(x) { .identifier = #x, .type_name = x }

static struct mach_msg_type_name_to_value{
    char* identifier;
    mach_msg_bits_t type_name;
} MM[]={
    SHORTCUT(MACH_MSG_TYPE_MOVE_RECEIVE),
    SHORTCUT(MACH_MSG_TYPE_MOVE_SEND),
    SHORTCUT(MACH_MSG_TYPE_MOVE_SEND_ONCE),
    SHORTCUT(MACH_MSG_TYPE_COPY_SEND),
    SHORTCUT(MACH_MSG_TYPE_MAKE_SEND),
    SHORTCUT(MACH_MSG_TYPE_MAKE_SEND_ONCE),
    SHORTCUT(MACH_MSG_TYPE_COPY_RECEIVE)
};

#define MM_SIZE (sizeof(MM)/sizeof(MM[0]))

static mach_msg_bits_t get_name_to_bits(const char* name){
    for (int i = 0; i<MM_SIZE; i++) {
        size_t len = strlen(MM[i].identifier);
        if (strncmp(name, MM[i].identifier, len) == 0) {
            return MM[i].type_name;
        }
    }
    return -1;
}

static void usage(int argc, char* argvp[]){
    fprintf(stderr, "Usage: %s -l <local_port_disposition> "
            "-r <remote_port_disposition>\n", argvp[0]);
    fprintf(stderr, "Possible port dispositions are:\n");
    for (int i = 0; i<MM_SIZE; i++) {
        fprintf(stderr, "\t%s\n", MM[i].identifier);
    }
}

int main(int argc, char* argvp[]){
    char print_buffer[1024];
    mach_port_t boot_port = bootstrap_port;
    mach_port_t server_port;
    mach_port_t recv_port;
    mach_port_t task_self_port = task_self_trap();
    kern_return_t result;
    receive_message_t received_msg;
    send_message_t    sent_msg;
    mach_msg_header_t* mach_hdr;
    mach_msg_bits_t remote_bits = -1, local_bits = -1;
    int choice;

    while((choice = getopt(argc, argvp, "r:l:")) != -1){
        switch(choice){
            case 'r':
                remote_bits = get_name_to_bits(optarg);
                break;
            case 'l':
                local_bits = get_name_to_bits(optarg);
                break;
            default:
                usage(argc, argvp);
                return -1;
        }
    }

    if ((remote_bits == -1) || (local_bits == -1)) {
        usage(argc, argvp);
        return -1;
    }

    str_mach_port_name(task_self_port, print_buffer, 1024);
    printf("Task self port: %s\n", print_buffer);

    str_mach_port_name(boot_port, print_buffer, 1024);
    printf("Bootstrap port: %s\n", print_buffer);

    /* check in our service directly. No need for resgister etc. */
    result = bootstrap_look_up(boot_port,
                                SERVER_NAME,
                                &server_port);
    ERR_EXIT(result, "Bootstrap look_up failed.\n");
    str_mach_port_name(server_port, print_buffer, 1024);
    printf("Server port: %s\n", print_buffer);

    result = mach_port_allocate(task_self_port,
                                MACH_PORT_RIGHT_RECEIVE,
                                &recv_port);
    ERR_EXIT(result, "mach_port_allocate failed.\n");

    str_mach_port_name(recv_port, print_buffer, 1024);
    printf("Bootstrap port: %s\n", print_buffer);

    /* prepare for doing the real work. */

    do {

        memset(&sent_msg, 0, sizeof(sent_msg));
        printf("Enter something to send to server\n");
        scanf("%s", sent_msg.request);

        mach_hdr=&(sent_msg.msg_header);
        mach_hdr->msgh_id               = MULTIPLEX_ID;
        mach_hdr->msgh_remote_port      = server_port;
        mach_hdr->msgh_local_port       = recv_port;
        mach_hdr->msgh_bits             = MACH_MSGH_BITS(remote_bits,
                                                         local_bits);

        mach_hdr->msgh_size             = sizeof(sent_msg);

        fprintf(stdout, "**** Sending Message with header:\n");
        print_mach_msg_header(stdout, mach_hdr);
        fprintf(stdout, "\n\nList of ports and their status:\n");
        enumerate_ports_with_status(stdout);
        // Use LLDB to insert the above statement to find out
        // all the available ports in the process.
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
            break;
        }

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
                          recv_port,
                          MACH_MSG_TIMEOUT_NONE,
                          MACH_PORT_NULL);

        fprintf(stdout, "**** Received Message with header:\n");
        print_mach_msg_header(stdout, mach_hdr);

        if(result != KERN_SUCCESS){
            fprintf(stderr, "mach_msg(receive) failed. Error: %s\n",
                    mach_error_string(result));
		}
        fprintf(stdout,
                "Received message with ID: %d\nContents: %s\n",
                received_msg.msg_header.msgh_id,
                received_msg.response
                );
    } while (1);
    return 0;
}
