import serial
import time
import csv
from datetime import datetime

# Seri port ayarları
COM_PORT = 'COM11'       # Windows için örnek, Linux/Mac: '/dev/ttyUSB0' veya benzeri
BAUD_RATE = 115200      # ESP32 kodu ile uyumlu olmalı

ser = serial.Serial(COM_PORT, BAUD_RATE)
time.sleep(2)  # Bağlantının oturması için kısa bir gecikme

with open('sensor_log12.csv', 'w', newline='') as csvfile:
    csv_writer = csv.writer(csvfile)
    csv_writer.writerow(["Timestamp", "Temperature", "Humidity"])  # Başlık satırı

    print("Veri kaydı başlatıldı. Çıkmak için Ctrl+C basınız...")

    try:
        while True:
            line = ser.readline().decode('utf-8').strip()
            if line:
                # Beklenen format: "temperature,humidity"
                data = line.split(',')
                if len(data) == 2:
                    # Zaman damgasını sistemden al
                    timestamp = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                    csv_writer.writerow([timestamp, data[0], data[1]])
                    print([timestamp, data[0], data[1]])
    except KeyboardInterrupt:
        print("Kayıt sonlandırıldı.")

ser.close()


