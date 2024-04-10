#include <main.h>

static t_log *logger;
static t_config *config;
static int server_fd;
static t_kernel_config *cfg_kernel;

void config_init()
{
    config = config_create(CONFIG_PATH);
    if (!config)
    {
        perror("error al crear el config");
        exit(1);
    }

    cfg_kernel = malloc(sizeof(t_kernel_config));

    cfg_kernel->puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");
    cfg_kernel->ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    cfg_kernel->puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    cfg_kernel->ip_cpu = config_get_string_value(config, "IP_CPU");
    cfg_kernel->puerto_cpu_dispatch = config_get_string_value(config, "PUERTO_CPU_DISPATCH");
    cfg_kernel->puerto_cpu_interrupt = config_get_string_value(config, "PUERTO_CPU_INTERRUPT");
    cfg_kernel->algoritmo_planificacion = config_get_string_value(config, "ALGORITMO_PLANIFICACION");
    cfg_kernel->quantum = config_get_int_value(config, "QUANTUM");
    cfg_kernel->recursos = config_get_array_value(config, "RECURSOS");
    cfg_kernel->instancias_recursos = config_get_array_value(config, "INSTANCIAS_RECURSOS");
    cfg_kernel->grado_multiprogramacion = config_get_int_value(config, "GRADO_MULTIPROGRAMACION");

    config_destroy(config);
}

void init_kernel()
{
    logger = log_create(LOG_PATH, PROCESS_NAME, 1, LOG_LEVEL);
    if (!logger)
    {
        perror("error al crear el logger");
        exit(1);
    }

    config_init();

    server_fd = socket_createTcpServer(NULL, cfg_kernel->puerto_escucha);
    if (server_fd == -1)
    {
        log_error(logger, "error: %s", strerror(errno));
        exit(1);
    }

    log_info(logger, "server starting");
}

void kernel_close()
{
    log_destroy(logger);
    free(cfg_kernel);
    close(server_fd);
}

void sighandler(int signal)
{
    kernel_close();
    exit(0);
}

int main(int argc, char *argv[])
{
    signal(SIGINT, sighandler);
    init_kernel();

    int fd_interrupt = socket_connectToServer(cfg_kernel->ip_cpu, cfg_kernel->puerto_cpu_interrupt);
    int fd_dispatch = socket_connectToServer(cfg_kernel->ip_cpu, cfg_kernel->puerto_cpu_interrupt);
    int fd_memory = socket_connectToServer(cfg_kernel->ip_memoria, cfg_kernel->puerto_memoria);

    if (fd_interrupt == -1 || fd_dispatch == -1 || fd_memory == -1)
    {
        log_error(logger, "err: %s", strerror(errno));
        return 1;
    }
    log_info(logger, "connected to server\n");

    t_packet *packet = packet_new(INTERRUPT_EXEC);
    packet_addString(packet, "hello interrupt port! I'm the kernel");
    packet_addUInt32(packet, 100);
    packet_addString(packet, "bye interrupt port! See u again!");
    packet_send(packet, fd_interrupt);
    printf("packet sent\n");
    packet_free(packet);

    packet = packet_new(EXEC_PROCESS);
    packet_addString(packet, "hello dispatch port! I'm the kernel");
    packet_addUInt32(packet, 100);
    packet_addString(packet, "bye dispatch port! See u again!");
    packet_send(packet, fd_dispatch);
    printf("packet sent\n");
    packet_free(packet);

    packet = packet_new(CREATE_PROCESS);
    packet_addString(packet, "hello memory ! I'm the kernel");
    packet_send(packet, fd_memory);
    printf("packet sent\n");
    packet_free(packet);

    socket_acceptOnDemand(server_fd, logger, process_conn);
    kernel_close();

    return 0;
}
