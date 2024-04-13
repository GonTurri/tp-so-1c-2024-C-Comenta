#ifndef MEM_H
#define MEM_H

#include <stdlib.h>
#include <stdio.h>
#include <utils/hello.h>
#include <errno.h>
#include <sockets/sockets.h>
#include <commons/log.h>
#include <commons/config.h>
#include <comms_mem.h>
#include <sys/signal.h>

#define CONFIG_PATH "mem.config"
#define LOG_LEVEL LOG_LEVEL_INFO
#define LOG_PATH "mem.log"
#define PROCESS_NAME "MEMORIA"

typedef struct 
{
char * puerto_escucha;
int tam_memoria;
int tam_pagina;
char* path_instrucciones;
int retardo_respuesta;
}t_mem_config;


#endif