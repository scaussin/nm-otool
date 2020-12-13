#include <iostream>
#include <unistd.h>
#include <iomanip>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mach-o/loader.h>
#include <mach-o/nlist.h>
#include <vector>

#define  __darwin__NULL_Ptr_  nullptr

using namespace std;
bool parseMachoHeader(char *dataStart);
void parseLoadCommandSegment(char *dataStart, void *data);
void hexdumpBuf(char *buf, uint64_t len);
void parseSymtab(char *dataStart, void *data);
char get_symbol_letter(nlist_64 *sym);
char match_symbol_section(nlist_64 *symbol);

struct s_segComHrdVecSecHrd{
    s_segComHrdVecSecHrd () : segComHrd({0}), vecSecHrd(0)
    {
        /*segComHrd = {0};
        vecSecHrd = vector<section_64>(0);*/
    }
    segment_command_64 segComHrd;
    vector<section_64> vecSecHrd;
};

vector<s_segComHrdVecSecHrd> vecStructSegComHrdVecSecHrd; //Oh yeah baby !

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

    parseMachoHeader((char *)file_start);

    return 0;
}

bool parseMachoHeader(char *dataStart)
{
    //cout << "taille load command " << sizeof(load_command) << endl;
    mach_header_64 *h = (mach_header_64 *) dataStart;
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
        lc = (struct load_command *) (dataStart + offset);
        //cout << "command: " << lc->cmd << " size " << lc->cmdsize << endl;
        if (lc->cmd == LC_SEGMENT_64)
        {
            parseLoadCommandSegment(dataStart, lc);
        }
        else if (lc->cmd == LC_SYMTAB)
        {
            parseSymtab(dataStart, lc);
        }

        offset += lc->cmdsize;
    }
    void *end = (char *)(dataStart) + offset ;
    return true;
}

void parseLoadCommandSegment(char *dataStart, void *data)
{
    s_segComHrdVecSecHrd a;
    memcpy(&(a.segComHrd), data, sizeof(segment_command_64));


    //cout << "offset: " << a.segComHrd.fileoff << " | addr: " << a.segComHrd.vmaddr << " | size: " << a.segComHrd.filesize<< " | nsects: " << a.segComHrd.nsects << endl;
    for (int i =0; i < a.segComHrd.nsects; i++)
    {
        section_64 *section64 = (section_64 *) ((char *)data + sizeof(segment_command_64) + i * sizeof(section_64));
        a.vecSecHrd.push_back(*section64);

        if (string(section64->segname) + string(section64->sectname) == "__TEXT__text")
        {
            hexdumpBuf(dataStart + section64->offset, section64->size);
        }

        //cout <<"    section64: " << section64->segname << "." << section64->sectname << " addr: " << section64->addr << " off: " << section64->offset << endl;
    }

    vecStructSegComHrdVecSecHrd.push_back(a);
}


char get_symbol_letter(nlist_64 *sym)
{
    if (N_STAB & sym->n_type)
        return 's'; // Debug symbols -> s comme debug
    else if ((N_TYPE & sym->n_type) == N_UNDF)
    {
        /*if (sym->name_not_found)
            return 'C';
        else*/
        if (sym->n_type & N_EXT)
            return 'U';
        else
            return '?';
    } else if ((N_TYPE & sym->n_type) == N_SECT) {

        return match_symbol_section(sym); // We have to match it with our saved sections
    } else if ((N_TYPE & sym->n_type) == N_ABS) {
        return 'A';
    } else if ((N_TYPE & sym->n_type) == N_INDR) {
        return 'I';
    }
    return '$';
}


section_64 *find_mysection(unsigned char section)
{
    unsigned char i = 0;
    for (s_segComHrdVecSecHrd &a : vecStructSegComHrdVecSecHrd)
    {

        for (section_64 &sec : a.vecSecHrd)
        {
            if (section == i)
            {
                return &sec;
            }
            i++;
        }
        if (a.vecSecHrd.empty())
            i++;
    }
    return __darwin__NULL_Ptr_; // 😎
}

char match_symbol_section(nlist_64 *symbol)
{
    char ret = 'X';
    if (section_64 *sect = find_mysection(symbol->n_sect))
    {
        if (!strcmp(sect->sectname, SECT_TEXT))
            ret = 'T';
        else if (!strcmp(sect->sectname, SECT_DATA))
            ret = 'D';
        else if (!strcmp(sect->sectname, SECT_BSS))
            ret = 'B';
        else
            ret = 'S';

        if (!(symbol->n_type & N_EXT))

            ret -= 'A' - 'a';
    }
    return ret;
}
struct symb {
    string name;
    nlist_64 *symbole;
};
vector<symb> symbs;

void parseSymtab(char *dataStart, void *data)
{
    symtab_command sc;
    memcpy(&sc, data, sizeof(symtab_command));
    char *strtab = dataStart + sc.stroff;
    char *symtab = dataStart + sc.symoff;
    uint32_t nbsym = sc.nsyms;
    int i = 0;

    while (i < nbsym)
    {
        char *name = strtab + ( ( struct nlist_64 *)symtab + i)->n_un.n_strx;
        struct nlist_64 *symbole = ( struct nlist_64 *)symtab + i;
        symbs.push_back({string(name), symbole});
        i++;
    }


    sort(symbs.begin(), symbs.end(), [](symb &s1, symb &s2) {
        return (s1.name < s2.name);
    });

    for (auto &s : symbs)
    {
        if (s.symbole->n_value > 0)
        {
           cout << std::setw(16) << std::setfill('0') << hex << s.symbole->n_value << " ";
        }
        else
           cout << "                 ";
        cout << get_symbol_letter(s.symbole) << " " << s.name << endl;
    }

}

void hexdumpBuf(char *buf, uint64_t len)
{
    cout << endl;
    for (uint64_t i = 0; i < len; i++)
    {
        cout << std::setw(2) << std::setfill('0') << std::hex << (uint16_t) ((uint8_t) buf[i]) << " " << std::flush;

        if (i % 8 == 7 && i % 16 != 15)
        {
            cout << " " << std::flush;
        }
        else if (i % 16 == 15)
        {
            cout << endl << std::flush;
        }
    }
    cout << endl << std::dec << std::flush;
}