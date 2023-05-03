#include "ElfViewer.h"
#include "imgui.h"

ElfViewer::ElfViewer(Elf* elf)
{
    this->elf = elf;
}

//function to display Self/Elf window
void ElfViewer::display(bool enabled)
{
    ImGui::Begin("Self/Elf Viewer", &enabled);
    if (elf->isSelfFile())
    {
        if (ImGui::TreeNode("Self"))
        {
            if (ImGui::TreeNode("Self Header"))
            {
                ImGui::TreePop();
            }
            
            if (ImGui::TreeNode("Self Segment Header"))
            {
                ImGui::TreePop();
            }
            if (ImGui::TreeNode("Self Id Header"))
            {
                ImGui::TreePop();
            }
            ImGui::TreePop();
        }
    }
    if (ImGui::TreeNode("Elf"))
    {
        if (ImGui::TreeNode("Elf Header"))
        {
            ImGui::TreePop();
        }

        if (ImGui::TreeNode("Elf Program Headers"))
        {
            const auto* elf_header = elf->GetElfHeader();
            const auto* program_header = elf->GetProgramHeader();
            for (u16 i = 0; i < elf_header->e_phnum; i++)
            {
                if (ImGui::TreeNode((void*)(intptr_t)i, "%d", i))
                {
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }
        if (ImGui::TreeNode("Elf Section Headers"))
        {
            ImGui::TreePop();
        }
        ImGui::TreePop();
    }
    ImGui::End();
    
}