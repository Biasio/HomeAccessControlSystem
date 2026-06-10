# Home Access Control System 

<!-- TABLE OF CONTENTS -->
<details>
  <summary>Table of Contents</summary>
  <ol>
    <li><a href="#about-the-project">About The Project</a></li>
    <li><a href="#project-layout">Project Layout</a></li>
    <li>
      <a href="#requirements">Requirements</a>
      <ul>
        <li><a href="#hardware-setup">Hardware Setup</a></li>
      </ul>
      <ul>
        <li><a href="#software-setup">Software Setup</a></li>
      </ul>
    </li>
    <li><a href="#iot-integration">IoT Integration</a></li>
    <li><a href="#authors">Authors</a></li>
    <li><a href="#links">Links</a></li>
  </ol>
</details>

## About The Project
Welcome to the Home Access Control System!

This project implements a smart door access control system that combines local hardware interaction with IoT remote management.
Users can authenticate using unlock codes, while the administrator can access a dedicated secure menu using an RFID tag.
Beyond the physical interface, an integrated Telegram bot handles remote interactions, enabling the administrator to oversee access permissions and allowing users to request or manage their own codes.
The system also features a database that logs all access events for monitoring purposes.

The architecture is split between two microcontrollers to safely separate local hardware logic from network tasks:
- **MSP432**: Acts as the brain for local operations, handling sensor inputs, the user interface, and mechanical outputs.
- **ESP32-S3**: Connected to the MSP, this board is dedicated to WiFi connectivity, fetching the clock time, and handling the Telegram Bot logic.



## Repository Structure

```text
.
├── ShortStaysAccessControlSystem/     # Firmware project (MSP432 Microcontroller)
│   ├── msp432p401r.cmd                # Memory linker script
│   ├── src/                           # Source code directory
│   │   ├── external_src/              # External hardware libraries
│   │   │   └── vl53l0x_msp432/        # Distance sensor submodule (Git)
│   │   │       ├── drivers/           # Sensor native API registers
│   │   │       │   ├── config.h       # Timing configuration
│   │   │       │   ├── i2c.c / .h     # I2C driver
│   │   │       │   ├── macro.h        # Internal macros
│   │   │       │   └── vl53l0x.c / .h # Main ranging core
│   │   │       ├── main.c             # Sensor standalone test
│   │   │       └── README.md          # Submodule info
│   │   ├── LcdDriver/                 # Display graphics library
│   │   │   ├── Crystalfontz128x128_ST7735.c / .h                       # ST7735 controller primitives
│   │   │   └── HAL_MSP_EXP432P401R_Crystalfontz128x128_ST7735.c / .h   # Display pin mapping
│   │   ├── buzzer.c / .h              # Buzzer acoustic alerts
│   │   ├── comm_esp.c / .h            # Serial communication with ESP32
│   │   ├── database.c / .h            # Local access credentials memory
│   │   ├── display.c / .h             # LCD high-level UI menus
│   │   ├── fsm_helpers.c / .h         # State machine utilities
│   │   ├── fsm.c / .h                 # Main application logic (FSM)
│   │   ├── irqHandlers.c / .h         # Hardware interrupt routines
│   │   ├── joystick.c / .h            # Analog joystick driver
│   │   ├── main.c                     # System entry point & loop
│   │   ├── push_button.c / .h         # Buttons and debouncing
│   │   ├── sensors.c / .h             # Sensors hardware data polling
│   │   └── timers.c / .h              # Periodic timer configurations
│   ├── startup_msp432p401r_ccs.c      # Microcontroller vector table
│   └── system_msp432p401r.c           # System clock configuration
├── TelegramBot/                       # PlatformIO project (ESP32 Microcontroller)
│   ├── include/                       # Global header headers
│   │   └── README                     # Folder info
│   ├── lib/                           # Custom local libraries
│   │   ├── DoorBotManager/            # Telegram connection & events logic
│   │   │   ├── DoorBotManager.cpp     # Bot API implementation
│   │   │   └── DoorBotManager.h       # Bot class definition
│   │   └── README                     # Lib folder info
│   ├── platformio.ini                 # Build and dependency settings
│   ├── src/                           # Application source code
│   │   ├── idf_component.yml          # ESP-IDF component packages
│   │   ├── main.cpp                   # Main bot loop & Wi-Fi init
│   │   └── mainTest.txt               # Text test file
│   └── test/                          # Unit testing folder
│       └── README                     # Test folder info
└── README.md                          # Project documentation
```

## Requirements

### Hardware Setup (+ Project wiring) (Pietro)
- You will need an MSP432p401r of the Texas Instrument company with its own expansion: the BOOSTXL-EDUMKII. 
The system integrates the following components and sensors
- **RFID**: Scans tags to allow the administrator to access the local admin menu.
- **Stepper Motor**: Controls the physical opening and closing mechanism of the door.
- **Display**: Renders the local user interface.
- **Buttons and Joystick**: Allow users to navigate through the system interface.
- **Buzzer**: Provides acoustic feedback, such as warning signals for incorrect code inputs and general alerts.

Cosa scrivere: protocollo di comunicazione, pin utilizzati, funzioni significative.

  (Basic Project wiring: schema con tutti i pin, come in questo schema)
<img width="990" height="720" alt="image" src="https://github.com/user-attachments/assets/c90ca7c7-3b36-445c-9efc-e507df5f13b0" />


### Software Setup (CCSTudio + PlatformIO) (Alessandro)

## IoT Integration

## User Guide + Youtube Video and PowerPoint
Commentare quello che si vede nel video
Contenuti del video:
- Intro: spiegazione del problema
- Spiegazione lato MSP:
  - admin accede al menu, usa rfid e mostra database (con accessi già presenti)
- IoT:
  - accesso admin al bot telegram, si mostra il menù
  - User richiede un pin
  - Admin accetta il pin
  - User inserisce il pin corretto, apertura porta
  - refresh dell'MSP e nuovo tentativo di accesso con vecchio pin user
  - User verifica durata rimasta del pin e poi se lo revoca autonomamente
  - User inserisce il pin sbagliato, buzzer
  - Admin rimuove tutti i pin

## Authors

- Pietro Baroni:
  1. Display
  2. Joystick and push buttons

- Michele Martini:
  1. Telegram bot
  2. UART Communication

- Michele Casagrande:
  1. Database
  2. Motor

- Alessandro Biasoli
  1. RFID
  2. ToF Senosor
  3. Buzzer
  

-------------------------

# EmbeddedProject
Access control for door opening in short stays

## Sensors

- (Biasio) ultrasound sensor for proximity when in front of the board to lit the display or (PIR sensor)
- RFID sensor for admin magnetic access
- Servo motor for opening/closing the door
- (Pietro) Joystick for menu navigation
- (Pietro) Buttons and joystick for menu navigation
- Leds for signal
- (Mich M) Wifi module for IoT connectivity via telegram Bot and clock time
- Hand clap sequence for opening
- (Biasio) Buzzer for wrong code input and in general for signaling

## Features

### 
- (Biasio) Sleep mode with AOD, while proximity sensor doesn't trigger
- First time initialization of the code.
- RFID access displays the menu for pincode setup, wifi setup, enable/disable rfid, factory reset.
- (Mich M) Wifi connection via WPS or via file. [ UART PINS -> MSP(RX = J1.3 , TX = J1.4 ) ; ESP32(RX = 16 , TX = 17 ) ]
- (Mich C) Database of access and access attempts.
- 2 pin codes, one for admin log in and one for user log in.
- Too  many wrong attempts blocks the pin access for some time, too many wrong-blocking timed attempts trigger an admin block.
  
### TELEGRAM BOT
#### USER side
- Request temporary code access for the time of the stay
- Shows how much time is left before the PIN expires

#### ADMIN side
- Approve code request from clients
- Admin receives attempts access notifications and can view access logs (NOT IMPLEMENTED)
- Remove an user from the system
- Revoke all active pins and remove the corresponding user from the system
- Set the duration of temporary pins



## Prerequisites

### Software
Ensure you have the following installed before cloning:
* **[Git LFS](https://git-lfs.com/)**: Required. We use Large File Storage to track the SIMPLELINK SDK libraries.
* **Code Composer Studio (CCS) v12.8**.

### Hardware:
* **TexasInstruments' MSP-EXP432P401R Board**
* **Texas Instruments' BoosterPack BOOSTXL-EDUMKII**

## Getting Started

Follow these steps to build the project:
clone the sdk: simplelink_msp432p4_sdk_3_40_01_02](https://www.ti.com/tool/download/SIMPLELINK-MSP432-SDK/3.40.01.02
### 1. Clone the Repository
Because we use Git LFS, make sure LFS is initialized on your machine before cloning:
```bash
git lfs install
git clone <your-repository-url>
cd <your-repository-name>


### 2. Import the project in CCStudio
1. The git repo is a full workspace for ccstudio, so when opening the IDE select the cloned folder as the active workspace. 
2. The project should be already configured and working out of the box; if no project is automatically imported do "Import Project > Code Composer Studio > CCS Projects > Browse... > Select the ShortStaysAccessControlSystem folder inside the repo location > tick the ccs project > click Finish "
```
