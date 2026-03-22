/**
 * Focus Timer Bridge - SenseCAP Indicator RP2040
 * 
 * Receives timer status from ESP32 via internal UART (Serial1)
 * Exposes I2C slave on Grove connector for external devices
 * 
 * I2C Protocol (address 0x42):
 *   Register 0x00: Status (0=stopped, 1=running)
 *   Register 0x01: Minutes remaining
 *   Register 0x02: Seconds remaining
 */

#include <Arduino.h>
#include <PacketSerial.h>
#include <Wire.h>

#define DEBUG 1

// I2C slave address
#define I2C_SLAVE_ADDR 0x42

// Packet type for timer status (matches ESP32)
#define PKT_TYPE_TIMER_STATUS 0xC0

// Timer state (received from ESP32)
volatile uint8_t timer_running = 0;
volatile uint8_t timer_minutes = 0;
volatile uint8_t timer_seconds = 0;

// I2C register being accessed
volatile uint8_t i2c_register = 0;

PacketSerial myPacketSerial;

/************************ I2C Slave Callbacks ****************************/

void i2c_receive(int numBytes) {
  if (Wire.available()) {
    i2c_register = Wire.read();
  }
}

void i2c_request() {
  uint8_t response = 0;
  
  switch (i2c_register) {
    case 0x00:  // Status
      response = timer_running;
      break;
    case 0x01:  // Minutes
      response = timer_minutes;
      break;
    case 0x02:  // Seconds
      response = timer_seconds;
      break;
    default:
      response = 0xFF;
      break;
  }
  
  Wire.write(response);
  
#if DEBUG
  Serial.printf("I2C request: reg=0x%02X, val=%d\n", i2c_register, response);
#endif
}

/************************ ESP32 Communication ****************************/

void onPacketReceived(const uint8_t *buffer, size_t size) {
#if DEBUG
  Serial.printf("<--- recv len:%d, type:0x%02X ", size, buffer[0]);
  for (int i = 0; i < size; i++) {
    Serial.printf("0x%02X ", buffer[i]);
  }
  Serial.println("");
#endif

  if (size < 1) {
    return;
  }
  
  switch (buffer[0]) {
    case PKT_TYPE_TIMER_STATUS:
      if (size >= 4) {
        timer_running = buffer[1];
        timer_minutes = buffer[2];
        timer_seconds = buffer[3];
        
#if DEBUG
        Serial.printf("Timer: running=%d, %d:%02d\n", 
                      timer_running, timer_minutes, timer_seconds);
#endif
      }
      break;
      
    default:
      break;
  }
}

/************************ Setup ****************************/

void setup() {
  // USB Serial for debugging
  Serial.begin(115200);
  
  // Wait for serial (optional, for debugging)
  delay(1000);
  Serial.println("\n=== Focus Timer Bridge ===");
  Serial.println("SenseCAP Indicator RP2040");
  Serial.println("I2C Slave at 0x42 on Grove");
  
  // Internal UART to ESP32
  Serial1.setRX(17);
  Serial1.setTX(16);
  Serial1.begin(115200);
  myPacketSerial.setStream(&Serial1);
  myPacketSerial.setPacketHandler(&onPacketReceived);
  Serial.println("ESP32 UART ready (TX:16, RX:17)");
  
  // I2C Slave on Grove connector (GP0=SDA, GP1=SCL)
  Wire.setSDA(0);
  Wire.setSCL(1);
  Wire.begin(I2C_SLAVE_ADDR);
  Wire.onReceive(i2c_receive);
  Wire.onRequest(i2c_request);
  Serial.printf("I2C Slave ready at 0x%02X (SDA:GP0, SCL:GP1)\n", I2C_SLAVE_ADDR);
  
  Serial.println("\nWaiting for timer status from ESP32...");
}

/************************ Loop ****************************/

void loop() {
  // Process incoming packets from ESP32
  myPacketSerial.update();
  
  // Check for overflow
  if (myPacketSerial.overflow()) {
    Serial.println("PacketSerial overflow!");
  }
  
  delay(10);
}
