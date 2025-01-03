#include <common/logging/log.h>
#include "net_obj.h"

namespace Libraries::Net {

static void convertOrbisNetSockaddrToPosix(const OrbisNetSockaddr* src, sockaddr* dst) {
    if (src == nullptr || dst == nullptr)
        return;
    memset(dst, 0, sizeof(sockaddr));
    const OrbisNetSockaddrIn* src_in = (const OrbisNetSockaddrIn*)src;
    sockaddr_in* dst_in = (sockaddr_in*)dst;
    dst_in->sin_family = src_in->sin_family;
    dst_in->sin_port = src_in->sin_port;
    memcpy(&dst_in->sin_addr, &src_in->sin_addr, 4);
}

static int convert_error_codes(int retval) {
    if (retval < 0) {
        LOG_INFO(Lib_Net, "function returned an error = {}", retval);
    }
    return retval;
}

int NetInternal::net_socket(int family, int type, int protocol) {
    std::scoped_lock lock{m_mutex};
    s_socket sock = ::socket(family, type, protocol);
    auto id = ++next_sock_id;
    socks.emplace(id, sock);
    LOG_INFO(Lib_Net, "socket created with id = {}", id);
    return id;
}

int NetInternal::send_packet(s_socket sock, const void* msg, unsigned int len, int flags,
                             const OrbisNetSockaddr* to, unsigned int tolen) {
    sockaddr addr2;
    convertOrbisNetSockaddrToPosix(to, &addr2);
    return convert_error_codes(
        sendto(sock, (const char*)msg, len, flags, &addr2, sizeof(sockaddr_in)));
}
s_socket NetInternal::findsock(int sockid) {
    std::scoped_lock lock{m_mutex};
    const auto it = socks.find(sockid);
    if (it != socks.end()) {
        return it->second;
    }
    return 0;
}
} // namespace Libraries::Net