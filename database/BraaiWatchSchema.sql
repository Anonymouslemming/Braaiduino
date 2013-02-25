-- SCHEMA
create table cook
	(id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL, 
		started DATETIME NOT null, 
		ended DATETIME,
		active INT NOT NULL default 0);

create table temperature (
	id INTEGER PRIMARY KEY AUTOINCREMENT, 
	cookid INTEGER NOT NULL, 
	temp1 NUMERIC NOT NULL, 
	temp2 NUMERIC, 
	temp3 NUMERIC, 
	timestamp DATETIME NOT NULL default current_timestamp,
	FOREIGN KEY(cookid) REFERENCES cook(id));

-- SAMPLE DATA
--insert into cook 
--	(started, ended) 
--	VALUES 
--	('2013-01-21 08:04:34', '2013-01-21 15:04:34');

--insert into cook 
--	(started, ended) 
--	VALUES 
--	('2013-01-23 12:04:34', '2013-01-23 15:04:34');

--insert into cook 
--	(started, active) 
--	VALUES 
--	('2013-01-23 12:04:34', 1);
