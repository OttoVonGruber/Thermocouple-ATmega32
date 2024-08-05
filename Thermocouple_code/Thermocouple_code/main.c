#define F_CPU 1000000 // Set the CPU frequency to 1 MHz
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <math.h>

//[-------------Global---------------]
#define RS 7 // Register Select pin for LCD
#define E 6  // Enable pin for LCD

//[-------------Prototyp---------------]
void lcd_init(void); // Initializes the LCD
void lcd_cmd(uint8_t cmd); // Sends a command to the LCD
void lcd_data(char data); // Sends data (a character) to the LCD
void lcd_print(char *str); // Prints a string on the LCD
void lcd_set_cursor(uint8_t row, uint8_t col); // Sets the cursor position on the LCD
void display_temperatures_test(float thermocouple_temp, float internal_temp); // Displays thermocouple and internal temperatures on the LCD
void int_to_string_fixed(char* buffer, int value, uint8_t width); // Converts an integer to a fixed-width string

void spi_init(void); // Initializes SPI communication
uint8_t spi_transfer(void); // Transfers data over SPI and receives the response
int32_t read_max31855(void); // Reads a value from the MAX31855 thermocouple amplifier
float get_thermocouple_temp(int32_t value); // Converts raw value to thermocouple temperature
float get_internal_temp(int32_t value); // Converts raw value to internal temperature


int main(void)
{
	lcd_init();
	spi_init();
	
	while (1)
	{
		int32_t value = read_max31855();
		float thermocouple_temp = get_thermocouple_temp(value);
		float internal_temp = get_internal_temp(value);
		
		display_temperatures_test(thermocouple_temp, internal_temp);
		_delay_ms(1000);
	}
}

//[------------Function-----------]
void lcd_init(void) {
	DDRD = 0xFF; // Set PORTD as output for LCD data
	DDRC |= ((1 << E) | (1 << RS)); // Set RS and E as output
	_delay_ms(100); // Delay for LCD power on
	lcd_cmd(0x30); // Initialize LCD in 8-bit mode
	lcd_cmd(0x30); // Repeat initialization
	lcd_cmd(0x30); // Repeat initialization
	lcd_cmd(0x38); // Function set: 8-bit, 2 line, 5x7 dots
	lcd_cmd(0x0E); // Display on, cursor on
	lcd_cmd(0x06); // Entry mode, increment cursor
	lcd_cmd(0x01); // Clear display
}

void lcd_cmd(uint8_t cmd) {
	PORTC &= ~(1 << RS); // RS = 0 for command
	PORTD = cmd; // Put command on data bus
	PORTC |= (1 << E); // Enable pulse
	_delay_us(5);
	PORTC &= ~(1 << E); // Disable pulse
	_delay_ms(2); // Wait for command to be processed
}

void lcd_data(char data) {
	PORTC |= (1 << RS); // RS = 1 for data
	PORTD = data; // Put data on data bus
	PORTC |= (1 << E); // Enable pulse
	_delay_us(5);
	PORTC &= ~(1 << E); // Disable pulse
	_delay_ms(1); // Wait for data to be processed
}

void lcd_print(char *str) {
	while (*str) {
		lcd_data(*str++); // Send characters one by one
	}
}

void lcd_set_cursor(uint8_t row, uint8_t col) {
	uint8_t address = (row == 0 ? 0x80 : 0xC0) + col; // Calculate address
	lcd_cmd(address); // Send command to set cursor
}

void spi_init(void) {
	DDRB = (1 << PB7) | (1 << PB4); // Set SCK and SS as output
	DDRB &= ~(1 << PB6); // Set MISO as input
	SPCR = (1 << SPE) | (1 << MSTR); // Enable SPI, set as master
	SPSR = (1 << SPI2X); // Enable double speed
}

uint8_t spi_transfer(void) {
	SPDR = 0x00; // Start transmission with dummy data
	while (!(SPSR & (1 << SPIF))); // Wait for transmission complete
	return SPDR; // Return received data
}

int32_t read_max31855(void) {
	int32_t value = 0;
	PORTB &= ~(1 << PB4); // CS Low to start communication
	_delay_us(1);
	value |= ((int32_t)spi_transfer() << 24); // Read MSB
	value |= ((int32_t)spi_transfer() << 16);
	value |= ((int32_t)spi_transfer() << 8);
	value |= (int32_t)spi_transfer(); // Read LSB
	PORTB |= (1 << PB4); // CS High to end communication
	return value;
}

float get_thermocouple_temp(int32_t value) {
	if (value & 0x7) return NAN; // Check for errors
	value >>= 18; // Discard unnecessary bits
	if (value & 0x2000) { // Check if negative temperature
		value |= 0xFFFFC000; // Sign extend
	}
	return value * 0.25; // Convert to Celsius
}

float get_internal_temp(int32_t value) {
	value >>= 4; // Discard unnecessary bits
	if (value & 0x800) { // Check if negative temperature
		value |= 0xFFFFF000; // Sign extend
	}
	return (value & 0xFFF) * 0.0625; // Convert to Celsius
}

void int_to_string_fixed(char* buffer, int value, uint8_t width) {
	if (value < 0) {
		*buffer++ = '-'; // Add minus sign for negative values
		value = -value;
		width--;
	}
	
	buffer += width; // Move to the end of the buffer
	*buffer-- = '\0'; // Null-terminate the string
	
	for (int i = 0; i < width; i++) {
		if (value > 0) {
			*buffer-- = (value % 10) + '0'; // Convert digit to character
			value /= 10;
			} else {
			*buffer-- = '0'; // Fill remaining width with zeros
		}
	}
}

void display_temperatures_test(float thermocouple_temp, float internal_temp) {
	char buffer[16];
	
	lcd_cmd(0x80 | 0x00);  // Move cursor to the first line
	if (!isnan(thermocouple_temp)) {
		int_to_string_fixed(buffer, (int)thermocouple_temp, 4); // Convert temperature to string
		lcd_print("Temp:");
		lcd_print(buffer);
		lcd_print("_C");
		} else {
		lcd_print("Temp:Error");
	}
	
	lcd_cmd(0x80 | 0x40);  // Move cursor to the second line
	if (!isnan(internal_temp)) {
		int_to_string_fixed(buffer, (int)internal_temp, 4); // Convert temperature to string
		lcd_print("Cold:");
		lcd_print(buffer);
		lcd_print("_C");
		} else {
		lcd_print("Cold:Error");
	}
}

