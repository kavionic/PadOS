// This file is part of PadOS.
//
// Copyright (c) 2026 Kurt Skauen
//
// SPDX-License-Identifier: Apache-2.0
///////////////////////////////////////////////////////////////////////////////

#pragma once

#include <cstdint>
#include <limits>
#include <utility>
#include <RPC/RPCDefinition.h>
#include <RPC/RPCInvoker.h>

class PDisplayDriver;
enum class PDisplayCommandID : uint32_t;

class PDisplayCommandAllocator
{
public:
    PDisplayCommandAllocator(PDisplayDriver& driver, PDisplayCommandID commandID) : m_Driver(driver), m_CommandID(commandID) {}

    void* operator()(size_t size) const;

private:
    PDisplayDriver& m_Driver;
    PDisplayCommandID m_CommandID;
};

template<PDisplayCommandID TCommandID, typename TSignature>
class PDisplayDriverInvoker
    : public PRPCInvoker<
        PRPCDefinition<std::to_underlying(TCommandID), false, TSignature>::IsConst,
        typename PRPCDefinition<std::to_underlying(TCommandID), false, TSignature>::Signature,
        PDisplayCommandAllocator,
        PRPCNoTransmission>
{
public:
    using Definition = PRPCDefinition<std::to_underlying(TCommandID), false, TSignature>;
    using Base = PRPCInvoker<Definition::IsConst, typename Definition::Signature, PDisplayCommandAllocator, PRPCNoTransmission>;

    explicit PDisplayDriverInvoker(PDisplayDriver& driver)
        : Base(PRPCNoTransmission(), PDisplayCommandAllocator(driver, TCommandID))
    {
    }

    // The returned argument area is followed by payloadSize bytes owned by the caller until submission.
    void* Alloc(size_t payloadSize = 0) const
    {
        constexpr size_t ARGUMENT_SIZE = Base::GetFixedSize();
        if (payloadSize > std::numeric_limits<size_t>::max() - ARGUMENT_SIZE) {
            PERROR_THROW_CODE(PErrorCode::OVERFLOW);
        }
        return this->AllocBuffer(ARGUMENT_SIZE + payloadSize);
    }
};
