// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#include <cerrno>
#include <cstdlib>

#include <gtest/gtest.h>
#include <unistd.h>

#include <Storage/Directory.h>
#include <Storage/File.h>
#include <Storage/FileReference.h>
#include <Storage/TempFile.h>

namespace file_creation_tests
{

template<typename NodeType>
class FileCreationTest : public testing::Test
{
protected:
    virtual void SetUp() override
    {
        char directoryPath[] = "/tmp/pfile_creation_XXXXXX";
        ASSERT_NE(mkdtemp(directoryPath), nullptr);
        m_DirectoryPath = directoryPath;
        m_Path = m_DirectoryPath;
        m_Path.Append("file");
        ASSERT_TRUE(m_Directory.Open(m_DirectoryPath));
        ASSERT_TRUE(m_Reference.SetTo(m_Directory, "file"));
    }

    virtual void TearDown() override
    {
        m_Reference.Unset();
        m_Directory.Close();
        if (!m_DirectoryPath.empty())
        {
            if (unlink(m_Path.c_str()) != 0) {
                EXPECT_EQ(errno, ENOENT);
            }
            EXPECT_EQ(rmdir(m_DirectoryPath.c_str()), 0);
        }
    }

    void VerifyCreatedFile(PFSNode& node, bool writable)
    {
        ASSERT_TRUE(node.IsValid());
        EXPECT_EQ((node.GetMode() & S_IWUSR) != 0, writable);
        EXPECT_EQ(write(node.GetFileDescriptor(), "original", 8), 8);
        node.Close();
        EXPECT_EQ(node.Open(m_Path, O_WRONLY | O_TRUNC), writable);
        if (node.IsValid()) {
            EXPECT_EQ(write(node.GetFileDescriptor(), "replacement", 11), 11);
        }
        node.Close();
        ASSERT_EQ(unlink(m_Path.c_str()), 0);
    }

    PString m_DirectoryPath;
    PPath m_Path;
    PDirectory m_Directory;
    PFileReference m_Reference;
};

using FileCreationTypes = testing::Types<PFSNode, PFile>;
TYPED_TEST_SUITE(FileCreationTest, FileCreationTypes);

TYPED_TEST(FileCreationTest, ConstructorDefaultsAllowReopeningForWrite)
{
    TypeParam pathNode(this->m_Path.GetPath(), O_WRONLY | O_CREAT | O_EXCL);
    ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(pathNode, true));

    TypeParam directoryNode(this->m_Directory, "file", O_WRONLY | O_CREAT | O_EXCL);
    ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(directoryNode, true));

    TypeParam referenceNode(this->m_Reference, O_WRONLY | O_CREAT | O_EXCL);
    ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(referenceNode, true));
}

TYPED_TEST(FileCreationTest, ConstructorsHonorExplicitReadOnlyMode)
{
    TypeParam pathNode(this->m_Path.GetPath(), O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
    ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(pathNode, false));

    TypeParam directoryNode(this->m_Directory, "file", O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
    ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(directoryNode, false));

    TypeParam referenceNode(this->m_Reference, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR);
    ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(referenceNode, false));
}

TYPED_TEST(FileCreationTest, OpenOverloadsHonorReadOnlyAndWritableModes)
{
    for (mode_t accessMode : {S_IRUSR, S_IRUSR | S_IWUSR})
    {
        const bool writable = (accessMode & S_IWUSR) != 0;
        TypeParam node;
        ASSERT_TRUE(node.Open(this->m_Path.GetPath(), O_WRONLY | O_CREAT | O_EXCL, accessMode));
        ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(node, writable));

        ASSERT_TRUE(node.Open(this->m_Path, O_WRONLY | O_CREAT | O_EXCL, accessMode));
        ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(node, writable));

        ASSERT_TRUE(node.Open(this->m_Directory, "file", O_WRONLY | O_CREAT | O_EXCL, accessMode));
        ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(node, writable));

        ASSERT_TRUE(node.Open(this->m_Reference, O_WRONLY | O_CREAT | O_EXCL, accessMode));
        ASSERT_NO_FATAL_FAILURE(this->VerifyCreatedFile(node, writable));
    }
}

TEST(FileCreation, TemporaryFilesHonorExplicitReadOnlyMode)
{
    PTempFile file("creation_mode_", "/tmp", S_IRUSR);
    ASSERT_TRUE(file.IsValid());
    EXPECT_EQ(file.GetMode() & S_IWUSR, 0);
    const PString path = file.GetPath();
    file.Close();
    PFile reopened(path, O_WRONLY);
    EXPECT_FALSE(reopened.IsValid());
}

}
