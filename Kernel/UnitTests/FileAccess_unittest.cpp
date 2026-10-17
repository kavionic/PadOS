// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <gtest/gtest.h>
#include <unistd.h>
#include <string.h>
#include <utility>
#include <Kernel/KProcess.h>
#include <Kernel/FSDrivers/VirtualFSBase.h>
#include <Kernel/VFS/FileIO.h>

namespace kernel
{

TEST(FileAccessCredentials, UsesOwnerThenPrimaryOrSupplementaryGroupThenOtherPermissions)
{
    KFileAccessCredentials credentials;
    credentials.UserID = 1001;
    credentials.GroupID = 2001;
    credentials.GroupCount = 1;
    credentials.Groups[0] = 3001;

    struct stat fileStats = {};
    fileStats.st_uid = 1001;
    fileStats.st_gid = 3001;
    fileStats.st_mode = S_IFREG | S_IRGRP | S_IWOTH;
    EXPECT_FALSE(credentials.HasAccess(fileStats, R_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, W_OK));
    EXPECT_TRUE(credentials.HasAccess(fileStats, F_OK));

    fileStats.st_uid = 1002;
    EXPECT_TRUE(credentials.HasAccess(fileStats, R_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, W_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, R_OK | W_OK));

    fileStats.st_gid = 2001;
    EXPECT_TRUE(credentials.HasAccess(fileStats, R_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, W_OK));

    fileStats.st_gid = 4001;
    EXPECT_FALSE(credentials.HasAccess(fileStats, R_OK));
    EXPECT_TRUE(credentials.HasAccess(fileStats, W_OK));

    fileStats.st_uid = 1001;
    fileStats.st_mode |= S_IRUSR | S_IWUSR;
    EXPECT_TRUE(credentials.HasAccess(fileStats, R_OK | W_OK));
}

TEST(FileAccessCredentials, RootUserIDDoesNotImplicitlyOverridePermissions)
{
    const KFileAccessCredentials credentials;
    struct stat fileStats = {};
    fileStats.st_mode = S_IFREG;
    EXPECT_FALSE(credentials.HasAccess(fileStats, R_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, W_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, X_OK));
    EXPECT_TRUE(credentials.HasAccess(fileStats, F_OK));
}

TEST(FileAccessCredentials, PermissionOverrideStillNeedsAnExecuteBitForRegularFiles)
{
    KFileAccessCredentials credentials;
    credentials.UserID = 1001;
    credentials.CanOverridePermissions = true;
    struct stat fileStats = {};
    fileStats.st_mode = S_IFREG;
    EXPECT_TRUE(credentials.HasAccess(fileStats, R_OK | W_OK));
    EXPECT_FALSE(credentials.HasAccess(fileStats, X_OK));
    fileStats.st_mode |= S_IXOTH;
    EXPECT_TRUE(credentials.HasAccess(fileStats, R_OK | W_OK | X_OK));
    fileStats.st_mode = S_IFDIR;
    EXPECT_TRUE(credentials.HasAccess(fileStats, X_OK));
}

class FileAccessTest : public ::testing::Test
{
protected:
    void SetUp() override
    {
        m_Filesystem = ptr_new<KVirtualFilesystemBase>();
        m_Volume = m_Filesystem->Mount(-1, "", 0, nullptr, 0);
        m_Volume->SetFlags(0);
        m_Volume->m_Filesystem = m_Filesystem;
        m_Root = ptr_static_cast<KVirtualFSBaseInode>(m_Volume->m_RootNode);
        m_Nodes.push_back(m_Root);
        m_DirectoryFD = kopen_from_inode_trw(m_Root, O_PATH | O_DIRECTORY);
        m_Credentials.UserID = 1001;
        m_Credentials.GroupID = 2001;
    }

    void TearDown() override
    {
        if (m_DirectoryFD >= 0) {
            EXPECT_EQ(kclose(m_DirectoryFD), PErrorCode::Success);
        }
        for (const Ptr<KVirtualFSBaseInode>& inode : m_Nodes) {
            inode->Detach();
        }
        m_Volume->m_RootNode = nullptr;
        m_Root = nullptr;
        m_Nodes.clear();
        m_Volume = nullptr;
        m_Filesystem = nullptr;
    }

    Ptr<KVirtualFSBaseInode> AddNode(Ptr<KVirtualFSBaseInode> parent, const char* name, mode_t mode)
    {
        const Ptr<KVirtualFSBaseInode> inode = ptr_new<KVirtualFSBaseInode>(
            m_Filesystem,
            m_Volume,
            ptr_raw_pointer_cast(parent),
            ptr_raw_pointer_cast(m_Filesystem),
            mode);
        m_Nodes.push_back(inode);
        parent->m_Children[name] = inode;
        return inode;
    }

    PErrorCode ResolveError(const char* path, bool followSymlinks = true)
    {
        try
        {
            KLocateFlags flags = KLocateFlag::CrossMount;
            if (followSymlinks) {
                flags.SetFlag(KLocateFlag::FollowSymlinks);
            }
            klocate_inode_by_path_trw(flags, m_Root, path, strlen(path), &m_Credentials);
            return PErrorCode::Success;
        }
        PERROR_CATCH_RET_CODE;
    }

    PErrorCode AccessError(const char* path, int mode, int flags = 0)
    {
        try
        {
            kfaccessat_trw(m_DirectoryFD, path, mode, flags);
            return PErrorCode::Success;
        }
        PERROR_CATCH_RET_CODE;
    }

    Ptr<KVirtualFilesystemBase> m_Filesystem;
    Ptr<KFSVolume> m_Volume;
    Ptr<KVirtualFSBaseInode> m_Root;
    std::vector<Ptr<KVirtualFSBaseInode>> m_Nodes;
    KFileAccessCredentials m_Credentials;
    int m_DirectoryFD = -1;
};

TEST_F(FileAccessTest, ChecksEveryDirectoryIncludingDotDotAndTrailingSlashes)
{
    const Ptr<KVirtualFSBaseInode> blocked = AddNode(m_Root, "blocked", S_IFDIR | S_IRWXU);
    AddNode(blocked, "file", S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);

    EXPECT_EQ(ResolveError("blocked"), PErrorCode::Success);
    EXPECT_EQ(ResolveError("blocked/file"), PErrorCode::ACCES);
    EXPECT_EQ(ResolveError("blocked/missing"), PErrorCode::ACCES);
    EXPECT_EQ(ResolveError("blocked/.."), PErrorCode::ACCES);
    EXPECT_EQ(ResolveError("blocked/"), PErrorCode::ACCES);
    EXPECT_EQ(ResolveError("blocked//"), PErrorCode::ACCES);

    blocked->m_FileMode |= S_IXOTH;
    EXPECT_EQ(ResolveError("blocked/file"), PErrorCode::Success);
    EXPECT_EQ(ResolveError("blocked/.."), PErrorCode::Success);
    EXPECT_EQ(ResolveError("blocked/"), PErrorCode::Success);
}

TEST_F(FileAccessTest, ChecksSymlinkTargetsAndSupportsNoFollow)
{
    const Ptr<KVirtualFSBaseInode> blocked = AddNode(m_Root, "blocked", S_IFDIR | S_IRWXU);
    AddNode(blocked, "file", S_IFREG | S_IROTH);
    const Ptr<KVirtualFSBaseInode> link = AddNode(m_Root, "link", S_IFLNK | S_IRWXU | S_IRWXG | S_IRWXO);
    static constexpr char TARGET[] = "blocked/file";
    link->m_FileData.assign(TARGET, TARGET + sizeof(TARGET) - 1);

    EXPECT_EQ(ResolveError("link"), PErrorCode::ACCES);
    EXPECT_EQ(ResolveError("link", false), PErrorCode::Success);
    EXPECT_EQ(AccessError("link", F_OK, AT_SYMLINK_NOFOLLOW), PErrorCode::Success);

    link->m_FileData.clear();
    EXPECT_EQ(ResolveError("link"), PErrorCode::NOENT);
}

TEST_F(FileAccessTest, AcceptsPathDescriptorsAndRejectsInvalidRequestsAndReadOnlyWrites)
{
    AddNode(m_Root, "file", S_IFREG | S_IRWXU | S_IRWXG | S_IRWXO);
    EXPECT_EQ(AccessError("file", R_OK), PErrorCode::Success);
    EXPECT_EQ(AccessError("missing", F_OK), PErrorCode::NOENT);
    EXPECT_EQ(AccessError("", F_OK), PErrorCode::NOENT);
    EXPECT_EQ(AccessError("file/", F_OK), PErrorCode::NOTDIR);
    EXPECT_EQ(AccessError("file", -1), PErrorCode::INVAL);
    EXPECT_EQ(AccessError("file", F_OK, -1), PErrorCode::INVAL);
    EXPECT_NO_THROW(kfaccessat_trw(-1234, "/", F_OK, 0));

    m_Volume->SetFlags(std::to_underlying(FSVolumeFlags::FS_IS_READONLY));
    EXPECT_EQ(AccessError("file", W_OK), PErrorCode::ROFS);
    EXPECT_EQ(AccessError("file", F_OK), PErrorCode::Success);
    EXPECT_EQ(AccessError("file", R_OK), PErrorCode::Success);
}

TEST_F(FileAccessTest, AcceptsKernelDirectoryDescriptors)
{
    AddNode(m_Root, "file", S_IFREG | S_IRUSR | S_IRGRP | S_IROTH);
    const int kernelDirectoryFD = kopen_from_inode_trw(m_Root, O_KERNEL | O_PATH | O_DIRECTORY);
    PScopeExit closeDirectory([kernelDirectoryFD]() { EXPECT_EQ(kclose(kernelDirectoryFD), PErrorCode::Success); });
    EXPECT_NO_THROW(kfaccessat_trw(kernelDirectoryFD, "file", R_OK, 0));
}

TEST_F(FileAccessTest, DistinguishesRealAndEffectiveCredentials)
{
    if (kgetuid() != 0 || kgeteuid() != 0) {
        GTEST_SKIP() << "Requires root real and effective credentials.";
    }
    AddNode(m_Root, "private", S_IFREG);
    PScopeExit restoreCredentials([]() { EXPECT_EQ(kseteuid(0), PErrorCode::Success); });
    ASSERT_EQ(kseteuid(1001), PErrorCode::Success);
    EXPECT_EQ(AccessError("private", R_OK), PErrorCode::Success);
    EXPECT_EQ(AccessError("private", R_OK, AT_EACCESS), PErrorCode::ACCES);
}

} // namespace kernel
