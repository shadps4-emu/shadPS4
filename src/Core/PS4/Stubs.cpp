#include "Stubs.h"

#include "Util/aerolib.h"

#include "Util/log.h"

// Helper to provide stub implementations for missing functions
// 
// This works by pre-compiling generic stub functions ("slots"), and then
// on lookup, setting up the nid_entry they are matched with
// 
// If it runs out of stubs with name information, it will return
// a default implemetnation without function name details

// Up to 512, larger values lead to more resolve stub slots
// and to longer compile / CI times
// 
// Must match STUBS_LIST define
#define MAX_STUBS 128

u64 UnresolvedStub() {
    LOG_ERROR("Unknown Stub: called, returning zero\n");
    return 0;
}

static u64 UnknownStub() {
    LOG_ERROR("Unknown Stub: called, returning zero\n");
    return 0;
}


static aerolib::nid_entry* stub_nids[MAX_STUBS];

template <int stub_index>
static u64 CommonStub() {
    auto entry = stub_nids[stub_index];

    LOG_ERROR("Stub: {} ({}) called, returning zero\n", entry->name, entry->nid);
    return 0;
}

static u32 UsedStubEntries;

#define XREP_1(x)        \
    &CommonStub<x>,

#define XREP_2(x) XREP_1(x) XREP_1(x + 1)
#define XREP_4(x) XREP_2(x) XREP_2(x + 2)
#define XREP_8(x) XREP_4(x) XREP_4(x + 4)
#define XREP_16(x) XREP_8(x) XREP_8(x + 8)
#define XREP_32(x) XREP_16(x) XREP_16(x + 16)
#define XREP_64(x) XREP_32(x) XREP_32(x + 32)
#define XREP_128(x) XREP_64(x) XREP_64(x + 64)
#define XREP_256(x) XREP_128(x) XREP_128(x + 128)
#define XREP_512(x) XREP_256(x) XREP_256(x + 256)

#define STUBS_LIST XREP_128(0)

static u64 (*stub_handlers[MAX_STUBS])() = {
    STUBS_LIST
};

u64 GetStub(const char* nid) {
	auto entry = aerolib::find_by_nid(nid);
    if (!entry || UsedStubEntries >= MAX_STUBS) {
        return (u64)&UnknownStub;
    } else {
        stub_nids[UsedStubEntries] = entry;
        return (u64)stub_handlers[UsedStubEntries++];
    }
}