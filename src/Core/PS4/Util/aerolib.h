#include <stdint.h>

namespace aerolib {

    struct nid_entry {
        const char* nid;
        const char* name;
    };

    nid_entry* find_by_nid(const char* nid);
};