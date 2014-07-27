var NodeIdToReboot = 1;

const dbAddress                                         = '127.0.0.1';
const dbPort                                            = 27017;
const dbName                                            = 'MySensorsDb';

var dbc = require('mongodb').MongoClient;
dbc.connect('mongodb://' + dbAddress + ':' + dbPort + '/' + dbName, function(err, db) {
        if (err) {
                console.log('Error connecting to database at mongodb://' + dbAddress + ':' + dbPort + '/' + dbName);
        } else {
	        db.collection('node', function(err, c) {
        	        c.update({
                	        'id': NodeIdToReboot
                	}, {
                        	$set: {
                                	'reboot': 1
	                        }
        	        }, function(err, result) {
                	        if (err)
                        	        console.log("Error writing reboot request to database");
				else
                                        console.log("Reboot request saved to database");
                	});
        	});
	}
});

