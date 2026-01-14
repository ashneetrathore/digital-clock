# :watch: EMBEDDED DIGITAL CLOCK

## :open_book: OVERVIEW
Date: May 2025\
Developer(s): Ashneet Rathore\
Based on assignment instructions from Prof. Amir Rahmani

Embedded Digital Clock is an embedded systems project using an ATMega32 microcontroller. The system integrates a keypad and LCD on a breadboard circuit to display the current date and time and respond to real-time user input. Using the keypad, users can set the date and time, as well as toggle between military time and 12-hour AM/PM format.

View more of my embedded programming projects on GitHub [here](https://github.com/stars/ashneetrathore/lists/systems-programming-software)

## :brain: FIRMWARE DESIGN
Run in **Microchip Studio**, the firmware for the embedded digital clock is built around a **state machine architecture** that cleanly separates normal timekeeping from user configuration. In the `RUNNING` state, the clock advances and displays time continuously, while transitioning through the `SET_YEAR`, `SET_MONTH`, `SET_DAY`, `SET_HOUR`, `SET_MINUTE`, and `SET_SECOND` states allow the user to modify each field sequentially. A `STATIC` state temporarily halts time progression while configuration is finalized, ensuring predictable and controlled state transitions.

The core time data is encapsulated in a `DateTime` struct, storing the current year, month, day, hour, minute, and second. Time progression is handled by the `advance_dt()` function, which increments the clock in one-second steps and correctly propagates rollovers across seconds, minutes, hours, days, months, and years. Calendar correctness is maintained through **invalid time checking**, where user inputs are validated to enforce legal ranges for each field, and **leap year handling**, where February's day count is dynamically updated based on the current year. The `print_dt()` function is responsible for formatting and displaying the date and time on the LCD, supporting both **military (24-hour) mode** and **standard 12-hour AM/PM mode**, which can be toggled by the user at runtime. 

User interaction is implemented using **polling-based keypad scanning**, allowing real-time input without interrupts. Key presses drive state transitions, numeric entry for date and time fields, and mode toggling, with immediate feedback provided on the LCD. Invalid entries are detected and reported to the user, who is then prompted to reenter the value for the field. **Timing**, implemented through the `avr_wait()` function, is managed using simple millisecond delays, which is sufficient for reliable one-second clock updates and responsive user input.

## :open_file_folder: PROJECT FILE STRUCTURE
```bash
EmbeddedDigitalClock/
│── main.c                  # Implements main program logic for the digital clock
│── avr.h                   # Defines AVR macros and timing utilities
│── lcd.h                   # Declares LCD control and display functions
│── lcd.c                   # Implements LCD control and display functions
│── assets/               
│   │── circuit_image.jpg   # Image of finished circuit
│   └── schematic.png       # Schematic of circuit
│── README.md               # Project documentation
└── .gitignore              # Excludes files and folders from version control
```

## :gear: CIRCUIT SET UP GUIDE
> [!NOTE]
> View the circuit schematic and an image of the finished circuit in the [assets folder](assets/).

### :toolbox: REQUIRED HARDWARE
- ATMega 32 Microcontroller
- Breadboard (with solid core or jumper wires)
- ATMEL-ICE Programmer
- 9V Battery (with 9V battery connector)
- LCD Display
- 4 by 4 Keypad
- 5V Voltage Regulator
- 0.1µF Capacitor
- 8 MHz Crystal
- 1K Resistor

### :battery: BUILDING THE POWER SUPPLY
The voltage regulator has three pins: input (Pin 1), ground (Pin 2), and output (Pin 3). The circuit takes +9V as input and regulates it down to a +5V output, using the capacitor to smooth out the output voltage.
1. Connect the positive terminal of the 9V Battery to Pin 1.
2. Connect the negative terminal of the 9V Battery to Pin 2.
3. Connect the negative (shorter) leg of a capacitor to Pin 2, and the positive leg of a capacitor to Pin 3.
4. Connect Pin 3 to the positive rail of the breadboard and Pin 2 to the negative rail to power one side of the breadboard.
5. Join the two power rails on each side of the breadboard (positive together, negative together) to deliver voltage to both sides.

### :electric_plug: CONNECTING THE PROGRAMMER TO THE MICROCONTROLLER
1. Connect the ATMEGA32 microcontroller to the breadboard securely.
2. Connect the six programmer pins to the microcontroller as shown below:

    | Programmer Pin    | Microcontroller Pin |
    |-------------------|---------------------|
    | 1                 | PB6 (Pin 7)         |
    | 2                 | PB7 (Pin 8)         |
    | 3                 | RESET (Pin 9)       |
    | 4                 | VCC (Pin 10)        |
    | 5                 | PB5 (Pin 6)         |
    | 6                 | GND (Pin 11)        | 
3. Power the microcontroller by connecting VCC (Pin 10) to the positive rail of the breadboard and GND (Pin 11) to the negative rail.

### :hourglass: CONNECTING THE 8 MHZ CRYSTAL TO THE MICROCONTROLLER
1. Connect one leg of a 8 MHz crystal to XTAL2 (Pin 12) on the microcontroller.
2. Connect the other leg of the crystal to XTAL1 (Pin 13) on the microcontroller.
>[!IMPORTANT]
> When running the program in Microchip Studio, configure the fuses:
> 1. In the top navigation bar, go to *Tools* → *Device Programming* → *Fuses* → *LOW_SUT_CKSEL*.
> 2. Select *Ext.Crystal/Resonator High Freq: Start-up time: 16k CK + 64 ms*.
> 3. Click *Program*.

### :1234: CONNECTING THE KEYPAD TO THE MICROCONTROLLER
1. Connect the 8 keypad pins to PORT C of the microcontroller as shown below:

    | Keypad Pin        | Microcontroller Pin |
    |-------------------|---------------------|
    | C0                | PC0 (Pin 22)        |
    | C1                | PC1 (Pin 23)        |
    | C2                | PC2 (Pin 24)        |
    | C3                | PC3 (Pin 25)        |
    | R0                | PC7 (Pin 29)        |
    | R1                | PC6 (Pin 28)        | 
    | R2                | PC5 (Pin 27)        |
    | R3                | PC4 (Pin 26)        | 

### :framed_picture: CONNECTING THE LCD TO THE MICROCONTROLLER
The LCD has a total of 16 pins, including source pins that supply power to the display, control pins that manage its operation, and data pins that carry the information displayed.
1. To supply power to the LCD, connect VSS (LCD Pin 1) to the negative rail of the breadboard and VDD (LCD Pin 2) to the positive rail.
2. Connect one leg of a 1k resistor to VO (LCD Pin 3) and the other leg to the negative rail of the breadboard.
3. Connect the control pins of the LCD to PORT B of the microcontroller as shown below:

    | LCD Pin           | Microcontroller Pin |
    |-------------------|---------------------|
    | RS (Pin 4)        | PB0 (Pin 1)         |
    | R/W (Pin 5)       | PB1 (Pin 2)         |
    | E (Pin 6)         | PB2 (Pin 3)         |

4. Connect the data pins of the LCD to PORT D of the microcontroller as shown below:

    | LCD Pin           | Microcontroller Pin |
    |-------------------|---------------------|
    | DB0 (Pin 7)       | PD0 (Pin 14)        |
    | DB1 (Pin 8)       | PD1 (Pin 15)        |
    | DB2 (Pin 9)       | PD2 (Pin 16)        | 
    | DB3 (Pin 10)      | PD3 (Pin 17)        |
    | DB4 (Pin 11)      | PD4 (Pin 18)        | 
    | DB5 (Pin 12)      | PD5 (Pin 19)        |
    | DB6 (Pin 13)      | PD6 (Pin 20)        |
    | DB7 (Pin 14)      | PD7 (Pin 21)        | 

>[!NOTE]
> LCD Pins 15 and 16 are unused for this project.

## :point_up_2: KEYPAD CONTROLS
| Key | State      | Function                                                  |
|-----|------------|-----------------------------------------------------------|
| A   | All states | Toggle between miltary mode and AM/PM mode                |
| *   | `RUNNING`  | Enter date/time configuration state                       |
| *   | `STATIC`   | Exit configuration state and return to `RUNNING`          |
| #   | `SET_*`    | Confirm input for current field and advance to next field |
| 0-9 | `SET_*`    | Enter numeric values for the date/time fields             |
