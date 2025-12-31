# LCD Control Userspace Tool

A command-line interface (CLI) tool for interacting with the [lcdcontrol](https://github.com/stradiot/lcdcontrol) Linux kernel driver. This application allows users to write text, clear the screen, configure display settings, and run a live system monitor dashboard on a 16x2 HD44780 LCD connected to a Raspberry Pi.

## 🚀 Features

* **Live Monitor:** Displays real-time system stats (IP address, CPU temperature, Uptime, Hostname).
* **Direct Output:** Print text strings directly to the LCD from the console.
* **Configuration:** Toggle the display, underline cursor, and blinking block cursor.
* **Screen Management:** Clear the display and reset the cursor.
* **Inspection:** Read the current state of the LCD memory from the driver.

## 🛠 Compilation

The project includes a `Makefile` for automated building.

### Option 1: Native Compilation (On Target)
If you are compiling directly on the Raspberry Pi:

```bash
make
```
The binary will be created at `build/lcdtool`.

### Option 2: Cross-Compilation (Yocto/Buildroot)
This project is Yocto-friendly. The `Makefile` uses standard variables (`CC`, `CFLAGS`), allowing the build system to override them automatically.

**Manual Cross-Compilation Example:**
```bash
export CC=aarch64-linux-gnu-gcc
make
```

### Cleaning Up
To remove build artifacts:
```bash
make clean
```

## 📖 Usage

The tool defaults to using `/dev/lcdcontrol`. You can override this with the `-d` flag.

```bash
./build/lcdtool [options] <command> [arguments...]
```

### Global Options
| Option | Description | Default |
| :--- | :--- | :--- |
| `-d <path>` | Specify the device node path. | `/dev/lcdcontrol` |

---

### 🖥️ Commands

#### 1. Live Monitor
Continuously updates the display with system information. Press `Ctrl+C` to exit.

**Syntax:** `monitor <row1_source> <row2_source>`

**Available Sources:**
* `ip`: Current IPv4 address (skips loopback).
* `temp`: CPU Temperature (from `/sys/class/thermal`).
* `uptime`: System uptime (formatted as Days/Hours or HH:MM:SS).
* `host`: System Hostname.

**Example:**
```bash
# Show IP on top line, Temp on bottom line
./build/lcdtool monitor ip temp
```

#### 2. Print Text (write to the /dev/lcdcontrol file)
Writes a string to the display. Handles newline `\n` characters to move to the second line.

*Note: The driver uses a scrolling log format. New text appears on the bottom line, pushing previous text to the top line.*

**Syntax:** `print "<text>"`

**Example:**
```bash
./build/lcdtool print "Hello World"
./build/lcdtool print "Line 1\nLine 2"
```

#### 3. Configuration (implemented using IOCTL)
Controls the hardware display state using 1 (On) or 0 (Off).

*Implementation Note: This command uses the `LCD_IOCTL_SET_CONFIG` ioctl to modify the display control register directly without rewriting text.*

**Syntax:** `config <display> <cursor> <blink>`

**Example:**
```bash
# Display ON, Cursor OFF, Blink ON
./build/lcdtool config 1 0 1
```

#### 4. Clear Screen (implemented using IOCTL)
Wipes the display memory and resets the cursor to the home position (0,0).

*Implementation Note: This command triggers the `LCD_IOCTL_CLEAR` ioctl, which performs a hardware clear command and resets the driver's internal line buffer.*

**Syntax:** `clear`

**Example:**
```bash
./build/lcdtool clear
```

#### 5. Read Memory (read from the /dev/lcdcontrol file)
Dumps the driver's internal screen buffer to the console. Useful for debugging what *should* be on the screen.

**Syntax:** `read`

**Example:**
```bash
./build/lcdtool clear
./build/lcdtool print "Hello World"
./build/lcdtool read
# Output (Newest line is at the bottom):
# |                |
# |Hello World     |
```

## ⚠️ Troubleshooting

### "Error opening /dev/lcdcontrol: Permission denied"
* **Cause:** The current user does not have permission to access the device node.
* **Fix:** Run with `sudo` or add a udev rule to grant access to the `gpio` group.

### "Error opening /dev/lcdcontrol: No such file or directory"
* **Cause:** The kernel module is not loaded.
* **Fix:** Run `modprobe lcdcontrol` and ensure the driver loaded successfully (`dmesg`).

### Garbage characters in Monitor mode
* **Cause:** If the text is longer than 16 characters, it is truncated.
* **Fix:** The monitor automatically truncates strings to fit, but extremely long hostnames might still be cut off.

## 📜 License
MIT / GPL Compatible
