"""
Main
"""

import RPi_I2C_driver
from time import *
import serial
import pymongo
import ssl
import sqlite3
import datetime
import Adafruit_DHT

connstring=""
with (open("secret.txt","r")) as fr:
      connstring=fr.readline()

sensor1=Adafruit_DHT.DHT11
pin1=24

sensor2=Adafruit_DHT.DHT22
pin2=5

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
print(con.readlines(8))
##for i in range(8):
##    print(con.readline())

# retrieve and display temperature and weight readings
while True:
    try:
        reading=con.readline().decode("utf-8").split(",")
        assert len(reading)==6
        lcd.lcd_clear()
        lcd.lcd_display_string("Temp1: "+reading[3]+" °C",1)
        lcd.lcd_display_string("Weight: "+reading[1]+reading[2],2)

    except AssertionError:
        print("Assertion Error: Unexpected length of reading from the scale.")

    finally:
        print(reading)
        sleep(5)

    try: 
        reading2=Adafruit_DHT.read_retry(sensor1,pin1)
        assert len(reading2)==2
        humidity1, temp2 = reading2
        lcd.lcd_clear()
        lcd.lcd_display_string("Temp2: "+ str(temp2)+" °C",1)
        lcd.lcd_display_string("Humidity: "+str(humidity1)+" %",2)

    except AssertionError:
        print("Error - reading from the DHT sensor connected to pin "+str(pin1))

    finally:
        print(reading2)    
        sleep(5)

    try:
        reading3=Adafruit_DHT.read_retry(sensor2,pin2)
        humidity2, temp3=reading3
        print(temp3,humidity2)
        lcd.lcd_clear()
        lcd.lcd_display_string("Temp3: "+str(temp3)+" °C",1)
        lcd.lcd_display_string("Humidity2: "+str(humidity2)+"%",2)

    except AssertionError:
        print("Error - reading from the DHT sensor connected to pin "+str(pin1))

    finally:
        print(reading3)    
        sleep(5)


con.close()


