package main

import (
	"bufio"
	"database/sql"
	"errors"
	"fmt"
	"log"
	"os"
	"strconv"

	_ "github.com/mattn/go-sqlite3"
)

// delete all currently registered waypoints and routes from the database
// for debugging purposes
func cleanDB() error {
	err := Migrate()
	if err != nil {
		Error.Println(err)
		return err
	}
	query := `DELETE FROM Waypoints`
	err = transactionExec(query)
	if err != nil {
		Error.Println(err)
		return err
	}

	query = `DELETE FROM aeac_routes`
	err = transactionExec(query)
	if err != nil {
		Error.Println(err)
		return err
	}

	return nil
}

// Creates a local sqlite database file "database.sqlite" in the root directory
// if it does not already exist, and starts a database connection
func connectToDB() *sql.DB {
	// check if db file exists, and if necessary, create it
	if _, err := os.Stat("./database.sqlite"); err != nil {
		tmpFile, err := os.Create("./database.sqlite")
		if err != nil {
			Error.Println(err)
			panic(err)
		}
		// defer tmpFile.Close()
		err = tmpFile.Close()
		if err != nil {
			Error.Println(err)
			panic(err)
		}
	}

	// open connection to db file
	db, err := sql.Open("sqlite3", "./database.sqlite")
	if err != nil {
		panic(err)
	}

	return db
}

// Reads the queries from migrate.sql line by line, and executes each
// query against the database connected to be connectToBB() to initialize
// any required tables
func Migrate() error {
	//open db connection
	db := connectToDB()

	//read queries from "migrate.sql" line by line
	file, err := os.Open("./migrate.sql")
	if err != nil {
		Error.Println(err)
		return err
	}
	defer file.Close()

	scanner := bufio.NewScanner(file)
	for scanner.Scan() {
		// execute each sql query in file
		queryText := scanner.Text()

		tx, err := db.Begin()
		if err != nil {
			Error.Println(err)
			return err
		}
		defer tx.Rollback()

		stmt, err := tx.Prepare(queryText)
		if err != nil {
			Error.Println(err)
			return err
		}
		defer stmt.Close()

		_, err = stmt.Exec()
		if err != nil {
			Error.Println(err)
			return err
		}

		err = tx.Commit()
		if err != nil {
			Error.Println(err)
			return err
		}
	}

	if err := scanner.Err(); err != nil {
		Error.Println(err)
		return err
	}

	return nil
}

// use: call "waypointObject.create()" and it registers
// the waypoint based on its type values in the database
//
// the parameter (Waypoint) just before the function name is
// called a "receiver". it requires you call this method
// on a struct. it's like an object oriented design pattern
//
// notice that we're passing pointers (by reference) in functions that modify the struct,
// and passing by value in cases where we are not modifying the struct.
// consider this a "security feature"
func (wp *Waypoint) Create() error {
	// IMPORTANT NOTE: the ID is assigned by the database serialization:
	// the Waypoint passed as the receiver MUST HAVE an ID with some sentinel
	// value, let's say -1 (impossible ID), and then be assigned an ID
	// by the database upon being created in the database.
	// throw an error if an ID is passed that is not -1, since there
	// is an obvious issue. have a distinct type for this error
	// and describe it as something like "non-sentinel ID passed to Waypoint.create()"

	db := connectToDB()
	var query string
	var existingId int

	if wp.ID != -1 {
		return errors.New(
			"non-sentinel ID passed to Waypoint.Create()" +
				"\n Expected ID: -1 but got ID: " + strconv.Itoa(wp.ID))
	}

	//check that the waypoint does not already exist in the database
	query = `SELECT id FROM Waypoints WHERE name = $1 AND longitude = $2 AND latitude = $3 AND altitude = $4`
	row := db.QueryRow(query, wp.Name, wp.Longitude, wp.Latitude, wp.Altitude)
	if (row != nil) && (row.Scan(&existingId) != sql.ErrNoRows) {
		Warning.Println("Waypoint already exists in database, skipping")
		wp.ID = existingId
		return nil
	}

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}
	defer tx.Rollback()

	query = `INSERT INTO Waypoints (name, longitude, latitude, altitude) VALUES ($1, $2, $3, $4)`

	stmt, err := tx.Prepare(query)
	if err != nil {
		Error.Println(err)
		return err
	}
	defer stmt.Close()

	_, err = stmt.Exec(wp.Name, wp.Longitude, wp.Latitude, wp.Altitude)
	if err != nil {
		Error.Println(err)
		return err
	}

	err = tx.Commit()
	if err != nil {
		Error.Println(err)
		return err
	}

	err = wp.Get()
	if err != nil {
		Error.Println(err)
		return err
	}

	return nil
}

// Update the database record for a given Waypoint based on its ID.
// The current values held in the fields of this Waypoint struct will
// be copied over to their corresponding columns in the database
func (wp Waypoint) Update() error {
	// using the receiver, update the waypoint's values in the database based on its ID

	db := connectToDB()

	if wp.ID == -1 {
		return errors.New(
			"sentinel ID (-1) passed to Waypoint.Update(). " +
				"Did you remember to call Waypoint.Create() beforehand?")
	}

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
	}
	defer tx.Rollback()

	query := `UPDATE Waypoints SET name = ($1), longitude = ($2), latitude = ($3), altitude = ($4) WHERE id = ($5)`

	stmt, err := tx.Prepare(query)
	if err != nil {
		Error.Println(err)
	}
	defer stmt.Close()

	_, err = stmt.Exec(wp.Name, wp.Longitude, wp.Latitude, wp.Altitude, wp.ID)
	if err != nil {
		Error.Println(err)
	}

	err = tx.Commit()
	if err != nil {
		Error.Println(err)
	}

	return nil
}

// Delete the database record for a given Waypoint based on its ID.
func (wp Waypoint) Delete() error {
	// using the receiver, delete the waypoint from the database based on its ID

	db := connectToDB()

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}
	defer tx.Rollback()

	query := `DELETE FROM Waypoints WHERE id = ($1)`

	stmt, err := tx.Prepare(query)
	if err != nil {
		Error.Println(err)
		return err
	}
	defer stmt.Close()

	_, err = stmt.Exec(wp.ID)
	if err != nil {
		Error.Println(err)
		return err
	}

	err = tx.Commit()
	if err != nil {
		Error.Println(err)
		return err
	}

	return nil
}

// Writes the values stored in the database for this given Waypoint's ID
// to the Waypoint object.
// If this method is called on a Waypoint that has not yet been registered in the database,
// (i.e. Waypoint.Create() has not been called), then the method queries the database
// for records that match all four fields of (name, longitude, latitude, altitude).
// Otherwise, this method queries the database for records that match only the ID.
func (wp *Waypoint) Get() error {
	// the opposite of Waypoint.create()!
	// instead, populate all fields from the database
	// based on the ID provided

	db := connectToDB()

	id := wp.ID

	var row *sql.Row

	if id == -1 {
		//do based off fields
		query := `SELECT * FROM Waypoints WHERE 
			name = ($1) AND 
			longitude = ($2) AND
			latitude = ($3) AND
			altitude = ($4)`

		stmt, err := db.Prepare(query)
		if err != nil {
			Error.Println(err)
			return err
		}
		defer stmt.Close()

		row = stmt.QueryRow(wp.Name, wp.Longitude, wp.Latitude, wp.Altitude)

	} else {
		//do based off ID

		query := `SELECT * FROM Waypoints WHERE id = ($1)`

		stmt, err := db.Prepare(query)
		if err != nil {
			Error.Println(err)
			return err
		}
		defer stmt.Close()

		row = stmt.QueryRow(wp.ID)
		if err != nil {
			Error.Println(err)
			return err
		}
	}

	err := row.Scan(&wp.ID, &wp.Name, &wp.Longitude, &wp.Latitude, &wp.Altitude)
	if err != nil {
		Error.Println(err)
		return err
	}

	return nil
}

// returns a pointer to a Queue struct that contains all the waypoints currently registered in the database
func getAllWaypoints() (*Queue, error) {
	db := connectToDB()

	query := `SELECT * FROM Waypoints`

	rows, err := db.Query(query)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	var waypoints []Waypoint

	for rows.Next() {
		var wp Waypoint
		err = rows.Scan(&wp.ID, &wp.Name, &wp.Longitude, &wp.Latitude, &wp.Altitude)
		if err != nil {
			Error.Println(err)
			return nil, err
		}
		waypoints = append(waypoints, wp)
	}

	q := Queue{waypoints}

	return &q, nil
}

// Definitions for AEACRoutes DB methods

// Initializes an AEACRoute in the database
// and assigns it a non-sentinel ID if it does not already exist
// requires: ID == -1
func (r *AEACRoutes) Create() error {
	db := connectToDB()
	var query string
	var existingId int

	if r.ID != -1 {
		errMsg := "Non-Sentinel ID passed into AEACRoutes.Create()"
		Error.Println(errMsg)
		return errors.New(errMsg)
	}

	//check that the AEACRoute does not already exist in the database
	query = `SELECT id FROM aeac_routes WHERE 
				number = $1 AND 
				start_waypoint = $2 
				AND end_waypoint = $3 
				AND passengers = $4
				AND max_weight = $5
				AND value = $6
				AND remarks = $7
				AND odr = $8`
	row := db.QueryRow(query,
		r.Number,
		r.StartWaypoint,
		r.EndWaypoint,
		r.Passengers,
		r.MaxVehicleWeight,
		r.Value,
		r.Remarks,
		r.Order)
	if (row != nil) && (row.Scan(&existingId) != sql.ErrNoRows) {
		Warning.Println("AEACRoute already exists in database, skipping")
		r.ID = existingId
		return nil
	}

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}
	stmt, err := tx.Prepare(`INSERT INTO aeac_routes 
		(number, start_waypoint, end_waypoint, passengers, max_weight, value, remarks, odr) VALUES 
		(?, ?, ?, ?, ?, ?, ?, ?) RETURNING id`)

	if err != nil {
		Error.Println(err)
		return err
	}

	defer stmt.Close()
	err = stmt.QueryRow(r.Number,
		r.StartWaypoint,
		r.EndWaypoint,
		r.Passengers,
		r.MaxVehicleWeight,
		r.Value,
		r.Remarks,
		r.Order).Scan(&r.ID)

	if err != nil {
		Error.Println(err)
		return err
	}

	err = tx.Commit()
	if err != nil {
		Error.Println(err)
		return err
	}
	return nil
}

// Updates the database entry with id == ID
// with data in the AEACRoute struct.
// requires: ID != -1
func (r AEACRoutes) Update() error {
	db := connectToDB()

	if r.ID == -1 {
		errMsg := "uncreated ID passed into AEACRoutes.Update()"
		Error.Println(errMsg)
		return errors.New(errMsg)
	}

	stmt, err := db.Prepare(`UPDATE aeac_routes SET 
		number = ?,
		start_waypoint = ?,
		end_waypoint = ?,
		passengers = ?,
		max_weight = ?,
		value = ?,
		remarks = ?,
		odr= ? WHERE
		id = ?`)
	if err != nil {
		Error.Println(err)
		return err
	}

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}

	_, err = tx.Stmt(stmt).Exec(r.Number,
		r.StartWaypoint,
		r.EndWaypoint,
		r.Passengers,
		r.MaxVehicleWeight,
		r.Value,
		r.Remarks,
		r.Order,
		r.ID)

	if err != nil {
		tx.Rollback()
		Error.Println(err)
		return err
	} else {
		err = tx.Commit()
		return err
	}
}

// Deletes all AEACRoute from the database
// with id == ID
// requires: ID != -1
func (r AEACRoutes) Delete() error {
	db := connectToDB()

	if r.ID == -1 {
		errMsg := "uncreated ID passed into AEACRoutes.Delete()"
		Error.Println(errMsg)
		return errors.New(errMsg)
	}

	stmt, err := db.Prepare(`DELETE FROM aeac_routes WHERE id = ?`)
	if err != nil {
		Error.Println(err)
		return err
	}

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}

	_, err = tx.Stmt(stmt).Exec(r.ID)

	if err != nil {
		tx.Rollback()
		Error.Println(err)
		return err
	} else {
		err = tx.Commit()
		return err
	}
}

// Fetches data of an AEACRoute with id == iD
// from the database and populates the struct
// requires: ID != -1
// returns: sql.ErrNoRows if no such entry exists
func (r *AEACRoutes) Get() error {
	db := connectToDB()

	if r.ID == -1 {
		errMsg := "uncreated ID passed into AEACRoutes.Get()"
		Error.Println(errMsg)
		return errors.New(errMsg)
	}

	query := `SELECT * FROM aeac_routes WHERE id = ?`
	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}

	defer tx.Rollback()

	row := tx.QueryRow(query, r.ID)

	if err != nil {
		Error.Println(err)
		return err
	}

	err = row.Scan(&r.ID,
		&r.Number,
		&r.StartWaypoint,
		&r.EndWaypoint,
		&r.Passengers,
		&r.MaxVehicleWeight,
		&r.Value,
		&r.Remarks,
		&r.Order,
	)

	if err == sql.ErrNoRows {
		log.Printf("No Entry for ID %s", fmt.Sprint(r.ID))
		return err
	} else if err != nil {
		Error.Println(err)
		return err
	}
	return nil
}

// returns a pointer to an AEACRoutes array that contains all the routes currently registered in the database
func getAllRoutes() (*[]AEACRoutes, error) {
	query := `SELECT * FROM aeac_routes`

	rows, err := querySelect(query)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	var routes []AEACRoutes

	for rows.Next() {
		var r AEACRoutes
		err = rows.Scan(&r.ID, &r.Number, &r.StartWaypoint, &r.EndWaypoint, &r.Passengers,
			&r.MaxVehicleWeight, &r.Value, &r.Remarks, &r.Order)
		if err != nil {
			Error.Println(err)
			return nil, err
		}
		routes = append(routes, r)
	}

	return &routes, nil
}

// execute a select query and return the rows
func querySelect(query string) (*sql.Rows, error) {
	db := connectToDB()

	rows, err := db.Query(query)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	return rows, nil
}

// execute a transaction aganist the database
func transactionExec(query string) error {
	db := connectToDB()

	tx, err := db.Begin()
	if err != nil {
		Error.Println(err)
		return err
	}
	defer tx.Rollback()

	stmt, err := tx.Prepare(query)
	if err != nil {
		Error.Println(err)
		return err
	}
	defer stmt.Close()

	_, err = stmt.Exec()
	if err != nil {
		Error.Println(err)
		return err
	}

	err = tx.Commit()
	if err != nil {
		Error.Println(err)
		return err
	}

	return nil
}
