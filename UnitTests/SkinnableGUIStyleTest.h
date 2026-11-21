// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <array>
#include <cerrno>
#include <functional>
#include <sys/stat.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include <pugixml.hpp>
#include <Storage/File.h>
#include <Storage/Path.h>
#include <Storage/StandardPaths.h>
#include <System/ExceptionHandling.h>

// Validation tests edit private XML copies while sharing the installed assets.
class PSkinnableGUIStyleTestDirectory
{
public:
    explicit PSkinnableGUIStyleTestDirectory(const char* testName)
        : m_Path(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/{}", testName)))
    {
        for (size_t index = 0; index < FILE_NAMES.size(); ++index)
        {
            m_FilePaths[index] = m_Path;
            m_FilePaths[index].Append(FILE_NAMES[index]);
        }
        if (mkdir(m_Path.c_str(), S_IRWXU) != 0) {
            PERROR_THROW_CODE(PErrorCode(errno));
        }
        m_OwnsDirectory = true;
        PScopeFail cleanup([this]() { Cleanup(); });
        for (const char* fileName : FILE_NAMES) {
            WriteFile(fileName);
        }
    }

    ~PSkinnableGUIStyleTestDirectory() { Cleanup(); }

    PSkinnableGUIStyleTestDirectory(const PSkinnableGUIStyleTestDirectory&) = delete;
    PSkinnableGUIStyleTestDirectory& operator=(const PSkinnableGUIStyleTestDirectory&) = delete;

    const PPath& GetPath() const { return m_Path; }

    // Each edit starts from the original XML so invalid values do not accumulate.
    void WriteFile(const char* fileName, const std::function<void(pugi::xml_node)>& edit = {})
    {
        const PPath sourcePath(PStandardPaths::GetPath(PStandardPath::GUI, PString::format_string("Styles/Skinned/Normal/{}", fileName)));
        PFile source(sourcePath.GetPath());
        PString xml;
        if (!source.IsValid() || !source.Read(xml)) {
            PERROR_THROW_CODE_WHAT(PErrorCode::IO, "Unable to read test skin XML");
        }
        pugi::xml_document document;
        if (!document.load_buffer(xml.data(), xml.size())) {
            PERROR_THROW_CODE_WHAT(PErrorCode::INVAL, "Unable to parse test skin XML");
        }
        RewriteAssetPaths(document.document_element());
        if (edit) {
            edit(document.document_element());
        }
        PString output;
        StringWriter writer(output);
        document.save(writer);
        PPath targetPath(m_Path);
        targetPath.Append(fileName);
        PFile target(targetPath.GetPath(), O_WRONLY | O_CREAT | O_TRUNC);
        if (!target.IsValid() || !target.Write(output)) {
            PERROR_THROW_CODE_WHAT(PErrorCode::IO, "Unable to write test skin XML");
        }
    }

private:
    class StringWriter : public pugi::xml_writer
    {
    public:
        explicit StringWriter(PString& content) : m_Content(content) {}
        virtual void write(const void* data, size_t size) override { m_Content.append(static_cast<const char*>(data), size); }

    private:
        PString& m_Content;
    };

    static void RewriteAssetPaths(pugi::xml_node node)
    {
        for (const char* name : {"image", "background_texture"})
        {
            pugi::xml_attribute attribute = node.attribute(name);
            if (!attribute.empty() && attribute.value()[0] != '\0')
            {
                const PString relativePath = PString("../Normal/") + attribute.value();
                if (!attribute.set_value(relativePath.c_str())) {
                    PERROR_THROW_CODE(PErrorCode::NOMEM);
                }
            }
        }
        for (pugi::xml_node child : node.children()) {
            RewriteAssetPaths(child);
        }
    }

    void Cleanup() noexcept
    {
        if (m_OwnsDirectory)
        {
            for (const PPath& path : m_FilePaths)
            {
                if (unlink(path.c_str()) != 0) {
                    EXPECT_EQ(errno, ENOENT);
                }
            }
            EXPECT_EQ(rmdir(m_Path.c_str()), 0);
            m_OwnsDirectory = false;
        }
    }

    static constexpr std::array<const char*, 10> FILE_NAMES = {
        "Style.xml", "Button.xml", "CheckBox.xml", "RadioButton.xml", "Slider.xml",
        "ScrollBar.xml", "TabView.xml", "DropdownMenu.xml", "GroupView.xml", "ProgressBar.xml"
    };

    PPath m_Path;
    std::array<PPath, FILE_NAMES.size()> m_FilePaths;
    bool m_OwnsDirectory = false;
};
