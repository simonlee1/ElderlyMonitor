from flask import Flask, request, render_template, jsonify
import sqlite3
from datetime import datetime
import smtplib, ssl


def createNewTable():
	con = sqlite3.connect('ArduinoData.db')
	cursor = con.cursor()
	cursor.execute("""CREATE TABLE IF NOT EXISTS HeartRates (
					HeartRateId integer PRIMARY KEY AUTOINCREMENT,
					HeartRate integer NOT NULL,
					CreationTimestamp text
					);
					""")

	cursor.execute("""CREATE TABLE IF NOT EXISTS FallHistory (
					FallHistoryId integer PRIMARY KEY AUTOINCREMENT,
					CreationTimestamp text
					);
					""")
createNewTable()

def sendEmail(content):
	port = 465
	password = "5549398aB"
	senderEmail = "arduinoservertest123@gmail.com"
	receiverEmail = "simonlee252@gmail.com"
	smptServer = "smtp.gmail.com"

	context = ssl.create_default_context()

	with smtplib.SMTP_SSL(smptServer, port, context=context) as server:
		server.login(senderEmail, password)
		server.sendmail(senderEmail, receiverEmail, content)

def addHeartRate(heartRate):
	con = sqlite3.connect('ArduinoData.db')
	cursor = con.cursor()
	curTime = datetime.now().timestamp()
	heartRateEntry = (heartRate, curTime)
	sql = """INSERT INTO HeartRates (HeartRate, CreationTimestamp) VALUES (?,?)"""

	cursor.execute(sql, heartRateEntry)
	con.commit()

	if int(heartRate) > 100:
		sendEmail("Your Elderly's HeartRate is high, consider contacting your elderly \n Heart Rate: {}".format(heartRate))

def addFallHistory():
	con = sqlite3.connect('ArduinoData.db')
	cursor = con.cursor()
	curTime = datetime.now()
	sql = "INSERT INTO FallHistory (CreationTimestamp) VALUES ('{}')".format(curTime)

	cursor.execute(sql)
	con.commit()

	sendEmail("Oh no! Your elderly might have fallen!")

app = Flask(__name__)

@app.route("/RecordHeartRate", methods=["GET"])
def recordHeartrate():
	heartRate = request.args["heartRate"]
	addHeartRate(heartRate)

	return "Sucess"

@app.route("/RecordFall", methods=["GET"])
def recordFall():
	addFallHistory()

	return "Sucess"

@app.route("/")
def index():
	return render_template("index.html")

@app.route("/UpdateData", methods=["GET"])
def updateData():
	con = sqlite3.connect('ArduinoData.db')
	cursor = con.cursor()

	cursor.execute("SELECT * FROM HeartRates ORDER BY HeartRateId DESC LIMIT 20")
	heartRates = [{"id": x[0], "heartRate": x[1], "timestamp": x[2]} for x in cursor.fetchall()]

	cursor.execute("SELECT * FROM FallHistory")
	falls = [{"id": x[0], "timestamp": x[1]} for x in cursor.fetchall()]

	return jsonify({"heartRate" : heartRates, "falls" : falls})
