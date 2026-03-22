/**
 * Kiosk Bridge - SenseCAP Indicator RP2040
 * 
 * Receives rating from ESP32 via internal UART (Serial1)
 * Outputs JSON on Grove connector for Maker Pi
 * 
 * Grove Serial Output Format:
 *   {"rating":N}\n
 */

#include <Arduino.h>
#include <PacketSerial.h>

#define DEBUG 1

// Packet types (must match ESP32)
#define PKT_TYPE_TIMER_STATUS 0xC0
#define PKT_TYPE_RATING       0xC1

// Current rating
volatile uint8_t current_rating = 5;
volatile bool rating_changed = false;

PacketSerial myPacketSerial;

/************************ ESP32 Communication ****************************/

void onPacketReceived(const uint8_t *buffer, size_t size) {
#if DEBUG
  Serial.printf("<--- recv len:%d, type:0x%02X\n", size, buffer[0]);
  for (size_t i = 0; i < size; i++) {
    Serial.printf(" %02X", buffer[i]);
  }
  Serial.println();
#endif

  if (size < 1) return;
  
  // Handle rating packet
  if (buffer[0] == PKT_TYPE_RATING && size >= 2) {
    current_rating = buffer[1];
    rating_changed = true;
#if DEBUG
    Serial.printf("Rating: %d\n", current_rating);
#endif
  }
  
  // Also handle timer status for backwards compat
  if (buffer[0] == PKT_TYPE_TIMER_STATUS && size >= 4) {
#if DEBUG
    Serial.printf("Timer: running=%d, %d:%02d\n", 
                  buffer[1], buffer[2], buffer[3]);
#endif
  }
}

/************************ Setup ****************************/

void setup() {
  // USB Serial for debugging
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n=== Kiosk Bridge v1 ===");
  Serial.println("Receives rating from ESP32, outputs JSON on Grove");
  
  // Internal UART to ESP32
  Serial1.setRX(17);
  Serial1.setTX(16);
  Serial1.begin(115200);
  myPacketSerial.setStream(&Serial1);
  myPacketSerial.setPacketHandler(&onPacketReceived);
  Serial.println("ESP32 UART ready (RX:17, TX:16)");
  
  // Grove UART output (TX only on GP20)
  Serial2.setTX(20);
  Serial2.begin(9600);
  Serial.println("Grove Serial ready (TX:GP20, 9600 baud)");
  
  Serial.println("\nWaiting for ratings...");
}

/************************ Loop ****************************/

void loop() {
  myPacketSerial.update();
  
  // Send rating on Grove serial when changed
  if (rating_changed) {
    char buf[20];
    snprintf(buf, sizeof(buf), "{\"rating\":%d}\n", current_rating);
    Serial2.print(buf);
    Serial.printf("Sent to Grove: %s", buf);
    rating_changed = false;
  }
  
  delay(10);
}
