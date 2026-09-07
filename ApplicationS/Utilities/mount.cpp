// This file is part of PadOS.
//
// Copyright (C) 2026 Kurt Skauen <http://kavionic.com/>
//
// PadOS is free software : you can redistribute it and / or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// PadOS is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with PadOS. If not, see <http://www.gnu.org/licenses/>.
///////////////////////////////////////////////////////////////////////////////

#include <string>
#include <vector>

#include <unistd.h>

#include <argparse/argparse.hpp>

#include <PadOS/Filesystem.h>
#include <System/AppDefinition.h>
#include <Utils/String.h>

#include "FileUtilityHelpers.h"


namespace shutil_mount
{

int mount_main(int argc, char* argv[])
{
    argparse::ArgumentParser program(
        argv[0],
        "1.0",
        argparse::default_arguments::none);

    program.add_description("Mount a filesystem.");
    program.add_argument("--help")
        .help("Print argument help.")
        .flag();
    program.add_argument("-t", "--type")
        .help("Filesystem type.")
        .metavar("TYPE");
    program.add_argument("-o", "--options")
        .help("Filesystem-specific mount options.")
        .metavar("OPTIONS");
    program.add_argument("operands")
        .help("Device and mount-point paths.")
        .metavar("DEVICE DIRECTORY")
        .nargs(argparse::nargs_pattern::any);

    try
    {
        program.parse_args(argc, argv);
    }
    catch (const std::exception& exception)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string("{}\n", exception.what()));
        shutil::WriteAll(STDERR_FILENO, program.help().str());
        return 1;
    }

    if (program.get<bool>("--help"))
    {
        shutil::WriteAll(STDOUT_FILENO, program.help().str());
        return 0;
    }

    const std::vector<std::string> operands =
        program.is_used("operands")
            ? program.get<std::vector<std::string>>("operands")
            : std::vector<std::string>();

    if (!program.is_used("--type") || operands.size() != 2)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: expected -t TYPE DEVICE DIRECTORY\n",
                argv[0]));
        return 1;
    }

    const std::string& filesystemType = program.get("--type");
    const std::string mountOptions =
        program.is_used("--options")
            ? program.get("--options")
            : std::string();

    const PErrorCode result = mount(
        operands[0].c_str(),
        operands[1].c_str(),
        filesystemType.c_str(),
        0,
        mountOptions.empty() ? nullptr : mountOptions.data(),
        mountOptions.size());

    if (result != PErrorCode::Success)
    {
        shutil::WriteAll(
            STDERR_FILENO,
            PString::format_string(
                "{}: failed to mount '{}' on '{}': {}\n",
                argv[0],
                operands[0],
                operands[1],
                p_strerror(result)));
        return 1;
    }
    return 0;
}

static PAppDefinition g_MountAppDef(
    "mount",
    "Mount a filesystem.",
    mount_main);

} // namespace shutil_mount
