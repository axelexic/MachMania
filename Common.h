//
//  Common.h
//  MachMania
//
//  Created by Yogesh Swami on 2/3/12.
//  Copyright (c) 2012 __MyCompanyName__. All rights reserved.
//

#ifndef MachMania_Common_h
#define MachMania_Common_h

#include <mach/mach_types.h>
#include <mach/message.h>
#include <stdio.h>

#define SERVER_NAME "com.axelexic.mach_ipc_server"
#define MAX_MSG_BODY_SIZE   512
#define MULTIPLEX_ID        500


typedef struct send_message{
    mach_msg_header_t msg_header;
    char        request[MAX_MSG_BODY_SIZE];
}send_message_t;

typedef struct receive_message{
    mach_msg_header_t msg_header;
    char        response[MAX_MSG_BODY_SIZE];
    mach_msg_trailer_t tailer;
}receive_message_t;

#define SEND_MSG_LEN sizeof(send_message_t)
#define RECV_MSG_LEN sizeof(receive_message_t)

void enumerate_ports_with_status(FILE* io);

/*
 * Print the descriptive name, with port index and
 * generation number with it.
 */
int str_mach_port_name(mach_port_name_t name, char* buffer, int len);

/* Print the mach msg header. */
void print_mach_msg_header(FILE* io,
                           const mach_msg_header_t* const hdr);
#endif
