import serial

con=serial.Serial()
con.port="/dev/ttyUSB0"
con.baurdrate=9600

con.open()

# read heading
for i in range(7):
    print(con.readline())

for i in range(5):
    print("reading:")
    print(con.readline())

con.close()
