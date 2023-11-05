#pragma once

namespace Core::Loader {
class Elf;
}

class ElfViewer {
public:
    explicit ElfViewer(Core::Loader::Elf* elf);

    void Display(bool enabled);

private:
    Core::Loader::Elf* elf;
};
