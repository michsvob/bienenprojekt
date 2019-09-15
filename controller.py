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
with (open("secret.txt","r") as fr:
      connstring=fr.readline()

sensor1=Adafruit_DHT.DHT11
pin1=24


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
    sleep(5)

    humidity, temp2=Adafruit_DHT.readretry(sensor1,pin1)
    lcd.lcd_display_string("Temp2: "+ str(temp2))
    lcd.lcd_display_string("Humidity: "+str(humidity)+" %")
    sleep(5)

con.close()


