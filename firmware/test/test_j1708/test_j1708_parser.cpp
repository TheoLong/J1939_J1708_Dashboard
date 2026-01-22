/**
 * @file test_j1708_parser.cpp
 * @brief Unit tests for J1708/J1587 parser module
 */

#include <unity.h>
#include "j1708_parser.h"
#include <string.h>

/*===========================================================================*/
/*                        CHECKSUM TESTS                                    */
/*===========================================================================*/

void test_validate_checksum_valid(void) {
    // Valid message: MID 128, PID 190, data 0x50, 0x14 (650 RPM), checksum
    // Sum: 128 + 190 + 0x50 + 0x14 + checksum = 0 mod 256
    // 128 + 190 + 80 + 20 = 418 = 0x1A2 -> need 256 - 0xA2 = 0x5E
    uint8_t data[] = {128, 190, 0x50, 0x14, 0x5E};
    
    TEST_ASSERT_TRUE(j1708_validate_checksum(data, 5));
}

void test_validate_checksum_invalid(void) {
    uint8_t data[] = {128, 190, 0x50, 0x14, 0x00};  // Wrong checksum
    
    TEST_ASSERT_FALSE(j1708_validate_checksum(data, 5));
}

void test_calculate_checksum(void) {
    uint8_t data[] = {128, 190, 0x50, 0x14};
    
    uint8_t checksum = j1708_calculate_checksum(data, 4);
    
    // Verify: sum of all bytes + checksum should be 0
    uint8_t sum = 0;
    for (int i = 0; i < 4; i++) sum += data[i];
    sum += checksum;
    
    TEST_ASSERT_EQUAL_UINT8(0, sum);
}

/*===========================================================================*/
/*                        PID LENGTH TESTS                                  */
/*===========================================================================*/

void test_get_pid_length_fixed(void) {
    TEST_ASSERT_EQUAL_UINT8(1, j1708_get_pid_length(84));   // Road speed
    TEST_ASSERT_EQUAL_UINT8(1, j1708_get_pid_length(110));  // Coolant temp
    TEST_ASSERT_EQUAL_UINT8(2, j1708_get_pid_length(190));  // Engine speed
    TEST_ASSERT_EQUAL_UINT8(4, j1708_get_pid_length(247));  // Engine hours
}

void test_get_pid_length_variable(void) {
    TEST_ASSERT_EQUAL_UINT8(0, j1708_get_pid_length(194));  // Diagnostic codes
    TEST_ASSERT_EQUAL_UINT8(0, j1708_get_pid_length(234));  // Component ID
}

/*===========================================================================*/
/*                        PARAMETER DECODING TESTS                          */
/*===========================================================================*/

void test_decode_road_speed(void) {
    // PID 84: 0.5 mph/bit, convert to km/h
    // 120 * 0.5 mph = 60 mph = 96.56 km/h
    uint8_t data[] = {120};
    
    float speed = j1708_decode_road_speed(data, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 96.56f, speed);
}

void test_decode_engine_rpm(void) {
    // PID 190: 0.25 rpm/bit, 16-bit little-endian
    // 2600 * 0.25 = 650 RPM
    uint8_t data[] = {0x28, 0x0A};  // 0x0A28 = 2600
    
    float rpm = j1708_decode_engine_rpm(data, 2);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 650.0f, rpm);
}

void test_decode_coolant_temp(void) {
    // PID 110: 1°F/bit, convert to °C
    // 212°F = 100°C
    uint8_t data[] = {212};
    
    float temp = j1708_decode_coolant_temp(data, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 100.0f, temp);
}

void test_decode_oil_pressure(void) {
    // PID 100: 4 kPa/bit
    // 100 * 4 = 400 kPa
    uint8_t data[] = {100};
    
    float pressure = j1708_decode_oil_pressure(data, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 400.0f, pressure);
}

void test_decode_battery_voltage(void) {
    // PID 168: 0.05V/bit
    // 252 * 0.05 = 12.6V
    uint8_t data[] = {252};
    
    float voltage = j1708_decode_battery_voltage(data, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 12.6f, voltage);
}

void test_decode_fuel_level(void) {
    // PID 96: 0.5%/bit
    // 100 * 0.5 = 50%
    uint8_t data[] = {100};
    
    float level = j1708_decode_fuel_level(data, 1);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 50.0f, level);
}

/*===========================================================================*/
/*                        MESSAGE PARSING TESTS                             */
/*===========================================================================*/

void test_parse_message_simple(void) {
    // MID 128, PID 110 (coolant temp), value 212°F, checksum
    uint8_t data[] = {128, 110, 212};
    uint8_t checksum = j1708_calculate_checksum(data, 3);
    uint8_t msg_data[] = {128, 110, 212, checksum};
    
    j1708_message_t msg;
    bool result = j1708_parse_message(msg_data, 4, &msg);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(128, msg.mid);
    TEST_ASSERT_TRUE(msg.checksum_valid);
    TEST_ASSERT_EQUAL_UINT8(1, msg.param_count);
    TEST_ASSERT_EQUAL_UINT8(110, msg.params[0].pid);
    TEST_ASSERT_EQUAL_UINT8(212, msg.params[0].data[0]);
}

void test_parse_message_multiple_params(void) {
    // MID 128, PID 110 (temp), PID 100 (oil pressure)
    uint8_t data[] = {128, 110, 200, 100, 75};
    uint8_t checksum = j1708_calculate_checksum(data, 5);
    uint8_t msg_data[] = {128, 110, 200, 100, 75, checksum};
    
    j1708_message_t msg;
    bool result = j1708_parse_message(msg_data, 6, &msg);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(2, msg.param_count);
    TEST_ASSERT_EQUAL_UINT8(110, msg.params[0].pid);
    TEST_ASSERT_EQUAL_UINT8(100, msg.params[1].pid);
}

void test_parse_message_16bit_param(void) {
    // MID 128, PID 190 (engine speed, 2 bytes)
    uint8_t data[] = {128, 190, 0x28, 0x0A};  // 650 RPM
    uint8_t checksum = j1708_calculate_checksum(data, 4);
    uint8_t msg_data[] = {128, 190, 0x28, 0x0A, checksum};
    
    j1708_message_t msg;
    bool result = j1708_parse_message(msg_data, 5, &msg);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT8(1, msg.param_count);
    TEST_ASSERT_EQUAL_UINT8(190, msg.params[0].pid);
    TEST_ASSERT_EQUAL_UINT8(2, msg.params[0].data_length);
}

void test_parse_message_bad_checksum(void) {
    uint8_t msg_data[] = {128, 110, 212, 0x00};  // Bad checksum
    
    j1708_message_t msg;
    bool result = j1708_parse_message(msg_data, 4, &msg);
    
    TEST_ASSERT_FALSE(result);
}

void test_parse_message_too_short(void) {
    uint8_t msg_data[] = {128};  // Too short
    
    j1708_message_t msg;
    bool result = j1708_parse_message(msg_data, 1, &msg);
    
    TEST_ASSERT_FALSE(result);
}

/*===========================================================================*/
/*                        PARSER CONTEXT TESTS                              */
/*===========================================================================*/

void test_parser_init(void) {
    j1708_parser_context_t ctx;
    j1708_parser_init(&ctx);
    
    TEST_ASSERT_EQUAL(J1708_RX_IDLE, ctx.state);
    TEST_ASSERT_EQUAL_UINT32(0, ctx.messages_received);
    TEST_ASSERT_EQUAL_UINT32(0, ctx.checksum_errors);
}

/*===========================================================================*/
/*                        FAULT CODE PARSING TESTS                          */
/*===========================================================================*/

void test_parse_fault_codes(void) {
    // PID 194 format: PID/SID, FMI pairs
    uint8_t data[] = {
        110, 0x03,  // PID 110 (coolant temp), FMI 3 (voltage high)
        100, 0x04   // PID 100 (oil pressure), FMI 4 (voltage low)
    };
    
    j1587_fault_code_t faults[4];
    uint8_t count = j1708_parse_fault_codes(128, data, 4, faults, 4);
    
    TEST_ASSERT_EQUAL_UINT8(2, count);
    TEST_ASSERT_EQUAL_UINT8(110, faults[0].pid_or_sid);
    TEST_ASSERT_EQUAL_UINT8(3, faults[0].fmi);
    TEST_ASSERT_EQUAL_UINT8(100, faults[1].pid_or_sid);
    TEST_ASSERT_EQUAL_UINT8(4, faults[1].fmi);
}

/*===========================================================================*/
/*                        STRING LOOKUP TESTS                               */
/*===========================================================================*/

void test_get_mid_name(void) {
    TEST_ASSERT_EQUAL_STRING("Engine #1", j1708_get_mid_name(128));
    TEST_ASSERT_EQUAL_STRING("Transmission", j1708_get_mid_name(130));
    TEST_ASSERT_EQUAL_STRING("Tractor ABS", j1708_get_mid_name(172));
    TEST_ASSERT_EQUAL_STRING("Unknown", j1708_get_mid_name(99));
}

void test_get_pid_name(void) {
    TEST_ASSERT_EQUAL_STRING("Road Speed", j1708_get_pid_name(84));
    TEST_ASSERT_EQUAL_STRING("Engine Speed", j1708_get_pid_name(190));
    TEST_ASSERT_EQUAL_STRING("Coolant Temperature", j1708_get_pid_name(110));
    TEST_ASSERT_EQUAL_STRING("Unknown", j1708_get_pid_name(99));
}

/*===========================================================================*/
/*                        TEST RUNNER                                       */
/*===========================================================================*/

void setUp(void) {}
void tearDown(void) {}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // Checksum tests
    RUN_TEST(test_validate_checksum_valid);
    RUN_TEST(test_validate_checksum_invalid);
    RUN_TEST(test_calculate_checksum);
    
    // PID length tests
    RUN_TEST(test_get_pid_length_fixed);
    RUN_TEST(test_get_pid_length_variable);
    
    // Parameter decoding tests
    RUN_TEST(test_decode_road_speed);
    RUN_TEST(test_decode_engine_rpm);
    RUN_TEST(test_decode_coolant_temp);
    RUN_TEST(test_decode_oil_pressure);
    RUN_TEST(test_decode_battery_voltage);
    RUN_TEST(test_decode_fuel_level);
    
    // Message parsing tests
    RUN_TEST(test_parse_message_simple);
    RUN_TEST(test_parse_message_multiple_params);
    RUN_TEST(test_parse_message_16bit_param);
    RUN_TEST(test_parse_message_bad_checksum);
    RUN_TEST(test_parse_message_too_short);
    
    // Parser context tests
    RUN_TEST(test_parser_init);
    
    // Fault code tests
    RUN_TEST(test_parse_fault_codes);
    
    // String lookup tests
    RUN_TEST(test_get_mid_name);
    RUN_TEST(test_get_pid_name);
    
    return UNITY_END();
}
