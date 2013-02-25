//-var sqlite3 = require('sqlite3').verbose();
//-var db = new sqlite3.Database('../database/braaiwatch.db', sqlite3.OPEN_READWRITE);

exports.findById = function(req, res) {
	var db = app.get('db');
	var id = req.params.id;
	
	stmt = db.prepare("SELECT * FROM cook where id=?");
	stmt.bind(id);
	stmt.all(function(err,row) {
		if (!row[0]) {
			console.log("Empty resultset for id: " + id);
			res.send({});
		}
		res.send(row);
	});
	stmt.finalize();
	
};

exports.findAll = function(req, res) {
	var db = app.get('db');
	db.all("SELECT * FROM cook", function(err, row) {
		res.send(row)
	});
};

exports.addCook = function(req, res) {
	var db = app.get('db');
	var cook = req.body;

	if (cook.action == 'startcook') {
		console.log('Adding cook: ' + JSON.stringify(cook));
		stmt = db.prepare("INSERT INTO COOK (started, active) values (current_timestamp,?)");
		stmt.bind(1);
		stmt.run(function(err, result) {
			if (err) {
				res.send(JSON.stringify(err));
				res.send({'error':'An error has occurred'});
			} else {
				if (stmt.changes == 1) {
					console.log('Success. LastID: [' + JSON.stringify(stmt.lastID + ']'));
					var objToJson = {"result": stmt.lastID};
					res.send(JSON.stringify(objToJson));
				}
			}
		});
		stmt.finalize();
	} else {
		var result = 0;
		var objToJson = {"result": result};
		console.log("Invalid request to create cook.");
		res.send(JSON.stringify(objToJson));
	}
}

exports.updateCook = function(req, res) {
	var db = app.get('db');
	var id = req.params.id;
	var request = req.body;
	
	console.log('Updating cook: ' + id);
	console.log(JSON.stringify(request));
	
	if (request.action == "endcook") { 
		stmt = db.prepare("UPDATE COOK SET ended = current_timestamp, active=0 WHERE id=?");
		stmt.bind(id);
		stmt.run(function(err, result) {
			if (err) {
				res.send(JSON.stringify(err));
				res.send({'error':'An error has occurred'});
			} else {
				if (this.changes > 0) {
					console.log("Ended cook with ID " + id);
					var objToJson = {'id': id, 'status': 'ended'};
					res.send(JSON.stringify(objToJson));
				} else {
					console.log("Failed to update cook with id: " + id);
					var objToJson = {'id': id, 'status': 'failed'};
					res.send(JSON.stringify(objToJson));
				}
			}
		});
		stmt.finalize();
	} else {
		var objToJson = {'id': id, 'status': 'failed'};
		res.send(JSON.stringify(objToJson));
	}
}
