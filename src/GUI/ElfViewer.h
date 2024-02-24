// SPDX-FileCopyrightText: Copyright 2024 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

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
