var NodeIdToReboot = 1;

var gwType = 'Ethernet';
var gwAddress = '10.0.1.99';
var gwPort = 9999;

var BROADCAST_ADDRESS = 255;
var NODE_SENSOR_ID = 255;

function encode(destination, sensor, command, acknowledge, type, payload, binary) {
	var msg = destination.toString(10) + ";" + sensor.toString(10) + ";" + command.toString(10) + ";" + acknowledge.toString(10) + ";" + type.toString(10) + ";";
	if (binary) {
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

function rebootNode(destination, gw) {
	var sensor = NODE_SENSOR_ID;
	var command = 3; // C_INTERNAL
	var acknowledge = 0; // no ack
	var type = 13; // I_REBOOT
	var payload = "";
	var td = encode(destination, sensor, command, acknowledge, type, payload, false);
	console.log('-> ' + td.toString());
	gw.write(td);
}

var gw = require('net').Socket();
gw.connect(gwPort, gwAddress);
gw.setEncoding('ascii');
gw.on('connect', function() {
	console.log('connected to ethernet gateway at ' + gwAddress + ":" + gwPort);
	rebootNode(NodeIdToReboot, gw);
	console.log('reboot request submitted for ' + NodeIdToReboot);
}).on('end', function() {
	console.log('disconnected from gateway');
});
