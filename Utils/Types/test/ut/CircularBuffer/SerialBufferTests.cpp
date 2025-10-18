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
