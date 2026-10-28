// SPDX-FileCopyrightText: Copyright 2026 shadPS4 Emulator Project
// SPDX-License-Identifier: GPL-2.0-or-later

// The net library inside the kernel: net ids are file descriptors from the kernel's file table,
// the kernel's close/read/write/fstat reach them, and the libkernel / libScePosix socket calls
// report errors the POSIX way (-1 and the kernel errno).

#include <cstring>

#include <gtest/gtest.h>

#include "common/singleton.h"
#include "core/emulator_settings.h"
#include "core/file_sys/fs.h"
#include "core/libraries/kernel/file_system.h"
#include "core/libraries/kernel/kernel.h"
#include "core/libraries/net/net.h"
#include "core/libraries/net/net_error.h"
#include "core/libraries/net/net_kernel.h"
#include "core/libraries/net/net_p2p.h"
#include "core/net/guest_net.h"

using namespace Libraries::Net;
using FDTable = Common::Singleton<Core::FileSys::HandleTable>;

namespace {

class NetKernel : public ::testing::Test {
protected:
    static void SetUpTestSuite() {
        FDTable::Instance()->CreateStdHandles();
        InstallKernelIntegration();
        ASSERT_EQ(sceNetInit(), 0);
    }

    static int Errno() {
        return *Libraries::Kernel::__Error();
    }

    static OrbisNetSockaddrIn Loopback() {
        OrbisNetSockaddrIn in{};
        in.sin_len = sizeof(in);
        in.sin_family = ORBIS_NET_AF_INET;
        in.sin_addr = sceNetHtonl(0x7F000001);
        return in;
    }
};

TEST_F(NetKernel, SocketsAreFileDescriptors) {
    const int fd = sys_socket(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_DGRAM, 0);
    ASSERT_GE(fd, 3); // after stdin, stdout, stderr
    auto file = FDTable::Instance()->GetFile(fd);
    ASSERT_NE(file, nullptr);
    EXPECT_EQ(file->type, Core::FileSys::FileType::Socket);
    EXPECT_TRUE(file->is_opened);
    // select() on it: a fresh UDP socket is writable, not readable.
    Core::Net::SelectEntry sel[] = {{fd, Core::Net::Host::EvIn | Core::Net::Host::EvOut, 0}};
    EXPECT_EQ(KernelSelect(sel, 0), 1);
    EXPECT_EQ(sel[0].ready, Core::Net::Host::EvOut);

    // write() and read() on the descriptor are send and recv.
    auto addr = Loopback();
    ASSERT_EQ(sys_bind(fd, reinterpret_cast<OrbisNetSockaddr*>(&addr), sizeof(addr)), 0);
    u32 len = sizeof(addr);
    ASSERT_EQ(sys_getsockname(fd, reinterpret_cast<OrbisNetSockaddr*>(&addr), &len), 0);
    ASSERT_EQ(sys_connect(fd, reinterpret_cast<OrbisNetSockaddr*>(&addr), sizeof(addr)), 0);
    EXPECT_EQ(KernelWrite(fd, "data", 4), 4);
    char buf[8]{};
    EXPECT_EQ(KernelRead(fd, buf, sizeof(buf)), 4);
    EXPECT_STREQ(buf, "data");

    Libraries::Kernel::OrbisKernelStat st{};
    ASSERT_EQ(KernelFstat(fd, &st), 0);
    EXPECT_EQ(st.st_mode & 0170000, 0140000); // S_IFSOCK

    ASSERT_EQ(KernelClose(fd), 0);
    EXPECT_EQ(FDTable::Instance()->GetFile(fd), nullptr);
    EXPECT_FALSE(Core::Net::GetObjectKind(fd));
    EXPECT_EQ(KernelClose(fd), -1);
    EXPECT_EQ(Errno(), ORBIS_NET_EBADF);
    EXPECT_EQ(sceNetSocketClose(fd), ORBIS_NET_ERROR_EBADF);
}

TEST_F(NetKernel, DescriptorsAreReused) {
    const int a = sys_socket(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    ASSERT_EQ(sys_socketclose(a), 0); // the sceNet way frees the descriptor as well
    EXPECT_EQ(FDTable::Instance()->GetFile(a), nullptr);
    const int b = sys_socket(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0);
    EXPECT_EQ(b, a); // lowest free descriptor, as the kernel hands them out
    KernelClose(b);
}

TEST_F(NetKernel, PosixErrorsUseTheKernelErrno) {
    *sceNetErrnoLoc() = 1234;
    *Libraries::Kernel::__Error() = 0;
    EXPECT_EQ(sys_listen(1234, 1), -1);
    EXPECT_EQ(Errno(), ORBIS_NET_EBADF);
    EXPECT_EQ(sys_socket(99, ORBIS_NET_SOCK_STREAM, 0), -1);
    EXPECT_EQ(Errno(), ORBIS_NET_EPROTONOSUPPORT); // as sceNetSocket reports it
    EXPECT_EQ(*sceNetErrnoLoc(), 1234);
    char buf[4];
    EXPECT_EQ(KernelRead(1234, buf, sizeof(buf)), -1);
    EXPECT_EQ(*sceNetErrnoLoc(), 1234);
    // The sceNet* call itself does set it.
    EXPECT_EQ(sceNetListen(1234, 1), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(*sceNetErrnoLoc(), ORBIS_NET_EBADF);
    EXPECT_EQ(sys_htons(0x1234), 0x3412);
}

TEST_F(NetKernel, SocketPair) {
    int sv[2]{-1, -1};
    EXPECT_EQ(sys_socketpair(ORBIS_NET_AF_INET, ORBIS_NET_SOCK_STREAM, 0, sv), -1);
    EXPECT_EQ(Errno(), ORBIS_NET_EAFNOSUPPORT); // AF_UNIX only, as on FreeBSD
    ASSERT_EQ(sys_socketpair(ORBIS_NET_AF_UNIX, ORBIS_NET_SOCK_STREAM, 0, sv), 0);
    EXPECT_NE(FDTable::Instance()->GetFile(sv[0]), nullptr);
    EXPECT_NE(FDTable::Instance()->GetFile(sv[1]), nullptr);
    EXPECT_EQ(KernelWrite(sv[0], "abc", 3), 3);
    char buf[4]{};
    EXPECT_EQ(sys_recv(sv[1], buf, sizeof(buf), 0), 3);
    EXPECT_STREQ(buf, "abc");
    KernelClose(sv[0]);
    KernelClose(sv[1]);
}

TEST_F(NetKernel, EpollsAndResolversCloseToo) {
    const int ep = sceNetEpollCreate("ep", 0);
    const s32 pool = sceNetPoolCreate("pool", 16 * 1024, 0);
    const int rid = sceNetResolverCreate("rid", pool, 0);
    ASSERT_GE(ep, 3);
    ASSERT_GE(rid, 3);
    EXPECT_NE(FDTable::Instance()->GetFile(ep), nullptr);
    EXPECT_NE(FDTable::Instance()->GetFile(rid), nullptr);
    // Descriptors, but nothing to select on: never ready, and not an error.
    Core::Net::SelectEntry sel[] = {{ep, Core::Net::Host::EvIn, 0},
                                    {rid, Core::Net::Host::EvIn, 0}};
    EXPECT_EQ(KernelSelect(sel, 0), 0);
    EXPECT_EQ(sel[0].ready, 0u);
    char buf[4];
    EXPECT_EQ(KernelRead(ep, buf, sizeof(buf)), -1); // not readable
    EXPECT_EQ(KernelClose(ep), 0);
    EXPECT_EQ(KernelClose(rid), 0);
    EXPECT_EQ(FDTable::Instance()->GetFile(ep), nullptr);
    EXPECT_EQ(FDTable::Instance()->GetFile(rid), nullptr);
    EXPECT_EQ(sceNetResolverDestroy(rid), ORBIS_NET_ERROR_EBADF);
    EXPECT_EQ(sceNetPoolDestroy(pool), ORBIS_OK); // close() gave the pool back
}

TEST_F(NetKernel, P2PPortComesFromTheSettings) {
    EmulatorSettings.SetP2PPort(0); // any free port
    EXPECT_EQ(GetP2PConfiguredPort(), 0);
    ASSERT_TRUE(EnsureP2PTransport());
    EXPECT_NE(GetP2PBoundPort(), 0);
    EXPECT_EQ(GetP2PAdvertisedPort(), GetP2PBoundPort());
    StopP2P(); // offline by default: no UPnP involved either way
    EXPECT_FALSE(P2PTransportIsReady());

    EmulatorSettings.SetP2PPort(70000); // not a port: the console's
    EXPECT_EQ(GetP2PConfiguredPort(), 3658);
    EmulatorSettings.SetP2PPort(3658);
}

} // namespace
