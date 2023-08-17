#pragma once
#include <string>

namespace HLE::Kernel::Objects {
class EqueueInternal {
  public:
    EqueueInternal() = default;
    virtual ~EqueueInternal();
    void setName(const std::string& m_name) { this->m_name = m_name; }

  private:
    std::string m_name;
};
};  // namespace HLE::Kernel::Objects