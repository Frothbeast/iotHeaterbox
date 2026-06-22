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
            statusBits INT,
            rssiAvg INT,
            readingCount INT,
            notes VARCHAR(50)
        )
    """)
    conn.commit()
    cursor.close()
    conn.close()

@app.route('/')
def serve_index():
    return send_from_directory(app.static_folder, 'index.html')

@app.route('/api/heaterData')
def get_data():
    try:
        hours = request.args.get('hours', default=24, type=int)
        conn = get_db_connection()
        cursor = conn.cursor(dictionary=True)
        query = """
            SELECT datetime, id, tempBox, tempHeater, statusBits, rssiAvg, readingCount, notes 
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

@app.route('/api/cl1p', methods=['POST'])
def handle_cl1p_sync():
    urllib3.disable_warnings(urllib3.exceptions.InsecureRequestWarning)
    headers = {"Content-Type": "text/plain", "cl1papitoken": CL1P_TOKEN}

    try:
        conn = get_db_connection()
        cursor = conn.cursor(dictionary=True)
        
        if LOCATION == "home":
            query = "SELECT * FROM heaterData WHERE datetime >= NOW() - INTERVAL 7 DAY"
            cursor.execute(query)
            rows = cursor.fetchall()
            
            processed = []
            for row in rows:
                row['datetime'] = row['datetime'].strftime('%Y-%m-%d %H:%M:%S')
                if isinstance(row['tempBox'], Decimal): row['tempBox'] = float(row['tempBox'])
                if isinstance(row['tempHeater'], Decimal): row['tempHeater'] = float(row['tempHeater'])
                processed.append(row)

            requests.post(CL1P_URL, data=json.dumps(processed), headers=headers, verify=False)
            return jsonify({"status": "pushed", "count": len(processed)})

        elif LOCATION == "work":
            response = requests.get(CL1P_URL, headers=headers, verify=False)
            if response.status_code == 200:
                items = json.loads(response.text)
                added = 0
                for item in items:
                    cursor.execute("SELECT id FROM heaterData WHERE datetime = %s", (item['datetime'],))
                    if not cursor.fetchone():
                        iq = """INSERT INTO heaterData (datetime, tempBox, tempHeater, statusBits, rssiAvg, readingCount) 
                                VALUES (%s, %s, %s, %s, %s, %s)"""
                        cursor.execute(iq, (item['datetime'], item['tempBox'], item['tempHeater'], 
                                           item['statusBits'], item['rssiAvg'], item['readingCount']))
                        added += 1
                conn.commit()
                return jsonify({"status": "pulled", "added": added})
                
        cursor.close()
        conn.close()
    except Exception as e:
        return jsonify({"error": str(e)}), 500

@app.route('/api/time', methods=['GET'])
def get_time():
    ontario_tz = pytz.timezone('America/Toronto')
    now_ontario = datetime.now(ontario_tz)
    return jsonify({"time": now_ontario.strftime("%I:%M %p")})


if __name__ == '__main__':
    bootstrap_db()
    app.run(host='0.0.0.0', port=int(os.getenv('API_PORT', 5000)))