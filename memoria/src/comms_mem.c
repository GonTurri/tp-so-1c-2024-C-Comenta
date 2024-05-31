#include <comms_mem.h>
#include <utils.h>

void process_conn(void *void_args)
{
    t_process_conn_args *args = (t_process_conn_args *)void_args;
    t_log *logger = args->logger;
    int client_fd = args->fd;
    free(args);

    while (client_fd != -1)
    {
        t_packet *packet = packet_new(0);
        if (packet_recv(client_fd, packet) == -1)
        {
            log_warning(logger, "client disconnect");
            packet_free(packet);
            return;
        }
        switch (packet->op_code)
        {
        case CPU_HANDSHAKE:
        {
            msleep(cfg_mem->retardo_respuesta);
            packet_addUInt32(packet, cfg_mem->tam_pagina);
            packet_send(packet, client_fd);
            break;
        }
        case RESIZE_PROCESS:
        {
            msleep(cfg_mem->retardo_respuesta);
            uint32_t pid = packet_getUInt32(packet->buffer);
            uint32_t size = packet_getUInt32(packet->buffer);

            t_process_in_mem *process = find_process_by_pid(pid);
            uint32_t target_pages = ceil((double)size / cfg_mem->tam_pagina);
            log_info(logger, "target: %d", target_pages);

            if (process->current_size < size)
                log_info(logger, "PID: %d - Tamanio actual: %d - Tamanio a Ampliar: %d", pid, process->current_size, size);
            else
                log_info(logger, "PID: %d - Tamanio actual: %d - Tamanio a Reducir: %d", pid, process->current_size, size);

            if (list_size(process->page_table) > target_pages)
            {
                // caso reduccion de espacio de memoria
                // no es lo mas optimo pero bueno

                do
                {
                    process_remove_last_page_from_table(process);
                } while (list_size(process->page_table) > target_pages);
                process->current_size = size;
                packet->op_code = RESIZE_OK;
                packet_send(packet, client_fd);
                break;
            }
            if (list_size(process->page_table) < target_pages)
            {
                // caso ampliacion de espacio
                t_list *possible_frames = list_create();
                int number_of_frames = cfg_mem->tam_memoria / cfg_mem->tam_pagina;
                uint32_t i = 0;
                // esta solucion se queda siempre con los primeros libres pero creo que da iugal
                while (list_size(possible_frames) < target_pages && i < number_of_frames)
                {
                    if (!test_frame(i))
                    {
                        uint32_t *buf = malloc(sizeof(uint32_t));
                        *buf = i;
                        list_add(possible_frames, buf);
                    }
                    i++;
                }
                if (list_size(possible_frames) < target_pages)
                {
                    // caso OUT OF MEMORY, mando el error
                    log_info(logger,"no hay suficiente espacio en memoria!");
                    packet->op_code = OUT_OF_MEMORY;
                    packet_send(packet, client_fd);
                    break;
                }
                void ocuppy_frame(void *elem)
                {
                    uint32_t *frame = (uint32_t *)elem;
                    set_frame_ocuppied(*frame);
                }
                // los marco como ocupados
                list_iterate(possible_frames, ocuppy_frame);
                list_add_all(process->page_table, possible_frames);
                list_destroy(possible_frames);
                // todo listo, le mando un ok
                packet->op_code = RESIZE_OK;
                packet_send(packet, client_fd);
                process->current_size = size;
                break;
            }
            // caso en el que no haya que hacer nada, ojo que el size puede ser distinto
            // lo que importa al final son las paginas con las que me tengo que quedar
            // ejemplo tam_pag = 4 RESIZE 9 = 3 pags luego RESIZE 12 =3 pags
            // deberia entrar por aca
            packet->op_code = RESIZE_OK;
            packet_send(packet, client_fd);
            process->current_size = size;
            break;
        }
        case READ_MEM:
        {
            msleep(cfg_mem->retardo_respuesta);
            t_memory_read_msg *msg = malloc(sizeof(t_memory_read_msg));
            memory_decode_read(packet->buffer, msg);
            log_info(logger, "PID : %d - Accion : LEER - Numero de pagina : %d - Desplazamiento %d", msg->pid, msg->page_number, msg->offset);

            // arbitrary test value
            char *str = string_substring_until("hello boy, how are you doing?", msg->size);
            memory_send_read_ok(client_fd, str, msg->size);
            free(str);

            memory_destroy_read(msg);
            break;
        }
        case WRITE_MEM:
        {
            msleep(cfg_mem->retardo_respuesta);
            t_memory_write_msg *msg = malloc(sizeof(t_memory_write_msg));
            memory_decode_write(packet->buffer, msg);

            log_info(logger, "PID : %d - Accion : ESCRIBIR - Numero de pagina : %d - Desplazamiento %d", msg->pid, msg->page_number, msg->offset);

            char *aux = malloc(msg->size + 1);
            memset(aux, 0x0, msg->size + 1);
            strncpy(aux, msg->value, msg->size);

            log_info(logger, "DATA TO SAVE: %s", aux);
            free(aux);

            memory_send_write_ok(client_fd);

            memory_destroy_write(msg);
            break;
        }
        case CREATE_PROCESS:
        {
            msleep(cfg_mem->retardo_respuesta);
            t_process_in_mem *process = t_process_in_mem_create();
            if (!process)
            {
                log_error(logger, "not enough space for creating process");
                break;
            }
            packet_get_process_in_mem(packet->buffer, process);
            if (!file_exists(process->path))
            {
                log_error(logger, "el archivo de instrucciones de este archivo no fue encontrado");
                t_process_in_mem_destroy(process);
                break;
            }
            log_info(logger, "CREATE PROCESS with pid: %d - path: %s", process->pid, process->path);
            add_process(process);
            break;
        }
        case END_PROCESS:
        {
            msleep(cfg_mem->retardo_respuesta);
            uint32_t pid = packet_getUInt32(packet->buffer);
            remove_process_by_pid(pid);
            log_info(logger, "ENDING PROCESS with pid: %d", pid);
            break;
        }
        case NEXT_INSTRUCTION:
        {
            msleep(cfg_mem->retardo_respuesta);
            log_debug(logger, "NEXT_INSTRUCTION");
            uint32_t pid = packet_getUInt32(packet->buffer);
            uint32_t pc = packet_getUInt32(packet->buffer);
            // BUSCO CUAL DEL LOS ARCHIVOS ESTA EN EXEC
            t_process_in_mem *process = find_process_by_pid(pid);
            if (!process)
            {
                log_error(logger, "process with pid %d not found", pid);
                break;
            }
            char *next_instruction = file_get_nth_line(process->path, pc);
            if (!next_instruction)
            {
                // mandame un error o algo no se
                log_warning(logger, "no hay proxima instruccion");
                packet->op_code = NO_INSTRUCTION;
                packet_send(packet, client_fd);
                break;
            }
            packet_addString(packet, next_instruction);
            packet_send(packet, client_fd);
            free(next_instruction);
            break;
        }
        case -1:
            log_error(logger, "client disconnect");
            packet_free(packet);
            return;

        default:
            log_error(logger, "undefined behaviour with opcode: %d", packet->op_code);
            break;
        }
        packet_free(packet);
    }
}