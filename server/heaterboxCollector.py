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

device_states = {}

def flush_device(device_id):
    global device_states
    state = device_states.get(device_id)
    if not state or not state["box_temps"]:
        return

    try:
        avg_box = sum(state["box_temps"]) / len(state["box_temps"])
        avg_heater = sum(state["heater_temps"]) / len(state["heater_temps"])
        avg_rssi = sum(state["rssi_values"]) / len(state["rssi_values"])
        last_status = state["statuses"][-1]
        count = len(state["box_temps"])
        now = datetime.now()

        conn_db = mysql.connector.connect(**DB_CONFIG)
        cursor = conn_db.cursor()

        query = """
            INSERT INTO heaterData 
            (datetime, tempBox, tempHeater, statusBits, rssiAvg, readingCount) 
            VALUES (%s, %s, %s, %s, %s, %s)
        """
        cursor.execute(query, (now, avg_box, avg_heater, last_status, int(avg_rssi), count))

        conn_db.commit()
        cursor.close()
        conn_db.close()

        device_states[device_id] = {
            "box_temps": [], "heater_temps": [], "statuses": [], 
            "rssi_values": [], "last_flush": now
        }

    except Exception as e:
        print(f"Flush Error: {e}")

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
                    if len(hex_str) == 12:
                        raw_box = int(hex_str[0:4], 16)
                        raw_heat = int(hex_str[4:8], 16)
                        status = int(hex_str[8:10], 16)
                        rssi = int(hex_str[10:12], 16)

                        dev_id = addr[0]
                        if dev_id not in device_states:
                            device_states[dev_id] = {
                                "box_temps": [], "heater_temps": [], 
                                "statuses": [], "rssi_values": [], "last_flush": datetime.now()
                            }

                        device_states[dev_id]["box_temps"].append(raw_box / 10.0)
                        device_states[dev_id]["heater_temps"].append(raw_heat / 10.0)
                        device_states[dev_id]["statuses"].append(status)
                        device_states[dev_id]["rssi_values"].append(rssi)

                        if datetime.now() - device_states[dev_id]["last_flush"] >= timedelta(minutes=10):
                            flush_device(dev_id)
                        conn.sendall(b"ACK")
        except Exception as e:
            time.sleep(2)

if __name__ == "__main__":
    start_collector()