# PRUZEAmini Hardware Configuration

## Board ===============================================

- [ ] RP2040 (Raspberry Pi Pico, Waveshare RP2040 Zero, etc.)
- [x] RP2350 (Raspberry Pi Pico 2, WeAct RP2350B, etc.)
- [ ] ESP32 (ESP32, ESP32-S3, ESP32-C3, ESP32-C6, etc.)


## Display ===============================================

- [x] ILI9341 (SPI)
     SPI_HOST : 1         <!-- 0 or 1 -->
     SPI_FREQUENCY : 62500000
     Rotation : 0         <!-- 0: Normal  3: Rotated 180 degrees -->

     SCLK     : -1
     MOSI     : -1
     DC       : -1
     CS       : -1
     RESET    : -1
     LED      : -1


- [ ] SSD1306 (I2C)
     I2C_PORT : 0
     I2C_ADDR : 0x3c      <!-- 0x3c or 0x3d, depending on the module -->
     Rotation : 2         <!-- 0: Normal  2: Rotated 180 degrees -->

     SDA      : -1
     SCL      : -1
     RESET    : -1


## Input ===============================================

- [x] GPIO Buttons
     UP       : -1
     DOWN     : -1
     LEFT     : -1
     RIGHT    : -1
     A        : -1
     B        : -1
     START    : -1
     VOL_UP   : -1
     VOL_DOWN : -1
     MUTE     : -1


- [ ] SNES Controller
     CLOCK    : -1
     LATCH    : -1
     DATA     : -1

     # Optional additional GPIO buttons
     VOL_UP   : -1
     VOL_DOWN : -1
     MUTE     : -1

## Audio ===============================================

- [x] PWM
    PWM       : -1

- [ ] I2S
    BCLK      : -1
    LRCLK     : -1    <!-- LRCLK/WS; on Pico(rp2040/rp2350), wsPin must equal bclkPin + 1. -->
    DATA      : -1

- [ ] None


## Storage ===============================================

- [x] EEPROM

- [ ] SD Card (SPI)
    SPI_HOST  : 0         <!-- 0 or 1 -->
    BAUDRATE  : 12000000
    
    MISO      : -1
    SCK       : -1
    MOSI      : -1
    CS        : -1

- [ ] None
