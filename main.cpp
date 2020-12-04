//#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>

//using namespace std;

int main(int ac, char **av)
{
    if (ac != 2)
    {
        return 1;
    }

    struct stat buf = {0};
    int32_t fd;
    void *file_start;

    if ((fd = open(av[1], O_RDONLY)) < 0)
        return 1;
    if (fstat(fd, &buf) < 0)
        return 1;
    if (buf.st_size == 0)
        return 1;
    if ((file_start = mmap(nullptr, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0)) == MAP_FAILED)
        return 1;


    return 0;
}

