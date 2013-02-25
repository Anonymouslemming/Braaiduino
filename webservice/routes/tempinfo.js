exports.findById = function(req, res) {
	var db = app.get('db');
	var cookid = req.params.cookid;
	stmt = db.prepare("SELECT * FROM temperature where cookid=? order by id asc");
	stmt.bind(cookid);
	stmt.all(function(err,row) {
		if (!row[0]) {
			console.log("Empty resultset for cookid: " + cookid);
			res.send({});
		}
		res.send(row);
	});
	stmt.finalize();
	
};

exports.getLastById = function(req, res) {
	var db = app.get('db');
	var cookid = req.params.cookid;
	var limit = req.params.limit;
	stmt = db.prepare("SELECT * FROM temperature where cookid=? order by timestamp desc LIMIT ?");
	stmt.bind(cookid, limit);
	stmt.all(function(err,row) {
		if (!row[0]) {
			console.log("Empty resultset for cookid: " + cookid);
			res.send({});
		}
		res.send(row);
	});
	stmt.finalize();

}

exports.findAll = function(req, res) {
	var db = app.get('db');
	db.all("SELECT * FROM temperature", function(err, row) {
		res.send(row)
	});
};

exports.addTemp = function(req, res) {
	var db = app.get('db');
	var temp = req.body;
	var cookid = temp.cookid;
	var temp1 = temp.temp1;
	var temp2 = temp.temp2;
	var temp3 = temp.temp3;
	
	if (temp.action == 'add') {
		console.log('Adding temp: ' + JSON.stringify(temp));
		
		stmt = db.prepare("INSERT INTO TEMPERATURE (cookid, temp1, temp2, temp3, timestamp) values (?, ?, ?, ?, current_timestamp)");
		stmt.bind(cookid, temp1, temp2, temp3);
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
		var objToJson = {"result": "invalid add request"};
		console.log("Invalid request to add temp.");
		res.send(JSON.stringify(objToJson));
	}
}

