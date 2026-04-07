 #include <stdio.h>
#include <unistd.h>
#include <altera_avalon_i2c.h>
#include <altera_avalon_i2c_regs.h>
#include "system.h"
#include <string.h>
#include <stdint.h>
#include <altera_avalon_pio_regs.h>
#define HP206C_I2C_ADDR 0x76 //  I2C address of HP206C
#define SLAVE_ADDR 0x3C  //  display's I2C address
#define DHT11_PIN DHT11_BASE  //  GPIO base address

//////////////////////// DHT11 start funcations ////////////////////

void sendStartSignal() {
    IOWR_ALTERA_AVALON_PIO_DIRECTION(DHT11_PIN, 1); // Set as output
    IOWR_ALTERA_AVALON_PIO_DATA(DHT11_PIN, 0);      // Pull low
    usleep(18000);                                  // At least 18ms
    IOWR_ALTERA_AVALON_PIO_DATA(DHT11_PIN, 1);      // Pull high
    usleep(40);                                     // 20-40us
}

int waitForResponse() {
    int counter;

    IOWR_ALTERA_AVALON_PIO_DIRECTION(DHT11_PIN, 0); // Set as input

    counter = 0;
    while (!IORD_ALTERA_AVALON_PIO_DATA(DHT11_PIN)) { // Wait for low
        if (++counter > 100) {
            return -1; // Timeout
        }
        usleep(1);
    }

    counter = 0;
    while (IORD_ALTERA_AVALON_PIO_DATA(DHT11_PIN)) { // Wait for high
        if (++counter > 100) {
            return -1; // Timeout
        }
        usleep(1);
    }

    return 0;
}

unsigned char readByte() {
    unsigned char data = 0;
    int i, j;

    for (i = 0; i < 8; i++) {
        j = 0;
        while (!IORD_ALTERA_AVALON_PIO_DATA(DHT11_PIN)) { // Wait for low
            if (++j > 100) {
                return -1; // Timeout
            }
            usleep(1);
        }

        usleep(35); // Wait for 28-50us

        if (IORD_ALTERA_AVALON_PIO_DATA(DHT11_PIN)) {
            data |= (1 << (7 - i));
        }

        j = 0;
        while (IORD_ALTERA_AVALON_PIO_DATA(DHT11_PIN)) { // Wait for high
            if (++j > 100) {
                return -1; // Timeout
            }
            usleep(1);
        }
    }
    return data;
}

int readDHT11(unsigned char *humidity, unsigned char *temperature) {
    unsigned char hum_int, hum_dec, temp_int, temp_dec, checksum, computed_checksum;

    sendStartSignal();

    if (waitForResponse() == -1) {
        return -1; // Sensor not responding
    }

    hum_int = readByte();
    hum_dec = readByte();
    temp_int = readByte();
    temp_dec = readByte();
    checksum = readByte();

    computed_checksum = hum_int + hum_dec + temp_int + temp_dec;
    if (checksum != computed_checksum) {
        return -2; // Checksum error
    }

    *humidity = hum_int;
    *temperature = temp_int;
    return 0; // Success
}
/////////////////////////////////////// DHT11 end funcations ////////////////////////







//////////////// lCD start funcations /////////////////////
void write_to_display(ALT_AVALON_I2C_DEV_t *i2c_dev, uint8_t *data, size_t len) {
    ALT_AVALON_I2C_STATUS_CODE status;

    alt_avalon_i2c_master_target_set(i2c_dev, SLAVE_ADDR);

    // Send the data to the I2C device
    status = alt_avalon_i2c_master_tx(i2c_dev, data, len, ALT_AVALON_I2C_NO_INTERRUPTS);

    if (status != ALT_AVALON_I2C_SUCCESS) {
        printf("I2C write failed\n");
    }
}

void write_command(ALT_AVALON_I2C_DEV_t *i2c_dev, uint8_t cmd) {
    uint8_t buffer[2] = {0x00, cmd}; // Control byte followed by command
    write_to_display(i2c_dev, buffer, 2);
}

void write_data(ALT_AVALON_I2C_DEV_t *i2c_dev, uint8_t data) {
    uint8_t buffer[2] = {0x40, data}; // Control byte followed by data
    write_to_display(i2c_dev, buffer, 2);
}

void init_display(ALT_AVALON_I2C_DEV_t *i2c_dev) {
    // Basic Initialization commands
    write_command(i2c_dev, 0x3A); // Function Set
    write_command(i2c_dev, 0x09); // Extended function set
    write_command(i2c_dev, 0x06); // Entry mode set
    write_command(i2c_dev, 0x1E); // Bias setting
    write_command(i2c_dev, 0x39); // Function Set
    write_command(i2c_dev, 0x1B); // Internal OSC
    write_command(i2c_dev, 0x6E); // Follower control
    write_command(i2c_dev, 0x56); // Power control
    write_command(i2c_dev, 0x7A); // Contrast Set
    write_command(i2c_dev, 0x38); // Function Set
    write_command(i2c_dev, 0x0F); // Display On

    // Select ROM B for character set
    write_command(i2c_dev, 0x3A); // Function Set: RE=1
    write_command(i2c_dev, 0x72); // ROM Selection command
    write_data(i2c_dev, 0x04);    // Select ROM B
    write_command(i2c_dev, 0x01);
    write_command(i2c_dev, 0x38); // Function Set: RE=0


    usleep(10000);  // Wait for commands to take effect
}

void clear_display(ALT_AVALON_I2C_DEV_t *i2c_dev) {
    write_command(i2c_dev, 0x01); // Clear display command
    usleep(2000); // Wait for clear display command to complete
}


void write_string(ALT_AVALON_I2C_DEV_t *i2c_dev, const char *str, uint8_t line) {
    uint8_t addr;
    switch (line) {
        case 1: addr = 0x80; break; // First line starts at DDRAM address 0x00
        case 2: addr = 0xA0; break; // Second line starts at DDRAM address 0x20
        case 3: addr = 0xC0; break; // Third line starts at DDRAM address 0x40
        case 4: addr = 0xE0; break; // Fourth line starts at DDRAM address 0x60
        default: return; // Invalid line number
    }

    write_command(i2c_dev, addr); // Set DDRAM address

    while (*str) {
        write_data(i2c_dev, *str++); // Write each character
    }
}

////end of LCD funcations ///////////////////////////////////////


//////////////////  HP206C sensor funcations  //////////////////////

union hp206c_RawData {
    unsigned char bytes[4];
    long value;
};

// Prototypes for HP206C functions
void hp206c_reset(ALT_AVALON_I2C_DEV_t *hp206c_i2c_dev);
int read_HP206C(int *temperature, int *pressure);

// HP206C Functions
void hp206c_reset(ALT_AVALON_I2C_DEV_t *hp206c_i2c_dev) {
	 alt_u8 hp206c_txbuffer[1];
	    ALT_AVALON_I2C_STATUS_CODE hp206c_status;

	    // Set the address of the HP206C sensor
	    alt_avalon_i2c_master_target_set(hp206c_i2c_dev, HP206C_I2C_ADDR);

	    // Send the reset command (replace 0x0B with the actual reset command for HP206C)
	    hp206c_txbuffer[0] = 0x06; // Example reset command (replace with actual command)
	    hp206c_status = alt_avalon_i2c_master_tx(hp206c_i2c_dev, hp206c_txbuffer, 1, ALT_AVALON_I2C_NO_INTERRUPTS);

	    if (hp206c_status != ALT_AVALON_I2C_SUCCESS) {
	        //printf("Reset Failed: %d \n", hp206c_status);
	    } else {
	        //printf("Sensor Reset Successfully\n");
	    }

	    // Add a delay after the reset to allow time for the sensor to recover
	    usleep(10000); // Example: 10ms (adjust as needed)
}

int read_HP206C(int *temperature, int *pressure) {

    ALT_AVALON_I2C_DEV_t *hp206c_i2c_dev;
    hp206c_i2c_dev = alt_avalon_i2c_open("/dev/I2C");
    if (hp206c_i2c_dev == NULL) {
        alt_printf("Error: Cannot find /dev/i2c_0\n");
        return 1;
    }

    //HP206C START
    alt_u8 hp206c_txbuffer[1];
    alt_u8 hp206c_rxbuffer[6];
    ALT_AVALON_I2C_STATUS_CODE hp206c_status;
    //HP206C END
    while (1) {
        // HP206C START
        hp206c_reset(hp206c_i2c_dev);
        alt_avalon_i2c_master_target_set(hp206c_i2c_dev, HP206C_I2C_ADDR);
        // Send the ADC_CVT command
        hp206c_txbuffer[0] = 0x50; // Example ADC_CVT command (replace with actual command)
        hp206c_status = alt_avalon_i2c_master_tx(hp206c_i2c_dev, hp206c_txbuffer, 1, ALT_AVALON_I2C_NO_INTERRUPTS);
        usleep(10000);
        if (hp206c_status != ALT_AVALON_I2C_SUCCESS) {
            printf("ADC_CVT Command Failed: %d \n", hp206c_status);
            return 1;
        }

        // Start pressure and temperature measurement
        hp206c_txbuffer[0] = 0x10; // HP206C command for pressure and temperature measurement
        hp206c_status = alt_avalon_i2c_master_tx(hp206c_i2c_dev, hp206c_txbuffer, 1, ALT_AVALON_I2C_NO_INTERRUPTS);

        if (hp206c_status != ALT_AVALON_I2C_SUCCESS) {
            printf("Start Measurement Failed: %d \n", hp206c_status);
            return 1;
        }
        // Wait for measurement to complete
        usleep(1000000); // Example: 100ms
        // Read the measurement data
        hp206c_status = alt_avalon_i2c_master_rx(hp206c_i2c_dev, hp206c_rxbuffer, 6, ALT_AVALON_I2C_NO_INTERRUPTS);
        if (hp206c_status != ALT_AVALON_I2C_SUCCESS) {
            printf("Read Measurement Failed: %d \n", hp206c_status);
            return 1;
        }
        // Process the received data as needed
        union hp206c_RawData temperatureData, pressureData;
        temperatureData.bytes[2] = hp206c_rxbuffer[0];
        temperatureData.bytes[1] = hp206c_rxbuffer[1];
        temperatureData.bytes[0] = hp206c_rxbuffer[2];
        temperatureData.bytes[3] = 0;
        pressureData.bytes[2] = hp206c_rxbuffer[3];
        pressureData.bytes[1] = hp206c_rxbuffer[4];
        pressureData.bytes[0] = hp206c_rxbuffer[5];
        pressureData.bytes[3] = 0;
    // Populate temperature and pressure from sensor data
    *temperature = (int)(float)temperatureData.value / 100;
    *pressure = (int)(float)pressureData.value / 100;

    return 0; // Success
}}




//////////////////////HP206C FUNCTIONS END/////////////


/// led blink ///
// Function definition
void LED_BLINK() {



        // Turn off both LEDs
        IOWR_ALTERA_AVALON_PIO_DATA(LED_GREEN_BASE, 0xFFFFFFFF);
        IOWR_ALTERA_AVALON_PIO_DATA(LED_RED_BASE, 0xFFFFFFFF);

        usleep(500000);  // Wait for 500 milliseconds

        // Turn on both LEDs
        IOWR_ALTERA_AVALON_PIO_DATA(LED_GREEN_BASE, 0x0);
        IOWR_ALTERA_AVALON_PIO_DATA(LED_RED_BASE, 0x0);

        usleep(500000);  // Wait for another 500 milliseconds



}

////end of led blink //////
int main() {
    ALT_AVALON_I2C_DEV_t *i2c_dev = alt_avalon_i2c_open("/dev/I2C");
    if (i2c_dev == NULL) {
        printf("Error: Cannot find /dev/I2C_Master\n");
        return -1;
    }

    init_display(i2c_dev);
    clear_display(i2c_dev);

    unsigned char humidity, temperature_DHT11;
    int temperature_HP206C, pressure, status_HP206C;
    int status_DHT11;
    int temperature_limit = 30;
    int pressure_limit =1000;
    int humidity_limit = 50;
    enum { SENSOR_READINGS, TEMP_LIMIT, PRESSURE_LIMIT, HUMIDITY_LIMIT } currentScreen = SENSOR_READINGS;

    while (1) {

        // Turn off both LEDs
        IOWR_ALTERA_AVALON_PIO_DATA(LED_GREEN_BASE, 0xFFFFFFFF);
        IOWR_ALTERA_AVALON_PIO_DATA(LED_RED_BASE, 0xFFFFFFFF);
        // Read UP and DOWN button states
        int upButtonPressed = (IORD_ALTERA_AVALON_PIO_DATA(UP_BASE) & 0x01);
        int downButtonPressed = (IORD_ALTERA_AVALON_PIO_DATA(DOWN_BASE) & 0x01);

        // Change screen based on button press
        if (upButtonPressed) {
            currentScreen = (currentScreen + 1) % 4; // Cycles through the screens
            usleep(200000); // Debounce delay.
        }
            else if (downButtonPressed) {
                currentScreen = (currentScreen - 1) % 4; // Cycles through the screens
                usleep(200000); // Debounce delay
        }


        // Display based on current screen mode
        if (currentScreen == SENSOR_READINGS) {
            // Sensor reading code
            status_DHT11 = readDHT11(&humidity, &temperature_DHT11);
            status_HP206C = read_HP206C(&temperature_HP206C, &pressure);

            if (status_DHT11 != -1 && status_DHT11 != -2 && status_HP206C == 0) {
                int avr_temperature = (temperature_DHT11 + temperature_HP206C) / 2;
                char tempStr[32], humStr[32], prsStr[32];
                snprintf(tempStr, sizeof(tempStr), "Temp: %dC", avr_temperature);
                snprintf(humStr, sizeof(humStr), "Hum:  %d%%", humidity);
                snprintf(prsStr, sizeof(prsStr), "Pres: %dhPa", pressure);
                clear_display(i2c_dev);
                write_string(i2c_dev, tempStr, 1);
                write_string(i2c_dev, humStr, 3);
                write_string(i2c_dev, prsStr, 4);

                if (avr_temperature >= temperature_limit || pressure >= pressure_limit || humidity >= humidity_limit) {
                    for (int i = 0; i < 5; ++i) {  // Flash the screen 5 times
                        clear_display(i2c_dev);


                        // Check for button press inside the loop
                        upButtonPressed = (IORD_ALTERA_AVALON_PIO_DATA(UP_BASE) & 0x01);
                        downButtonPressed = (IORD_ALTERA_AVALON_PIO_DATA(DOWN_BASE) & 0x01);

                        // If any button is pressed, break out of the loop
                        if (upButtonPressed || downButtonPressed) {
                            break;
                        }

                        if (avr_temperature >= temperature_limit) {
                            write_string(i2c_dev, "Temp High", 2);
                        }
                        if (pressure >= pressure_limit) {
                            write_string(i2c_dev, "Press High", 3);
                        }
                        if (humidity >= humidity_limit) {
                            write_string(i2c_dev, "Humid High", 4);
                        }

                        LED_BLINK();  // Blink LED

                        usleep(250000); // Wait for 250 ms
                    }
                    clear_display(i2c_dev);  // Clear the display after flashing
                }

            }
        } else if (currentScreen == TEMP_LIMIT) {
        	// Display temperature limit screen
        	clear_display(i2c_dev);
        	write_string(i2c_dev, "Temp Limit", 1); // Display "Temp Limit" on the first line

        	// Prepare the temperature limit value string
        	char tempLimitStr[32];
        	snprintf(tempLimitStr, sizeof(tempLimitStr), "-  %dC   +", temperature_limit);

        	// Display the temperature limit value centered on the second line
        	write_string(i2c_dev, tempLimitStr, 3);

            int rightButtonPressed = (IORD_ALTERA_AVALON_PIO_DATA(RIGHT_BASE) & 0x01);
            int leftButtonPressed = (IORD_ALTERA_AVALON_PIO_DATA(LEFT_BASE) & 0x01);

            // Adjust temperature limit based on button presses
                    if (rightButtonPressed) {
                        temperature_limit++;
                        usleep(200000); // Debounce delay
                    }
                    if (leftButtonPressed) {
                        temperature_limit--;
                        usleep(200000); // Debounce delay
                    }

        }

        // ...

        else if (currentScreen == PRESSURE_LIMIT) {
            // Display pressure limit screen
            clear_display(i2c_dev);
            write_string(i2c_dev, "Pres Limit", 1);

            char pressureLimitStr[32];
            snprintf(pressureLimitStr, sizeof(pressureLimitStr), "-%dhPa+ ",pressure_limit);
            write_string(i2c_dev, pressureLimitStr, 3);

            int rightButtonPressedPressure = (IORD_ALTERA_AVALON_PIO_DATA(RIGHT_BASE) & 0x01);
            int leftButtonPressedPressure = (IORD_ALTERA_AVALON_PIO_DATA(LEFT_BASE) & 0x01);

            if (rightButtonPressedPressure) {
                pressure_limit++;
                usleep(200000); // Debounce delay
            }
            if (leftButtonPressedPressure) {
                pressure_limit--;
                usleep(200000); // Debounce delay
            }
        } else if (currentScreen == HUMIDITY_LIMIT) {
            // Display humidity limit screen
            clear_display(i2c_dev);
            write_string(i2c_dev, "Hum Limit", 1);

            char humidityLimitStr[32];
            snprintf(humidityLimitStr, sizeof(humidityLimitStr), "-  %d%%   +", humidity_limit);
            write_string(i2c_dev, humidityLimitStr, 3);

            int rightButtonPressedHumidity = (IORD_ALTERA_AVALON_PIO_DATA(RIGHT_BASE) & 0x01);
            int leftButtonPressedHumidity = (IORD_ALTERA_AVALON_PIO_DATA(LEFT_BASE) & 0x01);

            if (rightButtonPressedHumidity) {
                humidity_limit++;
                usleep(200000); // Debounce delay
            }
            if (leftButtonPressedHumidity) {
                humidity_limit--;
                usleep(200000); // Debounce delay
            }
        }



        // Small delay for main loop
        usleep(100000); // 100ms delay
    }

    return 0;
}
