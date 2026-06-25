import os
import socket
import mysql.connector
import time
from datetime import datetime, timedelta
from dotenv import load_dotenv

load_dotenv()

DB_CONFIG = {
    'host': os.getenv('DB_HOST'),
    'user': os.getenv('DB_USER'),
    'password': os.getenv('DB_PASS'),
    'database': os.getenv('DB_NAME'),
}



def start_collector():
    server_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server_socket.bind((os.getenv('BIND_HOST'), int(os.getenv('COLLECTOR_PORT'))))
    server_socket.listen(5)

    while True:
        try:
            conn, addr = server_socket.accept()
            with conn:
                data = conn.recv(1024)
                if data:
                    hex_str = data.decode('ascii').strip()
                    # 18 chars = 9 bytes
                    if len(hex_str) == 20:
                        # Parsing the 9-byte packet
                        raw_h = int(hex_str[0:4], 16)   # Bytes 0-1
                        raw_b = int(hex_str[4:8], 16)   # Bytes 2-3
                        fan = int(hex_str[8:10], 16)    # Byte 4
                        light = int(hex_str[10:12], 16) # Byte 5
                        heater = int(hex_str[12:14], 16)# Byte 6
                        setpoint = int(hex_str[14:18], 16) # Bytes 7-8
                        rssi = int(hex_str[18:20], 16)
                        
                        conn.sendall(b"ACK")

                        now = datetime.now()

                        conn_db = mysql.connector.connect(**DB_CONFIG)
                        cursor = conn_db.cursor()

                        query = """
                            INSERT INTO heaterData 
                            (datetime, tempBox, tempHeater, fan, light, heater, setpoint, RSSI) 
                            VALUES (%s, %s, %s, %s, %s, %s, %s, %s)
                        """
                        cursor.execute(query, (now, raw_b/10, raw_h/10, fan, light, heater, setpoint/10, rssi))

                        conn_db.commit()
                        cursor.close()
                        conn_db.close()


        except Exception as e:
            time.sleep(2)

if __name__ == "__main__":
    start_collector()