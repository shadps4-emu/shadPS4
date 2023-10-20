#pragma once
#include <types.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <optional>
#include <vector>

#include "Lib/Threads.h"

namespace Emulator::Host::Fs {

class MntPoints {
  public:
    struct MntPair {
        std::string host_path;
        std::string guest_path;  // e.g /app0/
    };

    MntPoints() {}
    virtual ~MntPoints() {}
    void mount(const std::string& host_folder, const std::string& guest_folder);
    void unMount(const std::string& path);
    void unMountAll();

  private:
    std::vector<MntPair> m_mnt_pairs;
    Lib::Mutex m_mutex;

};
struct File {
    bool valid = false;          // Το descriptor ειναι οντως ανοιχτο;
    FILE* file;                  // File handle του αρχειο
    std::filesystem::path path;  // Path του στο host FS

    File() : valid(true) {}
};

class HandleTable {
    std::vector<File> files;
    u32 openFileCount = 0;  // Ποσα descriptors εχουμε ανοιχτα;

  public:
    static constexpr u32 MAX_FILE_DESCRIPTORS = 65536;

    HandleTable() {
        files.reserve(128);  // Κανουμε reserve χωρο για μερικα αρχεια για να αποφυγουμε allocations
        reset();
    }

    void reset() {
        for (auto& f : files) {
            if (f.valid) {
                // Κανουμε fclose το αρχειο και ο,τι αλλο ειναι να γινει
            }
        }

        files.clear();
        openFileCount = 0;
    }

    // Επιστρεφει handle για να δημιουργησουμε νεο αρχείο
    std::optional<u32> createHandle() {
        if (openFileCount >= MAX_FILE_DESCRIPTORS) {
            // Δεν μπορουμε να ανοιξουμε αλλα descriptors, εχουμε βαρεσει το max
            return std::nullopt;
        }

        // Μπορουμε σιγουρα να δημιουργησουμε handle, οποτε αυξανουμε το handle count
        openFileCount += 1;

        // Κοιταμε αν κανενα απτα files στο vector ειναι ελευθερο για να το κανουμε reuse
        for (size_t i = 0; i < files.size(); i++) {
            // Βρηκαμε ελευθερο αρχειο, επιστρεφουμε το index του
            if (!files[i].valid) {
                return i;
            }
        }

        // Δεν υπαρχει ελευθερο στο vector αλλα δεν εχουμε εξαντλησει το cap, οποτε μεγαλωνουμε το vector
        // Και επιστρεφουμε το index του καινουργιου
        u32 handle = files.size();
        files.push_back(File());

        return handle;
    }

    void freeHandle(u32 handle) {
        if (handle >= files.size()) {
            // Στανταρ invalid
            printf("POUTSA\n");
            return;
        }

        if (files[handle].valid) {
            // Παλι κανουμε fclose και πιπες και το μαρκαρουμε για reuse
            files[handle].valid = false;
            // Μειον ενα ανοιχτο αρχειο
            openFileCount -= 1;
        }
    }
};
}  // namespace Emulator::Host::Fs