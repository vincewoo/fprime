/*
 * CircularBuffer.hpp:
 *
 * Buffer used to efficiently store data in ring data structure. Uses an externally supplied
 * data store as the backing for this buffer. Thus it is dependent on receiving sole ownership
 * of the supplied buffer.
 *
 * Note: this given implementation loses one byte of the data store in order to ensure that a
 * separate wrap-around tracking variable is not needed.
 *
 *  Created on: Apr 4, 2019
 *      Author: lestarch
 *  Revised March 2022
 *      Author: bocchino
 */

#ifndef TYPES_CIRCULAR_BUFFER_HPP
#define TYPES_CIRCULAR_BUFFER_HPP

#include <Fw/FPrimeBasicTypes.hpp>
#include <Fw/Types/Assert.hpp>
#include <Fw/Types/Serializable.hpp>

namespace Types {

class CircularBuffer : public Fw::SerialBufferBase {
    friend class CircularBufferTester;

    // Friend declaration for static helper function
    template <typename T>
    friend Fw::SerializeStatus serializeMultibyteValue(CircularBuffer* buffer, T value, Fw::Endianness mode);

  public:
    /**
     * Circular buffer constructor. Wraps the supplied buffer as the new data store. Buffer
     * size is supplied in the 'size' argument.
     *
     * Note: specification of storage buffer must be done using `setup` before use.
     */
    CircularBuffer();

    /**
     * Circular buffer constructor. Wraps the supplied buffer as the new data store. Buffer
     * size is supplied in the 'size' argument. This is equivalent to calling the no-argument constructor followed
     * by setup(buffer, size).
     *
     * Note: ownership of the supplied buffer is held until the circular buffer is deallocated
     *
     * \param buffer: supplied buffer used as a data store.
     * \param size: the of the supplied data store.
     */
    CircularBuffer(U8* const buffer, const FwSizeType size);

    /**
     * Wraps the supplied buffer as the new data store. Buffer size is supplied in the 'size' argument. Cannot be
     * called after successful setup.
     *
     * Note: ownership of the supplied buffer is held until the circular buffer is deallocated
     *
     * \param buffer: supplied buffer used as a data store.
     * \param size: the of the supplied data store.
     */
    void setup(U8* const buffer, const FwSizeType size);

    /**
     * Serialize a given buffer into this circular buffer. Will not accept more data than
     * space available. This means it will not overwrite existing data.
     * \param buffer: supplied buffer to be serialized.
     * \param size: size of the supplied buffer.
     * \return Fw::FW_SERIALIZE_OK on success or something else on error
     */
    DEPRECATED(Fw::SerializeStatus serialize(const U8* const buffer, const FwSizeType size),
               "Use serializeFrom(buffer, size, Fw::Serialization::OMIT_LENGTH) instead") {
        return serializeFrom(buffer, size, Fw::Serialization::OMIT_LENGTH);
    }

    /**
     * Deserialize data into the given variable without moving the head index
     * \param value: value to fill
     * \param offset: offset from head to start peak. Default: 0
     * \return Fw::FW_SERIALIZE_OK on success or something else on error
     */
    Fw::SerializeStatus peek(char& value, FwSizeType offset = 0) const;
    /**
     * Deserialize data into the given variable without moving the head index
     * \param value: value to fill
     * \param offset: offset from head to start peak. Default: 0
     * \return Fw::FW_SERIALIZE_OK on success or something else on error
     */
    Fw::SerializeStatus peek(U8& value, FwSizeType offset = 0) const;
    /**
     * Deserialize data into the given variable without moving the head index
     * \param value: value to fill
     * \param offset: offset from head to start peak. Default: 0
     * \return Fw::FW_SERIALIZE_OK on success or something else on error
     */
    Fw::SerializeStatus peek(U32& value, FwSizeType offset = 0) const;

    /**
     * Deserialize data into the given buffer without moving the head variable.
     * \param buffer: buffer to fill with data of the peek
     * \param size: size in bytes to peek at
     * \param offset: offset from head to start peak. Default: 0
     * \return Fw::FW_SERIALIZE_OK on success or something else on error
     */
    Fw::SerializeStatus peek(U8* buffer, FwSizeType size, FwSizeType offset = 0) const;

    /**
     * Rotate the head index, deleting data from the circular buffer and making
     * space. Cannot rotate more than the available space.
     * \param amount: amount to rotate by (in bytes)
     * \return Fw::FW_SERIALIZE_OK on success or something else on error
     */
    Fw::SerializeStatus rotate(FwSizeType amount);

    /**
     * Get the number of bytes allocated in the buffer
     * \return number of bytes
     */
    DEPRECATED(FwSizeType get_allocated_size() const, "Use getSize() instead") { return getSize(); }

    /**
     * Get the number of free bytes, i.e., the number
     * of bytes that may be stored in the buffer without
     * deleting data and without exceeding the buffer capacity
     */
    DEPRECATED(FwSizeType get_free_size() const, "Use getSerializeSizeLeft() instead") {
        return getSerializeSizeLeft();
    }

    /**
     * Get the logical capacity of the buffer, i.e., the number of available
     * bytes when the buffer is empty
     */
    DEPRECATED(FwSizeType get_capacity() const, "Use getCapacity() instead") { return getCapacity(); }

    /**
     * Return the largest tracked allocated size
     */
    FwSizeType get_high_water_mark() const;

    /**
     * Clear tracking of the largest allocated size
     */
    void clear_high_water_mark();

    // ----------------------------------------------------------------------
    // SerialBufferBase interface implementation
    // ----------------------------------------------------------------------

    // Serialization methods

    //! \brief Serialize an 8-bit unsigned integer value
    //!
    //! This method serializes a single 8-bit unsigned integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 8-bit unsigned integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(U8 val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize an 8-bit signed integer value
    //!
    //! This method serializes a single 8-bit signed integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 8-bit signed integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(I8 val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#if FW_HAS_16_BIT == 1
    //! \brief Serialize a 16-bit unsigned integer value
    //!
    //! This method serializes a single 16-bit unsigned integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 16-bit unsigned integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(U16 val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a 16-bit signed integer value
    //!
    //! This method serializes a single 16-bit signed integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 16-bit signed integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(I16 val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#endif
#if FW_HAS_32_BIT == 1
    //! \brief Serialize a 32-bit unsigned integer value
    //!
    //! This method serializes a single 32-bit unsigned integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 32-bit unsigned integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(U32 val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a 32-bit signed integer value
    //!
    //! This method serializes a single 32-bit signed integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 32-bit signed integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(I32 val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#endif
#if FW_HAS_64_BIT == 1
    //! \brief Serialize a 64-bit unsigned integer value
    //!
    //! This method serializes a single 64-bit unsigned integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 64-bit unsigned integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(U64 val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a 64-bit signed integer value
    //!
    //! This method serializes a single 64-bit signed integer value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 64-bit signed integer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(I64 val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#endif

    //! \brief Serialize a 32-bit floating point value
    //!
    //! This method serializes a single 32-bit floating point value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 32-bit floating point value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(F32 val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a 64-bit floating point value
    //!
    //! This method serializes a single 64-bit floating point value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The 64-bit floating point value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(F64 val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a boolean value
    //!
    //! This method serializes a single boolean value into the buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The boolean value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(bool val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a pointer value
    //!
    //! This method serializes a pointer value into the buffer. Note that only
    //! the pointer value itself is serialized, not the contents it points to.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param val The pointer value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(const void* val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a data buffer
    //!
    //! This method serializes a buffer of bytes into the serialization buffer.
    //! The endianness of the serialization can be controlled via the mode parameter.
    //!
    //! \param buff Pointer to the buffer containing data to serialize
    //! \param length Number of bytes to serialize from the buffer
    //! \param endianMode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(const U8* buff,
                                      FwSizeType length,
                                      Fw::Endianness endianMode = Fw::Endianness::BIG) override;

    //! \brief Serialize a byte buffer with optional length prefix
    //!
    //! This method serializes a buffer of bytes into the serialization buffer.
    //! If lengthMode is set to INCLUDE_LENGTH, the length is included as the first token.
    //! The endianness of the serialization can be controlled via the endianMode parameter.
    //!
    //! \param buff Pointer to the buffer containing data to serialize
    //! \param length Number of bytes to serialize from the buffer
    //! \param lengthMode Specifies whether to include length in serialization (INCLUDE_LENGTH or OMIT_LENGTH)
    //! \param endianMode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(const U8* buff,
                                      FwSizeType length,
                                      Fw::Serialization::t lengthMode,
                                      Fw::Endianness endianMode = Fw::Endianness::BIG) override;

    //! \brief Serialize another LinearBufferBase object
    //!
    //! This method serializes the contents of another LinearBufferBase object
    //! into this buffer. The endianness of the serialization can be controlled
    //! via the mode parameter.
    //!
    //! \param val Reference to the LinearBufferBase object to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(const Fw::LinearBufferBase& val,
                                      Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a Serializable object
    //!
    //! This method serializes an object derived from the Serializable base class
    //! into this buffer. The endianness of the serialization can be controlled
    //! via the mode parameter.
    //!
    //! \param val Reference to the Serializable object to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeFrom(const Fw::Serializable& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Serialize a size value
    //!
    //! This method serializes a size value (typically used for buffer sizes)
    //! into this buffer. The endianness of the serialization can be controlled
    //! via the mode parameter.
    //!
    //! \param size The size value to serialize
    //! \param mode Endianness mode for serialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeSize(const FwSizeType size, Fw::Endianness mode = Fw::Endianness::BIG) override;

    // Deserialization methods

    //! \brief Deserialize an 8-bit unsigned integer value
    //!
    //! This method reads an 8-bit unsigned integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 8-bit unsigned integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(U8& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize an 8-bit signed integer value
    //!
    //! This method reads an 8-bit signed integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 8-bit signed integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(I8& val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#if FW_HAS_16_BIT == 1
    //! \brief Deserialize a 16-bit unsigned integer value
    //!
    //! This method reads a 16-bit unsigned integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 16-bit unsigned integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(U16& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a 16-bit signed integer value
    //!
    //! This method reads a 16-bit signed integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 16-bit signed integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(I16& val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#endif
#if FW_HAS_32_BIT == 1
    //! \brief Deserialize a 32-bit unsigned integer value
    //!
    //! This method reads a 32-bit unsigned integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 32-bit unsigned integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(U32& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a 32-bit signed integer value
    //!
    //! This method reads a 32-bit signed integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 32-bit signed integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(I32& val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#endif
#if FW_HAS_64_BIT == 1
    //! \brief Deserialize a 64-bit unsigned integer value
    //!
    //! This method reads a 64-bit unsigned integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 64-bit unsigned integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(U64& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a 64-bit signed integer value
    //!
    //! This method reads a 64-bit signed integer value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 64-bit signed integer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(I64& val, Fw::Endianness mode = Fw::Endianness::BIG) override;
#endif

    //! \brief Deserialize a 32-bit floating point value
    //!
    //! This method reads a 32-bit floating point value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 32-bit floating point value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(F32& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a 64-bit floating point value
    //!
    //! This method reads a 64-bit floating point value from the deserialization
    //! buffer and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized 64-bit floating point value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(F64& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a boolean value
    //!
    //! This method reads a boolean value from the deserialization buffer
    //! and stores it in the provided reference. The endianness of the
    //! deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized boolean value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(bool& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a pointer value
    //!
    //! This method reads a pointer value from the deserialization buffer
    //! and stores it in the provided reference. Note that only the pointer
    //! value itself is deserialized, not the contents it points to. The
    //! endianness of the deserialization can be controlled via the mode parameter.
    //!
    //! \param val Reference to store the deserialized pointer value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(void*& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a data buffer
    //!
    //! This method reads a buffer of bytes from the deserialization buffer
    //! and stores them in the provided buffer. The endianness of the
    //! deserialization can be controlled via the endianMode parameter.
    //!
    //! \param buff Pointer to the buffer where deserialized data will be stored
    //! \param length Reference to store the actual number of bytes deserialized
    //! \param endianMode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(U8* buff,
                                      FwSizeType& length,
                                      Fw::Endianness endianMode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a byte buffer with optional length prefix
    //!
    //! This method reads a buffer of bytes from the deserialization buffer
    //! and stores them in the provided buffer. If lengthMode indicates that
    //! a length prefix was included, it will be read from the buffer first.
    //! The endianness of the deserialization can be controlled via the
    //! endianMode parameter.
    //!
    //! \param buff Pointer to the buffer where deserialized data will be stored
    //! \param length Reference to store the actual number of bytes deserialized
    //! \param lengthMode Specifies whether length was included in serialization (INCLUDE_LENGTH or OMIT_LENGTH)
    //! \param endianMode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(U8* buff,
                                      FwSizeType& length,
                                      Fw::Serialization::t lengthMode,
                                      Fw::Endianness endianMode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a Serializable object
    //!
    //! This method reads data from the deserialization buffer and reconstructs
    //! a Serializable object from it. The endianness of the deserialization
    //! can be controlled via the mode parameter.
    //!
    //! \param val Reference to the Serializable object that will be populated with deserialized data
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(Fw::Serializable& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a LinearBufferBase object
    //!
    //! This method reads data from the deserialization buffer and reconstructs
    //! a LinearBufferBase object from it. The endianness of the deserialization
    //! can be controlled via the mode parameter.
    //!
    //! \param val Reference to the LinearBufferBase object that will be populated with deserialized data
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeTo(Fw::LinearBufferBase& val, Fw::Endianness mode = Fw::Endianness::BIG) override;

    //! \brief Deserialize a size value
    //!
    //! This method reads a size value (typically used for buffer sizes)
    //! from the deserialization buffer. The endianness of the deserialization
    //! can be controlled via the mode parameter.
    //!
    //! \param size Reference to store the deserialized size value
    //! \param mode Endianness mode for deserialization (default is Endianness::BIG)
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeSize(FwSizeType& size, Fw::Endianness mode = Fw::Endianness::BIG) override;

    // Copy methods

    //! \brief Copy raw bytes from the source (this) into a destination buffer and advance source offset
    //!
    //! Copies exactly `size` bytes starting at the current deserialization pointer of `this` into `dest`.
    //! This operation does not prepend a length field and does not interpret the data.
    //!
    //! Preconditions:
    //! - `size` bytes must remain in the source (`getDeserializeSizeLeft() >= size`).
    //! - Destination must have sufficient capacity (`dest.getCapacity() >= size`).
    //!
    //! Postconditions on success:
    //! - `dest` contains exactly the copied bytes and its previous contents are discarded.
    //! - `this` has advanced its deserialization pointer by `size` bytes.
    //!
    //! \param dest Destination serialization buffer to receive the bytes (its contents are replaced)
    //! \param size Number of bytes to copy from the source
    //! \return `FW_SERIALIZE_OK` on success; `FW_SERIALIZE_NO_ROOM_LEFT` if destination capacity is insufficient;
    //!         `FW_DESERIALIZE_BUFFER_EMPTY` if source does not contain `size` bytes remaining
    Fw::SerializeStatus copyRaw(Fw::SerialBufferBase& dest, Fw::Serializable::SizeType size) override;

    //! \brief Append raw bytes to destination (no length) and advance source offset
    //!
    //! Appends exactly `size` bytes from the current deserialization pointer of `this` into `dest` using
    //! `Serialization::OMIT_LENGTH`, preserving any existing bytes already serialized in `dest`.
    //!
    //! Preconditions:
    //! - `size` bytes must remain in the source (`getDeserializeSizeLeft() >= size`).
    //! - Destination must have space for the append (`dest.getCapacity() >= dest.getSize() + size`).
    //!
    //! Postconditions on success:
    //! - `dest` gains `size` additional bytes at the end; no length token is written.
    //! - `this` has advanced its deserialization pointer by `size` bytes.
    //!
    //! \param dest Destination serialization buffer to append to
    //! \param size Number of bytes to copy from the source and append to dest
    //! \return `FW_SERIALIZE_OK` on success; `FW_SERIALIZE_NO_ROOM_LEFT` if destination capacity is insufficient;
    //!         `FW_DESERIALIZE_BUFFER_EMPTY` if source does not contain `size` bytes remaining
    Fw::SerializeStatus copyRawOffset(Fw::SerialBufferBase& dest, Fw::Serializable::SizeType size) override;

    // Buffer management methods

    //! \brief Reset serialization pointer to beginning of buffer
    //!
    //! This method resets the serialization pointer to the beginning of the buffer,
    //! allowing the buffer to be reused for new serialization operations. Any
    //! data that was previously serialized in the buffer will be overwritten.
    //!
    //! \return None
    void resetSer() override;

    //! \brief Reset deserialization pointer to beginning of buffer
    //!
    //! This method resets the deserialization pointer to the beginning of the buffer,
    //! allowing the buffer to be reused for new deserialization operations. The buffer
    //! contents are not modified, but the pointer is reset to allow reading from the
    //! start of the data.
    //!
    //! \return None
    void resetDeser() override;

    //! \brief Move serialization pointer to specified offset
    //!
    //! This method moves the serialization pointer to the specified offset within
    //! the buffer. This allows for skipping over data or positioning the serializer
    //! at a specific location in the buffer.
    //!
    //! \param offset The offset to move the serialization pointer to
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus moveSerToOffset(FwSizeType offset) override;

    //! \brief Move deserialization pointer to specified offset
    //!
    //! This method moves the deserialization pointer to the specified offset within
    //! the buffer. This allows for skipping over data or positioning the deserializer
    //! at a specific location in the buffer.
    //!
    //! \param offset The offset to move the deserialization pointer to
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus moveDeserToOffset(FwSizeType offset) override;

    //! \brief Skip specified number of bytes during serialization
    //!
    //! This method advances the serialization pointer by the specified number of bytes
    //! without writing any data. This can be used to reserve space in the buffer or skip
    //! over data that will be written later.
    //!
    //! \param numBytesToSkip Number of bytes to skip during serialization
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus serializeSkip(FwSizeType numBytesToSkip) override;

    //! \brief Skip specified number of bytes during deserialization
    //!
    //! This method advances the deserialization pointer by the specified number of bytes
    //! without reading any data. This can be used to skip over data in the buffer that
    //! is not needed or to advance to the next relevant data segment.
    //!
    //! \param numBytesToSkip Number of bytes to skip during deserialization
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus deserializeSkip(FwSizeType numBytesToSkip) override;

    //! \brief Get buffer capacity
    //!
    //! This method returns the total capacity of the buffer, which is the maximum
    //! amount of data that can be stored in the buffer. This is not the same as
    //! the current size, which indicates how much data is currently in the buffer.
    //!
    //! \return The capacity of the buffer in bytes
    Fw::Serializable::SizeType getCapacity() const override;

    //! \brief Get current buffer size
    //!
    //! This method returns the current size of the buffer, which indicates how
    //! much data is currently stored in the buffer. This may be less than or
    //! equal to the buffer's capacity.
    //!
    //! \return The current size of the buffer in bytes
    Fw::Serializable::SizeType getSize() const override;

    //! \brief Get remaining deserialization buffer size
    //!
    //! This method returns the amount of data that remains to be deserialized
    //! from the buffer. It indicates how much data is left starting from the
    //! current deserialization pointer to the end of the buffer.
    //!
    //! \return The remaining size of the deserialization buffer in bytes
    Fw::Serializable::SizeType getDeserializeSizeLeft() const override;

    //! \brief Get remaining serialization buffer size
    //!
    //! This method returns the amount of space available for serialization
    //! in the buffer. It indicates how much data can still be written to the
    //! buffer starting from the current serialization pointer to the end of
    //! the buffer's capacity.
    //!
    //! \return The remaining size of the serialization buffer in bytes
    Fw::Serializable::SizeType getSerializeSizeLeft() const override;

    //! \brief Set buffer contents from external source
    //!
    //! This method sets the contents of the buffer from an external source.
    //! It copies the specified number of bytes from the source pointer into
    //! the buffer and updates the buffer size accordingly.
    //!
    //! \param src Pointer to the external data source
    //! \param length Number of bytes to copy from the source
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus setBuff(const U8* src, Fw::Serializable::SizeType length) override;

    //! \brief Set buffer length manually
    //!
    //! This method manually sets the length of the buffer without modifying
    //! its contents. This can be used after filling the buffer with data through
    //! other means to indicate how much valid data is in the buffer.
    //!
    //! \param length The new length to set for the buffer
    //! \return SerializeStatus indicating the result of the operation
    Fw::SerializeStatus setBuffLen(Fw::Serializable::SizeType length) override;

  private:
    /**
     * Returns a wrap-advanced index into the store.
     * Inline hint for performance - called frequently in hot paths.
     * \param idx: index to advance and wrap.
     * \param amount: amount to advance
     * \return: new index value
     */
    inline FwSizeType advance_idx(FwSizeType idx, FwSizeType amount = 1) const;

    /**
     * Helper function for raw byte copying without endianness conversion.
     * Used for single bytes, byte arrays, and pre-serialized data.
     * \param buffer: pointer to source data
     * \param size: number of bytes to copy
     * \return: serialization status
     */
    Fw::SerializeStatus serializeRaw(const U8* const buffer, const FwSizeType size);

    /**
     * Helper function to check if there is sufficient space for serialization.
     * Validates that writing 'size' bytes at the current serialization index
     * will not exceed the buffer capacity. Inline hint for performance.
     * \param size: number of bytes to check space for
     * \return: serialization status (FW_SERIALIZE_OK or FW_SERIALIZE_NO_ROOM_LEFT)
     */
    inline Fw::SerializeStatus checkSerializeSpace(const FwSizeType size) const;

    //! Physical store backing this circular buffer
    U8* m_store;
    //! Size of the physical store
    FwSizeType m_store_size;
    //! Index into m_store of byte zero in the logical store.
    //! When memory is deallocated, this index moves forward and wraps around.
    FwSizeType m_head_idx;
    //! Allocated size (size of the logical store)
    FwSizeType m_allocated_size;
    //! Maximum allocated size
    FwSizeType m_high_water_mark;
    //! Deserialization index (offset from head for reading)
    FwSizeType m_deser_idx;
    //! Serialization index (offset from head for writing)
    FwSizeType m_ser_idx;
};
}  // End Namespace Types
#endif
