# 16_MP3_Deck

A DENON-inspired MP3 deck example for PRUZEAmini.

## Required libraries

Install these separately in the Arduino IDE:

- **ESP8266Audio** by Earle F. Philhower, III

The MP3 library is not included with PRUZEAmini. XPT2046 support is built into PRUZEAmini through LovyanGFX.

## SD card

Place MP3 files in:

```text
/music/
    track01.mp3
    track02.mp3
```

Up to 64 MP3 files are indexed.

## Controls

| Operation | Touch | Button |
|---|---|---|
| Previous track | PREV | LEFT |
| Play / pause | PLAY | A or START |
| Stop | STOP | B |
| Next track | NEXT | RIGHT |
| Volume down | VOL - | L |
| Volume up | VOL + | R |
| Rescan SD | RETRY | SELECT |

## Architecture

- PRUZEAmini uses `AudioStubConfig` and `StorageStubConfig`.
- The app owns the SD card, MP3 decoder, and I2S output.
- RP2040/RP2350 use `setup1()` / `loop1()` for audio decoding.
- ESP32 uses a pinned FreeRTOS task.
- The UI receives only a small fixed-size `PlayerView` snapshot.

## Recommended RP2040/RP2350 SPI arrangement

| Peripheral | SPI |
|---|---|
| ILI9341 LCD | SPI1 |
| SD card | SPI0 |
| XPT2046 touch | SPI1, shared with the ILI9341 through LovyanGFX |

## Notes

- `AudioStubConfig` must not launch PRUZEAmini's normal audio worker.
- Replace all required `-1` pin values before compiling.
- This first version shows elapsed time only. MP3 duration and seeking are intentionally omitted.

## Touch SPI

The XPT2046 and ILI9341 share SPI1 with separate CS pins. LovyanGFX owns both devices and switches their transactions. The SD card stays on SPI0 so MP3 reads do not contend with display and touch transfers.
