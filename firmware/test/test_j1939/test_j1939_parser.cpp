/**
 * @file test_j1939_parser.cpp
 * @brief Unit tests for J1939 parser module
 * 
 * Tests PGN extraction, parameter decoding, and Transport Protocol handling.
 */

#include <unity.h>
#include "j1939_parser.h"
#include <string.h>
#include <math.h>

// Test helper for float comparison
#define FLOAT_EPSILON 0.01f
#define ASSERT_FLOAT_NEAR(expected, actual) \
    TEST_ASSERT_FLOAT_WITHIN(FLOAT_EPSILON, expected, actual)

/*===========================================================================*/
/*                        PGN EXTRACTION TESTS                              */
/*===========================================================================*/

void test_extract_pgn_pdu2_format(void) {
    // PGN 65262 (0xFEEE) - ET1 - PDU2 format (PF >= 240)
    // CAN ID: 0x18FEEE00 = Priority 6, PGN 65262, SA 0x00
    uint32_t can_id = 0x18FEEE00;
    uint32_t pgn = j1939_extract_pgn(can_id);
    
    TEST_ASSERT_EQUAL_UINT32(65262, pgn);
}

void test_extract_pgn_pdu1_format(void) {
    // PGN 61444 (0xF004) - EEC1 - PDU1 format (PF < 240)
    // CAN ID: 0x0CF00400 = Priority 3, PGN 61444, SA 0x00
    // Note: For PDU1, PS (0x04) is destination address, not part of PGN
    // So PGN should be 0xF004 = 61444
    uint32_t can_id = 0x0CF00400;
    uint32_t pgn = j1939_extract_pgn(can_id);
    
    // For PDU1 (PF < 240), PGN = DP:PF:00
    // PF = 0xF0 = 240 which is NOT < 240, so this is actually PDU2!
    // Let's use a true PDU1 example: PGN 0 (TC1) with PF = 0
    // Actually, 0xF0 = 240, which is PDU2 boundary
    // PGN 61444 = 0xF004 has PF = 0xF0 = 240, so it's PDU2
    TEST_ASSERT_EQUAL_UINT32(61444, pgn);
}

void test_extract_pgn_true_pdu1(void) {
    // Request PGN 59904 (0xEA00) - PDU1 format
    // CAN ID with destination 0x00 from source 0xF9
    // 0x18EA00F9 = Priority 6, PGN 59904, DA 0x00, SA 0xF9
    uint32_t can_id = 0x18EA00F9;
    uint32_t pgn = j1939_extract_pgn(can_id);
    
    // PF = 0xEA = 234 < 240, so PDU1
    // PGN should be 0xEA00 = 59904 (PS is destination, not part of PGN)
    TEST_ASSERT_EQUAL_UINT32(59904, pgn);
}

void test_extract_source_address(void) {
    uint32_t can_id = 0x18FEEE00;  // SA = 0x00
    TEST_ASSERT_EQUAL_UINT8(0x00, j1939_extract_source_address(can_id));
    
    can_id = 0x18FEEE03;  // SA = 0x03 (transmission)
    TEST_ASSERT_EQUAL_UINT8(0x03, j1939_extract_source_address(can_id));
    
    can_id = 0x0CF004F9;  // SA = 0xF9 (diagnostic tool)
    TEST_ASSERT_EQUAL_UINT8(0xF9, j1939_extract_source_address(can_id));
}

void test_extract_priority(void) {
    uint32_t can_id = 0x18FEEE00;  // Priority 6 (bits 26-28 = 0b110)
    TEST_ASSERT_EQUAL_UINT8(6, j1939_extract_priority(can_id));
    
    can_id = 0x0CF00400;  // Priority 3 (bits 26-28 = 0b011)
    TEST_ASSERT_EQUAL_UINT8(3, j1939_extract_priority(can_id));
}

void test_build_can_id(void) {
    // Build CAN ID for PGN 65262, SA 0x00, Priority 6
    uint32_t can_id = j1939_build_can_id(65262, 0x00, 6);
    
    // Should reconstruct to same PGN
    TEST_ASSERT_EQUAL_UINT32(65262, j1939_extract_pgn(can_id));
    TEST_ASSERT_EQUAL_UINT8(0x00, j1939_extract_source_address(can_id));
    TEST_ASSERT_EQUAL_UINT8(6, j1939_extract_priority(can_id));
}

/*===========================================================================*/
/*                        ENGINE SPEED DECODING TESTS                       */
/*===========================================================================*/

void test_decode_engine_speed_normal(void) {
    // EEC1 data with engine speed 2000 RPM
    // 2000 RPM / 0.125 = 16000 = 0x3E80
    uint8_t data[8] = {0x00, 0x7D, 0x7D, 0x80, 0x3E, 0x00, 0x00, 0x00};
    
    float rpm = j1939_decode_engine_speed(data);
    ASSERT_FLOAT_NEAR(2000.0f, rpm);
}

void test_decode_engine_speed_idle(void) {
    // Engine speed 650 RPM
    // 650 / 0.125 = 5200 = 0x1450
    uint8_t data[8] = {0x00, 0x7D, 0x7D, 0x50, 0x14, 0x00, 0x00, 0x00};
    
    float rpm = j1939_decode_engine_speed(data);
    ASSERT_FLOAT_NEAR(650.0f, rpm);
}

void test_decode_engine_speed_not_available(void) {
    // Not available value (0xFFFF)
    uint8_t data[8] = {0x00, 0x7D, 0x7D, 0xFF, 0xFF, 0x00, 0x00, 0x00};
    
    float rpm = j1939_decode_engine_speed(data);
    TEST_ASSERT_EQUAL_FLOAT(-1.0f, rpm);
}

/*===========================================================================*/
/*                        COOLANT TEMPERATURE DECODING TESTS                */
/*===========================================================================*/

void test_decode_coolant_temp_normal(void) {
    // ET1 data with coolant temp 100°C
    // 100 + 40 = 140 = 0x8C
    uint8_t data[8] = {0x8C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float temp = j1939_decode_coolant_temp(data);
    ASSERT_FLOAT_NEAR(100.0f, temp);
}

void test_decode_coolant_temp_cold(void) {
    // Coolant temp -20°C
    // -20 + 40 = 20 = 0x14
    uint8_t data[8] = {0x14, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float temp = j1939_decode_coolant_temp(data);
    ASSERT_FLOAT_NEAR(-20.0f, temp);
}

void test_decode_coolant_temp_not_available(void) {
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float temp = j1939_decode_coolant_temp(data);
    TEST_ASSERT_EQUAL_FLOAT(-9999.0f, temp);
}

/*===========================================================================*/
/*                        VEHICLE SPEED DECODING TESTS                      */
/*===========================================================================*/

void test_decode_vehicle_speed_highway(void) {
    // CCVS data with speed 105 km/h
    // 105 * 256 = 26880 = 0x6900
    uint8_t data[8] = {0xFF, 0x00, 0x69, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float speed = j1939_decode_vehicle_speed(data);
    ASSERT_FLOAT_NEAR(105.0f, speed);
}

void test_decode_vehicle_speed_city(void) {
    // Speed 50 km/h
    // 50 * 256 = 12800 = 0x3200
    uint8_t data[8] = {0xFF, 0x00, 0x32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float speed = j1939_decode_vehicle_speed(data);
    ASSERT_FLOAT_NEAR(50.0f, speed);
}

void test_decode_vehicle_speed_stopped(void) {
    // Speed 0 km/h
    uint8_t data[8] = {0xFF, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float speed = j1939_decode_vehicle_speed(data);
    ASSERT_FLOAT_NEAR(0.0f, speed);
}

/*===========================================================================*/
/*                        OIL PRESSURE DECODING TESTS                       */
/*===========================================================================*/

void test_decode_oil_pressure_normal(void) {
    // EFLP1 with oil pressure 400 kPa
    // 400 / 4 = 100 = 0x64
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0x64, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float pressure = j1939_decode_oil_pressure(data);
    ASSERT_FLOAT_NEAR(400.0f, pressure);
}

void test_decode_oil_pressure_low(void) {
    // Low oil pressure 100 kPa (warning level)
    // 100 / 4 = 25 = 0x19
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0x19, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float pressure = j1939_decode_oil_pressure(data);
    ASSERT_FLOAT_NEAR(100.0f, pressure);
}

/*===========================================================================*/
/*                        BOOST PRESSURE DECODING TESTS                     */
/*===========================================================================*/

void test_decode_boost_pressure_loaded(void) {
    // IC1 with boost 200 kPa
    // 200 / 2 = 100 = 0x64
    uint8_t data[8] = {0xFF, 0x64, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float pressure = j1939_decode_boost_pressure(data);
    ASSERT_FLOAT_NEAR(200.0f, pressure);
}

void test_decode_boost_pressure_idle(void) {
    // Boost at idle ~100 kPa (atmospheric)
    // 100 / 2 = 50 = 0x32
    uint8_t data[8] = {0xFF, 0x32, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float pressure = j1939_decode_boost_pressure(data);
    ASSERT_FLOAT_NEAR(100.0f, pressure);
}

/*===========================================================================*/
/*                        BATTERY VOLTAGE DECODING TESTS                    */
/*===========================================================================*/

void test_decode_battery_voltage_running(void) {
    // VEP1 with battery voltage 14.2V
    // 14.2 / 0.05 = 284 = 0x011C
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x1C, 0x01};
    
    float voltage = j1939_decode_battery_voltage(data);
    ASSERT_FLOAT_NEAR(14.2f, voltage);
}

void test_decode_battery_voltage_parked(void) {
    // Battery voltage 12.6V
    // 12.6 / 0.05 = 252 = 0x00FC
    uint8_t data[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFC, 0x00};
    
    float voltage = j1939_decode_battery_voltage(data);
    ASSERT_FLOAT_NEAR(12.6f, voltage);
}

/*===========================================================================*/
/*                        FUEL LEVEL DECODING TESTS                         */
/*===========================================================================*/

void test_decode_fuel_level_half(void) {
    // DD with fuel level 50%
    // 50 / 0.4 = 125 = 0x7D
    uint8_t data[8] = {0xFF, 0x7D, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float level = j1939_decode_fuel_level(data);
    ASSERT_FLOAT_NEAR(50.0f, level);
}

void test_decode_fuel_level_full(void) {
    // Fuel level 100%
    // 100 / 0.4 = 250 = 0xFA
    uint8_t data[8] = {0xFF, 0xFA, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float level = j1939_decode_fuel_level(data);
    ASSERT_FLOAT_NEAR(100.0f, level);
}

/*===========================================================================*/
/*                        CURRENT GEAR DECODING TESTS                       */
/*===========================================================================*/

void test_decode_current_gear_8th(void) {
    // ETC2 with gear 8
    // 8 + 125 = 133 = 0x85
    uint8_t data[8] = {0x85, 0xFF, 0xFF, 0x85, 0xFF, 0xFF, 0xFF, 0xFF};
    
    int8_t gear = j1939_decode_current_gear(data);
    TEST_ASSERT_EQUAL_INT8(8, gear);
}

void test_decode_current_gear_neutral(void) {
    // Neutral = 0
    // 0 + 125 = 125 = 0x7D
    uint8_t data[8] = {0x7D, 0xFF, 0xFF, 0x7D, 0xFF, 0xFF, 0xFF, 0xFF};
    
    int8_t gear = j1939_decode_current_gear(data);
    TEST_ASSERT_EQUAL_INT8(0, gear);
}

void test_decode_current_gear_reverse(void) {
    // Reverse = -1
    // -1 + 125 = 124 = 0x7C
    uint8_t data[8] = {0x7C, 0xFF, 0xFF, 0x7C, 0xFF, 0xFF, 0xFF, 0xFF};
    
    int8_t gear = j1939_decode_current_gear(data);
    TEST_ASSERT_EQUAL_INT8(-1, gear);
}

/*===========================================================================*/
/*                        ENGINE HOURS DECODING TESTS                       */
/*===========================================================================*/

void test_decode_engine_hours(void) {
    // HOURS with 50000 hours
    // 50000 / 0.05 = 1000000 = 0x000F4240
    uint8_t data[8] = {0x40, 0x42, 0x0F, 0x00, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float hours = j1939_decode_engine_hours(data);
    ASSERT_FLOAT_NEAR(50000.0f, hours);
}

/*===========================================================================*/
/*                        FUEL RATE DECODING TESTS                          */
/*===========================================================================*/

void test_decode_fuel_rate_highway(void) {
    // LFE with fuel rate 35 L/h
    // 35 / 0.05 = 700 = 0x02BC
    uint8_t data[8] = {0xBC, 0x02, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    
    float rate = j1939_decode_fuel_rate(data);
    ASSERT_FLOAT_NEAR(35.0f, rate);
}

/*===========================================================================*/
/*                        DM1 PARSING TESTS                                 */
/*===========================================================================*/

void test_parse_dm1_single_fault(void) {
    // DM1 with single fault: SPN 110 (coolant temp), FMI 0 (high severe)
    // SPN 110 = 0x00006E, FMI 0
    uint8_t data[8] = {
        0x00, 0x10,  // Lamp status (MIL on)
        0x6E, 0x00,  // SPN low 16 bits
        0x00,        // SPN high bits (0) | FMI (0)
        0x01,        // Occurrence count
        0xFF, 0xFF
    };
    
    j1939_lamp_status_t lamps;
    j1939_dtc_t dtcs[4];
    
    uint8_t count = j1939_parse_dm1(data, 8, &lamps, dtcs, 4);
    
    TEST_ASSERT_EQUAL_UINT8(1, count);
    TEST_ASSERT_TRUE(lamps.malfunction_lamp);
    TEST_ASSERT_EQUAL_UINT32(110, dtcs[0].spn);
    TEST_ASSERT_EQUAL_UINT8(0, dtcs[0].fmi);
    TEST_ASSERT_EQUAL_UINT8(1, dtcs[0].oc);
}

void test_parse_dm1_no_faults(void) {
    // DM1 with no active faults
    uint8_t data[8] = {
        0x00, 0x00,  // All lamps off
        0x00, 0x00, 0x00, 0x00,  // No DTC (SPN=0, FMI=0)
        0xFF, 0xFF
    };
    
    j1939_lamp_status_t lamps;
    j1939_dtc_t dtcs[4];
    
    uint8_t count = j1939_parse_dm1(data, 8, &lamps, dtcs, 4);
    
    TEST_ASSERT_EQUAL_UINT8(0, count);
    TEST_ASSERT_FALSE(lamps.malfunction_lamp);
}

/*===========================================================================*/
/*                        FRAME PARSING TESTS                               */
/*===========================================================================*/

void test_parse_frame_basic(void) {
    uint32_t can_id = 0x18FEEE00;
    uint8_t data[8] = {0x8C, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    j1939_message_t msg;
    
    bool result = j1939_parse_frame(can_id, data, 8, 1000, &msg);
    
    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_UINT32(65262, msg.pgn);
    TEST_ASSERT_EQUAL_UINT8(0x00, msg.source_address);
    TEST_ASSERT_EQUAL_UINT8(6, msg.priority);
    TEST_ASSERT_EQUAL_UINT32(1000, msg.timestamp_ms);
    TEST_ASSERT_EQUAL_UINT8(8, msg.data_length);
}

void test_parse_frame_null_data(void) {
    j1939_message_t msg;
    bool result = j1939_parse_frame(0x18FEEE00, NULL, 8, 1000, &msg);
    TEST_ASSERT_FALSE(result);
}

void test_parse_frame_invalid_length(void) {
    uint8_t data[8] = {0};
    j1939_message_t msg;
    
    bool result = j1939_parse_frame(0x18FEEE00, data, 0, 1000, &msg);
    TEST_ASSERT_FALSE(result);
    
    result = j1939_parse_frame(0x18FEEE00, data, 9, 1000, &msg);
    TEST_ASSERT_FALSE(result);
}

/*===========================================================================*/
/*                        PARSER CONTEXT TESTS                              */
/*===========================================================================*/

void test_parser_init(void) {
    j1939_parser_context_t ctx;
    j1939_parser_init(&ctx);
    
    TEST_ASSERT_EQUAL_UINT32(0, ctx.messages_received);
    TEST_ASSERT_EQUAL_UINT32(0, ctx.parse_errors);
    
    for (int i = 0; i < J1939_MAX_ACTIVE_TP; i++) {
        TEST_ASSERT_EQUAL(TP_STATE_IDLE, ctx.tp_sessions[i].state);
    }
}

/*===========================================================================*/
/*                        VALIDITY CHECK TESTS                              */
/*===========================================================================*/

void test_is_valid_8(void) {
    TEST_ASSERT_TRUE(j1939_is_valid_8(0));
    TEST_ASSERT_TRUE(j1939_is_valid_8(100));
    TEST_ASSERT_TRUE(j1939_is_valid_8(253));
    TEST_ASSERT_FALSE(j1939_is_valid_8(0xFE));  // Error
    TEST_ASSERT_FALSE(j1939_is_valid_8(0xFF));  // Not available
}

void test_is_valid_16(void) {
    TEST_ASSERT_TRUE(j1939_is_valid_16(0));
    TEST_ASSERT_TRUE(j1939_is_valid_16(10000));
    TEST_ASSERT_TRUE(j1939_is_valid_16(0xFDFF));
    TEST_ASSERT_FALSE(j1939_is_valid_16(0xFE00));  // Error base
    TEST_ASSERT_FALSE(j1939_is_valid_16(0xFFFF));  // Not available
}

/*===========================================================================*/
/*                        TEST RUNNER                                       */
/*===========================================================================*/

void setUp(void) {
    // Called before each test
}

void tearDown(void) {
    // Called after each test
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    
    // PGN extraction tests
    RUN_TEST(test_extract_pgn_pdu2_format);
    RUN_TEST(test_extract_pgn_pdu1_format);
    RUN_TEST(test_extract_pgn_true_pdu1);
    RUN_TEST(test_extract_source_address);
    RUN_TEST(test_extract_priority);
    RUN_TEST(test_build_can_id);
    
    // Engine speed tests
    RUN_TEST(test_decode_engine_speed_normal);
    RUN_TEST(test_decode_engine_speed_idle);
    RUN_TEST(test_decode_engine_speed_not_available);
    
    // Coolant temperature tests
    RUN_TEST(test_decode_coolant_temp_normal);
    RUN_TEST(test_decode_coolant_temp_cold);
    RUN_TEST(test_decode_coolant_temp_not_available);
    
    // Vehicle speed tests
    RUN_TEST(test_decode_vehicle_speed_highway);
    RUN_TEST(test_decode_vehicle_speed_city);
    RUN_TEST(test_decode_vehicle_speed_stopped);
    
    // Oil pressure tests
    RUN_TEST(test_decode_oil_pressure_normal);
    RUN_TEST(test_decode_oil_pressure_low);
    
    // Boost pressure tests
    RUN_TEST(test_decode_boost_pressure_loaded);
    RUN_TEST(test_decode_boost_pressure_idle);
    
    // Battery voltage tests
    RUN_TEST(test_decode_battery_voltage_running);
    RUN_TEST(test_decode_battery_voltage_parked);
    
    // Fuel level tests
    RUN_TEST(test_decode_fuel_level_half);
    RUN_TEST(test_decode_fuel_level_full);
    
    // Gear tests
    RUN_TEST(test_decode_current_gear_8th);
    RUN_TEST(test_decode_current_gear_neutral);
    RUN_TEST(test_decode_current_gear_reverse);
    
    // Engine hours tests
    RUN_TEST(test_decode_engine_hours);
    
    // Fuel rate tests
    RUN_TEST(test_decode_fuel_rate_highway);
    
    // DM1 parsing tests
    RUN_TEST(test_parse_dm1_single_fault);
    RUN_TEST(test_parse_dm1_no_faults);
    
    // Frame parsing tests
    RUN_TEST(test_parse_frame_basic);
    RUN_TEST(test_parse_frame_null_data);
    RUN_TEST(test_parse_frame_invalid_length);
    
    // Parser context tests
    RUN_TEST(test_parser_init);
    
    // Validity check tests
    RUN_TEST(test_is_valid_8);
    RUN_TEST(test_is_valid_16);
    
    return UNITY_END();
}
