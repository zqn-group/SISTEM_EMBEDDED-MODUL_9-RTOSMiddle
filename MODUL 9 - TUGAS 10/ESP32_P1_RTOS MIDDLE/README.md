# Program 35: Notification as Value

## Concept
Notification as Value demonstrates fundamental embedded systems concepts including hardware peripheral management and/or FreeRTOS real-time operating system features.

## Architecture Diagram
```
┌─────────────────┐
│   Hardware      │
│  (GPIO/UART)    │
└────────┬────────┘
         │
    ┌────▼────┐
    │ Main    │
    │ Loop    │
    └────┬────┘
         │
    ┌────▼──────────────────────────────┐
    │  FreeRTOS (if applicable)          │
    │  - Tasks                           │
    │  - Queues / Semaphores / Timers   │
    └────────────────────────────────────┘
```

## Key Learning Points
1. **Core Concept**: Understanding the main functionality
2. **Implementation**: Code patterns and best practices
3. **Performance**: Throughput, latency, and efficiency
4. **Debugging**: Serial logging and data verification

## Hardware Connections
```
STM32F103C8T6 Blue Pill:
  PA9  (TX) → USB-TTL RX
  PA10 (RX) → USB-TTL TX
  PA1  → LED1 (Red)
  PA2  → LED2 (Yellow)
  PA3  → LED3 (Green)
  GND  → USB-TTL GND

ESP32 DevKitC:
  GPIO1 (TX) → USB-TTL RX
  GPIO3 (RX) → USB-TTL TX
  GPIO2  → LED1 (Red)
  GPIO4  → LED2 (Yellow)
  GPIO5  → LED3 (Green)
  GND  → USB-TTL GND
```

## Serial Output Example
```
=== Program 35: Notification as Value ===
[Task1] Running...
[Task2] Running...
[Timer] Callback
...
```

## Code Explanation

### Initialization
The program initializes hardware (UART, GPIO, clocks) and FreeRTOS components (tasks, queues, semaphores).

### Main Loop
The FreeRTOS scheduler manages task execution based on priority and event availability.

### Synchronization
Tasks communicate through queues and synchronize using semaphores and mutexes.

## Compilation & Flashing

### STM32
```bash
cd 35-Notification as Value
pio run -t upload
pio device monitor -b 115200
```

### ESP32
```bash
cd 35-Notification as Value
pio run -t upload
pio device monitor -b 115200
```

## Testing Checklist
- [ ] Compilation successful
- [ ] Device flashing complete
- [ ] Serial output appears
- [ ] Tasks executing as expected
- [ ] No errors or warnings

## Key Concepts Tested
- ✅ Hardware configuration (GPIO, UART, Timer)
- ✅ FreeRTOS task scheduling
- ✅ Real-time communication
- ✅ Error handling and logging

## Advanced Topics for Follow-Up
- Interrupt handling optimization
- Memory management and profiling
- Power consumption analysis
- Performance benchmarking
