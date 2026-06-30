from flask import Flask, request, jsonify, send_from_directory
import mysql.connector
import json
from datetime import datetime
import time
import os
import requests
import urllib3
from flask_cors import CORS
from dotenv import load_dotenv
import sys
from decimal import Decimal
import pytz
import socket

static_dir = os.environ.get('STATIC_FOLDER', '/app/client/build')
app = Flask(__name__, static_folder=static_dir, static_url_path='/')

CORS(app)
load_dotenv()

# Env Config
DB_USER = os.getenv('DB_USER')
DB_PASS = os.getenv('DB_PASS')
DB_HOST = os.getenv('DB_HOST')
DB_NAME = os.getenv('DB_NAME')
CL1P_TOKEN = os.getenv('CL1P_TOKEN')
CL1P_URL = os.getenv('CL1P_URL')
LOCATION = os.getenv('LOCATION')

HEATER_ESP_IP = os.getenv('HEATER_ESP_IP')
COLLECTOR_HOST_PORT = os.getenv('COLLECTOR_HOST_PORT')

db_config = {
    'host': DB_HOST,
    'user': DB_USER,
    'password': DB_PASS,
    'database': DB_NAME
}

def datetime_handler(x):
    if isinstance(x, datetime):
        return x.isoformat()
    if isinstance(x, Decimal):
        return float(x)
    raise TypeError(f"Unknown type: {type(x)}")

def get_db_connection():
    try:
        return mysql.connector.connect(**db_config)
    except mysql.connector.Error as err:
        sys.stderr.write(f"DB Error: {err}\n")
        return None

def bootstrap_db():
    conn = get_db_connection()
    if not conn: return
    cursor = conn.cursor()
    cursor.execute("""
        CREATE TABLE IF NOT EXISTS heaterData (
            id INT AUTO_INCREMENT PRIMARY KEY,
            datetime TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
            tempBox DECIMAL(5,2),
            tempHeater DECIMAL(5,2),
            fan INT,
            light INT,
            heater INT,
            control INT,
            extra INT,
            setpoint DECIMAL(5,2),
            rssi INT       
        )
    """)
    conn.commit()
    cursor.close()
    conn.close()


@app.route('/api/send-command', methods=['POST'])
def send_command():
    data = request.json
    print(f"DEBUG: Received command: {data.get('hex')}", flush=True)
    hex_str = data.get('hex') 
    
    try:
        prefix = b'\xAA'
        
        payload = hex_str.encode('ascii')
        
        command_bytes = prefix + payload
        
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.settimeout(2)
            s.connect((HEATER_ESP_IP, int(COLLECTOR_HOST_PORT)))
            s.sendall(command_bytes)
            # Wait for 1-byte ACK from ESP
            ack = s.recv(1)
            print(f"DEBUG: Received ACK: {ack}", flush=True)
            if ack == b'\x06': # ACK byte
                return jsonify({"status": "success"}), 200
            return jsonify({"status": "fail", "error": "No ACK"}), 500
            
        return jsonify({"status": "success"}), 200
    except Exception as e:
        result = f"Error: {str(e)}"
        print(f"DEBUG: Socket send result: {result}", flush=True)
        return jsonify({"status": "fail", "error": str(e)}), 500

@app.route('/', defaults={'path': ''})
@app.route('/<path:path>')
def serve(path):
    if path != "" and os.path.exists(os.path.join(app.static_folder, path)):
        return send_from_directory(app.static_folder, path)
    return send_from_directory(app.static_folder, 'index.html')

@app.route('/api/heaterData')
def get_data():
    try:
        hours = request.args.get('hours', default=24, type=int)
        conn = get_db_connection()
        cursor = conn.cursor(dictionary=True)
        query = """
            SELECT datetime, tempBox, tempHeater, fan, light, heater, control, extra, setpoint, rssi 
            FROM heaterData 
            WHERE datetime > NOW() - INTERVAL %s HOUR 
            ORDER BY datetime DESC;
        """
        cursor.execute(query, (hours,))
        rows = cursor.fetchall()
        cursor.close()
        conn.close()
        return app.response_class(
            response=json.dumps(rows, default=datetime_handler),
            status=200, mimetype='application/json'
        )
    except Exception as e:
        return jsonify({"error": str(e)}), 500

# @app.route('/api/cl1p', methods=['POST'])
# def handle_cl1p_sync():
#     urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
#     headers = {"Content-Type": "text/plain", "cl1papitoken": CL1P_TOKEN}

#     try:
#         conn = get_db_connection()
#         cursor = conn.cursor(dictionary=True)
        
#         if LOCATION == "home":
#             query = "SELECT * FROM heaterData WHERE datetime >= NOW() - INTERVAL 7 DAY"
#             cursor.execute(query)
#             rows = cursor.fetchall()
            
#             processed = []
#             for row in rows:
#                 row['datetime'] = row['datetime'].strftime('%Y-%m-%d %H:%M:%S')
#                 if isinstance(row['tempBox'], Decimal): row['tempBox'] = float(row['tempBox'])
#                 if isinstance(row['tempHeater'], Decimal): row['tempHeater'] = float(row['tempHeater'])
#                 processed.append(row)

#             requests.post(CL1P_URL, data=json.dumps(processed), headers=headers, verify=False)
#             return jsonify({"status": "pushed", "count": len(processed)})

#         elif LOCATION == "work":
#             response = requests.get(CL1P_URL, headers=headers, verify=False)
#             if response.status_code == 200:
#                 items = json.loads(response.text)
#                 added = 0
#                 for item in items:
#                     cursor.execute("SELECT id FROM heaterData WHERE datetime = %s", (item['datetime'],))
#                     if not cursor.fetchone():
#                         iq = """INSERT INTO heaterData (datetime, tempBox, tempHeater, statusBits, rssiAvg, readingCount) 
#                                 VALUES (%s, %s, %s, %s, %s, %s)"""
#                         cursor.execute(iq, (item['datetime'], item['tempBox'], item['tempHeater'], 
#                                            item['statusBits'], item['rssiAvg'], item['readingCount']))
#                         added += 1
#                 conn.commit()
#                 return jsonify({"status": "pulled", "added": added})
                
#         cursor.close()
#         conn.close()
#     except Exception as e:
#         return jsonify({"error": str(e)}), 500

@app.route('/api/time', methods=['GET'])
def get_time():
    ontario_tz = pytz.timezone('America/Toronto')
    now_ontario = datetime.now(ontario_tz)
    return jsonify({"time": now_ontario.strftime("%I:%M %p")})


if __name__ == '__main__':
    bootstrap_db()
    app.run(host='0.0.0.0', port=int(os.getenv('API_PORT', 5000)))