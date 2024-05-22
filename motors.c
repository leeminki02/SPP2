#include <stdio.h>
#include <unistd.h>

#include <wiringPi.h>
#include <wiringPiI2C.h>
// move motors connected to the raspberry pi
/* GPIO ports
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 |     |     |    3.3v |      |   |  1 || 2  |   |      | 5v      |     |     |
 |   2 |   8 |   SDA.1 | ALT0 | 1 |  3 || 4  |   |      | 5v      |     |     |
 |   3 |   9 |   SCL.1 | ALT0 | 1 |  5 || 6  |   |      | 0v      |     |     |
 |   4 |   7 | GPIO. 7 |   IN | 1 |  7 || 8  | 1 | ALT0 | TxD     | 15  | 14  |
 |     |     |      0v |      |   |  9 || 10 | 1 | ALT0 | RxD     | 16  | 15  |
 |  17 |   0 | GPIO. 0 |   IN | 1 | 11 || 12 | 0 | IN   | GPIO. 1 | 1   | 18  |
 |  27 |   2 | GPIO. 2 |   IN | 1 | 13 || 14 |   |      | 0v      |     |     |
 |  22 |   3 | GPIO. 3 |   IN | 1 | 15 || 16 | 1 | IN   | GPIO. 4 | 4   | 23  |
 |     |     |    3.3v |      |   | 17 || 18 | 0 | IN   | GPIO. 5 | 5   | 24  |
 |  10 |  12 |    MOSI | ALT0 | 1 | 19 || 20 |   |      | 0v      |     |     |
 |   9 |  13 |    MISO | ALT0 | 1 | 21 || 22 | 0 | IN   | GPIO. 6 | 6   | 25  |
 |  11 |  14 |    SCLK | ALT0 | 0 | 23 || 24 | 1 | OUT  | CE0     | 10  | 8   |
 |     |     |      0v |      |   | 25 || 26 | 1 | OUT  | CE1     | 11  | 7   |
 |   0 |  30 |   SDA.0 |   IN | 1 | 27 || 28 | 1 | IN   | SCL.0   | 31  | 1   |
 |   5 |  21 | GPIO.21 |   IN | 1 | 29 || 30 |   |      | 0v      |     |     |
 |   6 |  22 | GPIO.22 |   IN | 1 | 31 || 32 | 0 | IN   | GPIO.26 | 26  | 12  |
 |  13 |  23 | GPIO.23 |   IN | 0 | 33 || 34 |   |      | 0v      |     |     |
 |  19 |  24 | GPIO.24 |   IN | 0 | 35 || 36 | 1 | IN   | GPIO.27 | 27  | 16  |
 |  26 |  25 | GPIO.25 |   IN | 0 | 37 || 38 | 0 | IN   | GPIO.28 | 28  | 20  |
 |     |     |      0v |      |   | 39 || 40 | 0 | IN   | GPIO.29 | 29  | 21  |
 +-----+-----+---------+------+---+----++----+---+------+---------+-----+-----+
 | BCM | wPi |   Name  | Mode | V | Physical | V | Mode | Name    | wPi | BCM |
 +-----+-----+---------+------+---+---Pi 4B--+---+------+---------+-----+-----+
 */

#define LED_B 20
#define LED_R 21
#define DEVICE_ID 0x16

int main(void)
{
  // uses BCM numbering of the GPIOs and directly accesses the GPIO registers.
  wiringPiSetupGpio();

  // pin mode ..(INPUT, OUTPUT, PWM_OUTPUT, GPIO_CLOCK)
/*     pinMode(LED_B, OUTPUT);
    pinMode(LED_R, OUTPUT);

    for (int i=0; i < 10; i++) {
        digitalWrite(LED_B, HIGH);
        delay(500);
        digitalWrite(LED_R, HIGH);
        pwmToneWrite(BUZZR, 0);
        delay(500);
        digitalWrite(LED_B, LOW);
        delay(500);
        digitalWrite(LED_R, LOW);
        delay(500);
    }
 */
  // testing I2C connection
  int fd = wiringPiI2CSetup(DEVICE_ID);
  
  if (fd == -1) {
    printf("Error: cannot init I2C communication\n");
    return -1;
  }
  printf("I2C setup Completed.\n");

  // write to I2C
  // reg: 0x01, value: 1, 100, 1, 100
  // wiringPiI2CWriteReg8(fd, 0x01, 1*256^3+100*256^2+1*256+100);
  int* data = [1, 100, 1, 100];
  wiringPiI2CWriteBlockData(fd, 0x01, data, 4);
  // printf("I2C write Completed: [1, 100, 1, 100]\n");
  sleep(1);
  data = [1, 0, 1, 0]; // change back to zero values
  wiringPiI2CWriteBlockData(fd, 0x01, data, 4);
  // wiringPiI2CWriteReg8(fd, 0x01, 1*256^3+0*256^2+1*256+0);
  printf("I2C write Completed: [1, 0, 1, 0]\n");
  
  return 0;

}
