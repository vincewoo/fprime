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

CircularBuffer ::CircularBuffer()
    : m_store(nullptr), m_store_size(0), m_head_idx(0), m_allocated_size(0), m_high_water_mark(0), m_deser_idx(0) {}

CircularBuffer ::CircularBuffer(U8* const buffer, const FwSizeType size)
    : m_store(nullptr), m_store_size(0), m_head_idx(0), m_allocated_size(0), m_high_water_mark(0), m_deser_idx(0) {
    setup(buffer, size);
}

void CircularBuffer ::setup(U8* const buffer, const FwSizeType size) {
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
}

FwSizeType CircularBuffer ::get_allocated_size() const {
    return m_allocated_size;
}

FwSizeType CircularBuffer ::get_free_size() const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_allocated_size <= m_store_size, static_cast<FwAssertArgType>(m_allocated_size));
    return m_store_size - m_allocated_size;
}

FwSizeType CircularBuffer ::advance_idx(FwSizeType idx, FwSizeType amount) const {
    FW_ASSERT(idx < m_store_size, static_cast<FwAssertArgType>(idx));
    return (idx + amount) % m_store_size;
}

Fw::SerializeStatus CircularBuffer ::serialize(const U8* const buffer, const FwSizeType size) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buffer != nullptr);
    // Check there is sufficient space
    if (size > get_free_size()) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
    // Copy in all the supplied data
    FwSizeType idx = advance_idx(m_head_idx, m_allocated_size);
    for (U32 i = 0; i < size; i++) {
        FW_ASSERT(idx < m_store_size, static_cast<FwAssertArgType>(idx));
        m_store[idx] = buffer[i];
        idx = advance_idx(idx);
    }
    m_allocated_size += size;
    FW_ASSERT(m_allocated_size <= this->get_capacity(), static_cast<FwAssertArgType>(m_allocated_size));
    m_high_water_mark = (m_high_water_mark > m_allocated_size) ? m_high_water_mark : m_allocated_size;
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer ::peek(char& value, FwSizeType offset) const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return peek(reinterpret_cast<U8&>(value), offset);
}

Fw::SerializeStatus CircularBuffer ::peek(U8& value, FwSizeType offset) const {
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

Fw::SerializeStatus CircularBuffer ::peek(U32& value, FwSizeType offset) const {
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

Fw::SerializeStatus CircularBuffer ::peek(U8* buffer, FwSizeType size, FwSizeType offset) const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buffer != nullptr);
    // Check there is sufficient data
    if ((size + offset) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    FwSizeType idx = advance_idx(m_head_idx, offset);
    // Deserialize all the bytes from network format
    for (FwSizeType i = 0; i < size; i++) {
        FW_ASSERT(idx < m_store_size, static_cast<FwAssertArgType>(idx));
        buffer[i] = m_store[idx];
        idx = advance_idx(idx);
    }
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer ::rotate(FwSizeType amount) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    // Check there is sufficient data
    if (amount > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    m_head_idx = advance_idx(m_head_idx, amount);
    m_allocated_size -= amount;
    return Fw::FW_SERIALIZE_OK;
}

FwSizeType CircularBuffer ::get_capacity() const {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return m_store_size;
}

FwSizeType CircularBuffer ::get_high_water_mark() const {
    return m_high_water_mark;
}

void CircularBuffer ::clear_high_water_mark() {
    m_high_water_mark = 0;
}

// ----------------------------------------------------------------------
// SerialBufferBase interface implementation
// ----------------------------------------------------------------------

// Helper macro for serializing multi-byte values with endianness support
#define SERIALIZE_MULTIBYTE(TYPE, VAL, MODE) \
    do { \
        if (sizeof(TYPE) > get_free_size()) { \
            return Fw::FW_SERIALIZE_NO_ROOM_LEFT; \
        } \
        U8 bytes[sizeof(TYPE)]; \
        TYPE temp = (VAL); \
        if ((MODE) == Fw::Endianness::BIG) { \
            for (FwSizeType i = 0; i < sizeof(TYPE); i++) { \
                bytes[sizeof(TYPE) - 1 - i] = static_cast<U8>(temp & 0xFF); \
                temp >>= 8; \
            } \
        } else { \
            for (FwSizeType i = 0; i < sizeof(TYPE); i++) { \
                bytes[i] = static_cast<U8>(temp & 0xFF); \
                temp >>= 8; \
            } \
        } \
        return serialize(bytes, sizeof(TYPE)); \
    } while(0)

// Helper macro for deserializing multi-byte values with endianness support
#define DESERIALIZE_MULTIBYTE(TYPE, VAL, MODE) \
    do { \
        if (sizeof(TYPE) > (m_allocated_size - m_deser_idx)) { \
            return Fw::FW_DESERIALIZE_BUFFER_EMPTY; \
        } \
        U8 bytes[sizeof(TYPE)]; \
        Fw::SerializeStatus status = peek(bytes, sizeof(TYPE), m_deser_idx); \
        if (status != Fw::FW_SERIALIZE_OK) { \
            return status; \
        } \
        (VAL) = 0; \
        if ((MODE) == Fw::Endianness::BIG) { \
            for (FwSizeType i = 0; i < sizeof(TYPE); i++) { \
                (VAL) = static_cast<TYPE>(static_cast<TYPE>((VAL) << 8) | static_cast<TYPE>(bytes[i])); \
            } \
        } else { \
            for (FwSizeType i = 0; i < sizeof(TYPE); i++) { \
                (VAL) = static_cast<TYPE>((VAL) | static_cast<TYPE>(static_cast<TYPE>(bytes[i]) << (i * 8))); \
            } \
        } \
        m_deser_idx += sizeof(TYPE); \
        return Fw::FW_SERIALIZE_OK; \
    } while(0)

Fw::SerializeStatus CircularBuffer::serializeFrom(U8 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    return serialize(&val, sizeof(val));
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I8 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U8>(val), mode);
}

#if FW_HAS_16_BIT == 1
Fw::SerializeStatus CircularBuffer::serializeFrom(U16 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    SERIALIZE_MULTIBYTE(U16, val, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I16 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U16>(val), mode);
}
#endif

#if FW_HAS_32_BIT == 1
Fw::SerializeStatus CircularBuffer::serializeFrom(U32 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    SERIALIZE_MULTIBYTE(U32, val, mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(I32 val, Fw::Endianness mode) {
    return serializeFrom(static_cast<U32>(val), mode);
}
#endif

#if FW_HAS_64_BIT == 1
Fw::SerializeStatus CircularBuffer::serializeFrom(U64 val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    SERIALIZE_MULTIBYTE(U64, val, mode);
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
    return serialize(&byte, sizeof(byte));
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const void* val, Fw::Endianness mode) {
    return serializeFrom(reinterpret_cast<PlatformPointerCastType>(val), mode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const U8* buff, FwSizeType length, Fw::Endianness endianMode) {
    return serializeFrom(buff, length, Fw::Serialization::INCLUDE_LENGTH, endianMode);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const U8* buff, FwSizeType length, Fw::Serialization::t lengthMode, Fw::Endianness endianMode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buff != nullptr);
    if (lengthMode == Fw::Serialization::INCLUDE_LENGTH) {
        Fw::SerializeStatus status = serializeSize(length, endianMode);
        if (status != Fw::FW_SERIALIZE_OK) {
            return status;
        }
    }
    return serialize(buff, length);
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const Fw::SerializeBufferBase& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(val.getBuffAddr() != nullptr);
    Fw::SerializeStatus status = serializeSize(val.getSize(), mode);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    return serialize(val.getBuffAddr(), val.getSize());
}

Fw::SerializeStatus CircularBuffer::serializeFrom(const Fw::Serializable& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    // Create a temporary external buffer wrapping our circular buffer's tail
    // This is a workaround since we can't directly serialize into circular buffer
    // We'll need to use a temporary linear buffer
    U8 tempBuf[512];  // Reasonable size for most serializable objects
    Fw::ExternalSerializeBuffer tempBuffer(tempBuf, sizeof(tempBuf));
    
    Fw::SerializeStatus status = val.serializeTo(tempBuffer, mode);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    
    return serialize(tempBuffer.getBuffAddr(), tempBuffer.getSize());
}

Fw::SerializeStatus CircularBuffer::serializeSize(const FwSizeType size, Fw::Endianness mode) {
    FwSizeStoreType storeSize = static_cast<FwSizeStoreType>(size);
    return serializeFrom(storeSize, mode);
}

// Deserialization methods

Fw::SerializeStatus CircularBuffer::deserializeTo(U8& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
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
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    DESERIALIZE_MULTIBYTE(U16, val, mode);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I16& val, Fw::Endianness mode) {
    U16 temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        val = static_cast<I16>(temp);
    }
    return status;
}
#endif

#if FW_HAS_32_BIT == 1
Fw::SerializeStatus CircularBuffer::deserializeTo(U32& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    DESERIALIZE_MULTIBYTE(U32, val, mode);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I32& val, Fw::Endianness mode) {
    U32 temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        val = static_cast<I32>(temp);
    }
    return status;
}
#endif

#if FW_HAS_64_BIT == 1
Fw::SerializeStatus CircularBuffer::deserializeTo(U64& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    DESERIALIZE_MULTIBYTE(U64, val, mode);
}

Fw::SerializeStatus CircularBuffer::deserializeTo(I64& val, Fw::Endianness mode) {
    U64 temp;
    Fw::SerializeStatus status = deserializeTo(temp, mode);
    if (status == Fw::FW_SERIALIZE_OK) {
        val = static_cast<I64>(temp);
    }
    return status;
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

Fw::SerializeStatus CircularBuffer::deserializeTo(U8* buff, FwSizeType& length, Fw::Serialization::t lengthMode, Fw::Endianness endianMode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(buff != nullptr);
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    
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
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    // Create a temporary buffer with remaining data
    FwSizeType remaining = m_allocated_size - m_deser_idx;
    if (remaining == 0) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    
    U8 tempBuf[512];
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

Fw::SerializeStatus CircularBuffer::deserializeTo(Fw::SerializeBufferBase& val, Fw::Endianness mode) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    FW_ASSERT(val.getBuffAddr() != nullptr);
    FwSizeType size;
    Fw::SerializeStatus status = deserializeSize(size, mode);
    if (status != Fw::FW_SERIALIZE_OK) {
        return status;
    }
    
    if (size > val.getCapacity()) {
        return Fw::FW_DESERIALIZE_SIZE_MISMATCH;
    }
    
    if ((m_deser_idx + size) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    
    status = peek(val.getBuffAddr(), size, m_deser_idx);
    if (status == Fw::FW_SERIALIZE_OK) {
        status = val.setBuffLen(size);
        if (status == Fw::FW_SERIALIZE_OK) {
            m_deser_idx += size;
        }
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
}

void CircularBuffer::resetDeser() {
    // Reset deserialization pointer to beginning
    m_deser_idx = 0;
}

Fw::SerializeStatus CircularBuffer::moveSerToOffset(FwSizeType offset) {
    // For circular buffer, we can't arbitrarily move serialization offset
    // We can only append to the end. Return error if offset doesn't match current end.
    if (offset != m_allocated_size) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
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
    // For circular buffer, skipping during serialization doesn't make sense
    // since we always append to the end
    if (numBytesToSkip > get_free_size()) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
    // Allocate space by advancing allocated size
    m_allocated_size += numBytesToSkip;
    return Fw::FW_SERIALIZE_OK;
}

Fw::SerializeStatus CircularBuffer::deserializeSkip(FwSizeType numBytesToSkip) {
    FW_ASSERT(m_store != nullptr && m_store_size != 0);  // setup method was called
    FW_ASSERT(m_deser_idx <= m_allocated_size, static_cast<FwAssertArgType>(m_deser_idx), static_cast<FwAssertArgType>(m_allocated_size));
    if ((m_deser_idx + numBytesToSkip) > m_allocated_size) {
        return Fw::FW_DESERIALIZE_BUFFER_EMPTY;
    }
    m_deser_idx += numBytesToSkip;
    return Fw::FW_SERIALIZE_OK;
}

Fw::Serializable::SizeType CircularBuffer::getCapacity() const {
    return get_capacity();
}

Fw::Serializable::SizeType CircularBuffer::getSize() const {
    return m_allocated_size;
}

Fw::Serializable::SizeType CircularBuffer::getDeserializeSizeLeft() const {
    FW_ASSERT(m_deser_idx <= m_allocated_size);
    return m_allocated_size - m_deser_idx;
}

Fw::Serializable::SizeType CircularBuffer::getSerializeSizeLeft() const {
    return get_free_size();
}

Fw::SerializeStatus CircularBuffer::setBuff(const U8* src, Fw::Serializable::SizeType length) {
    FW_ASSERT(src != nullptr);
    FW_ASSERT(m_store != nullptr && m_store_size != 0);
    
    // Clear existing data and serialize new data
    resetSer();
    return serialize(src, length);
}

Fw::SerializeStatus CircularBuffer::setBuffLen(Fw::Serializable::SizeType length) {
    if (length > m_store_size) {
        return Fw::FW_SERIALIZE_NO_ROOM_LEFT;
    }
    m_allocated_size = length;
    m_deser_idx = 0;
    return Fw::FW_SERIALIZE_OK;
}

}  // End Namespace Types
