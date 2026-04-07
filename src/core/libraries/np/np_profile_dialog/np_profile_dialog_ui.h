#pragma once

#include <string>
#include <variant>

#include "common/fixed_value.h"
#include "common/types.h"
#include "core/libraries/system/commondialog.h"
#include "imgui/imgui_layer.h"

namespace Libraries::Np::NpProfileDialog {

struct NpProfileDialogState {
    std::string onlineId;
    int userId{};
};

class NpProfileDialogUi : public ImGui::Layer {
public:
    explicit NpProfileDialogUi(NpProfileDialogState* state = nullptr, int* result = nullptr);
    ~NpProfileDialogUi();

    NpProfileDialogUi(const NpProfileDialogUi& other) = delete;
    NpProfileDialogUi(NpProfileDialogUi&& other) noexcept;
    NpProfileDialogUi& operator=(NpProfileDialogUi other);

    void Draw() override;

private:
    void Finish(int result_code);

    NpProfileDialogState* state{};
    int* result{};
    bool first_render{true};
};

} // namespace Libraries::Np::NpProfileDialog
