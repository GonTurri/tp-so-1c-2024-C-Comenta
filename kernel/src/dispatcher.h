#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <pcb.h>
#include <proto/proto.h>
#include <commons/log.h>

extern int fd_dispatch;

void send_context_to_cpu(t_exec_context *context);

int wait_for_dispatch_reason(t_log *logger);

#endif