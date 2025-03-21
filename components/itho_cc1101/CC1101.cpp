/*
   Author: Klusjesman, modified bij supersjimmie for Arduino/ESP8266
*/

#include "CC1101.h"

// default constructor
CC1101::CC1101()
{
  SPI.begin();
  pinMode(SS, OUTPUT);
} // CC1101

// default destructor
CC1101::~CC1101()
{
} //~CC1101

/***********************/
// SPI helper functions select() and deselect()
inline void CC1101::select(void)
{
  digitalWrite(SS, LOW);
}

inline void CC1101::deselect(void)
{
  digitalWrite(SS, HIGH);
}

void CC1101::spi_waitMiso()
{
  while (digitalRead(MISO) == HIGH)
    yield();
}

void CC1101::init()
{
  reset();
}

void CC1101::reset()
{
  deselect();
  delayMicroseconds(5);
  select();
  delayMicroseconds(10);
  deselect();
  delayMicroseconds(45);
  select();

  spi_waitMiso();
  SPI.transfer(CC1101_SRES);
  delay(10);
  spi_waitMiso();
  deselect();
}

uint8_t CC1101::writeCommand(uint8_t command)
{
  uint8_t result;

  select();
  spi_waitMiso();
  result = SPI.transfer(command);
  deselect();

  return result;
}

void CC1101::writeRegister(uint8_t address, uint8_t data)
{
  select();
  spi_waitMiso();
  SPI.transfer(address);
  SPI.transfer(data);
  deselect();
}

uint8_t CC1101::readRegister(uint8_t address)
{
  uint8_t val;

  select();
  spi_waitMiso();
  SPI.transfer(address);
  val = SPI.transfer(0);
  deselect();

  return val;
}

uint8_t CC1101::readRegisterMedian3(uint8_t address)
{
  uint8_t val, val1, val2, val3;

  select();
  spi_waitMiso();
  SPI.transfer(address);
  val1 = SPI.transfer(0);
  SPI.transfer(address);
  val2 = SPI.transfer(0);
  SPI.transfer(address);
  val3 = SPI.transfer(0);
  deselect();
  // reverse sort (largest in val1) because this is te expected order for TX_BUFFER
  if (val3 > val2)
  {
    val = val3; // Swap(val3,val2)
    val3 = val2;
    val2 = val;
  }
  if (val2 > val1)
  {
    val = val2; // Swap(val2,val1)
    val2 = val1, val1 = val;
  }
  if (val3 > val2)
  {
    val = val3; // Swap(val3,val2)
    val3 = val2, val2 = val;
  }

  return val2;
}

/* Known SPI/26MHz synchronization bug (see CC1101 errata)
  This issue affects the following registers: SPI status byte (fields STATE and FIFO_BYTES_AVAILABLE),
  FREQEST or RSSI while the receiver is active, MARCSTATE at any time other than an IDLE radio state,
  RXBYTES when receiving or TXBYTES when transmitting, and WORTIME1/WORTIME0 at any time.*/
// uint8_t CC1101::readRegisterWithSyncProblem(uint8_t address, uint8_t registerType)
uint8_t /* ICACHE_RAM_ATTR */ CC1101::readRegisterWithSyncProblem(uint8_t address, uint8_t registerType)
{
  uint8_t value1, value2;
  value1 = readRegister(address | registerType);
  value2 = readRegister(address | registerType);

  if (value1 == value2)
    return value1;

  // if two consecutive reads gives us the same result then we know we are ok
  do
  {
    value2 = value1;
    value1 = readRegister(address | registerType);
  } while (value1 != value2);

  return value1;
}

// registerType = CC1101_CONFIG_REGISTER or CC1101_STATUS_REGISTER
uint8_t CC1101::readRegister(uint8_t address, uint8_t registerType)
{
  switch (address)
  {
  case CC1101_FREQEST:
  case CC1101_MARCSTATE:
  case CC1101_RXBYTES:
  case CC1101_TXBYTES:
  case CC1101_WORTIME1:
  case CC1101_WORTIME0:
    return readRegisterWithSyncProblem(address, registerType);

  default:
    return readRegister(address | registerType);
  }
}

void CC1101::writeBurstRegister(const uint8_t address, const uint8_t *data, const uint8_t length)
{
  uint8_t i;

  select();
  spi_waitMiso();
  SPI.transfer(address | CC1101_WRITE_BURST);
  for (i = 0; i < length; i++)
  {
    SPI.transfer(data[i]);
  }
  deselect();
}

void CC1101::readBurstRegister(uint8_t *buffer, const uint8_t address, const uint8_t length)
{
  uint8_t i;

  select();
  spi_waitMiso();
  SPI.transfer(address | CC1101_READ_BURST);

  for (i = 0; i < length; i++)
  {
    buffer[i] = SPI.transfer(0x00);
  }

  deselect();
}

size_t CC1101::readData(CC1101Packet *packet, size_t maxLen)
{
  packet->length = 0;
  size_t length = maxLen;
  uint8_t bytesInFIFO = 0;

  bytesInFIFO = readRegisterWithSyncProblem(CC1101_RXBYTES, CC1101_READ_BURST) & CC1101_BITS_RX_BYTES_IN_FIFO;

  size_t readBytes = 0;
  uint32_t lastFIFORead = millis();

  // keep reading from FIFO until we get all the packet.
  while (readBytes < length)
  {
    if (bytesInFIFO == 0)
    {
      if (millis() - lastFIFORead > 5)
      {
        break;
      }
      else
      {
        bytesInFIFO = readRegisterWithSyncProblem(CC1101_RXBYTES, CC1101_READ_BURST) & CC1101_BITS_RX_BYTES_IN_FIFO;
        continue;
      }
    }

    uint8_t bytesToRead = min((uint8_t)(length - readBytes), bytesInFIFO); // only read from bytesInFIFO if there is buffer length available
    if (bytesToRead > 1)
    { // CC1101 errata: RX FIFO pointer is sometimes not properly updated and the last read byte is duplicated, leave on byte in buffer until only one byte is left
      bytesToRead -= 1;
    }
    readBurstRegister(&(packet->data[readBytes]), CC1101_RXFIFO, bytesToRead);
    readBytes += bytesToRead;
    packet->length = readBytes;
    lastFIFORead = millis();

    // Get how many bytes are left in FIFO.
    bytesInFIFO = readRegisterWithSyncProblem(CC1101_RXBYTES, CC1101_READ_BURST) & CC1101_BITS_RX_BYTES_IN_FIFO;
  }

  writeCommand(CC1101_SIDLE); // idle
  writeCommand(CC1101_SFRX);  // flush RX buffer
  writeCommand(CC1101_SRX);   // switch to RX state

  return readBytes;
}

// This function is able to send packets bigger then the FIFO size.
void CC1101::sendData(CC1101Packet *packet)
{
  uint8_t index = 0;
  uint8_t txStatus, MarcState;
  uint8_t length;

  writeCommand(CC1101_SIDLE); // idle

  txStatus = readRegisterWithSyncProblem(CC1101_TXBYTES, CC1101_STATUS_REGISTER);

  // clear TX fifo if needed
  if (txStatus & CC1101_BITS_TX_FIFO_UNDERFLOW)
  {
    writeCommand(CC1101_SIDLE); // idle
    writeCommand(CC1101_SFTX);  // flush TX buffer
  }

  writeCommand(CC1101_SIDLE); // idle

  // determine how many bytes to send
  length = (packet->length <= CC1101_DATA_LEN ? packet->length : CC1101_DATA_LEN);

  writeBurstRegister(CC1101_TXFIFO, packet->data, length);

  writeCommand(CC1101_SIDLE);
  // start sending packet
  writeCommand(CC1101_STX);

  // continue sending when packet is bigger than 64 bytes
  if (packet->length > CC1101_DATA_LEN)
  {
    index += length;
    // loop until all bytes are transmitted
    while (index < packet->length)
    {
      // check if there is free space in the fifo
      while ((txStatus = (readRegisterMedian3(CC1101_TXBYTES | CC1101_STATUS_REGISTER) & CC1101_BITS_RX_BYTES_IN_FIFO)) > (CC1101_DATA_LEN - 2))
        ;

      // calculate how many bytes we can send
      length = (CC1101_DATA_LEN - txStatus);
      length = ((packet->length - index) < length ? (packet->length - index) : length);

      // send some more bytes
      for (int i = 0; i < length; i++)
        writeRegister(CC1101_TXFIFO, packet->data[index + i]);

      index += length;
    }
  }
  // wait until transmission is finished (TXOFF_MODE is expected to be set to 0/IDLE or TXFIFO_UNDERFLOW)
  do
  {
    MarcState = (readRegisterWithSyncProblem(CC1101_MARCSTATE, CC1101_STATUS_REGISTER) & CC1101_BITS_MARCSTATE);
    //		if (MarcState == CC1101_MARCSTATE_TXFIFO_UNDERFLOW) Serial.print("TXFIFO_UNDERFLOW occured in sendData() \n");
  } while ((MarcState != CC1101_MARCSTATE_IDLE) && (MarcState != CC1101_MARCSTATE_TXFIFO_UNDERFLOW));
}