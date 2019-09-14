"""
Main 
"""

import RPi_I2C_driver
from time import *
import serial




lcd=RPi_I2C_driver.lcd()


lcd.lcd_clear()
lcd.lcd_display_string("Bienenprojekt",1)
sleep(5)
lcd.lcd_display_string("Starting!",2)
sleep(5)
lcd.lcd_display_string("                ",2)

con=serial.Serial()
con.port="/dev/ttyUSB0"
con.baurdrate=9600
con.open()

# read heading
for i in range(7):
    print(con.readline())

for i in range(5):
    print("reading:")
    reading=con.readline().decode("utf-8").split(",")
    print(reading)
    lcd.lcd_display_string(reading[1]+reading[2],2)

con.close()
