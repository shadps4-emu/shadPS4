#include "core/loader/elf.h"
#include "GUI/ElfViewer.h"
#include "imgui.h"

ElfViewer::ElfViewer(Core::Loader::Elf* elf) {
    this->elf = elf;
}

void ElfViewer::Display(bool enabled) {
    int SELF_HEADER = 0;
    int ELF_HEADER = 1;
    int SEG_HEADER_START = 100;
    int ELF_PROGRAM_HEADER_START = 200;

    static int selected = -1;
    ImGui::Begin("Self/Elf Viewer", &enabled);
   
    ImGui::BeginChild("Left Tree pane", ImVec2(300, 0), false);//left tree
    if (elf->isSelfFile())
    {
        if (ImGui::TreeNode("Self"))
        {            
            if (ImGui::TreeNodeEx("Self Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Self Header"))
            {
                if (ImGui::IsItemClicked())
                    selected = SELF_HEADER;
            }
          
            if (ImGui::TreeNode("Self Segment Header"))
            {
                const auto self = elf->GetSElfHeader();
                for (u16 i = 0; i < self.segment_count; i++)
                {
                    if (ImGui::TreeNodeEx((void*)(intptr_t)i, ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%d", i))
                    {
                        if (ImGui::IsItemClicked())
                            selected = SEG_HEADER_START+i;
                    }
                }
                ImGui::TreePop();
            }
            ImGui::TreeNodeEx("Self Id Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Self Id Header");
            ImGui::TreePop();
        }
    }
    if (ImGui::TreeNode("Elf"))
    {
        const auto elf_header = elf->GetElfHeader();
        if (ImGui::TreeNodeEx("Elf Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Elf Header"))
        {
            if (ImGui::IsItemClicked())
                selected = ELF_HEADER;
        }
        if (ImGui::TreeNode("Elf Program Headers"))
        {
            const auto pheader = elf->GetProgramHeader();
            for (u16 i = 0; i < elf_header.e_phnum; i++)
            {
                std::string ProgramInfo = elf->ElfPheaderFlagsStr(pheader[i].p_flags) + " " + elf->ElfPheaderTypeStr(pheader[i].p_type);
                if (ImGui::TreeNodeEx((void*)(intptr_t)i,ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "%d - %s", i,ProgramInfo.c_str()))
                {
                    if (ImGui::IsItemClicked())
                        selected = ELF_PROGRAM_HEADER_START + i;
                }
            }
            ImGui::TreePop();
        }
        if (elf_header.e_shnum != 0)
        {
            if (ImGui::TreeNode("Elf Section Headers"))
            {
                ImGui::TreePop();
            }
        }
        ImGui::TreePop();
    }
    ImGui::EndChild();//end of left tree

    ImGui::SameLine();

    ImGui::BeginChild("Table View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
    if (selected == SELF_HEADER) {
        ImGui::TextWrapped("%s", elf->SElfHeaderStr().c_str());
    }
    if (selected >= 100 && selected < 200)
    {
        ImGui::TextWrapped("%s", elf->SELFSegHeader(selected-100).c_str());
    }
    if (selected == ELF_HEADER) {
        ImGui::TextWrapped("%s", elf->ElfHeaderStr().c_str());
    }
    if (selected >= 200 && selected < 300)
    {
        ImGui::TextWrapped("%s", elf->ElfPHeaderStr(selected - 200).c_str());
    }
    ImGui::EndChild();
    ImGui::End();
}
