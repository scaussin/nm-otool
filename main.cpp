#include <iostream>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mach-o/loader.h>

using namespace std;
bool parseMachoHeader(void *data);
void parseLoadCommandSegment(void *data);

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

    parseMachoHeader(file_start);

    return 0;
}

bool parseMachoHeader(void *data)
{
    cout << "taille load command " << sizeof(load_command) << endl;
    mach_header_64 *h = (mach_header_64 *) data;
    if (h->filetype != MH_EXECUTE)
    {
        cout << "not a binary" << endl;
        return false;
    }
    if (h->magic != MH_MAGIC && h->magic != MH_MAGIC_64 )
    {
        cout << "not a 64 bits binary" << endl;
        return false;
    }
    int nbcmds = h->ncmds;
    int offset = sizeof(mach_header_64);
    for (int i = 0; i < nbcmds; i++)
    {
        struct load_command *lc;
        lc = (struct load_command *) ((char*)data + offset);
        //cout << "command: " << lc->cmd << " size " << lc->cmdsize << endl;
        if (lc->cmd == LC_SEGMENT_64)
        {
            parseLoadCommandSegment(lc);
        }

        offset += lc->cmdsize;
        //offset += 80;
    }
    return true;
}

void parseLoadCommandSegment(void *data)
{
    //segment_command_64 *loadCommandSegment = (segment_command_64 *)data;
    segment_command_64 loadCommandSegment;
    memcpy(&loadCommandSegment, data, sizeof(segment_command_64));
    //uint64_t *addr = (uint64_t *)data;
    cout << "offset: " << loadCommandSegment.fileoff << " | addr: " << loadCommandSegment.vmaddr << " | size: " << loadCommandSegment.filesize << endl;
    for (int i =0; i < loadCommandSegment.nsects; i++)
    {
        section_64 *section64 = (section_64 *) ((char *)data + sizeof(segment_command_64) + i * sizeof(section_64));
        cout <<"    section64: " << section64->segname << "." << section64->sectname << endl;
    }

    /*if (loadCommandSegment.nsects)
    {

        section_64 *section64 = (section_64 *) ((char *)data + sizeof(segment_command_64));
        cout << "offset: " << loadCommandSegment.fileoff << " | addr: " << loadCommandSegment.vmaddr << " | size: " << loadCommandSegment.filesize << " | section64: " << section64->sectname << endl;

    }
    else
    {
        cout << "offset: " << loadCommandSegment.fileoff << " | addr: " << loadCommandSegment.vmaddr << " | size: " << loadCommandSegment.filesize << endl;
    }*/

   // cout << "offset: " << loadCommandSegment.fileoff << " | addr: " << loadCommandSegment.vmaddr << " | size: " << loadCommandSegment.filesize << " | section64: " << section64 << endl;
    //cout << addr << endl;
    //section_64 *section = data + loadCommandSegment->fileoff;
}
