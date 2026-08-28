# Glitch Goblin

**Glitch Goblin** is an embedded communications fault-injection and validation platform built around an **STM32F401RE** target and an **ESP32** test controller.

The project uses a custom binary protocol called **SERUM** to test how an embedded target behaves under bad communication conditions such as corrupted packets, dropped packets, duplicates, latency, lost acknowledgements, and replayed sequence IDs.

Glitch Goblin can automatically inject faults, retry failed transmissions, measure round-trip latency, collect reliability statistics, and produce automated pass/fail results.

---

## What It Does

Glitch Goblin can:

- Send structured SERUM packets between an ESP32 and STM32
- Detect corrupted packets using CRC-16
- Acknowledge valid packets with sequence-matched ACKs
- Retransmit packets when ACKs time out
- Suppress duplicate command execution
- Detect recently replayed sequence IDs using an 8-entry sequence history
- Measure packet round-trip time in microseconds
- Collect delivery, retry, timeout, and recovery statistics
- Inject random communication faults
- Run automated benchmark and chaos tests
- Change test settings at runtime from the ESP32 serial console
- Request telemetry from the STM32

---

## System Architecture

```text
                     USB Serial Console
                            │
                            ▼
                    ┌────────────────┐
                    │     ESP32      │
                    │ Test Controller│
                    │                │
                    │ - CLI          │
                    │ - Fault Engine │
                    │ - Benchmarks   │
                    │ - Chaos Tests  │
                    │ - Reliability  │
                    └───────┬────────┘
                            │
                       UART / SERUM
                            │
                            ▼
                    ┌────────────────┐
                    │ STM32F401RE    │
                    │ Target Device  │
                    │                │
                    │ - Parser       │
                    │ - CRC Check    │
                    │ - ACKs         │
                    │ - Replay       │
                    │   Protection   │
                    │ - Telemetry    │
                    └───────┬────────┘
                            │
                            ▼
                    Target Application
```

The ESP32 acts as the **test controller and fault injector**.

The STM32 acts as the **device under test**.

---

## Hardware

### Main Boards

- ST NUCLEO-F401RE
- ESP32 DevKit V1
- USB cables
- Jumper wires

### UART Wiring

| STM32F401RE | ESP32 | Purpose |
|---|---|---|
| PA9 / USART1 TX | GPIO16 / RX2 | STM32 → ESP32 |
| PA10 / USART1 RX | GPIO17 / TX2 | ESP32 → STM32 |
| GND | GND | Common ground |

Both boards are powered independently over USB.

Do **not** connect the board 5 V rails together.

### Additional Interfaces

- STM32 USART2 / ST-LINK Virtual COM Port → PC debug output
- ESP32 USB serial → Glitch Goblin command console

UART settings:

```text
115200 baud
8 data bits
No parity
1 stop bit
```

---

## SERUM Protocol

SERUM is the custom packet protocol used by Glitch Goblin.

### Packet Format

```text
53 45 | VERSION | TYPE | LENGTH (2B) | SEQUENCE (2B) | PAYLOAD | CRC16 (2B)
```

The sync bytes are:

```text
0x53 0x45
  S    E
```

Current protocol version:

```text
0x01
```

### Message Types

| Type | Value | Purpose |
|---|---:|---|
| PING | `0x01` | Link testing |
| COMMAND | `0x02` | Target commands |
| TELEMETRY | `0x03` | STM32 statistics |
| ERROR | `0x04` | Error reporting |
| ACK | `0x05` | Positive acknowledgement |
| NACK | `0x06` | Reserved negative acknowledgement |

### Current Commands

| Command | Value |
|---|---:|
| Toggle LED | `0x01` |
| Get statistics | `0x02` |

---

## CRC Integrity Checking

SERUM uses:

```text
CRC-16/CCITT-FALSE
Polynomial: 0x1021
Initial value: 0xFFFF
```

The CRC is calculated across:

```text
Version
Type
Length
Sequence
Payload
```

The sync bytes and received CRC field are excluded.

A corrupted packet is rejected and is **not ACKed**. That naturally causes the sender's reliability layer to time out and retry.

---

## Reliability

SERUM currently uses a simple **stop-and-wait ARQ** reliability model.

```text
Send packet
     │
     ▼
Wait for matching ACK
     │
 ┌───┴────┐
 │        │
ACK     Timeout
 │        │
 ▼        ▼
Done    Retry
```

Each new transaction gets a new sequence number.

A retransmission keeps the **same sequence number**.

Example:

```text
TX #42 attempt 1
ACK timeout

TX #42 attempt 2
ACK #42 received
Delivered after 2 attempt(s)
```

This allows the STM32 to recognize the second packet as a retry instead of a new command.

---

## Duplicate and Replay Protection

The STM32 keeps a recent history of accepted sequence numbers.

Current history size:

```text
8 sequence IDs
```

Example:

```text
#20 → NEW
#21 → NEW
#22 → NEW
#23 → NEW

#20 arrives again

→ DUPLICATE
→ command is NOT executed again
→ ACK is still returned
```

ACKing duplicates is important because the sender may be retransmitting only because the original ACK was lost.

The replay history is implemented as a small circular buffer, costing only:

```text
8 × uint16_t = 16 bytes
```

---

## Fault Injection

The ESP32 can intentionally simulate several communication problems.

### CRC Corruption

Flips a bit in the encoded CRC before transmission.

```text
Valid packet
    ↓
CRC modified
    ↓
STM32 CRC check fails
    ↓
No ACK
    ↓
Sender times out
```

### Packet Drop

The ESP32 pretends to transmit the packet but sends nothing.

### Duplicate Packet

The same encoded packet is transmitted twice. The STM32 processes the first copy and ignores the duplicate.

### Artificial Delay

Transmission is intentionally delayed to simulate latency.

### Replay

An older sequence number is transmitted after newer packets have already been processed. The STM32 sequence history detects and rejects the replay.

---

## Chaos Mode

Chaos mode automatically chooses faults using configurable probabilities.

Default distribution:

```text
Normal:     70%
Bad CRC:    10%
Drop:       10%
Duplicate:   5%
Delay:       5%
```

Each retransmission receives a new random fault selection.

Example:

```text
TX #73 attempt 1 [DROP]
ACK timeout

TX #73 attempt 2 [BAD CRC]
ACK timeout

TX #73 attempt 3 [NORMAL]
ACK #73 received
```

---

## Runtime Configuration

Test behavior can be changed from the ESP32 console without recompiling firmware.

Examples:

```text
retries 5
timeout 300
bench 1000
chaos 1000
```

Fault probabilities are also configurable:

```text
faults 60 10 15 10 5
```

which represents:

```text
Normal:     60%
CRC:        10%
Drop:       15%
Duplicate:  10%
Delay:       5%
```

The five values must add up to `100`.

View the current settings with:

```text
config
```

---

## Console Commands

| Command | Description |
|---|---|
| `normal` | Send a normal SERUM PING |
| `crc` | Send a packet with a corrupted CRC |
| `drop` | Simulate a dropped packet |
| `duplicate` | Send the same packet twice |
| `delay` | Artificially delay a packet |
| `stats` | Request STM32 telemetry |
| `reliable` | Run one reliable transaction |
| `replay` | Test recent-sequence replay protection |
| `bench` | Run benchmark using configured packet count |
| `bench <n>` | Set benchmark size and run |
| `chaos` | Run chaos test using configured packet count |
| `chaos <n>` | Set chaos size and run |
| `retries <n>` | Set maximum transmission attempts |
| `timeout <ms>` | Set ACK timeout |
| `faults <N C D U L>` | Configure fault probabilities |
| `config` | Display current configuration |
| `help` | Display commands |

---

## Metrics

Glitch Goblin tracks:

### Reliability

- Successful transactions
- Failed transactions
- First-attempt successes
- Transactions recovered by retry
- Total retransmissions
- ACK timeouts
- Delivery percentage
- Recovery percentage

### Performance

- Minimum RTT
- Maximum RTT
- Average RTT

### Fault Injection

- Normal transmissions
- CRC faults
- Packet drops
- Duplicate injections
- Delay injections
- Total transmission attempts

### Delivery Rate

```text
successful transactions
----------------------- × 100%
 total transactions
```

### Recovery Rate

```text
       recovered transactions
------------------------------------ × 100%
recovered transactions + failed transactions
```

This measures how often retransmission successfully rescued a transaction that did not succeed on its first attempt.

---

## Automated Test Verdict

Chaos mode compares measured performance against test requirements.

Current thresholds:

```text
Minimum delivery rate: 95%
Minimum recovery rate: 90%
```

Example:

```text
=== CHAOS TEST VERDICT ===
Delivery requirement: 95%
Recovery requirement: 90%

RESULT: PASS
==========================
```

---

## Example Chaos Test

One 100-transaction test produced:

```text
=== FAULT INJECTION STATS ===
Normal:      87
CRC faults:  13
Drops:       8
Duplicates:  4
Delays:      9
TX attempts: 121
=============================

=== SERUM LINK STATS ===
Successful:             100
Failed:                 0
First-attempt success:  81
Recovered by retry:     19
Retries:                21
Timeouts:               21
Delivery:               100.00%
Recovery rate:          100.00%
Min RTT:                5092 us
Max RTT:                189125 us
Average RTT:            16306 us
========================

=== CHAOS TEST VERDICT ===
Delivery requirement: 95%
Recovery requirement: 90%

RESULT: PASS
==========================
```

The project has also been exercised with larger benchmark and chaos runs without firmware crashes.

---

## STM32 Firmware Architecture

```text
STM32 firmware
│
├── main.c
├── serum_app.c
├── serum.c
├── ring_buffer.c
├── sequence_tracker.c
│
└── drivers/
    ├── clock.c
    ├── gpio.c
    ├── uart.c
    └── timebase.c
```

### `main.c`

Application startup only.

### `serum_app.c`

Handles incoming SERUM packets, commands, ACK generation, telemetry, and duplicate/replay handling.

### `serum.c`

Portable SERUM protocol implementation:

- Packet encoder
- Byte-by-byte parser
- Parser state machine
- CRC-16

### `ring_buffer.c`

Single-producer/single-consumer receive buffering used by the interrupt-driven UART receive path.

### `sequence_tracker.c`

Maintains the recent sequence history used for replay protection.

---

## STM32 Register-Level Drivers

The STM32 firmware includes register-level implementations for:

- GPIO
- USART1
- USART2
- SysTick timebase
- RCC / PLL clock configuration

Target clocks:

```text
SYSCLK: 84 MHz
APB1:   42 MHz
APB2:   84 MHz
```

USART1 runs from APB2.

USART2 runs from APB1.

UART baud-rate generation accounts for the different peripheral clocks.

---

## ESP32 Firmware Architecture

```text
ESP32 firmware
│
├── main.cpp
├── serum.cpp
├── serum_link.cpp
├── fault_engine.cpp
├── stats.cpp
├── test_runner.cpp
├── console.cpp
└── config.cpp
```

### `main.cpp`

Startup and the main loop only.

### `console.cpp`

Parses commands from the USB serial console.

### `test_runner.cpp`

Runs manual fault tests, reliable packet tests, replay tests, benchmarks, and chaos tests.

### `serum_link.cpp`

Handles ESP32 UART, ACK matching, telemetry reception, retransmission, and RTT measurement.

### `fault_engine.cpp`

Implements random fault selection, packet corruption, drops, duplicates, and delay injection.

### `stats.cpp`

Tracks and reports test metrics.

### `config.cpp`

Owns runtime-configurable test settings.

---

## Repository Layout

```text
glitch-goblin/
│
├── firmware/
│   ├── stm32/
│   │   └── glitch-goblin/
│   │       ├── include/
│   │       └── src/
│   │
│   └── esp32/
│       └── glitch-goblin-esp32/
│           ├── include/
│           └── src/
│
└── README.md
```

---

## Building

The firmware is built with **PlatformIO**.

### STM32

```ini
[env:nucleo_f401re]
platform = ststm32
board = nucleo_f401re
framework = stm32cube
upload_protocol = stlink
debug_tool = stlink
monitor_speed = 115200
```

### ESP32

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

Build:

```bash
pio run
```

Upload:

```bash
pio run -t upload
```

Monitor:

```bash
pio device monitor
```

---

## Example Workflow

Check the current configuration:

```text
config
```

Run a normal link test:

```text
normal
```

Test CRC rejection:

```text
crc
```

Test reliable delivery:

```text
reliable
```

Run a benchmark:

```text
bench 1000
```

Configure a harsher channel:

```text
faults 50 15 20 10 5
retries 5
timeout 500
```

Then run:

```text
chaos 1000
```

Glitch Goblin will inject faults, retry failed packets, measure the results, and automatically report whether the test passed.

---

## Engineering Concepts Demonstrated

- Embedded C and C++
- STM32 register-level peripheral programming
- ESP32 firmware development
- UART communication
- Interrupt-driven receive paths
- Ring buffers
- Binary protocol design
- Finite-state-machine parsing
- CRC error detection
- Sequence numbering
- Stop-and-wait ARQ
- Timeout and retransmission logic
- Duplicate suppression
- Replay protection
- Telemetry encoding
- Big-endian serialization
- Runtime configuration
- Automated fault injection
- Reliability benchmarking
- Latency measurement
- Modular firmware architecture
- Git feature-branch workflow

---

## Current Status

**Glitch Goblin V1 core firmware is complete.**

Implemented and tested:

```text
SERUM framing                  ✓
CRC-16                         ✓
Parser state machine           ✓
Sequence numbers               ✓
ACKs                           ✓
Timeouts                       ✓
Retransmission                 ✓
Duplicate suppression          ✓
Recent replay protection       ✓
Telemetry                      ✓
RTT measurement                ✓
Benchmark mode                 ✓
Chaos mode                     ✓
Runtime configuration          ✓
Configurable fault rates       ✓
Automated PASS / FAIL          ✓
Large-run stress testing       ✓
Modular STM32 firmware         ✓
Modular ESP32 firmware         ✓
```

---

## Possible V2 Improvements

- Non-blocking reliability state machine
- DMA-based UART transmit/receive
- FreeRTOS tasks
- Sliding-window reliability
- Larger replay window
- Persistent configuration
- Python visualization/dashboard
- Automated CSV logging
- Hardware-in-the-loop test scripts
- Custom PCB
- Additional physical-layer fault injection
- Protocol fuzzing

These are intentionally outside the current V1 scope.

---

## Why "Glitch Goblin"?

Because if a communication link can break in a weird way, the goblin should probably try it.
