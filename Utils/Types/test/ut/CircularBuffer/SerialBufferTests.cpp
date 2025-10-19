/**
 * SerialBufferTests.cpp:
 *
 * Unit tests for CircularBuffer's SerialBufferBase interface implementation.
 * Tests the new serialization/deserialization methods added to CircularBuffer.
 *
 * Created: Oct 2025
 */

#include <gtest/gtest.h>
#include <Fw/Test/UnitTest.hpp>
#include <Utils/Types/CircularBuffer.hpp>
#include <Fw/Types/ExternalString.hpp>
#include <cstring>

#define TEST_BUFFER_SIZE 1024

class SerialBufferInterfaceTest : public ::testing::Test {
  protected:
    void SetUp() override {
        buffer.setup(storage, TEST_BUFFER_SIZE);
    }

    U8 storage[TEST_BUFFER_SIZE];
    Types::CircularBuffer buffer;
};

// Test 1: Basic primitive type serialization/deserialization
TEST_F(SerialBufferInterfaceTest, PrimitiveTypesSerialization) {
    // Serialize various types
    ASSERT_EQ(buffer.serializeFrom(static_cast<U8>(0x42)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U16>(0x1234)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0xDEADBEEF)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U64>(0x0123456789ABCDEFULL)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<F32>(3.14f)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<F64>(2.71828)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(true), Fw::FW_SERIALIZE_OK);

    // Deserialize and verify
    U8 u8_val;
    U16 u16_val;
    U32 u32_val;
    U64 u64_val;
    F32 f32_val;
    F64 f64_val;
    bool bool_val;

    ASSERT_EQ(buffer.deserializeTo(u8_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(u8_val, 0x42);
    
    ASSERT_EQ(buffer.deserializeTo(u16_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(u16_val, 0x1234);
    
    ASSERT_EQ(buffer.deserializeTo(u32_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(u32_val, 0xDEADBEEF);
    
    ASSERT_EQ(buffer.deserializeTo(u64_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(u64_val, 0x0123456789ABCDEFULL);
    
    ASSERT_EQ(buffer.deserializeTo(f32_val), Fw::FW_SERIALIZE_OK);
    ASSERT_FLOAT_EQ(f32_val, 3.14f);
    
    ASSERT_EQ(buffer.deserializeTo(f64_val), Fw::FW_SERIALIZE_OK);
    ASSERT_DOUBLE_EQ(f64_val, 2.71828);
    
    ASSERT_EQ(buffer.deserializeTo(bool_val), Fw::FW_SERIALIZE_OK);
    ASSERT_TRUE(bool_val);
}

// Test 2: Endianness handling
TEST_F(SerialBufferInterfaceTest, EndiannessHandling) {
    U32 big_endian_val = 0x12345678;
    U32 little_endian_val = 0xABCDEF01;
    
    // Serialize with different endianness
    ASSERT_EQ(buffer.serializeFrom(big_endian_val, Fw::Endianness::BIG), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(little_endian_val, Fw::Endianness::LITTLE), Fw::FW_SERIALIZE_OK);
    
    // Deserialize with matching endianness
    U32 result1, result2;
    ASSERT_EQ(buffer.deserializeTo(result1, Fw::Endianness::BIG), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result1, big_endian_val);
    
    ASSERT_EQ(buffer.deserializeTo(result2, Fw::Endianness::LITTLE), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result2, little_endian_val);
}

// Test 3: Buffer serialization with length
TEST_F(SerialBufferInterfaceTest, BufferSerializationWithLength) {
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    
    // Serialize with length prefix
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::INCLUDE_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Deserialize with length
    U8 result[10];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::INCLUDE_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result_len, sizeof(test_data));
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 4: Buffer serialization without length
TEST_F(SerialBufferInterfaceTest, BufferSerializationWithoutLength) {
    U8 test_data[] = {0xAA, 0xBB, 0xCC};
    
    // Serialize without length prefix
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Deserialize without length (must know size)
    U8 result[3];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 5: resetDeser() functionality
TEST_F(SerialBufferInterfaceTest, ResetDeserialization) {
    U32 val1 = 0x11111111;
    U32 val2 = 0x22222222;
    
    ASSERT_EQ(buffer.serializeFrom(val1), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(val2), Fw::FW_SERIALIZE_OK);
    
    // Read first value
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, val1);
    
    // Reset and read first value again
    buffer.resetDeser();
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, val1);
}

// Test 6: deserializeSkip() functionality
TEST_F(SerialBufferInterfaceTest, DeserializeSkip) {
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x11111111)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x22222222)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x33333333)), Fw::FW_SERIALIZE_OK);
    
    // Skip first value
    ASSERT_EQ(buffer.deserializeSkip(sizeof(U32)), Fw::FW_SERIALIZE_OK);
    
    // Read second value
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, 0x22222222);
}

// Test 7: getDeserializeSizeLeft()
TEST_F(SerialBufferInterfaceTest, DeserializeSizeLeft) {
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x12345678)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), sizeof(U32));
    
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 0);
}

// Test 8: Buffer overflow during serialization
TEST_F(SerialBufferInterfaceTest, SerializationOverflow) {
    // Fill buffer almost to capacity
    U8 large_data[TEST_BUFFER_SIZE - 10];
    memset(large_data, 0xFF, sizeof(large_data));
    ASSERT_EQ(buffer.serializeFrom(large_data, sizeof(large_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Try to serialize more data than available space
    U8 overflow_data[20];
    ASSERT_EQ(buffer.serializeFrom(overflow_data, sizeof(overflow_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_NO_ROOM_LEFT);
}

// Test 9: Deserialization buffer empty
TEST_F(SerialBufferInterfaceTest, DeserializationEmpty) {
    U32 result;
    // Try to deserialize from empty buffer
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_DESERIALIZE_BUFFER_EMPTY);
}

// Test 10: Circular buffer with rotate and deserialize
TEST_F(SerialBufferInterfaceTest, RotateWithDeserialize) {
    // Serialize some data
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0xAAAAAAAA)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0xBBBBBBBB)), Fw::FW_SERIALIZE_OK);
    
    // Deserialize first value
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, 0xAAAAAAAA);
    
    // Rotate to free space (consume first value)
    ASSERT_EQ(buffer.rotate(sizeof(U32)), Fw::FW_SERIALIZE_OK);
    
    // Reset deserialization and read second value
    buffer.resetDeser();
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, 0xBBBBBBBB);
}

// Test 11: SerializeBufferBase interoperability
TEST_F(SerialBufferInterfaceTest, SerializeBufferInterop) {
    // Create an ExternalSerializeBuffer
    U8 ext_storage[64];
    Fw::ExternalSerializeBuffer ext_buffer(ext_storage, sizeof(ext_storage));
    
    // Serialize data into external buffer
    ASSERT_EQ(ext_buffer.serializeFrom(static_cast<U32>(0x12345678)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(ext_buffer.serializeFrom(static_cast<U16>(0xABCD)), Fw::FW_SERIALIZE_OK);
    
    // Serialize the external buffer into circular buffer
    ASSERT_EQ(buffer.serializeFrom(ext_buffer), Fw::FW_SERIALIZE_OK);
    
    // Deserialize back into another external buffer
    Fw::ExternalSerializeBuffer result_buffer(ext_storage, sizeof(ext_storage));
    ASSERT_EQ(buffer.deserializeTo(result_buffer), Fw::FW_SERIALIZE_OK);
    
    // Verify contents
    U32 u32_result;
    U16 u16_result;
    ASSERT_EQ(result_buffer.deserializeTo(u32_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(u32_result, 0x12345678);
    ASSERT_EQ(result_buffer.deserializeTo(u16_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(u16_result, 0xABCD);
}

// Test 12: moveDeserToOffset()
TEST_F(SerialBufferInterfaceTest, MoveDeserToOffset) {
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x11111111)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x22222222)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x33333333)), Fw::FW_SERIALIZE_OK);
    
    // Move to second U32
    ASSERT_EQ(buffer.moveDeserToOffset(sizeof(U32)), Fw::FW_SERIALIZE_OK);
    
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, 0x22222222);
}

// Test 13: Signed integer types
TEST_F(SerialBufferInterfaceTest, SignedIntegerTypes) {
    I8 i8_val = -42;
    I16 i16_val = -1234;
    I32 i32_val = -987654321;
    I64 i64_val = -0x123456789ABCDEFLL;
    
    ASSERT_EQ(buffer.serializeFrom(i8_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(i16_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(i32_val), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(i64_val), Fw::FW_SERIALIZE_OK);
    
    I8 i8_result;
    I16 i16_result;
    I32 i32_result;
    I64 i64_result;
    
    ASSERT_EQ(buffer.deserializeTo(i8_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(i8_result, i8_val);
    ASSERT_EQ(buffer.deserializeTo(i16_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(i16_result, i16_val);
    ASSERT_EQ(buffer.deserializeTo(i32_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(i32_result, i32_val);
    ASSERT_EQ(buffer.deserializeTo(i64_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(i64_result, i64_val);
}

// Test 14: Boolean serialization edge cases
TEST_F(SerialBufferInterfaceTest, BooleanEdgeCases) {
    ASSERT_EQ(buffer.serializeFrom(true), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(false), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(true), Fw::FW_SERIALIZE_OK);
    
    bool b1, b2, b3;
    ASSERT_EQ(buffer.deserializeTo(b1), Fw::FW_SERIALIZE_OK);
    ASSERT_TRUE(b1);
    ASSERT_EQ(buffer.deserializeTo(b2), Fw::FW_SERIALIZE_OK);
    ASSERT_FALSE(b2);
    ASSERT_EQ(buffer.deserializeTo(b3), Fw::FW_SERIALIZE_OK);
    ASSERT_TRUE(b3);
}

// Test 15: getSize() and getCapacity()
TEST_F(SerialBufferInterfaceTest, SizeAndCapacity) {
    ASSERT_EQ(buffer.getCapacity(), TEST_BUFFER_SIZE);
    ASSERT_EQ(buffer.getSize(), 0);
    
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x12345678)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(U32));
    
    ASSERT_EQ(buffer.serializeFrom(static_cast<U16>(0xABCD)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(U32) + sizeof(U16));
}

// Test 16: Interaction between peek and deserialize
TEST_F(SerialBufferInterfaceTest, PeekAndDeserializeInteraction) {
    U32 val1 = 0xDEADBEEF;
    U32 val2 = 0xCAFEBABE;
    
    ASSERT_EQ(buffer.serializeFrom(val1), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(val2), Fw::FW_SERIALIZE_OK);
    
    // Peek at first value (non-destructive, from buffer head)
    U32 peek_result;
    ASSERT_EQ(buffer.peek(peek_result, 0), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(peek_result, val1);
    
    // Deserialize reads from beginning and advances m_deser_idx
    U32 deser_result;
    ASSERT_EQ(buffer.deserializeTo(deser_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(deser_result, val1);
    
    // Peek still peeks from buffer head (m_head_idx), not m_deser_idx
    // To peek at second value, use offset
    ASSERT_EQ(buffer.peek(peek_result, sizeof(U32)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(peek_result, val2);
    
    // Deserialize now reads second value
    ASSERT_EQ(buffer.deserializeTo(deser_result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(deser_result, val2);
}

// Test 17: resetSer() clears everything
TEST_F(SerialBufferInterfaceTest, ResetSerialization) {
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x12345678)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(U32));
    
    // Reset serialization (clears buffer)
    buffer.resetSer();
    ASSERT_EQ(buffer.getSize(), 0);
    
    // Should be able to serialize again
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0xABCDEF01)), Fw::FW_SERIALIZE_OK);
    
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, 0xABCDEF01);
}

// Test 18: serializeSkip allocates space
TEST_F(SerialBufferInterfaceTest, SerializeSkip) {
    // Skip some bytes (allocate space without writing)
    ASSERT_EQ(buffer.serializeSkip(10), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), 10);
    
    // Serialize actual data
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x12345678)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), 10 + sizeof(U32));
}

// Test 19: Deserialization past end fails
TEST_F(SerialBufferInterfaceTest, DeserializePastEnd) {
    ASSERT_EQ(buffer.serializeFrom(static_cast<U8>(0x42)), Fw::FW_SERIALIZE_OK);
    
    U8 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    
    // Try to deserialize again (buffer empty)
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_DESERIALIZE_BUFFER_EMPTY);
}

// Test 20: Multiple resets work correctly
TEST_F(SerialBufferInterfaceTest, MultipleResets) {
    U32 val = 0xDEADBEEF;
    ASSERT_EQ(buffer.serializeFrom(val), Fw::FW_SERIALIZE_OK);
    
    U32 result;
    
    // Read, reset, read again
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, val);
    
    buffer.resetDeser();
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, val);
    
    buffer.resetDeser();
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, val);
}

// Test 21: setBuff() replaces buffer contents
TEST_F(SerialBufferInterfaceTest, SetBuff) {
    U8 source_data[] = {0x01, 0x02, 0x03, 0x04};
    
    ASSERT_EQ(buffer.setBuff(source_data, sizeof(source_data)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(source_data));
    
    // Verify we can deserialize the data
    U8 result[4];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, source_data, sizeof(source_data)), 0);
}

// Test 22: setBuffLen() sets size without data
TEST_F(SerialBufferInterfaceTest, SetBuffLen) {
    ASSERT_EQ(buffer.setBuffLen(100), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), 100);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 100);
    
    // Try to set length larger than capacity
    ASSERT_EQ(buffer.setBuffLen(TEST_BUFFER_SIZE + 100), Fw::FW_SERIALIZE_NO_ROOM_LEFT);
}

// Test 23: getSerializeSizeLeft()
TEST_F(SerialBufferInterfaceTest, SerializeSizeLeft) {
    ASSERT_EQ(buffer.getSerializeSizeLeft(), TEST_BUFFER_SIZE);
    
    ASSERT_EQ(buffer.serializeFrom(static_cast<U32>(0x12345678)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSerializeSizeLeft(), TEST_BUFFER_SIZE - sizeof(U32));
}

// Test 24: Wrap-around deserialization for U16
TEST_F(SerialBufferInterfaceTest, WrapAroundU16) {
    // Fill buffer almost to the end, leaving only 1 byte before wrap
    U8 filler[TEST_BUFFER_SIZE - 1];
    memset(filler, 0xAA, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Rotate to consume the filler data
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Now serialize a U16 - it will wrap around (1 byte at end, 1 byte at beginning)
    U16 test_val = 0xBEEF;
    ASSERT_EQ(buffer.serializeFrom(test_val), Fw::FW_SERIALIZE_OK);
    
    // Reset deserialization and read it back
    buffer.resetDeser();
    U16 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, test_val);
}

// Test 25: Wrap-around deserialization for U32
TEST_F(SerialBufferInterfaceTest, WrapAroundU32) {
    // Fill buffer to leave only 2 bytes before wrap
    U8 filler[TEST_BUFFER_SIZE - 2];
    memset(filler, 0xBB, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Rotate to consume the filler
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Serialize a U32 - it will wrap (2 bytes at end, 2 bytes at beginning)
    U32 test_val = 0xDEADBEEF;
    ASSERT_EQ(buffer.serializeFrom(test_val), Fw::FW_SERIALIZE_OK);
    
    // Read it back
    buffer.resetDeser();
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, test_val);
}

// Test 26: Wrap-around deserialization for U64
TEST_F(SerialBufferInterfaceTest, WrapAroundU64) {
    // Fill buffer to leave only 3 bytes before wrap
    U8 filler[TEST_BUFFER_SIZE - 3];
    memset(filler, 0xCC, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Rotate to consume the filler
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Serialize a U64 - it will wrap (3 bytes at end, 5 bytes at beginning)
    U64 test_val = 0x0123456789ABCDEFULL;
    ASSERT_EQ(buffer.serializeFrom(test_val), Fw::FW_SERIALIZE_OK);
    
    // Read it back
    buffer.resetDeser();
    U64 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, test_val);
}

// Test 27: Wrap-around with multiple values
TEST_F(SerialBufferInterfaceTest, WrapAroundMultipleValues) {
    // Position buffer so multiple values will wrap
    U8 filler[TEST_BUFFER_SIZE - 10];
    memset(filler, 0xDD, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Rotate to consume the filler
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Serialize multiple values that will wrap
    U16 val1 = 0x1234;
    U32 val2 = 0xABCDEF01;
    U16 val3 = 0x5678;
    
    ASSERT_EQ(buffer.serializeFrom(val1), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(val2), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(val3), Fw::FW_SERIALIZE_OK);
    
    // Read them back
    buffer.resetDeser();
    U16 result1;
    U32 result2;
    U16 result3;
    
    ASSERT_EQ(buffer.deserializeTo(result1), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result1, val1);
    ASSERT_EQ(buffer.deserializeTo(result2), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result2, val2);
    ASSERT_EQ(buffer.deserializeTo(result3), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result3, val3);
}

// Test 28: Wrap-around with F32
TEST_F(SerialBufferInterfaceTest, WrapAroundF32) {
    // Position to wrap a float
    U8 filler[TEST_BUFFER_SIZE - 2];
    memset(filler, 0xEE, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    F32 test_val = 3.14159f;
    ASSERT_EQ(buffer.serializeFrom(test_val), Fw::FW_SERIALIZE_OK);
    
    buffer.resetDeser();
    F32 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_FLOAT_EQ(result, test_val);
}

// Test 29: Wrap-around with F64
TEST_F(SerialBufferInterfaceTest, WrapAroundF64) {
    // Position to wrap a double
    U8 filler[TEST_BUFFER_SIZE - 5];
    memset(filler, 0xFF, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    F64 test_val = 2.718281828459045;
    ASSERT_EQ(buffer.serializeFrom(test_val), Fw::FW_SERIALIZE_OK);
    
    buffer.resetDeser();
    F64 result;
    ASSERT_EQ(buffer.deserializeTo(result), Fw::FW_SERIALIZE_OK);
    ASSERT_DOUBLE_EQ(result, test_val);
}

// Test 30: Wrap-around with buffer array
TEST_F(SerialBufferInterfaceTest, WrapAroundBufferArray) {
    // Position so buffer array wraps
    U8 filler[TEST_BUFFER_SIZE - 8];
    memset(filler, 0x11, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Serialize a buffer that will wrap
    U8 test_data[] = {0xCA, 0xFE, 0xBA, 0xBE, 0xDE, 0xAD};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Read it back
    buffer.resetDeser();
    U8 result[6];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 31: Wrap-around with endianness (BIG)
TEST_F(SerialBufferInterfaceTest, WrapAroundEndiannessB) {
    // Position to wrap
    U8 filler[TEST_BUFFER_SIZE - 3];
    memset(filler, 0x22, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Serialize with BIG endianness across wrap boundary
    U32 test_val = 0x12345678;
    ASSERT_EQ(buffer.serializeFrom(test_val, Fw::Endianness::BIG), Fw::FW_SERIALIZE_OK);
    
    buffer.resetDeser();
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result, Fw::Endianness::BIG), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, test_val);
}

// Test 32: Wrap-around with endianness (LITTLE)
TEST_F(SerialBufferInterfaceTest, WrapAroundEndiannessLittle) {
    // Position to wrap
    U8 filler[TEST_BUFFER_SIZE - 3];
    memset(filler, 0x33, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Serialize with LITTLE endianness across wrap boundary
    U32 test_val = 0x9ABCDEF0;
    ASSERT_EQ(buffer.serializeFrom(test_val, Fw::Endianness::LITTLE), Fw::FW_SERIALIZE_OK);
    
    buffer.resetDeser();
    U32 result;
    ASSERT_EQ(buffer.deserializeTo(result, Fw::Endianness::LITTLE), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(result, test_val);
}

// Test 33: Basic copyRaw functionality
TEST_F(SerialBufferInterfaceTest, CopyRawBasic) {
    // Fill the buffer with test data
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create a destination buffer
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Copy data using copyRaw
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify the data was copied correctly
    ASSERT_EQ(dest_buffer.getSize(), sizeof(test_data));
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), test_data, sizeof(test_data)), 0);
    
    // Verify the deserialization index was updated (copyRaw advances the pointer)
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 0u);
}

// Test 34: copyRaw resets destination buffer
TEST_F(SerialBufferInterfaceTest, CopyRawResetsDestination) {
    // Fill source buffer
    U8 test_data[] = {0xAA, 0xBB, 0xCC};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create destination buffer with existing data
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    U8 existing_data[] = {0x11, 0x22};
    ASSERT_EQ(dest_buffer.serializeFrom(existing_data, sizeof(existing_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(dest_buffer.getSize(), 2u);
    
    // copyRaw should reset the destination buffer
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify destination only contains the new data (old data was cleared)
    ASSERT_EQ(dest_buffer.getSize(), sizeof(test_data));
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), test_data, sizeof(test_data)), 0);
}

// Test 35: copyRaw with wrap-around in circular buffer
TEST_F(SerialBufferInterfaceTest, CopyRawWrapAround) {
    // Fill buffer to near capacity, then rotate to create wrap-around scenario
    U8 filler[TEST_BUFFER_SIZE - 10];
    memset(filler, 0xAA, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Rotate to consume most data, leaving space at the end
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Now add data that will wrap around
    U8 test_data[15];
    for (U32 i = 0; i < sizeof(test_data); i++) {
        test_data[i] = static_cast<U8>(i);
    }
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create destination buffer
    U8 dest_storage[20] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Copy the wrapped data
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify data was copied correctly despite wrap-around
    ASSERT_EQ(dest_buffer.getSize(), sizeof(test_data));
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), test_data, sizeof(test_data)), 0);
}

// Test 36: copyRaw with insufficient source data
TEST_F(SerialBufferInterfaceTest, CopyRawInsufficientSource) {
    // Add some test data
    U8 test_data[] = {0x01, 0x02, 0x03};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create a destination buffer
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Try to copy more data than available
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(test_data) + 1), Fw::FW_DESERIALIZE_BUFFER_EMPTY);
    
    // Verify no data was copied (destination should be empty after reset)
    ASSERT_EQ(dest_buffer.getSize(), 0u);
}

// Test 37: copyRaw with insufficient destination space
TEST_F(SerialBufferInterfaceTest, CopyRawInsufficientDest) {
    // Add some test data
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create a destination buffer that's too small
    U8 dest_storage[3] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Try to copy more data than destination can hold
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_NO_ROOM_LEFT);
}

// Test 38: copyRaw advances deserialization pointer
TEST_F(SerialBufferInterfaceTest, CopyRawAdvancesPointer) {
    // Add multiple chunks of data
    U8 chunk1[] = {0x01, 0x02, 0x03};
    U8 chunk2[] = {0x04, 0x05, 0x06, 0x07};
    ASSERT_EQ(buffer.serializeFrom(chunk1, sizeof(chunk1), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.serializeFrom(chunk2, sizeof(chunk2), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Copy first chunk
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(chunk1)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), chunk1, sizeof(chunk1)), 0);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), sizeof(chunk2));
    
    // Copy second chunk (pointer should have advanced)
    ASSERT_EQ(buffer.copyRaw(dest_buffer, sizeof(chunk2)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), chunk2, sizeof(chunk2)), 0);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 0u);
}

// Test 39: Basic copyRawOffset functionality
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetBasic) {
    // Fill the buffer with test data
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create a destination buffer
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Copy data using copyRawOffset
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, 3), Fw::FW_SERIALIZE_OK);
    
    // Verify the data was copied correctly
    ASSERT_EQ(dest_buffer.getSize(), 3u);
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), test_data, 3), 0);
    
    // Verify the deserialization index WAS updated (both methods advance the pointer)
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 2u);
}

// Test 40: copyRawOffset appends to destination
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetAppendsToDestination) {
    // Fill source buffer
    U8 test_data[] = {0xAA, 0xBB, 0xCC};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create destination buffer with existing data
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    U8 existing_data[] = {0x11, 0x22};
    ASSERT_EQ(dest_buffer.serializeFrom(existing_data, sizeof(existing_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(dest_buffer.getSize(), 2u);
    
    // copyRawOffset should append to the destination buffer (not reset)
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify destination contains both old and new data
    ASSERT_EQ(dest_buffer.getSize(), 2u + sizeof(test_data));
    U8 expected[] = {0x11, 0x22, 0xAA, 0xBB, 0xCC};
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), expected, sizeof(expected)), 0);
}

// Test 41: copyRawOffset with wrap-around
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetWrapAround) {
    // Create wrap-around scenario
    U8 filler[TEST_BUFFER_SIZE - 8];
    memset(filler, 0xFF, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Add data that wraps around
    U8 test_data[12];
    for (U32 i = 0; i < sizeof(test_data); i++) {
        test_data[i] = static_cast<U8>(i + 0x10);
    }
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create destination buffer
    U8 dest_storage[20] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Copy the wrapped data using copyRawOffset
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify data was copied correctly
    ASSERT_EQ(dest_buffer.getSize(), sizeof(test_data));
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), test_data, sizeof(test_data)), 0);
}

// Test 42: copyRawOffset advances deserialization pointer
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetAdvancesPointer) {
    // Add data
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // First copyRawOffset
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, 3), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(dest_buffer.getSize(), 3u);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 4u);
    
    // Second copyRawOffset (should continue from where first left off)
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, 2), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(dest_buffer.getSize(), 5u);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 2u);
    
    // Verify the combined data is sequential
    U8 expected[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), expected, sizeof(expected)), 0);
}

// Test 43: Mixing copyRaw and copyRawOffset
TEST_F(SerialBufferInterfaceTest, MixingCopyRawAndCopyRawOffset) {
    // Add data
    U8 test_data[] = {0x10, 0x20, 0x30, 0x40, 0x50, 0x60};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Use copyRawOffset to append first chunk
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, 2), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(dest_buffer.getSize(), 2u);
    U8 expected1[] = {0x10, 0x20};
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), expected1, 2), 0);
    
    // Use copyRaw which resets destination
    ASSERT_EQ(buffer.copyRaw(dest_buffer, 3), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(dest_buffer.getSize(), 3u);
    U8 expected2[] = {0x30, 0x40, 0x50};
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), expected2, 3), 0);
    
    // Verify pointer advanced through both operations
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 1u);
}

// Test 44: copyRawOffset with insufficient destination space
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetInsufficientDest) {
    // Add test data
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    // Create a destination buffer that's too small
    U8 dest_storage[3] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Fill destination partially
    U8 existing[] = {0xAA};
    ASSERT_EQ(dest_buffer.serializeFrom(existing, sizeof(existing), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    
    // Try to copy more data than remaining space (capacity=3, used=1, need=5)
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, sizeof(test_data)), Fw::FW_SERIALIZE_NO_ROOM_LEFT);
    
    // Verify deserialization pointer was not advanced on error
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), sizeof(test_data));
}

// Test 45: copyRaw and copyRawOffset with resetDeser
TEST_F(SerialBufferInterfaceTest, CopyWithResetDeser) {
    // Add test data
    U8 test_data[] = {0xA1, 0xA2, 0xA3, 0xA4};
    ASSERT_EQ(buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    
    U8 dest_storage[10] = {0};
    Fw::ExternalSerializeBuffer dest_buffer(dest_storage, sizeof(dest_storage));
    
    // Copy some data
    ASSERT_EQ(buffer.copyRawOffset(dest_buffer, 2), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 2u);
    
    // Reset deserialization pointer
    buffer.resetDeser();
    ASSERT_EQ(buffer.getDeserializeSizeLeft(), 4u);
    
    // Copy again from the beginning
    ASSERT_EQ(buffer.copyRaw(dest_buffer, 4), Fw::FW_SERIALIZE_OK);
    
    // Verify we copied from the beginning
    ASSERT_EQ(dest_buffer.getSize(), 4u);
    ASSERT_EQ(memcmp(dest_buffer.getBuffAddr(), test_data, 4), 0);
}

// Test 46: copyRaw from linear buffer to CircularBuffer - basic
TEST_F(SerialBufferInterfaceTest, CopyRawFromLinearToCircular) {
    // Create a linear source buffer with test data
    U8 source_storage[20];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 test_data[] = {0x01, 0x02, 0x03, 0x04, 0x05};
    ASSERT_EQ(source_buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    
    // Reset deserialization to start
    source_buffer.resetDeser();
    
    // Copy from linear buffer to circular buffer using copyRaw
    ASSERT_EQ(source_buffer.copyRaw(buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify data was copied correctly
    ASSERT_EQ(buffer.getSize(), sizeof(test_data));
    
    // Read back and verify
    U8 result[5];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 47: copyRawOffset from linear buffer to CircularBuffer - basic
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetFromLinearToCircular) {
    // Create a linear source buffer with test data
    U8 source_storage[20];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 test_data[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE};
    ASSERT_EQ(source_buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    
    // Reset deserialization to start
    source_buffer.resetDeser();
    
    // Copy from linear buffer to circular buffer using copyRawOffset
    ASSERT_EQ(source_buffer.copyRawOffset(buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify data was copied
    ASSERT_EQ(buffer.getSize(), sizeof(test_data));
    
    // Verify the data matches
    U8 result[5];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 48: copyRaw from linear buffer to CircularBuffer with wrap-around
TEST_F(SerialBufferInterfaceTest, CopyRawFromLinearWithWrapAround) {
    // Fill circular buffer almost to capacity, then rotate to create wrap-around scenario
    U8 filler[TEST_BUFFER_SIZE - 15];
    memset(filler, 0xFF, sizeof(filler));
    ASSERT_EQ(buffer.serializeFrom(filler, sizeof(filler), Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.rotate(sizeof(filler)), Fw::FW_SERIALIZE_OK);
    
    // Create linear source buffer with test data
    U8 source_storage[30];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 test_data[20];
    for (U32 i = 0; i < sizeof(test_data); i++) {
        test_data[i] = static_cast<U8>(i + 0x10);
    }
    ASSERT_EQ(source_buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    source_buffer.resetDeser();
    
    // Copy from linear to circular - this will wrap around in the circular buffer
    ASSERT_EQ(source_buffer.copyRaw(buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify data was copied correctly despite wrap-around
    ASSERT_EQ(buffer.getSize(), sizeof(test_data));
    U8 result[20];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 49: copyRawOffset appends to CircularBuffer
TEST_F(SerialBufferInterfaceTest, CopyRawOffsetAppendsToCircular) {
    // Add some initial data to circular buffer
    U8 initial_data[] = {0x11, 0x22};
    ASSERT_EQ(buffer.serializeFrom(initial_data, sizeof(initial_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    
    // Create linear source buffer
    U8 source_storage[10];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 test_data[] = {0xAA, 0xBB, 0xCC};
    ASSERT_EQ(source_buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    source_buffer.resetDeser();
    
    // copyRawOffset should append to circular buffer
    ASSERT_EQ(source_buffer.copyRawOffset(buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify both old and new data present
    ASSERT_EQ(buffer.getSize(), sizeof(initial_data) + sizeof(test_data));
    U8 result[5];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    U8 expected[] = {0x11, 0x22, 0xAA, 0xBB, 0xCC};
    ASSERT_EQ(memcmp(result, expected, sizeof(expected)), 0);
}

// Test 50: copyRaw resets CircularBuffer
TEST_F(SerialBufferInterfaceTest, CopyRawResetsCircular) {
    // Add some initial data to circular buffer
    U8 initial_data[] = {0x11, 0x22};
    ASSERT_EQ(buffer.serializeFrom(initial_data, sizeof(initial_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), 2u);
    
    // Create linear source buffer
    U8 source_storage[10];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 test_data[] = {0xAA, 0xBB, 0xCC};
    ASSERT_EQ(source_buffer.serializeFrom(test_data, sizeof(test_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    source_buffer.resetDeser();
    
    // copyRaw should reset circular buffer (discard old data)
    ASSERT_EQ(source_buffer.copyRaw(buffer, sizeof(test_data)), Fw::FW_SERIALIZE_OK);
    
    // Verify only new data present
    ASSERT_EQ(buffer.getSize(), sizeof(test_data));
    U8 result[3];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, test_data, sizeof(test_data)), 0);
}

// Test 51: Multiple copyRawOffset calls from linear to circular
TEST_F(SerialBufferInterfaceTest, MultipleCopyRawOffsetFromLinear) {
    // Create linear source buffer with multiple chunks
    U8 source_storage[20];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 chunk1[] = {0x01, 0x02, 0x03};
    U8 chunk2[] = {0x04, 0x05};
    U8 chunk3[] = {0x06, 0x07, 0x08, 0x09};
    ASSERT_EQ(source_buffer.serializeFrom(chunk1, sizeof(chunk1), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(source_buffer.serializeFrom(chunk2, sizeof(chunk2), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(source_buffer.serializeFrom(chunk3, sizeof(chunk3), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    source_buffer.resetDeser();
    
    // Copy chunks one at a time using copyRawOffset
    ASSERT_EQ(source_buffer.copyRawOffset(buffer, sizeof(chunk1)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(chunk1));
    
    ASSERT_EQ(source_buffer.copyRawOffset(buffer, sizeof(chunk2)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(chunk1) + sizeof(chunk2));
    
    ASSERT_EQ(source_buffer.copyRawOffset(buffer, sizeof(chunk3)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(buffer.getSize(), sizeof(chunk1) + sizeof(chunk2) + sizeof(chunk3));
    
    // Verify all data copied sequentially
    U8 result[9];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    U8 expected[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09};
    ASSERT_EQ(memcmp(result, expected, sizeof(expected)), 0);
}

// Test 52: copyRaw from linear to circular - insufficient destination space
TEST_F(SerialBufferInterfaceTest, CopyRawFromLinearInsufficientSpace) {
    // Create linear source with large data
    U8 source_storage[TEST_BUFFER_SIZE + 100];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 large_data[TEST_BUFFER_SIZE + 50];
    memset(large_data, 0xAA, sizeof(large_data));
    ASSERT_EQ(source_buffer.serializeFrom(large_data, sizeof(large_data), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    source_buffer.resetDeser();
    
    // Try to copy more than circular buffer capacity
    ASSERT_EQ(source_buffer.copyRaw(buffer, sizeof(large_data)), Fw::FW_SERIALIZE_NO_ROOM_LEFT);
    
    // Verify circular buffer is empty (no partial copy)
    ASSERT_EQ(buffer.getSize(), 0u);
}

// Test 53: copyRaw advances source deserialization pointer
TEST_F(SerialBufferInterfaceTest, CopyRawAdvancesSourcePointer) {
    // Create linear source with multiple values
    U8 source_storage[20];
    Fw::ExternalSerializeBuffer source_buffer(source_storage, sizeof(source_storage));
    U8 data1[] = {0x01, 0x02, 0x03};
    U8 data2[] = {0x04, 0x05, 0x06, 0x07};
    ASSERT_EQ(source_buffer.serializeFrom(data1, sizeof(data1), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(source_buffer.serializeFrom(data2, sizeof(data2), Fw::Serialization::OMIT_LENGTH),
              Fw::FW_SERIALIZE_OK);
    source_buffer.resetDeser();
    
    // Copy first chunk
    ASSERT_EQ(source_buffer.copyRaw(buffer, sizeof(data1)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(source_buffer.getDeserializeSizeLeft(), sizeof(data2));
    
    // Copy second chunk (source pointer should have advanced)
    ASSERT_EQ(source_buffer.copyRaw(buffer, sizeof(data2)), Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(source_buffer.getDeserializeSizeLeft(), 0u);
    
    // Verify second chunk was copied (buffer was reset by second copyRaw)
    U8 result[4];
    FwSizeType result_len = sizeof(result);
    ASSERT_EQ(buffer.deserializeTo(result, result_len, Fw::Serialization::OMIT_LENGTH), 
              Fw::FW_SERIALIZE_OK);
    ASSERT_EQ(memcmp(result, data2, sizeof(data2)), 0);
}

