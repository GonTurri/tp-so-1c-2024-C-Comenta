#ifndef MEMORY_K_H
#define MEMORY_K_H

#include<proto/proto.h>
#include<pcb.h>

extern int fd_memory;

void send_create_process(t_pcb *pcb);

void send_end_process(uint32_t pid);

#endif