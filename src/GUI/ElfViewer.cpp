#include "ElfViewer.h"
#include "imgui.h"

ElfViewer::ElfViewer(Elf* elf)
{
    this->elf = elf;
}

//function to display Self/Elf window
void ElfViewer::display(bool enabled)
{
    enum
    {
        ELF_HEADER
    };
    static int selected = -1;
    ImGui::Begin("Self/Elf Viewer", &enabled);
   
    ImGui::BeginChild("Left Tree pane", ImVec2(200, 0), false);//left tree
    if (elf->isSelfFile())
    {
        if (ImGui::TreeNode("Self"))
        {            
            ImGui::TreeNodeEx("Self Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Self Header");
          
            if (ImGui::TreeNode("Self Segment Header"))
            {
                ImGui::TreePop();
            }
            ImGui::TreeNodeEx("Self Id Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Self Id Header");
            ImGui::TreePop();
        }
    }
    if (ImGui::TreeNode("Elf"))
    {
        if (ImGui::TreeNodeEx("Elf Header", ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen, "Elf Header"))
        {
            if (ImGui::IsItemClicked())
                selected = ELF_HEADER;
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
    ImGui::EndChild();//end of left tree

    ImGui::SameLine();

    ImGui::BeginChild("Table View", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
    if (selected == ELF_HEADER) {
        ImGui::Text("hello");
    }
    ImGui::EndChild();
    selected = -1;
    ImGui::End();
    
}