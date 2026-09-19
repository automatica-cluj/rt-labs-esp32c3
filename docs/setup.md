# Setting up: Arduino IDE and the ESP32-C3 Super Mini

Do this once on your own computer before Lab 1. It takes about 20 minutes,
most of it downloading.

## 1. Install

1. Install **Arduino IDE 2** from arduino.cc (Windows, macOS or Linux).
2. *File > Preferences > Additional boards manager URLs*, add
   `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
3. *Tools > Board > Boards Manager*, search **esp32**, install
   **esp32 by Espressif Systems**, version **3.3.x**. The labs were tested with
   3.3.7. Version 2.x has a different timer API and will not compile Lab 4.

## 2. Board settings

Plug the board in with a data-capable USB-C cable. In the *Tools* menu:

| Setting | Value |
|---|---|
| Board | **ESP32C3 Dev Module** |
| Port | the new port that appeared (`COMx`, `/dev/cu.usbmodem...`, `/dev/ttyACM0`) |
| USB CDC On Boot | **Enabled**, otherwise you will see no output at all |
| CPU Frequency | 160 MHz (the default) |
| Everything else | leave the defaults |

The Super Mini has no USB-to-serial chip. The ESP32-C3 talks USB itself, on
pins 18 and 19. That is why *USB CDC On Boot* matters: it routes `Serial` to
that USB port.

On **Linux**, add yourself to the `dialout` group (`sudo usermod -aG dialout
$USER`, then log out and in) so you may open the port.

## 3. First upload

1. Open `lab1-time-latency/clocks/clocks.ino`.
2. Click *Upload* (the arrow). The first compile takes a few minutes.
3. Open *Tools > Serial Monitor* and set it to **115200 baud**.
4. Press the **RST** button on the board. You should see
   `=== lab 1: clocks ===` followed by the measurements.

To send a key to a sketch, type it in the serial monitor's input line and
press Enter. The sketches ignore the Enter itself.

## 4. When the upload fails or the port disappears

A sketch that crashes or keeps the CPU busy can take the USB port down with
it. The chip itself is fine. Put it in bootloader mode by hand:

1. Hold **BOOT**.
2. Press and release **RST** (or unplug and replug the cable).
3. Release **BOOT**.
4. Upload again, then press **RST** to start the sketch.

If *Upload* hangs at "Connecting...", do the same thing while it is waiting.

## 5. The board

| | |
|---|---|
| CPU | one RISC-V core, 160 MHz |
| RAM | 400 KB (about 320 KB free for sketches) |
| Flash | 4 MB, code runs from it through a 16 KB cache |
| Blue LED | GPIO 8, lit when the pin is LOW |
| BOOT button | GPIO 9, reads LOW when pressed |
| Free pins | 0-7, 10, 20, 21 (the labs need none of them) |

Pins 2, 8 and 9 are strapping pins: their level at reset decides how the chip
boots. Do not wire anything to them that pulls them hard in either direction.

## 6. Command line (optional)

Everything the IDE does is available from `arduino-cli`. The IDE ships a copy.

```
arduino-cli compile --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc lab5-shared-resources/race
arduino-cli upload  --fqbn esp32:esp32:esp32c3:CDCOnBoot=cdc -p /dev/ttyACM0 lab5-shared-resources/race
arduino-cli monitor -p /dev/ttyACM0 --config baudrate=115200
```

`arduino-cli monitor ... | tee run.log` also saves everything to a file, which
`tools/plot_serial.py` can plot (Lab 1).
