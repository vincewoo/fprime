/*
 * CircularBuffer.cpp:
 *
 * Buffer used to efficiently store data in ring data structure. Uses an externally supplied
 * data store as the backing for this buffer. Thus it is dependent on receiving sole ownership
 * of the supplied buffer.
 *
 * This implementation file contains the function definitions.
 *
 *  Created on: Apr 4, 2019
 *      Author: lestarch
 *  Revised March 2022
 *      Author: bocchino
 */
#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Utils/Types/CircularBuffer.hpp>
#include <cstring>

namespace Types {

// Helper function for serializing multi-byte values with endianness support
template <typename T>
inline Fw::SerializeStatus serializeMultibyteValue(CircularBuffer* buffer, T value, Fw::Endianness mode) {
    U8 bytes[sizeof(T)];
    if (mode == Fw::Endianness::BIG) {
        // Big-endian: MSB first
        for (FwSizeType i = 0; i < sizeof(T); i++) {
            bytes[i] = static_cast<U8>((value >> ((sizeof(T) - 1 - i) * 8)) & 0xFF);
        }
    } else {
        // Little-endian: LSB first
        T temp = value;
        for (FwSizeType i = 0; i < sizeof(T); i++) {
            bytes[i] = static_cast<U8>(temp & 0xFF);
            temp >>= 8;
        }
    }
    return buffer->serializeRaw(bytes, sizeof(T));
}

CircularBuffer::CircularBuffer()
    : m_store(nullptr),
      m_store_size(0),
      m_head_idx(0),
      m_allocated_size(0),
      m_high_water_mark(0),
      m_deser_idx(0),
      m_ser_idx(0) {}

CircularBuffer::CircularBuffer(U8* const buffer, const FwSizeType size)
    : m_store(nullptr),
      m_store_size(0),
      m_head_idx(0),
      m_allocated_size(0),
      m_high_water_mark(0),
      m_deser_idx(0),
      m_ser_idx(0) {
    setup(buffer, size);
}

void CircularBuffer::setup(U8* const buffer, const FwSizeType size) {
    FW_ASSERT(size > 0);
    FW_ASSERT(buffer != nullptr);
    FW_ASSERT(m_store == nullptr && m_store_size == 0);  // Not already setup

    // Initialize buffer data
    m_store = buffer;
    m_store_size = size;
    m_head_idx = 0;
    m_allocated_size = 0;
    m_high_water_mark = 0;
    m_deser_idx = 0;
    m_ser_idx = 0;
}

inline FwSizeType CircularBuffer::advance_idx(FwSizeType idx, FwSizeType amount) const {
    FW_ASSERT(idx < m_store_size, static_cast<FwAssertArgType>(idx));
    if (amount >= m_store_size) {
        amount %= m_store_size;
    }
    FwSizeType new_idx = idx + amount;
    if (new_idx >= m_store_size) {
        new_idx -= m_store_size;
    }
    return new_idx;
}

inline Fw::SerializeStatus CircularBuffer::checkSerializeSpace(const FwSizeType size) const {
    // Check if the serialization would exceed the buffer capacity
    FwSizeType end_offset = m_ser_idx + size;
    if (end_offset > m_store_size) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::serializeRaw(const U8* const buffer, const FwSizeType size) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buffer != nullptr);
    Fw::SerializeStatus status = this->checkSerializeSpace(size);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    // Copy in all the supplied data (no endianness conversion)
    FwSizeType idx = advance_idx(m_head_idx, m_ser_idx);
    FwSizeType bytes_to_end = m_store_size - idx;

    if (size <= bytes_to_end) {
        // Data fits without wrapping - single memcpy
        FW_ASSERT(idx + size <= m_store_size, static_cast<FwAssertArgType>(idx + size));
        (void)memcpy(&m_store[idx], buffer, size);
    } else {
        // Data wraps around - two memcpy operations
        FW_ASSERT(idx + bytes_to_end <= m_store_size, static_cast<FwAssertArgType>(idx + bytes_to_end));
        (void)memcpy(&m_store[idx], buffer, bytes_to_end);
        FwSizeType remaining = size - bytes_to_end;
        FW_ASSERT(remaining <= m_store_size, static_cast<FwAssertArgType>(remaining));
        (void)memcpy(&m_store[0], &buffer[bytes_to_end], remaining);
    }

    m_ser_idx += size;
    // Update allocated size if we've written beyond the current end
    if (m_ser_idx > m_allocated_size) {
        m_allocated_size = m_ser_idx;
        if (m_allocated_size > m_high_water_mark) {
            m_high_water_mark = m_allocated_size;
        }
    }
    FW_ASSERT(m_allocated_size <= this->getCapacity(), static_cast<FwAssertArgType>(m_allocated_size));
    FW_ASSERT(m_ser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_ser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::peek(char& value, FwSizeType offset) const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return peek(reinterpret_cast<U8&>(value), offset);
}

Fw::SerializeStatus CircularBuffer::peek(U8& value, FwSizeType offset) const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    // Check there is sufficient data
    if ((sizeof(U8) + offset) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    const FwSizeType idx = advance_idx(m_head_idx, offset);
    FW_ASSERT(idx < m_store_size, static_cast<FwAssertArgType>(idx));
    value = m_store[idx];
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::peek(U32& value, FwSizeType offset) const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    // Check there is sufficient data
    if ((sizeof(U32) + offset) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    value = 0;
    FwSizeType idx = advance_idx(m_head_idx, offset);

    // Deserialize all the bytes from network format
    for (FwSizeType i = 0; i < sizeof(U32); i++) {
        FW_ASSERT(idx < m_store_size, static_cast<FwAssertArgType>(idx));
        value = (value << 8) | static_cast<U32>(m_store[idx]);
        idx = advance_idx(idx);
    }
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::peek(U8* buffer, FwSizeType size, FwSizeType offset) const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buffer != nullptr);
    // Check there is sufficient data
    if ((size + offset) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    FwSizeType idx = advance_idx(m_head_idx, offset);
    FwSizeType bytes_to_end = m_store_size - idx;

    if (size <= bytes_to_end) {
        // Data is contiguous - single memcpy
        FW_ASSERT(idx + size <= m_store_size, static_cast<FwAssertArgType>(idx + size));
        (void)memcpy(buffer, &m_store[idx], size);
    } else {
        // Data wraps around - two memcpy operations
        FW_ASSERT(idx + bytes_to_end <= m_store_size, static_cast<FwAssertArgType>(idx + bytes_to_end));
        (void)memcpy(buffer, &m_store[idx], bytes_to_end);
        FwSizeType remaining = size - bytes_to_end;
        FW_ASSERT(remaining <= m_store_size, static_cast<FwAssertArgType>(remaining));
        (void)memcpy(&buffer[bytes_to_end], &m_store[0], remaining);
    }

    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::rotate(FwSizeType amount) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    // Check there is sufficient data
    if (amount > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    m_head_idx = advance_idx(m_head_idx, amount);
    m_allocated_size -= amount;
    // Adjust serialization index: if it's beyond the rotated amount, subtract the rotation
    // Otherwise, it's in the rotated-away region, so reset to 0
    m_ser_idx = (m_ser_idx >= amount) ? (m_ser_idx - amount) : 0;
    // Adjust deserialization index: if it's beyond the rotated amount, subtract the rotation
    // Otherwise, it's in the rotated-away region, so reset to 0
    m_deser_idx = (m_deser_idx >= amount) ? (m_deser_idx - amount) : 0;
    return Fw::FW_SERIALIZE_OK;
}

FwSizeType CircularBuffer::get_high_water_mark() const {
    return m_high_water_mark;
}

void CircularBuffer::clear_high_water_mark() {
    m_high_water_mark = 0;
}

// ----------------------------------------------------------------------
// SerialBufferBase interface implementation
// ----------------------------------------------------------------------

/**
 * \brief Helper function for deserializing multi-byte integer values with endianness support
 *
 * Deserializes a multi-byte value from the circular buffer by reading bytes and reconstructing
 * the value according to the specified endianness. Handles buffer wrap-around correctly by
 * using the peek() method to copy bytes into a contiguous array before reconstruction.
 *
 * \param buffer Pointer to the CircularBuffer to deserialize from
 * \param value Reference to store the deserialized value
 * \param mode Endianness mode (BIG or LITTLE) for byte order interpretation
 * \param availableSize Total size of available data in the buffer for deserialization
 * \param deserIdx Reference to deserialization index (offset from buffer head); updated on success
 * \return FW_SERIALIZE_OK on success, FW_DESERIALIZE_BUFFER_EMPTY if insufficient data available
 */
template <typename T>
static inline Fw::SerializeStatus deserializeMultibyte(CircularBuffer* buffer,
                                                       T& value,
                                                       Fw::Endianness mode,
                                                       FwSizeType availableSize,
                                                       FwSizeType& deserIdx) {
    if (sizeof(T) > (availableSize - deserIdx)) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    U8 bytes[sizeof(T)];
    Fw::SerializeStatus status = buffer->peek(bytes, sizeof(T), deserIdx);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    value = 0;
    if (mode == Fw::Endianness::BIG) {
        // Big-endian: MSB first
        for (FwSizeType i = 0; i < sizeof(T); i++) {
            value = static_cast<T>((value << 8) | static_cast<T>(bytes[i]));
        }
    } else {
        // Little-endian: LSB first
        for (FwSizeType i = 0; i < sizeof(T); i++) {
            value = static_cast<T>(value | (static_cast<T>(bytes[i]) << (i * 8)));
        }
    }
    deserIdx += sizeof(T);
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::serializeFrom(U8 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return this->serializeRaw(&val, sizeof(val));
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I8 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U8>(val), mode);
}

#if FW_HAS_16_BIT == 1
Fw::SerializeStatus CircularBuffer::serializeFrom(U16 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return serializeMultibyteValue<U16>(this, val, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I16 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U16>(val), mode);
}
#endif

#if FW_HAS_32_BIT == 1
Fw::SerializeStatus CircularBuffer::serializeFrom(U32 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return serializeMultibyteValue<U32>(this, val, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I32 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U32>(val), mode);
}
#endif

#if FW_HAS_64_BIT == 1
Fw::SerializeStatus CircularBuffer::serializeFrom(U64 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return serializeMultibyteValue<U64>(this, val, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I64 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U64>(val), mode);
}
#endif

Fw::SerializeStatus CircularBuffer::serializeFrom(F32 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    U32 temp;
    (void)memcpy(&temp, &val, sizeof(F32));
    return serializeFrom(temp, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(F64 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    U64 temp;
    (void)memcpy(&temp, &val, sizeof(F64));
    return serializeFrom(temp, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(bool val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    U8 byte = val ? FW_SERIALIZE_TRUE_VALUE : FW_SERIALIZE_FALSE_VALUE;
    // Single byte - endianness doesn't apply
    return this->serializeRaw(&byte, sizeof(byte));
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const void* val, Fw::Endianness mode) {
    return serializeFrom(reinterpret_cast<PlatformPointerCastType>(val), mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const U8* buff, FwSizeType length, Fw::Endianness endianMode) {
    return serializeFrom(buff, length, Fw::Serialization::INCLUDE_LENGTH, endianMode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const U8* buff,
                                                  FwSizeType length,
                                                  Fw::Serialization::t lengthMode,
                                                  Fw::Endianness endianMode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buff != nullptr);
    if (lengthMode == Fw::Serialization::INCLUDE_LENGTH) {
        Fw::SerializeStatus status = serializeSize(length, endianMode);
        if (status != Fw::FW_SERIALIZE_OK) {
            return status;
        }
    }
    // Raw byte array - no endianness conversion
    return this->serializeRaw(buff, length);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const Fw::LinearBufferBase& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(val.getBuffAddr() != nullptr);
    Fw::SerializeStatus status = serializeSize(val.getSize(), mode);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    // Already serialized data - no endianness conversion
    return this->serializeRaw(val.getBuffAddr(), val.getSize());
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const Fw::Serializable& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    // NOTE: This implementation uses a temporary buffer on the stack because the Serializable
    // interface requires a contiguous SerialBufferBase for serialization, which cannot be
    // directly provided by a circular buffer that may wrap. This is a known limitation.
    // For embedded systems with tight stack constraints, consider:
    // 1. Using serializeFrom(U8*, FwSizeType) for pre-serialized data when possible
    // 2. Ensuring Serializable objects are small enough to fit in FW_COM_BUFFER_MAX_SIZE
    // 3. Implementing a custom serialization path that avoids the temporary buffer
    U8 tempBuf[FW_COM_BUFFER_MAX_SIZE];  // Standard F' communication buffer size
    Fw::ExternalSerializeBuffer tempBuffer(tempBuf, sizeof(tempBuf));

    Fw::SerializeStatus status = val.serializeTo(tempBuffer, mode);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }

    // Already serialized data - no endianness conversion
    return this->serializeRaw(tempBuffer.getBuffAddr(), tempBuffer.getSize());
}

Fw::SerializeStatus CircularBuffer::serializeSize(const FwSizeType size, Fw::Endianness mode) {
    FwSizeStoreType storeSize = static_cast<FwSizeStoreType>(size);
    return serializeFrom(storeSize, mode);
}

// Deserialization methods

Fw::SerializeStatus CircularBuffer::deserializeTo(U8& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    if (m_deser_idx >= m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    return peek(val, m_deser_idx++);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I8& val, Fw::Endianness mode) {
    U8 temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        val = static_cast<I8>(temp);
    }
    return status;
}

#if FW_HAS_16_BIT == 1
Fw::SerializeStatus CircularBuffer::deserializeTo(U16& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return deserializeMultibyte<U16>(this, val, mode, m_allocated_size, m_deser_idx);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I16& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return deserializeMultibyte<I16>(this, val, mode, m_allocated_size, m_deser_idx);
}
#endif

#if FW_HAS_32_BIT == 1
Fw::SerializeStatus CircularBuffer::deserializeTo(U32& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return deserializeMultibyte<U32>(this, val, mode, m_allocated_size, m_deser_idx);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I32& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return deserializeMultibyte<I32>(this, val, mode, m_allocated_size, m_deser_idx);
}
#endif

#if FW_HAS_64_BIT == 1
Fw::SerializeStatus CircularBuffer::deserializeTo(U64& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return deserializeMultibyte<U64>(this, val, mode, m_allocated_size, m_deser_idx);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I64& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    return deserializeMultibyte<I64>(this, val, mode, m_allocated_size, m_deser_idx);
}
#endif

Fw::SerializeStatus CircularBuffer::deserializeTo(F32& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    U32 temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        (void)memcpy(&val, &temp, sizeof(F32));
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeTo(F64& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    U64 temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        (void)memcpy(&val, &temp, sizeof(F64));
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeTo(bool& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    U8 byte;
    Fw::SerializeStatus status = deserializeTo(byte, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        if (byte == FW_SERIALIZE_TRUE_VALUE) {
            val = true;
        } else if (byte == FW_SERIALIZE_FALSE_VALUE) {
            val = false;
        } else {
            return Fw::FW_DESERIALIZE_FORMAT_ERROR;
        }
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeTo(void*& val, Fw::Endianness mode) {
    PlatformPointerCastType temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        val = reinterpret_cast<void*>(temp);
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeTo(U8* buff, FwSizeType& length, Fw::Endianness endianMode) {
    return deserializeTo(buff, length, Fw::Serialization::INCLUDE_LENGTH, endianMode);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(U8* buff,
                                                  FwSizeType& length,
                                                  Fw::Serialization::t lengthMode,
                                                  Fw::Endianness endianMode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buff != nullptr);
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));

    FwSizeType deserLength = length;
    if (lengthMode == Fw::Serialization::INCLUDE_LENGTH) {
        Fw::SerializeStatus status = deserializeSize(deserLength, endianMode);
        if (status != Fw::FW_SERIALIZE_OK) {
            return status;
        }
        if (deserLength > length) {
            return Fw::FW_DESERIALIZE_SIZE_MISMATCH;
        }
    }

    if ((m_deser_idx + deserLength) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }

    Fw::SerializeStatus status = peek(buff, deserLength, m_deser_idx);
    if (status == Fw::FW_SERIALIZE_OK) {
        m_deser_idx += deserLength;
        length = deserLength;
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeTo(Fw::Serializable& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    // Create a temporary buffer with remaining data
    FwSizeType remaining = m_allocated_size - m_deser_idx;
    if (remaining == 0) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }

    U8 tempBuf[FW_COM_BUFFER_MAX_SIZE];
    FwSizeType copySize = (remaining < sizeof(tempBuf)) ? remaining : sizeof(tempBuf);

    Fw::SerializeStatus status = peek(tempBuf, copySize, m_deser_idx);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }

    Fw::ExternalSerializeBuffer tempBuffer(tempBuf, copySize);
    status = tempBuffer.setBuffLen(copySize);
    FW_ASSERT(status == Fw::FW_SERIALIZE_OK);

    FwSizeType beforeDeser = tempBuffer.getDeserializeSizeLeft();
    status = val.deserializeFrom(tempBuffer, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        FwSizeType consumed = beforeDeser - tempBuffer.getDeserializeSizeLeft();
        m_deser_idx += consumed;
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeTo(Fw::LinearBufferBase& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    FW_ASSERT(val.getBuffAddr() != nullptr);

    // Save deserialization index to restore on failure
    FwSizeType savedDeserIdx = m_deser_idx;

    FwSizeType size;
    Fw::SerializeStatus status = deserializeSize(size, mode);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }

    if (size > val.getCapacity()) {
        // Restore deserialization index on failure
        m_deser_idx = savedDeserIdx;
        return Fw::FW_DESERIALIZE_SIZE_MISMATCH;
    }

    if ((m_deser_idx + size) > m_allocated_size) {
        // Restore deserialization index on failure
        m_deser_idx = savedDeserIdx;
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }

    status = peek(val.getBuffAddr(), size, m_deser_idx);
    if (status == Fw::FW_SERIALIZE_OK) {
        status = val.setBuffLen(size);
        if (status == Fw::FW_SERIALIZE_OK) {
            m_deser_idx += size;
        } else {
            // Restore deserialization index on failure
            m_deser_idx = savedDeserIdx;
        }
    } else {
        // Restore deserialization index on failure
        m_deser_idx = savedDeserIdx;
    }
    return status;
}

Fw::SerializeStatus CircularBuffer::deserializeSize(FwSizeType& size, Fw::Endianness mode) {
    FwSizeStoreType temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        size = static_cast<FwSizeType>(temp);
    }
    return status;
}

// Buffer management methods

void CircularBuffer::resetSer() {
    // Reset serialization means clearing the buffer
    m_head_idx = 0;
    m_allocated_size = 0;
    m_deser_idx = 0;
    m_ser_idx = 0;
}

void CircularBuffer::resetDeser() {
    // Reset deserialization pointer to beginning
    m_deser_idx = 0;
}

Fw::SerializeStatus CircularBuffer::moveSerToOffset(FwSizeType offset) {
    FW_ASSERT(this->m_store != nullptr && this->m_store_size != 0);  // setup method was called
    // Check if offset is within the capacity of the circular buffer
    if (offset > this->getCapacity()) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
    this->m_ser_idx = offset;
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::moveDeserToOffset(FwSizeType offset) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    if (offset > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    m_deser_idx = offset;
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::serializeSkip(FwSizeType numBytesToSkip) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    Fw::SerializeStatus status = this->checkSerializeSpace(numBytesToSkip);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    // Advance serialization index
    m_ser_idx += numBytesToSkip;
    // Update allocated size if we've moved beyond the current end
    if (m_ser_idx > m_allocated_size) {
        m_allocated_size = m_ser_idx;
    }
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::deserializeSkip(FwSizeType numBytesToSkip) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx),
              static_cast<FwAssertArgType>(m_allocated_size));
    if ((m_deser_idx + numBytesToSkip) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    m_deser_idx += numBytesToSkip;
    return Fw::FW_SERIALIZE_OK;
}

Fw::Serializable::SizeType CircularBuffer::getCapacity() const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return m_store_size;
}

Fw::Serializable::SizeType CircularBuffer::getSize() const {
    return m_allocated_size;
}

Fw::Serializable::SizeType CircularBuffer::getDeserializeSizeLeft() const {
    FW_ASSERT(m_deser_idx <= m_allocated_size);
    return m_allocated_size - m_deser_idx;
}

Fw::Serializable::SizeType CircularBuffer::getSerializeSizeLeft() const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_allocated_size <= m_store_size, static_cast<FwAssertArgType>(m_allocated_size));
    return m_store_size - m_allocated_size;
}

Fw::SerializeStatus CircularBuffer::setBuff(const U8* src, Fw::Serializable::SizeType length) {
    FW_ASSERT(src != nullptr);
    FW_ASSERT(m_store != nullptr && m_store_size != 0);

    // Clear existing data and copy raw bytes
    resetSer();
    return this->serializeRaw(src, length);
}

Fw::SerializeStatus CircularBuffer::setBuffLen(Fw::Serializable::SizeType length) {
    if (length > m_store_size) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
    m_allocated_size = length;
    m_ser_idx = length;
    m_deser_idx = 0;
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::copyRaw(Fw::SerialBufferBase& dest, Fw::Serializable::SizeType size) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);

    // Check if there's enough data to copy
    if (size > this->getSize()) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }

    // Check if destination has enough capacity
    if (size > dest.getCapacity()) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }

    // Get the current read position (from deserialization index)
    FwSizeType read_idx = advance_idx(m_head_idx, m_deser_idx);

    // If data doesn't wrap, we can use setBuff directly
    FwSizeType bytes_to_end = m_store_size - read_idx;
    if (size <= bytes_to_end) {
        // Data is contiguous, use setBuff
        Fw::SerializeStatus status = dest.setBuff(&m_store[read_idx], size);
        if (status == Fw::FW_SERIALIZE_OK) {
            m_deser_idx += size;
        }
        return status;
    }

    // Data wraps around
    // Reset destination buffer first (copyRaw replaces contents)
    dest.resetSer();

    FwSizeType remaining = size;
    while (remaining > 0) {
        FwSizeType chunkSize = (remaining < (m_store_size - read_idx)) ? remaining : (m_store_size - read_idx);

        // Serialize each chunk directly into destination (appends data)
        Fw::SerializeStatus status = dest.serializeFrom(&m_store[read_idx], chunkSize, Fw::Serialization::OMIT_LENGTH);
        if (status != Fw::FW_SERIALIZE_OK) {
            return status;
        }

        remaining -= chunkSize;
        read_idx = advance_idx(read_idx, chunkSize);
    }

    // Update deserialization index after successful copy
    m_deser_idx += size;

    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::copyRawOffset(Fw::SerialBufferBase& dest, Fw::Serializable::SizeType size) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);

    // Check if there's enough data to copy
    if (size > this->getSize()) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }

    // Check if destination has enough space remaining (capacity check, not serialization space)
    if (dest.getCapacity() < size + dest.getSize()) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }

    // Get the current read position (from deserialization index)
    FwSizeType read_idx = advance_idx(m_head_idx, m_deser_idx);
    FwSizeType bytes_to_end = m_store_size - read_idx;
    if (size <= bytes_to_end) {
        Fw::SerializeStatus status = dest.serializeFrom(&m_store[read_idx], size, Fw::Serialization::OMIT_LENGTH);
        if (status == Fw::FW_SERIALIZE_OK) {
            m_deser_idx += size;
        }
        return status;
    }

    // Copy data in chunks that don't wrap around the circular buffer
    FwSizeType remaining = size;
    while (remaining > 0) {
        // Calculate how much we can copy in this chunk
        FwSizeType chunkSize = (remaining < (m_store_size - read_idx)) ? remaining : (m_store_size - read_idx);

        // Use serializeFrom with OMIT_LENGTH to avoid adding length prefixes
        Fw::SerializeStatus status = dest.serializeFrom(&m_store[read_idx], chunkSize, Fw::Serialization::OMIT_LENGTH);
        if (status != Fw::FW_SERIALIZE_OK) {
            return status;
        }

        // Update indices and remaining count
        remaining -= chunkSize;
        read_idx = advance_idx(read_idx, chunkSize);
        m_deser_idx += chunkSize;  // Update deserialization index
    }

    return Fw::FW_SERIALIZE_OK;
}

}  // End Namespace Types
