#include <rikerio/rikerio.h>
#include <stdint.h>

uint8_t scenes[12][2] =
{
    { 0x00, 0 },
    { 0x01, 0 },
    { 0x05, 0 },
    { 0x15, 0 },
    { 0x55, 0 },
    { 0xd4, 0 },
    { 0xf0, 0 },
    { 0xe8, 0 },
    { 0xaa, 1 },
    { 0x2A, 0 },
    { 0x0a, 0 },
    { 0x02, 0 },
};

adr_t** start_adr;
uint8_t current_scene = 0;
uint32_t adr_offset = 0;

char* start_link;

void task_init(task_t* task)
{

    start_adr = linker_get(task->client, start_link);

    task_done(task, RIO_OK);

}

void task_loop(task_t* task)
{

    buffer_setl_uint8(task->io, start_adr, scenes[current_scene][0]);

    current_scene = (current_scene + 1) % 12;


    task_done(task, RIO_OK);

}

void task_quit(task_t* task)
{
    task_done(task, RIO_OK);
}

int main(int argc, char* argv[])
{

    if (argc < 3)
    {
        printf("Syntax: snake rikerio-id link-name\n");
        return -1;
    }

    start_link = argv[2];

    log_init(argv[1]);
    task_t task = { 0 };
    task.name = argv[1];
    task.handler.init = task_init;
    task.handler.loop = task_loop;
    task.handler.quit = task_quit;

    int create_resp = task_create(&task);
    if (create_resp == -1)
    {
        log_error("Error creating task.");
        return -1;
    }
    log_info("Trying to connect to server.");
    int conn_ret = task_connect(&task);
    if (conn_ret == -1)
    {
        task_destroy(&task);
        log_error("Connection NOT established.");
        return -1;
    }
    task_start(&task);
    task_destroy(&task);
    return 0;

}
