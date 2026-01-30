// This file is part of PadOS.
//
// Copyright (C) 2001-2025 Kurt Skauen <http://kavionic.com/>
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


#include <vector>
#include <string.h>

#include <DataTranslation/DataTranslator.h>


static PTranslatorFactory g_DataTranslatorFactory;

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDataTranslator::PDataTranslator()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PDataTranslator::~PDataTranslator()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t PDataTranslator::AvailableDataSize()
{
    return m_OutBuffer.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

ssize_t PDataTranslator::Read(void* data, size_t length)
{
    const ssize_t curLength = std::min(m_OutBuffer.size(), length);

    if (curLength == 0) {
        return 0;
    }

    uint8_t* dst = static_cast<uint8_t*>(data);
    for (size_t i = 0; i < curLength; ++i) {
        *dst++ = m_OutBuffer[i];
    }
    m_OutBuffer.erase(m_OutBuffer.begin(), m_OutBuffer.begin() + curLength);

    return curLength;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

void PDataTranslator::AddProcessedData(void* data, size_t length, bool isFinal)
{
    if (VFDataReady.Empty())
    {
        m_OutBuffer.insert(m_OutBuffer.end(), static_cast<uint8_t*>(data), static_cast<uint8_t*>(data) + length);
    }
    else
    {
        if (!VFDataReady(data, length, isFinal)) {
            Abort();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTranslatorNode::PTranslatorNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTranslatorNode::~PTranslatorNode()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTranslatorFactory::PTranslatorFactory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTranslatorFactory::~PTranslatorFactory()
{
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTranslatorFactory& PTranslatorFactory::Get()
{
    return g_DataTranslatorFactory;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PDataTranslator> PTranslatorFactory::FindTranslator(const PString& srcType, PDataTranslatorType dstType, const void* data, size_t length, PDataTranslatorStatus& outStatus) const
{
    float bestQuality = -1000.0f;
    Ptr<PTranslatorNode> bestTranslator;
    PDataTranslatorStatus status = PDataTranslatorStatus::UnknownFormat;
    
    for (size_t i = 0 ;i < m_Nodes.size(); ++i)
    {
	    Ptr<PTranslatorNode> node = m_Nodes[i];
	    PTranslatorInfo info   = node->GetTranslatorInfo();

	    if (dstType != PDataTranslatorType::Unknown && info.DestType != dstType) {
	        continue;
	    }

	    const PDataTranslatorStatus result = node->Identify(srcType, dstType, data, length);

	    if (result == PDataTranslatorStatus::Success)
        {
	        if (info.Quality > bestQuality)
            {
		        bestQuality = info.Quality;
		        bestTranslator = node;
	        }
	    }
        else if (result == PDataTranslatorStatus::NotEnoughData)
        {
	        status = PDataTranslatorStatus::NotEnoughData;
	    }
    }

    Ptr<PDataTranslator> translator;
    if (bestTranslator != nullptr)
    {
        translator = bestTranslator->CreateTranslator();
	    if (translator != nullptr) {
	        status = PDataTranslatorStatus::Success;
	    } else {
	        status = PDataTranslatorStatus::NoMemory;
	    }
    }
    outStatus = status;
    return translator;
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

size_t PTranslatorFactory::GetTranslatorCount() const
{
    return m_Nodes.size();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PTranslatorNode> PTranslatorFactory::GetTranslatorNode(size_t index) const
{
    return m_Nodes[index];
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

PTranslatorInfo PTranslatorFactory::GetTranslatorInfo(size_t index) const
{
    return m_Nodes[index]->GetTranslatorInfo();
}

///////////////////////////////////////////////////////////////////////////////
/// \author Kurt Skauen
///////////////////////////////////////////////////////////////////////////////

Ptr<PDataTranslator> PTranslatorFactory::CreateTranslator(size_t index) const
{
    return m_Nodes[index]->CreateTranslator();
}
