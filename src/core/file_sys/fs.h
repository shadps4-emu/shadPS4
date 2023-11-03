#pragma once
#include <mutex>
#include <string>
#include <vector>

namespace Core::FileSys {

class MntPoints {
  public:
    struct MntPair {
        std::string host_path;
        std::string guest_path;  // e.g /app0/
    };

    MntPoints() {}
    virtual ~MntPoints() {}
    void mount(const std::string& host_folder, const std::string& guest_folder);
    void unmount(const std::string& path);
    void unmountAll();
    std::string getHostDirectory(const std::string& guest_directory);

  private:
    std::vector<MntPair> m_mnt_pairs;
    std::mutex m_mutex;
};

}  // namespace Core::FileSys