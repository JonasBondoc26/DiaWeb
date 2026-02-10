# ESP8266 Wiring Diagrams - Visual Reference

## Basic Wiring (Recommended for Beginners)

```
     ┌──────────────────────┐
     │   ESP8266 NodeMCU    │
     │                      │
     │  ┌────────────────┐  │
     │  │                │  │
     │  │   [ESP-12E]    │  │
     │  │                │  │
     │  └────────────────┘  │
     │                      │
     │  D0  D1  D2  D3  D4  │
     │   ○   ●   ○   ○   ○  │
     └────────┼──────────────┘
              │
              │ (GPIO5)
              │
              ↓
         ┌────────┐
         │   +    │
         │ BUZZER │
         │   -    │
         └────┬───┘
              │
              ↓
     ┌────────────────────┐
     │       GND          │
     └────────────────────┘

● = D1 (GPIO5) connected
○ = Not used
```

## Component Details

### NodeMCU ESP8266 Pinout
```
                    ┌─────────────────┐
                    │      RESET      │
                    ├─────────────────┤
               ADC0 │ A0          D0  │ GPIO16
             RESERVED│ RSV         D1  │ GPIO5  ← BUZZER
               GPIO9 │ D9          D2  │ GPIO4
              GPIO10 │ D10         D3  │ GPIO0
               GPIO9 │ SD3         D4  │ GPIO2  (LED)
              GPIO10 │ SD2         3V3 │ 3.3V
               GPIO9 │ SD1         GND │ Ground ← BUZZER
              GPIO10 │ CMD         D5  │ GPIO14
                GND │ GND         D6  │ GPIO12
               3.3V │ 3V3         D7  │ GPIO13
                 EN │ EN          D8  │ GPIO15
               RESET│ RST         RX  │ GPIO3
                GND │ GND         TX  │ GPIO1
                 5V │ VIN         GND │ Ground
                    └─────────────────┘
                         [USB Port]
```

### Piezo Buzzer Types

#### Passive Buzzer (Recommended)
```
    ┌───────────┐
    │    TOP    │
    │  ┌─────┐  │
    │  │ ))) │  │  ← Generates different tones
    │  └─────┘  │
    │           │
    │  +    -   │
    └───┬───┬───┘
        │   │
       RED BLACK
```

#### Active Buzzer
```
    ┌───────────┐
    │    TOP    │
    │  ┌─────┐  │
    │  │ PCB │  │  ← Has built-in oscillator
    │  └─────┘  │
    │           │
    │  +    -   │
    └───┬───┬───┘
        │   │
       RED BLACK
```

## Breadboard Layout

### Simple Setup
```
     5V ── (Not Connected)
    3.3V ── (Not Connected)
     GND ──┬── [Buzzer -]
           │
           └── [ESP8266 GND]
    
      D1 ──── [Buzzer +]
```

### With Volume Control Resistor
```
      D1 ──── [100Ω Resistor] ──── [Buzzer +]
     GND ──── [Buzzer -]
```

### Enhanced (Louder) with Transistor
```
                     +5V (from USB)
                      │
                      │
                 [Buzzer +]
                      │
                 [Buzzer -]
                      │
                   Collector
                      │
              ┌───[2N2222]───┐
              │   Transistor  │
         Base │               │ Emitter
              │               │
      D1 ──[1kΩ]─────────────┤
                              │
                             GND
```

## Physical Assembly Steps

### Step 1: Identify Components
```
ESP8266 Board:     [====USB====]
                   |  NodeMCU  |
                   |   v1.0    |
                   [============]

Piezo Buzzer:      ┌─────┐
                   │ +  -│
                   └─────┘

Jumper Wires:      ────────  (Male-Male)
                   or
                   ────○      (Male-Female)
```

### Step 2: Connect Buzzer Positive
```
1. Take a red jumper wire
2. Connect one end to Buzzer + (positive)
3. Connect other end to D1 pin on ESP8266

   [ESP D1] ─────RED WIRE───── [Buzzer +]
```

### Step 3: Connect Buzzer Negative
```
1. Take a black jumper wire
2. Connect one end to Buzzer - (negative/GND)
3. Connect other end to GND pin on ESP8266

   [ESP GND] ────BLACK WIRE──── [Buzzer -]
```

### Step 4: Connect USB Cable
```
1. Connect Micro-USB cable to ESP8266
2. Connect other end to computer or USB power adapter

   [Computer USB] ═════ [ESP8266 USB Port]
```

## Testing Continuity

### Check Your Connections
```
Use a multimeter to verify:

1. D1 to Buzzer +:
   Multimeter shows connection (beep or low resistance)

2. GND to Buzzer -:
   Multimeter shows connection (beep or low resistance)

3. D1 to GND:
   Should show NO direct connection (high resistance)
```

## Common Mistakes to Avoid

### ❌ Wrong: Reversed Polarity
```
D1  ──── [Buzzer -]  ← WRONG
GND ──── [Buzzer +]  ← WRONG
```

### ✅ Correct: Proper Polarity
```
D1  ──── [Buzzer +]  ← CORRECT
GND ──── [Buzzer -]  ← CORRECT
```

### ❌ Wrong: Using 5V Pin
```
5V  ──── [Buzzer +]  ← Can damage ESP8266 GPIO
GND ──── [Buzzer -]
```

### ✅ Correct: Using GPIO Pin
```
D1  ──── [Buzzer +]  ← Safe and controllable
GND ──── [Buzzer -]
```

## Alternative Pin Configurations

### Option 1: D1 (GPIO5) - Default
```
D1 (GPIO5) ──── Buzzer +
GND        ──── Buzzer -
```

### Option 2: D2 (GPIO4)
```
D2 (GPIO4) ──── Buzzer +
GND        ──── Buzzer -

Change in code:
#define BUZZER_PIN D2
```

### Option 3: D5 (GPIO14)
```
D5 (GPIO14) ──── Buzzer +
GND         ──── Buzzer -

Change in code:
#define BUZZER_PIN D5
```

### Option 4: D6 (GPIO12)
```
D6 (GPIO12) ──── Buzzer +
GND         ──── Buzzer -

Change in code:
#define BUZZER_PIN D6
```

## Final Assembly Photos Reference

### Top View
```
       ╔═══════════════╗
       ║  ESP8266      ║
       ║  NodeMCU      ║
       ║               ║
       ║  [USB Port]   ║
       ║               ║
       ║ ○ ○ ○ ○ ○ ○ ○ ║
       ╚═══════════════╝
            │  │
            │  └── Red wire to Buzzer +
            └── Black wire to GND
```

### Side View
```
    ┌──USB Cable──┐
    │             │
    ▼             │
┌───────┐         │
│ESP8266│         │
│       │         │
│  D1   ●─────────┼──→ Red → Buzzer +
│       │         │
│  GND  ●─────────┼──→ Black → Buzzer -
└───────┘         │
                  │
           ┌──────┴──────┐
           │  Buzzer     │
           │   +    -    │
           └─────────────┘
```

## Power Supply Options

### Option 1: Computer USB (Development)
```
[Computer] ═══USB Cable═══ [ESP8266]
   5V, 500mA              Powers board + buzzer
```

### Option 2: USB Wall Adapter (Permanent)
```
[Wall Outlet] → [USB Adapter] ═══ [ESP8266]
                  5V, 1A        Reliable 24/7
```

### Option 3: Power Bank (Portable)
```
[Power Bank] ═══USB Cable═══ [ESP8266]
  5V, 2A                    Good for mobile use
```

### Option 4: Dedicated 5V Supply
```
[5V Power Supply] ─── VIN pin (5V) → [ESP8266]
  2A recommended      GND → [ESP8266 GND]
```

## Enclosure Ideas

### 3D Printed Box
```
┌─────────────────────┐
│  ┌───────────────┐  │
│  │   ESP8266     │  │
│  └───────────────┘  │
│                     │
│    [Buzzer]         │
│      ◉◉             │
│                     │
│  [USB Port Hole]    │
└─────────────────────┘
```

### Plastic Project Box
```
┌──────────────────────┐
│ ┌──────┐  ╔═════╗   │
│ │ESP8266│  ║Buzzer║  │
│ └──────┘  ╚═════╝   │
│                      │
│   Mounted with       │
│   screws or tape     │
└──────────────────────┘
   Holes for sound →  ○○○
```

## LED Indicator Reference

The built-in LED on D4 (GPIO2) will:
- **Blink during WiFi connection**
- **Turn ON when buzzer plays**
- **Stay OFF when idle**

```
ESP8266 Board Top:
┌─────────────────┐
│   ◉← LED (D4)   │
│                 │
│   ESP-12E       │
│                 │
└─────────────────┘
```

---

**Once wired correctly, upload the code and your buzzer will sound for all DiaWeb notifications!**
