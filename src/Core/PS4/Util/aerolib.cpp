#include "aerolib.h"

#include "types.h"

#include <string.h>

#include "Util/log.h"

namespace aerolib {

// Use a direct table here + binary search as contents are static
nid_entry nids[] = {
#define STUB(nid, name) \
    { nid, #name },
#include "aerolib.inl"
#undef STUB
};

nid_entry* find_by_nid(const char* nid) {
    s64 l = 0;
    s64 r = sizeof(nids) / sizeof(nids[0]) - 1;

    while (l <= r) {
        size_t m = l + (r - l) / 2;

        int cmp = strcmp(nids[m].nid, nid);

        if (cmp == 0)
            return &nids[m];
        else if (cmp < 0)
            l = m + 1;
        else
            r = m - 1;
    }

    return nullptr;
}
}  // namespace aerolib