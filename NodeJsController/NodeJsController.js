var gwType = 'Ethernet';
var gwAddress = '10.0.1.99';
var gwPort = 9999;

//var gwType = 'Serial';
//var gwPort = 'COM4';
//var gwBaud = 115200;

var dbAddress = '127.0.0.1';
var dbPort = 27017;
var dbName = 'MySensorsDb';

var fwDefaultType = 0x0003; // set to 0x0001 for BlinkRed and 0x0002 for BlinkGreen and 0x0003 for Dallas Temperature

var FIRMWARE_BLOCK_SIZE = 16;
var BROADCAST_ADDRESS = 255;
var NODE_SENSOR_ID = 255;

var fs = require('fs');

function crcUpdate(old, value) {
	var c = old ^ value;
	for (var i = 0; i < 8; ++i) {
		if ((c & 1) > 0)
			c = ((c >> 1) ^ 0xA001);
		else
			c = (c >> 1);
	}
	return c;
}

function pullWord(arr, pos) {
	return arr[pos] + 256 * arr[pos + 1];
}
 
function pushWord(arr, val) {
	arr.push(val & 0x00FF);
	arr.push((val  >> 8) & 0x00FF);
}

function pushDWord(arr, val) {
	arr.push(val & 0x000000FF);
	arr.push((val  >> 8) & 0x000000FF);
	arr.push((val  >> 16) & 0x000000FF);
	arr.push((val  >> 24) & 0x000000FF);
}

function loadFirmware(fwtype, fwversion, filename, db) {
	console.log("loading firmware: " + filename);
	fwdata = [];
	var start = 0;
	var end = 0;
	var pos = 0;
	var hex = fs.readFileSync(filename).toString().split("\n");
	for(l in hex) {
		line = hex[l].trim();
		if (line.length > 0) {
			while (line.substring(0, 1) != ":")
				line = line.substring(1);
			var reclen = parseInt(line.substring(1, 3), 16);
			var offset = parseInt(line.substring(3, 7), 16);
			var rectype = parseInt(line.substring(7, 9), 16);
			var data = line.substring(9, 9 + 2 * reclen);
			var chksum = parseInt(line.substring(9 + (2 * reclen), 9 + (2 * reclen) + 2), 16);
			if (rectype == 0) {
				if ((start == 0) && (end == 0)) {
					if (offset % 128 > 0)
						throw new Error("error loading hex file - offset can't be devided by 128");
					start = offset;
					end = offset;
				}
				if (offset < end)
					throw new Error("error loading hex file - offset lower than end");
				while (offset > end) {
					fwdata.push(255);
					pos++;
					end++;
				}
				for (var i = 0; i < reclen; i++) {
					fwdata.push(parseInt(data.substring(i * 2, (i * 2) + 2), 16));
					pos++;
				}
				end += reclen;
			}
		}
	}	
	var pad = end % 128; // ATMega328 has 64 words per page / 128 bytes per page
	for (var i = 0; i < 128 - pad; i++) {
		fwdata.push(255);
		pos++;
		end++;
	}
	var blocks = (end - start) / FIRMWARE_BLOCK_SIZE;
	var crc = 0xFFFF;
	for (var i = 0; i < blocks * FIRMWARE_BLOCK_SIZE; ++i) {
		var v = crc;
		crc = crcUpdate(crc, fwdata[i]);
	}
	db.collection('firmware', function(err, c) {
		c.update({
			'type': fwtype,
			'version': fwversion
		}, {
			'type': fwtype,
			'version': fwversion,
			'filename': filename,
			'blocks': blocks,
			'crc': crc,
			'data': fwdata
		}, {
			upsert: true
		}, function(err, result) {
			if (err)
				console.log('Error writing firmware to database');
		});
	});
	console.log("loading firmware done. blocks: " + blocks + " / crc: " + crc);
}

/*
function decode(msg) {
	var msgs = msg.toString().split(";");
	rsender = +msgs[0];
	rsensor = +msgs[1];
	rcommand = +msgs[2];
	rtype = +msgs[3];
	var pl = msgs[4].trim();
	rpayload = [];
	for (var i = 0; i < pl.length; i+=2) {
		var b = parseInt(pl.substring(i, i + 2), 16);
		rpayload.push(b);
	}
}
*/

function encode(destination, sensor, command, acknowledge, type, payload) {
	var msg = destination.toString(10) + ";" + sensor.toString(10) + ";" + command.toString(10) + ";" + acknowledge.toString(10) + ";" + type.toString(10) + ";";
	if (command == 4) {
		for (var i = 0; i < payload.length; i++) {
			if (payload[i] < 16)
				msg += "0";
			msg += payload[i].toString(16);
		}
	} else {
		msg += payload;
	}
	msg += '\n';
	return msg.toString();
}

function saveValue(sender, sensor, type, pl, db) {
	var cn = "Value-" + sender.toString() + "-" + sensor.toString();
	db.createCollection(cn, function(err, c) { 
		c.save({
			'timestamp': new Date().getTime(),
			'type': type,
			'value': pl
		}, function(err, result) {
			console.log("Error writing value to database");
		});
	});
}

function saveBatteryLevel(sender, sensor, pl, db) {
	var cn = "BatteryLevel-" + sender.toString() + "-" + sensor.toString();
	db.createCollection(cn, function(err, c) { 
		c.save({
			'timestamp': new Date().getTime(),
			'value': pl
		}, function(err, result) {
			console.log("Error writing battery level to database");
		});
	});
}

function saveSketchName(sender, pl, db) {
	db.collection('node', function(err, c) {
		c.update({
			'id': sender
		}, {
			'sketchName': pl
		}, function(err, result) {
			console.log("Error writing sketch name to database");
		});
	});
}

function saveSketchVersion(sender, pl, db) {
	db.collection('node', function(err, c) {
		c.update({
			'id': sender
		}, {
			'sketchVersion': pl
		}, function(err, result) {
			console.log("Error writing sketch version to database");
		});
	});
}

function sendTime(destination, sensor, gw) {
	var payload = new Date().getTime();
	var command = 3; // C_INTERNAL
	var acknowledge = 0; // no ack
	var type = 1; // I_TIME
	var td = encode(destination, sensor, command, acknowledge, type, payload);
	console.log('-> ' + td.toString());
	gw.write(td);
}

function sendNextAvailableSensorId(db, gw) {
	db.collection('node', function(err, c) {
		c.find({
			$query: { },
			$orderby: {
				'id': 1
			}
		}).toArray(function(err, results) {
			if (err)
				console.log('Error finding nodes');
			var id = 1;
			for (var i = 0; i < results.length; i++)
				if (results[i].id > i + 1) {
					id = i + 1;
					break;
				}
			if (id < 255) {
				c.save({
					'id': id
				}, function(err, result) {
					if (err)
						console.log('Error writing node to database');
					var destination = BROADCAST_ADDRESS;
					var sensor = NODE_SENSOR_ID;
					var command = 3; // C_INTERNAL
					var acknowledge = 0; // no ack
					var type = 4; // I_ID_RESPONSE
					var payload = id;
					var td = encode(destination, sensor, command, acknowledge, type, payload);
					console.log('-> ' + td.toString());
					gw.write(td);
				});
			}
		});
	});
}

function sendFirmwareConfigResponse(destination, sensor, fwtype, fwversion, db, gw) {
	// keep track of type/versin info for each node
	// at the same time update the last modified date
	// could be used to remove nodes not seen for a long time etc.
	db.collection('node', function(err, c) {
		c.update({
			'id': destination
		}, {
			'id': destination,
			'type': fwtype,
			'version': fwversion
		}, {
			upsert: true
		}, function(err, result) { });
	});
	if (fwtype == 0xFFFF) {
		// sensor does not know which type / blank EEPROM
		// take predefined type (ideally selected in UI prior to connection of new sensor)
		if (fwDefaultType == 0xFFFF)
			throw new Error('No default sensor type defined');
		fwtype = fwDefaultType;
	}
	db.collection('firmware', function(err, c) {
		c.findOne({
			$query: {
				'type': fwtype
			},
			$orderby: {
				'version': -1
			}
		}, function(err, result) {
			if (err)
				console.log('Error finding firmware for type ' + fwtype);
			if (!result)
				console.log('No firmware found for type ' + fwtype);
			var payload = [];
			pushWord(payload, result.type);
			pushWord(payload, result.version);
			pushWord(payload, result.blocks);
			pushWord(payload, result.crc);
			var command = 4; // C_STREAM
			var acknowledge = 0; // no ack
			var type = 1; // I_FIRMWARE_CONFIG_RESPONSE
			var td = encode(destination, sensor, command, acknowledge, type, payload);
			console.log('-> ' + td.toString());
			gw.write(td);
		});
	});
}

function sendFirmwareResponse(destination, sensor, fwtype, fwversion, fwblock, db, gw) {
	db.collection('firmware', function(err, c) {
		c.findOne({
			'type': fwtype,
			'version': fwversion
		}, function(err, result) {
			if (err)
				console.log('Error finding firmware version ' + fwversion + ' for type ' + fwtype);
			var payload = [];
			pushWord(payload, result.type);
			pushWord(payload, result.version);
			pushWord(payload, fwblock);
			for (var i = 0; i < FIRMWARE_BLOCK_SIZE; i++)
				payload.push(result.data[fwblock * FIRMWARE_BLOCK_SIZE + i]);
			var command = 4; // C_STREAM
			var acknowledge = 0; // no ack
			var type = 3; // ST_FIRMWARE_RESPONSE
			var td = encode(destination, sensor, command, acknowledge, type, payload);
			console.log('-> ' + td.toString());
			gw.write(td);
		});
	});
}

function rebootNode(destination, gw) {
	var sensor = NODE_SENSOR_ID;
	var command = 3; // C_INTERNAL
	var acknowledge = 0; // no ack
	var type = 13; // I_REBOOT
	var payload = "";
	var td = encode(destination, sensor, command, acknowledge, type, payload);
	console.log('-> ' + td.toString());
	gw.write(td);
}

function rfReceived(data, db, gw) {
	console.log('<- ' + data);
	// decoding message
	var datas = data.toString().split(";");
	var sender = +datas[0];
	var sensor = +datas[1];
	var command = +datas[2];
	var type = +datas[3];
	var rawpayload = datas[4].trim();
	var payload;
	if (command == 4) {
		payload = [];
		for (var i = 0; i < rawpayload.length; i+=2)
			payload.push(parseInt(rawpayload.substring(i, i + 2), 16));
	} else {
		payload = rawpayload;
	}
	// decision on appropriate response
	switch (command) {
	case 0: // C_PRESENTATION
		break;
	case 1: // C_SET
		saveValue(sender, sensor, type, payload, db);
		break;
	case 2: // C_REQ
		break;
	case 3: // C_INTERNAL
		switch (type) {
		case 0: // I_BATTERY_LEVEL
			saveBatteryLevel(sender, sensor, payload, db);
			break;
		case 1: // I_TIME
			sendTime(sender, sensor, gw);
			break;
		case 2: // I_VERSION
			break;
		case 3: // I_ID_REQUEST
			sendNextAvailableSensorId(db, gw);
			break;
		case 4: // I_ID_RESPONSE
			break;
		case 5: // I_INCLUSION_MODE
			break;
		case 6: // I_CONFIG
			break;
		case 7: // I_PING
			break;
		case 8: // I_PING_ACK
			break;
		case 9: // I_LOG_MESSAGE
			break;
		case 10: // I_CHILDREN
			break;
		case 11: // I_SKETCH_NAME
			saveSketchName(sender, payload, db);
			break;
		case 12: // I_SKETCH_VERSION
			saveSketchVersion(sender, payload, db);
			break;
		case 13: // I_REBOOT
			break;
		}
		break;
	case 4: // C_STREAM
		switch (type) {
                case 0: // ST_FIRMWARE_CONFIG_REQUEST
                        var fwtype = pullWord(payload, 0);
                        var fwversion = pullWord(payload, 2);
                        sendFirmwareConfigResponse(sender, sensor, fwtype, fwversion, db, gw);
                        break;
                case 1: // ST_FIRMWARE_CONFIG_RESPONSE
                        break;
                case 2: // ST_FIRMWARE_REQUEST
                        var fwtype = pullWord(payload, 0);
                        var fwversion = pullWord(payload, 2);
                        var fwblock = pullWord(payload, 4);
                        sendFirmwareResponse(sender, sensor, fwtype, fwversion, fwblock, db, gw);
                        break;
                case 3: // ST_FIRMWARE_RESPONSE
                        break;
                case 4: // ST_SOUND
                        break;
                case 5: // ST_IMAGE
                        break;
		}
		break;
	}
}

var dbc = require('mongodb').MongoClient;
dbc.connect('mongodb://' + dbAddress + ':' + dbPort + '/' + dbName, function(err, db) {
	if(err) {
		console.log('Error connecting to database at mongodb://' + dbAddress + ':' + dbPort + '/' + dbName);
		return;
	}
	console.log('Connected to database at mongodb://' + dbAddress + ':' + dbPort + '/' + dbName);
	db.createCollection('node', function(err, collection) { });
	db.createCollection('firmware', function(err, collection) { });

	// ToDo : check for new hex files / only load if new / get type and version from filename
	loadFirmware(0x0001, 0x0001, 'BlinkRed.hex', db);
	loadFirmware(0x0002, 0x0001, 'BlinkGreen.hex', db);
	loadFirmware(0x0003, 0x0001, 'DallasTemperatureSensor.cpp.hex', db);

	var gw;
	if (gwType == 'Ethernet') {
		gw = require('net').Socket();
		gw.connect(gwPort, gwAddress);
		gw.setEncoding('ascii');
		gw.on('connect', function() {
			console.log('connected to ethernet gateway at ' + gwAddress + ":" + gwPort);
		}).on('data', function(rd) {
			rfReceived(rd.toString(), db, gw);
		}).on('end', function() {
			console.log('disconnected from gateway');
		}).on('error', function() {
			console.log('connection error - trying to reconnect');
			gw.connect(gwPort, gwAddress);
			gw.setEncoding('ascii');
		});
	} else if (gwType == 'Serial') {
		gw = require('serialport').SerialPort(gwPort, { baudrate: gwBaud }, false);
		gw.open();
		gw.on('open', function() {
			console.log('connected to serial gateway at ' + gwPort);
		}).on('data', function(rd) {
			rfReceived(rd.toString(), db, gw);
		}).on('end', function() {
			console.log('disconnected from gateway');
		}).on('error', function() {
			console.log('connection error - trying to reconnect');
			gw.open();
		});
	} else {
		throw new Error('unknown Gateway type');
	}
});

