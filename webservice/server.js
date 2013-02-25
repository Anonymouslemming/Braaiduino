var express = require('express'),
    cookinfo = require('./routes/cookinfo'),
    tempinfo = require('./routes/tempinfo'),
    sqlite3 = require('sqlite3').verbose();

app = express();

app.setupDb = function() {
	app.set('db', new sqlite3.Database('../database/braaiwatch.db', sqlite3.OPEN_READWRITE));
	var db = app.get('db');
	db.run("PRAGMA foreign_keys = ON", function(err,row){
		if (err) {
			console.log("Unable to enable foreign key support - quitting!");
			process.exit();
		} else {
			console.log("Connected to the database. Ready...");
		}
	});
}

app.configure(function () {
	app.use(express.logger('dev'));
	app.use(express.bodyParser());
	app.setupDb();
	exports.db = app.get('db');
});

//- Routes for web services
app.get('/cookinfo', cookinfo.findAll);
app.get('/cookinfo/:id', cookinfo.findById);
app.post('/cookinfo', cookinfo.addCook);
app.put('/cookinfo/:id', cookinfo.updateCook);
app.get('/tempinfo', tempinfo.findAll);
app.get('/tempinfo/:cookid', tempinfo.findById);
app.get('/tempinfo/:cookid/:limit', tempinfo.getLastById);
app.post('/tempinfo', tempinfo.addTemp);

app.listen(3000);
console.log('Listening on port 3000...');
