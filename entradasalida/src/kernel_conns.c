#include <kernel_conns.h>

static char* io_op_to_string(t_opcode operation);

int registerResourceInKernel(int kernel_fd, t_log *logger, t_io_config *cfg_io)
{
    log_info(logger, "Registering i/o interface %s", cfg_io->name);
    t_packet *packet = packet_new(NEW_INTERFACE);
    packet_addString(packet, cfg_io->name);
    packet_addString(packet, cfg_io->tipo_interfaz);
    int res = packet_send(packet, kernel_fd);
    packet_free(packet);

    return res;
}

void handleKernelIncomingMessage(uint8_t client_fd, uint8_t operation, t_buffer *buffer, void *_args)
{
    struct kernel_incoming_message_args *args =  (struct kernel_incoming_message_args *) _args;
    int kernel_fd = args->kernel_fd;
    t_log *logger = args->logger;
    t_io_config *config = args->config;
    // not freeing args coz they point to the same address in all requests
    free(_args);
    // si va el free marquitos, lo que estas liberando es el espacio reservado para los campos 
    // de args, no los args en si

    // todo operation dictionary to print the name instead of the number
    uint32_t pid = packet_getUInt32(buffer);
    log_info(logger, "PID: %d - Operacion: %d", pid, operation);

    void sendDone(uint8_t operation)
    {
        t_packet *packet = packet_new(operation);
        packet_send(packet, kernel_fd);
        packet_free(packet);
    }

    switch (operation)
    {
    case IO_GEN_SLEEP:
    {
        sleep(msToSeconds(config->unidad_trabajo));
        sendDone(IO_DONE);
        log_info(logger, "PID: %d - Operacion: %s DONE", pid, io_op_to_string(operation));
        break;
    }
    case IO_STD_IN_READ:
    {
        sendDone(IO_DONE);
        break;
    }
    case IO_STD_OUT_WRITE:
    {
        sendDone(IO_DONE);
        break;
    }
    case IO_FS_CREATE:
    {
        sendDone(IO_DONE);
        break;
    }
    case IO_FS_DELETE:
    {
        sendDone(IO_DONE);
        break;
    }
    case IO_FS_TRUNCATE:
    {
        sendDone(IO_DONE);
        break;
    }
    case IO_FS_READ:
    {
        sendDone(IO_DONE);
        break;
    }
    case IO_FS_WRITE:
    {
        sendDone(IO_DONE);
        break;
    }
    case DESTROY_PROCESS:
    {
        char *response = packet_getString(buffer);
        log_info(logger, "Hi I am IO i received %s", response);
        free(response);
        break;
    }
    default:
    {
        log_error(logger, "undefined behaviour with opcode: %d", operation);
        break;
    }
    }
}

static char* io_op_to_string(t_opcode operation){
    switch(operation){
        case IO_GEN_SLEEP: return "IO_GEN_SLEEP";
        case IO_STD_IN_READ: return "IO_STDIN_READ";
        case IO_STD_OUT_WRITE: return "IO_STDOUT_WRITE";
        case IO_FS_CREATE: return "IO_FS_CREATE";
        case IO_FS_DELETE: return "IO_FS_DELETE";
        case IO_FS_TRUNCATE: return "IO_FS_TRUNCATE";
        case IO_FS_READ: return "IO_FS_READ";
        case IO_FS_WRITE: return "IO_FS_WRITE";
        default: return "";
    }
}
