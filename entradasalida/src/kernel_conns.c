#include <kernel_conns.h>

static char *io_op_to_string(t_opcode operation);

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
    struct kernel_incoming_message_args *args = (struct kernel_incoming_message_args *)_args;
    int kernel_fd = args->kernel_fd;
    int memory_fd = args->memory_fd;
    t_log *logger = args->logger;
    t_io_config *config = args->config;
    // not freeing args coz they point to the same address in all requests

    uint32_t pid = packet_getUInt32(buffer);
    log_info(logger, "PID: %d - Operacion: %s", pid, io_op_to_string(operation));

    void send_done(void)
    {
        interface_send_io_done(kernel_fd, interface_name, pid);
    }

    void send_error(uint8_t error_code){
        interface_send_io_error(kernel_fd,interface_name,pid,error_code);
    }

    void do_work(int work)
    {
        msleep(config->unidad_trabajo * work);
    }

    switch (operation)
    {
    case IO_GEN_SLEEP:
    {
        t_interface_io_gen_sleep_msg *msg = malloc(sizeof(t_interface_io_gen_sleep_msg));
        interface_decode_io_gen_sleep(buffer, msg);

        do_work(msg->work_units);
        send_done();

        interface_destroy_io_gen_sleep(msg);
        break;
    }
    case IO_STDIN_READ:
    {
        t_interface_io_stdin_read_msg *msg = malloc(sizeof(t_interface_io_stdin_read_msg));
        interface_decode_io_stdin_read(buffer, msg);

        // ask for prompt
        char *str = malloc(msg->size);
        prompt(msg->size, str);

        memory_send_write(memory_fd, pid, msg->page_number, msg->offset, msg->size, str);

        t_packet *res = packet_new(-1);
        if (packet_recv(memory_fd, res) == -1)
        {
            interface_destroy_io_stdin_read(msg);
            packet_free(res);
            free(str);
            log_info(logger, "ERROR WHILE SAVING TO MEMORY");
            send_error(WRITE_MEM_FAILED);
            break;
        }
        if (res->op_code == WRITE_MEM_FAILED){
            interface_destroy_io_stdin_read(msg);
            packet_free(res);
            free(str);
            log_info(logger, "ERROR WHILE SAVING TO MEMORY");
            send_error(WRITE_MEM_FAILED);
            break;
        }

        log_info(logger, "MEMORY SUCCESSFULLY SAVED");

        send_done();

        interface_destroy_io_stdin_read(msg);
        free(str);
        packet_free(res);

        break;
    }
    case IO_STDOUT_WRITE:
    {
        do_work(1);

        t_interface_io_stdout_write_msg *msg = malloc(sizeof(t_interface_io_stdout_write_msg));
        interface_decode_io_stdout_write(buffer, msg);

        memory_send_read(memory_fd, pid, msg->page_number, msg->offset, msg->size);

        t_packet *res = packet_new(-1);
        if (packet_recv(memory_fd, res) == -1)
        {
            log_info(logger, "ERROR WHILE SAVING TO MEMORY");
            send_error(READ_MEM_FAIL);
            packet_free(res);
            interface_destroy_io_stdout_write(msg);
            break;
        }
        if(res->op_code == READ_MEM_FAIL){
            log_info(logger, "ERROR WHILE SAVING TO MEMORY");
            packet_free(res);
            send_error(READ_MEM_FAIL);
            interface_destroy_io_stdout_write(msg);
            break;
        }

        if (res->op_code == READ_MEM_OK)
        {
            t_memory_read_ok_msg *ok_msg = malloc(sizeof(t_memory_read_ok_msg));
            memory_decode_read_ok(res->buffer, ok_msg, msg->size);
            log_info(logger, "MEMORY VALUE: %s", (char *)ok_msg->value);
            memory_destroy_read_ok(ok_msg);
        }

        send_done();
        packet_free(res);
        interface_destroy_io_stdout_write(msg);

        break;
    }
    case IO_FS_CREATE:
    {
        send_done();
        break;
    }
    case IO_FS_DELETE:
    {
        send_done();
        break;
    }
    case IO_FS_TRUNCATE:
    {
        send_done();
        break;
    }
    case IO_FS_READ:
    {
        send_done();
        break;
    }
    case IO_FS_WRITE:
    {
        send_done();
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

static char *io_op_to_string(t_opcode operation)
{
    switch (operation)
    {
    case IO_GEN_SLEEP:
        return "IO_GEN_SLEEP";
    case IO_STDIN_READ:
        return "IO_STDIN_READ";
    case IO_STDOUT_WRITE:
        return "IO_STDOUT_WRITE";
    case IO_FS_CREATE:
        return "IO_FS_CREATE";
    case IO_FS_DELETE:
        return "IO_FS_DELETE";
    case IO_FS_TRUNCATE:
        return "IO_FS_TRUNCATE";
    case IO_FS_READ:
        return "IO_FS_READ";
    case IO_FS_WRITE:
        return "IO_FS_WRITE";
    default:
        return "";
    }
}
