"""
Main
"""

import RPi_I2C_driver
from time import *
import serial



lcd=RPi_I2C_driver.lcd()


lcd.lcd_clear()
lcd.lcd_display_string("Bienenprojekt",1)
sleep(1)
lcd.lcd_display_string("Starting!",2)
sleep(1)

con=serial.Serial()
con.port="/dev/ttyUSB0"
con.baurdrate=9600
con.open()

# read heading
for i in range(8):
    print(con.readline())

# retrieve and display temperature and weight readings
while True:
    print("reading:")
    reading=con.readline().decode("utf-8").split(",")
    print(reading)
    lcd.lcd_clear()
    lcd.lcd_display_string("Temp: "+reading[3],1)
    lcd.lcd_display_string("Weight:"+reading[1]+reading[2],2)

con.close()


