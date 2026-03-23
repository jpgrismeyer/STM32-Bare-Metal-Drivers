#include <ESP32SPISlave.h>

// La nueva librería usa este espacio de nombres
ESP32SPISlave slave;

static const int MOSI_PIN = 23;
static const int MISO_PIN = 19;
static const int SCK_PIN  = 18;
static const int SS_PIN   = 5;

uint8_t dataBuff[255];
uint8_t board_id[] = "ESP32_DEV";

#define NACK 0xA5
#define ACK  0xF5

// Comandos
#define COMMAND_LED_CTRL      0x50
#define COMMAND_SENSOR_READ   0x51
#define COMMAND_PRINT         0x53
#define COMMAND_ID_READ       0x54

void setup() {
  Serial.begin(115200);
  
  // En la versión 3.x, el begin es así:
  // (Puerto, SCK, MISO, MOSI, SS)
  slave.begin(SPI2_HOST, SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
  
  Serial.println("ESP32 Slave (Official Core 3.x) Ready");
}

void loop() {
  uint8_t rx_byte = 0;
  uint8_t tx_byte = NACK; // Valor por defecto para enviar

  // La función 'transfer' es la clave. Envía y recibe AL MISMO TIEMPO.
  // Espera a que el CS baje y suba (una transacción).
  if (slave.transfer(&tx_byte, &rx_byte, 1)) {
    
    uint8_t command = rx_byte;
    Serial.printf("Comando: 0x%02X\n", command);

    if (command == COMMAND_LED_CTRL) {
      uint8_t params_rx[2];
      uint8_t params_tx[2] = {ACK, ACK};
      // Recibimos los 2 bytes de parámetros
      slave.transfer(params_tx, params_rx, 2);
      Serial.printf("LED CTRL: Pin %d, Val %d\n", params_rx[0], params_rx[1]);

    } else if (command == COMMAND_ID_READ) {
      // Para enviar el ID, necesitamos otra transacción
      // El maestro debe pedir bytes después del comando
      slave.transfer(board_id, NULL, sizeof(board_id));
      Serial.println("ID enviado al Maestro");

    } else if (command == COMMAND_PRINT) {
      uint8_t len = 0;
      slave.transfer(NULL, &len, 1); // Recibir largo
      slave.transfer(NULL, dataBuff, len); // Recibir cadena
      dataBuff[len] = '\0';
      Serial.printf("Texto: %s\n", dataBuff);
    }
  }
}