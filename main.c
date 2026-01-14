#include "avr.h"
#include "lcd.h"
#include <stdio.h>
#include <string.h>

void avr_init(void)
{
	WDTCR = 15;
}

void avr_wait(unsigned short msec)
{
	TCCR0 = 3;
	while (msec--) {
		TCNT0 = (unsigned char)(256 - (XTAL_FRQ / 64) * 0.001);
		SET_BIT(TIFR, TOV0);
		WDR();
		while (!GET_BIT(TIFR, TOV0));
	}
	TCCR0 = 0;
}

typedef struct {
	int year;
	unsigned char month;
	unsigned char day;
	unsigned char hour;
	unsigned char minute;
	unsigned char second;
} DateTime;

// CurrState = current state the clock is in
enum State { RUNNING,  STATIC, SET_YEAR, SET_MONTH, SET_DAY, SET_HOUR, SET_MINUTE, SET_SECOND } CurrState;
const char* prompt_field[] = {"Year", "Month", "Day", "Hour", "Minute", "Second"};
char input_buff[5];
char buff_index = 0;
// Military mode is initially ON
char military_mode = 1;

// (Index of element + 1) represents the month
// Value of element represents the number of days in that month
// EX: Index 2 + 1 represents the 3rd month, 31 = number of days in the 3rd month
unsigned char days_per_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	
const char keys[] = {
	'1', '2', '3', 'A',
	'4', '5', '6', 'B',
	'7', '8', '9', 'C',
	'*', '0', '#', 'D'
};
	
void leap_year_logic(int year) {
	// A year is a leap year if it's divisible by 4
	// Additional requirement: If the year is also divisible by 100, it must also be divisible by 400 to be a leap year
	if (year % 4 == 0) {
		// If year isn't divisible by 100 -> leap year
		// If year is divisible by 100 AND divisible by 400 -> leap year
		if (year % 100 != 0 || year % 400 == 0) {
			days_per_month[1] = 29;
			return;
		}
	}
	days_per_month[1] = 28;
}

void init_dt(DateTime *dt) {
	// Initialize the clock to anytime
	dt->year = 2025;
	dt->month = 1;
	dt->day = 1;
	
	dt->hour = 0;
	dt->minute = 0;
	dt->second = 0;
	
	// Update days per month for 2nd month if necessary
	leap_year_logic(dt->year);
}

void advance_dt(DateTime *dt) {
	// Advance the clock by 1 second
	++dt->second;
	
	// Advance minute 
	if (dt->second == 60) {
		dt->second = 0;
		++dt->minute;
		
		// Advance hour
		if (dt->minute == 60) {
			dt->minute = 0;
			++dt->hour;
			
			// Advance day
			if (dt->hour == 24) {
				dt->hour = 0;
				++dt->day;
				
				// Advance month
				if (dt->day > days_per_month[dt->month - 1]) {				
					dt->day = 1;
					++dt->month;
					
					// Advance year
					if (dt->month == 13) {
						dt->month = 1;
						++dt->year;
						// Update days per month for 2nd month if necessary
						leap_year_logic(dt->year);
					}
				}
			}
		}
	}
}

void print_dt(const DateTime *dt) {
	char buf[17];
	
	lcd_clr();
	// Print date on top row (MM/DD/YYYY)
	lcd_pos(0, 0);
	sprintf(buf, "%02d/%02d/%04d", dt->month, dt->day, dt->year);
	lcd_puts2(buf);
	
	// Print time on bottom row (HH:MM:SS)
	lcd_pos(1, 0);
	if (military_mode) {
		// If military mode ON, display military time
		sprintf(buf, "%02d:%02d:%02d", dt->hour, dt->minute, dt->second);
	}
	else {
		// Otherwise, display am/pm time
		char hour;
		char am_or_pm; // am = 0, pm = 1
		if (dt->hour / 12 == 0) {
			am_or_pm = 0;
			hour = (dt->hour == 0) ? 12 : dt->hour;
		}
		else if (dt->hour / 12 == 1) {
			am_or_pm = 1;
			hour = (dt->hour == 12) ? 12 : dt->hour % 12;
		}
		
		if (!am_or_pm) {
			sprintf(buf, "%02d:%02d:%02d AM", hour, dt->minute, dt->second);
		}
		else {
			sprintf(buf, "%02d:%02d:%02d PM", hour, dt->minute, dt->second);
		}
	}
	
	lcd_puts2(buf);
}

int is_pressed(int r, int c) {
	// Set all 8 GPIOs to N/C (DDR = 0, PORT = 0)
	// Result: All rows/columns are floating (no connection -> clean slate)
	DDRC = 0x00;
	PORTC = 0x00;
	
	// Set row r to "0" to activate a specific row
	SET_BIT(DDRC, r); // Set row as output (1)
	CLR_BIT(PORTC, r); // Drive row LOW (0)
	
	// Set column c to "w1" to make it a listening input
	SET_BIT(PORTC, c + 4); // Drive column WEAK 1 (1)
	
	avr_wait(5); // Small delay for signal stabilization
	
	// If the pin value of column c reads 0, it means there's a connection between r and c
	// 0 means the column was driven from WEAK 1 to LOW externally (through a button press)
	// Key is pressed
	if (!GET_BIT(PINC, c + 4)) {
		return 1;
	}
	// Key isn't pressed
	return 0;
}

int get_key() {
	int i, j;
	// Scans the 4 by 4 keypad, checking if the key at the i-th row and j-th column was pressed
	for (i = 0; i < 4; ++i) {
		for (j = 0; j < 4; ++j) {
			if (is_pressed(i, j)) {
				// Maps the (i, j) position into a number from 1 to 16
				// EX: (0, 0) -> 0*4 + 0+1 = 1
				// EX: (3, 0) -> 3*4 + 0+1 = 13
				return i*4+j+1;
			}
		}
	}
}

void print_prompt(char field) {
	// Print prompts 'Set Year: ', 'Set Month: ', 'Set Day: ', asking user for input
	char buf[32];
	lcd_clr();
	
	lcd_pos(0, 0);
	sprintf(buf, "Set %s: ", prompt_field[field]);
	lcd_puts2(buf);
}

void print_invalid_prompt(char field) {
	char buf[32];
	lcd_clr();
	
	// Tell user their input was invalid
	lcd_pos(0, 0);
	sprintf(buf, "Invalid %s", prompt_field[field]);
	lcd_puts2(buf);
	
	// Print prompts 'Set Year: ', 'Set Month: ', 'Set Day: ', asking user for input
	lcd_pos(1, 0);
	sprintf(buf, "Set %s: ", prompt_field[field]);
	lcd_puts2(buf);
}

void clear_input_buffer() {
	// Reset the index to the start of the buffer
	buff_index = 0;
	// Clear the input buffer to ask for new input in the future
	input_buff[0] = '\0';
}

void add_key(char key) {
	// Add the key to the specified buffer index and then increment the index
	input_buff[buff_index] = key;
	++buff_index;
	// Place a terminating character to end the string
	input_buff[buff_index] = '\0';
	// Update the LCD to show the inputted key
	lcd_put(key);
}

void key_pressed(char key, DateTime *dt) {
	// If key is 'A' -> toggle between military mode and am/pm time -> Exit function early
	if (key == 'A') {
		military_mode = !military_mode;
		print_dt(dt);
		return;
	}
	
	// When '*' is pressed in RUNNING state -> User wants to set date/time on clock -> Enter SET_YEAR state
	// When '#' is pressed in "SET_*" states -> User is done entering input for that field...
			// If input valid, enter next state (for SET_SECOND, enter STATIC) and ask for new input
			// If input invalid, tell user that the field is invalid and ask for input again
	// When keys 1-9 are pressed in "SET_*" states -> Add to the input buffer and update the LCD
	// When '*' is pressed in STATIC state -> User is done setting the clock -> Return to RUNNING state
	
	switch(CurrState) {
		case RUNNING:
			if (key == '*') {
				CurrState = SET_YEAR;
				print_prompt(0);
			}
			break;
		case SET_YEAR:
			if (key == '#') {
				if (strlen(input_buff) != 4) {
					clear_input_buffer();
					print_invalid_prompt(0);
				}
				else {
					dt->year = atoi(input_buff);
					clear_input_buffer();
					leap_year_logic(dt->year);
					CurrState = SET_MONTH;
					print_prompt(1);
				}
			}
			else if (key >= '0' && key <= '9' && buff_index <= 3) {
				add_key(key);
			}
			break;
		case SET_MONTH:
			if (key == '#') {
				if (strlen(input_buff) == 0) {
					clear_input_buffer();
					print_invalid_prompt(1);
				}
				else {
					int month = atoi(input_buff);
					clear_input_buffer();
					if (month >= 1 && month <= 12) {
						dt->month = month;
						CurrState = SET_DAY;
						print_prompt(2);
					}
					else {
						print_invalid_prompt(1);
					}
				}
			}
			else if (key >= '0' && key <= '9' && buff_index <= 1) {
				add_key(key);
			}
			break;
		case SET_DAY:
			if (key == '#') {
				if (strlen(input_buff) == 0) {
					clear_input_buffer();
					print_invalid_prompt(2);
				}
				else {
					int day = atoi(input_buff);
					clear_input_buffer();
					if (day >= 1 && day <= days_per_month[dt->month - 1]) {
						dt->day = day;
						CurrState = SET_HOUR;
						print_prompt(3);
					}
					else {
						print_invalid_prompt(2);
					}
				}
			}
			else if (key >= '0' && key <= '9' && buff_index <= 1) {
				add_key(key);
			}
			break;
		case SET_HOUR:
			if (key == '#') {
				if (strlen(input_buff) == 0) {
					clear_input_buffer();
					print_invalid_prompt(3);
				}
				else {
					int hour = atoi(input_buff);
					clear_input_buffer();
					if (hour >= 0 && hour <= 23) {
						dt->hour = hour;
						CurrState = SET_MINUTE;
						print_prompt(4);
					}
					else {
						print_invalid_prompt(3);
					}
				}
			}
			else if (key >= '0' && key <= '9' && buff_index <= 1) {
				add_key(key);
			}
			break;
		case SET_MINUTE:
			if (key == '#') {
				if (strlen(input_buff) == 0) {
					clear_input_buffer();
					print_invalid_prompt(4);
				}
				else {
					int minute = atoi(input_buff);
					clear_input_buffer();
					if (minute >= 0 && minute <= 59) {
						dt->minute = minute;
						CurrState = SET_SECOND;
						print_prompt(5);
					}
					else {
						print_invalid_prompt(4);
					}
				}
			}
			else if (key >= '0' && key <= '9' && buff_index <= 1) {
				add_key(key);
			}
			break;
		case SET_SECOND:
			if (key == '#') {
				if (strlen(input_buff) == 0) {
					clear_input_buffer();
					print_invalid_prompt(5);
				}
				else {
					int second = atoi(input_buff);
					clear_input_buffer();
					if (second >= 0 && second <= 59) {
						dt->second = second;
						CurrState = STATIC;
						print_dt(dt);
					}
					else {
						print_invalid_prompt(5);
					}
				}
			}
			else if (key >= '0' && key <= '9' && buff_index <= 1) {
				add_key(key);
			}
			break;
		case STATIC:
			if (key == '*') {
				CurrState = RUNNING;
			}
			break;
	}
}


int main() {
	// Initialization functions
	DateTime dt;
	avr_init();
	lcd_init();
	init_dt(&dt);
	
	CurrState = RUNNING;
	
	while (1) {
		// Check for key press
		int key = get_key();
		if (key != 0) {     // If key is not 0 -> key press occurred
			key_pressed(keys[key - 1], &dt);
			// Adds a delay after a valid key press
			// This ensures 1 key press isn't registered multiple times
			// Shorter delay than 1000 ms b/c you want to check key presses more frequently
			avr_wait(250);
		}
		
		if (CurrState == RUNNING) {
			// Wait 1 second (1000 ms) to update time
			// Only wait 1 second when in RUNNING state. Otherwise, you're waiting for a key press
			avr_wait(1000);
			// Advance the digital clock
			advance_dt(&dt);
			// Update the time displayed on the LCD
			print_dt(&dt);
		}
	}
	return 0;
}
