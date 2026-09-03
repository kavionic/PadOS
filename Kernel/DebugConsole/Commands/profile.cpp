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
// Created: 03.09.2026 00:00

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <Kernel/DebugConsole/KConsoleCommand.h>
#include <Kernel/KGProfSampler.h>
#include <Kernel/KMutex.h>
#include <System/ErrorCodes.h>

namespace kernel
{

struct ProfileCommandOutputContext
{
    PString KernelPath;
    PString ApplicationPath;
    PString KernelTemporaryPath;
    PString ApplicationTemporaryPath;
    int KernelFile = -1;
    int ApplicationFile = -1;
    bool KernelPublished = false;
    bool ApplicationPublished = false;
};

static KMutex gk_ProfileCommandMutex("gprof_output", PEMutexRecursionMode_RaiseError);

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode ProfileCommandWrite(void* context, KGProfImage image, const void* data, size_t length) noexcept
{
    ProfileCommandOutputContext* outputContext = static_cast<ProfileCommandOutputContext*>(context);
    if (outputContext == nullptr) {
        return PErrorCode::INVAL;
    }

    int* file = (image == KGProfImage::Kernel) ? &outputContext->KernelFile : &outputContext->ApplicationFile;
    const PString& path = (image == KGProfImage::Kernel) ? outputContext->KernelTemporaryPath : outputContext->ApplicationTemporaryPath;
    if (*file < 0)
    {
        *file = open(path.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (*file < 0) {
            return PErrorCode(errno);
        }
    }

    const uint8_t* source = static_cast<const uint8_t*>(data);
    size_t bytesWritten = 0;

    while (bytesWritten < length)
    {
        const ssize_t result = write(*file, source + bytesWritten, length - bytesWritten);
        if (result < 0)
        {
            if (errno == EINTR) {
                continue;
            }
            return PErrorCode(errno);
        }
        if (result == 0) {
            return PErrorCode::IO;
        }
        bytesWritten += size_t(result);
    }
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode ProfileCommandSync(int file)
{
    for (;;)
    {
        if (fsync(file) == 0) {
            return PErrorCode::Success;
        }
        if (errno != EINTR) {
            return PErrorCode(errno);
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode ProfileCommandPublish(
    const PString& temporaryPath,
    const PString& finalPath,
    bool& published)
{
    if (rename(temporaryPath.c_str(), finalPath.c_str()) < 0) {
        return PErrorCode(errno);
    }
    published = true;
    return PErrorCode::Success;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static void ProfileCommandClose(int& file, PErrorCode& result)
{
    if (file < 0) {
        return;
    }

    const int fileToClose = file;
    file = -1;
    if (close(fileToClose) < 0 && result == PErrorCode::Success) {
        result = PErrorCode(errno);
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

static PErrorCode ProfileCommandDump(const PString& kernelPath, const PString& applicationPath)
{
    KMutexGuardRaw outputLock(gk_ProfileCommandMutex, true);
    ProfileCommandOutputContext context =
    {
        .KernelPath = kernelPath,
        .ApplicationPath = applicationPath,
        .KernelTemporaryPath = PString::format_string("{}.tmp", kernelPath),
        .ApplicationTemporaryPath = PString::format_string("{}.tmp", applicationPath)
    };
    PErrorCode result = kgprof_write_gmon(ProfileCommandWrite, &context);
    if (result == PErrorCode::Success && (context.KernelFile < 0 || context.ApplicationFile < 0)) {
        result = PErrorCode::IO;
    }
    if (result == PErrorCode::Success && context.KernelFile >= 0) {
        result = ProfileCommandSync(context.KernelFile);
    }
    if (result == PErrorCode::Success && context.ApplicationFile >= 0) {
        result = ProfileCommandSync(context.ApplicationFile);
    }
    if (result == PErrorCode::Success) {
        result = ProfileCommandPublish(
            context.KernelTemporaryPath,
            context.KernelPath,
            context.KernelPublished);
    }
    if (result == PErrorCode::Success) {
        result = ProfileCommandPublish(
            context.ApplicationTemporaryPath,
            context.ApplicationPath,
            context.ApplicationPublished);
    }
    if (result == PErrorCode::Success) {
        result = ProfileCommandSync(context.KernelFile);
    }
    if (result == PErrorCode::Success) {
        result = ProfileCommandSync(context.ApplicationFile);
    }

    ProfileCommandClose(context.KernelFile, result);
    ProfileCommandClose(context.ApplicationFile, result);

    if (!context.KernelPublished) {
        unlink(context.KernelTemporaryPath.c_str());
    }
    if (!context.ApplicationPublished) {
        unlink(context.ApplicationTemporaryPath.c_str());
    }
    return result;
}

class CCmdProfile : public KConsoleCommand
{
public:
    virtual int Invoke(std::vector<std::string>&& args) override
    {
        if (args.size() == 2 && args[1] == "start")
        {
            const PErrorCode result = kgprof_start();
            if (result != PErrorCode::Success)
            {
                Print("Failed to start profiler: {}\n", p_strerror(result));
                return 1;
            }
            const KGProfStatus status = kgprof_get_status();
            Print("Sampling thread-mode execution at {} Hz into {}-byte bins ({} bytes of counters).\n",
                status.SampleRateHz, status.BinSizeBytes, status.CounterBytes);
            return 0;
        }

        if (args.size() == 2 && args[1] == "stop")
        {
            const PErrorCode result = kgprof_stop();
            if (result != PErrorCode::Success)
            {
                Print("Failed to stop profiler: {}\n", p_strerror(result));
                return 1;
            }
            Print("Profiler stopped.\n");
            return 0;
        }

        if (args.size() == 2 && args[1] == "status")
        {
            const KGProfStatus status = kgprof_get_status();
            const char* state = "stopped";
            if (status.Running) {
                state = "running";
            } else if (status.Busy) {
                state = "busy";
            }
            Print("State: {}\n", state);
            Print("Capture: {}\n", status.HasCapture ? "available" : "none");
            Print("Samples: {} total, {} kernel, {} application, {} unmapped, {} saturated\n",
                status.TotalSamples,
                status.KernelSamples,
                status.ApplicationSamples,
                status.UnmappedSamples,
                status.SaturatedSamples);
            Print("Resolution: {} Hz, {} bytes/bin, {} bytes of counters\n",
                status.SampleRateHz, status.BinSizeBytes, status.CounterBytes);
            return 0;
        }

        if (args.size() == 3 && args[1] == "dump")
        {
            const PString kernelPath = PString::format_string("{}-kernel.gmon", args[2]);
            const PString applicationPath = PString::format_string("{}-application.gmon", args[2]);
            const PErrorCode result = ProfileCommandDump(kernelPath, applicationPath);
            if (result != PErrorCode::Success)
            {
                Print("Failed to write profile: {}\n", p_strerror(result));
                return 1;
            }
            const KGProfStatus status = kgprof_get_status();
            Print("Wrote '{}' and '{}'.\n", kernelPath, applicationPath);
            Print("Samples: {} total, {} kernel, {} application, {} unmapped, {} saturated\n",
                status.TotalSamples,
                status.KernelSamples,
                status.ApplicationSamples,
                status.UnmappedSamples,
                status.SaturatedSamples);
            return 0;
        }

        Print("Usage: profile start|stop|status|dump <path-prefix>\n");
        return 1;
    }

    static PString GetDescription() { return "Control GNU gprof thread-mode flat-profile sampling."; }
};

static KConsoleCommandRegistrator<CCmdProfile> g_RegisterCCmdProfile("profile");

} // namespace kernel
