from flask import Flask, jsonify, request, render_template
import sqlite3
import cv2
import numpy as np

app = Flask(__name__)

def get_db_connection():
    conn = sqlite3.connect('*****')
    conn.row_factory = sqlite3.Row
    return conn

def init_db():
    conn = get_db_connection()
    conn.execute('''
        CREATE TABLE IF NOT EXISTS inventory (
            id TEXT PRIMARY KEY,
            owner TEXT NOT NULL,
            status TEXT DEFAULT 'still looking'
        )
    ''')
    conn.commit()
    conn.close()

@app.route('/inventory', methods=['GET'])
def get_inventory():
    conn = get_db_connection()
    inventory = conn.execute('SELECT * FROM inventory').fetchall()
    conn.close()
    return render_template('inventory.html', inventory=inventory)

@app.route('/delete/<product_id>', methods=['DELETE'])
def delete_product(product_id):
    conn = get_db_connection()
    product = conn.execute('SELECT * FROM inventory WHERE id = ?', (product_id,)).fetchone()
    if product:
        conn.execute('DELETE FROM inventory WHERE id = ?', (product_id,))
        conn.commit()
        conn.close()
        return jsonify({"message": f"Parcel with ID {product_id} deleted!"}), 200
    else:
        conn.close()
        return jsonify({"message": "Parcel not found!"}), 404

@app.route('/add', methods=['POST'])
def add_product():
    data = request.get_json()
    conn = get_db_connection()
    if isinstance(data, list):
        for item in data:
            conn.execute('INSERT INTO inventory (id, owner) VALUES (?, ?)',
                         (item['id'], item['owner']))
    else:
        conn.execute('INSERT INTO inventory (id, owner) VALUES (?, ?)',
                     (data['id'], data['owner']))
    conn.commit()
    conn.close()
    return jsonify({"message": "Products added successfully!"}), 201

@app.route('/check_qr', methods=['POST'])
def check_qr():
    data = request.get_json()
    qr_code = data.get('qr_code')
    conn = get_db_connection()
    product = conn.execute('SELECT * FROM inventory WHERE id = ?', (qr_code,)).fetchone()
    if product:
        conn.execute('UPDATE inventory SET status = "found" WHERE id = ?', (qr_code,))
        conn.commit()
        inventory = conn.execute('SELECT * FROM inventory').fetchall()
        conn.close()
        return render_template('inventory.html', inventory=inventory)
    else:
        conn.close()
        return jsonify({"message": "Parcel not found!"}), 404

@app.route('/upload', methods=['POST'])
def upload_image():
    if 'file' not in request.files:
        return jsonify({"message": "No file part"}), 400

    file = request.files['file']
    if file.filename == '':
        return jsonify({"message": "No selected file"}), 400

    nparr = np.frombuffer(file.read(), np.uint8)
    img = cv2.imdecode(nparr, cv2.IMREAD_COLOR)

    detector = cv2.QRCodeDetector()
    data, bbox, _ = detector.detectAndDecode(img)

    if data:
        conn = get_db_connection()
        product = conn.execute('SELECT * FROM inventory WHERE id = ?', (data,)).fetchone()
        if product:
            conn.execute('UPDATE inventory SET status = "found" WHERE id = ?', (data,))
            conn.commit()
            conn.close()
            return jsonify({"message": f"Parcel with ID {data} found!"}), 200
        else:
            conn.close()
            return jsonify({"message": "Parcel not found!", "qr_code": data}), 404
    else:
        return jsonify({"message": "QR code not found"}), 400

if __name__ == '__main__':
    init_db()
    app.run(host='0.0.0.0', port=5000, debug=True)
