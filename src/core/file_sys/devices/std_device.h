#include "core/file_sys/devices/logger.h"
#include "core/file_sys/quasifs/quasifs_inode_device.h"

class std_device : public QuasiFS::Device {
private:
    Core::Devices::Logger* logger;

public:
    std_device(std::string name, bool is_err) {
        this->logger = new Core::Devices::Logger(name, is_err);
    }
    ~std_device() {
        delete logger;
    }

    virtual s64 pwrite(const void* buf, size_t count, u64 offset) override {
        return logger->write(buf, count);
    }
};
