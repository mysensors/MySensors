const dbAddress						= '127.0.0.1';
const dbPort						= 27017;
const dbName						= 'MySensorsDb';

var dbc = require('mongodb').MongoClient;
dbc.connect('mongodb://' + dbAddress + ':' + dbPort + '/' + dbName, function(err, db) {
	if (err) {
		console.log('Error connecting to database at mongodb://' + dbAddress + ':' + dbPort + '/' + dbName);
	} else {
		db.dropDatabase(function(err, result) {
			if (err)
				console.log('Error dropping database at mongodb://' + dbAddress + ':' + dbPort + '/' + dbName);
			else
                	        console.log('Successfully dropped database at mongodb://' + dbAddress + ':' + dbPort + '/' + dbName);
		});
	}
});
