package main

import (
	"bufio"
	"database/sql"
	"errors"
	"fmt"
	"log"
	"os"

	_ "github.com/mattn/go-sqlite3"
)


func connectToDB() *sql.DB {
	// this creates the db file if it doesn't already exist
	db, err := sql.Open("sqlite3", "db.sqlite")

	if err != nil {
		panic(err)
	}

	return db
}

// Using a predefined query that sets up the database structure,
// create/initialize a database file
func Migrate() error {
	db := connectToDB()
	var sqlLines []string

	f, err := os.Open(`./migrate.sql`)
	if err != nil {
		log.Fatal(err);
		return err;
	}
	
	scn := bufio.NewScanner(f)
	for scn.Scan() {
		sqlLines = append(sqlLines, scn.Text())
	}

	err = scn.Err()
	f.Close()
	if err != nil {
        log.Fatal(err)
		return err;
    }

	for _, stmtString := range sqlLines {
		stmt, err := db.Prepare(stmtString)
		if err != nil {
			log.Fatal(err)
			return err
		}

		tx, err := db.Begin();
		if err != nil {
			log.Fatal(err)
			return err
		}

		_, err = tx.Stmt(stmt).Exec()

		if err != nil {
			tx.Rollback()
			log.Fatal(err)
			return err;
		}
		tx.Commit()
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
func (Waypoint) Create() {
	// IMPORTANT NOTE: the ID is assigned by the database serialization:
	// the Waypoint passed as the receiver MUST HAVE an ID with some sentinel
	// value, let's say -1 (impossible ID), and then be assigned an ID
	// by the database upon being created in the database.
	// throw an error if an ID is passed that is not -1, since there
	// is an obvious issue. have a distinct type for this error
	// and describe it as something like "non-sentinel ID passed to Waypoint.create()"
}

func (Waypoint) Update() {
	// using the receiver, update the waypoint's values in the database based on its ID
}

func (Waypoint) Delete() {
	// using the receiver, delete the waypoint from the database based on its ID
}

func (*Waypoint) Get() {
	// the opposite of Waypoint.create()!
	// instead, populate all fields from the database
	// based on the ID provided
}

// Definitions for AEACRoutes DB methods

// Initializes an AEACRoute in the database 
// and assigns it a non-sentinel ID
// requires: ID == -1
func (r *AEACRoutes) Create() error {
	db := connectToDB()

	if r.ID != -1 {
		errMsg := "Non-Sentinel ID passed into AEACRoutes.Create()"
		log.Fatal(errMsg)
		return errors.New(errMsg)
	}

	tx, err := db.Begin();
	if err != nil {
		log.Fatal(err)
		return err
	}
	stmt, err := tx.Prepare(`INSERT INTO aeac_routes 
		(number, start_waypoint, end_waypoint, passengers, max_weight, value, remarks, odr) VALUES 
		(?, ?, ?, ?, ?, ?, ?, ?) RETURNING id`)

	if err != nil {
		log.Fatal(err)
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
		log.Fatal(err)
		return err;
	}
	
	err = tx.Commit()
	if err != nil {
		log.Fatal(err)
		return err;
	}
	return nil
}

// Updates the database entry with id == ID 
// with data in the AEACRoute struct.
// requires: ID != -1
func (r AEACRoutes) Update() error {
	db := connectToDB()

	if r.ID == -1 {
		errMsg := "Uncreated ID passed into AEACRoutes.Update()"
		log.Fatal(errMsg)
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
		odr = ? WHERE
		id = ?`)
	if err != nil {
		log.Fatal(err)
		return err
	}

	tx, err := db.Begin();
	if err != nil {
		log.Fatal(err)
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
		log.Fatal(err)
		return err;
	} else {
		err = tx.Commit()
		return err;
	}
}

// Deletes all AEACRoute from the database
// with id == ID
// requires: ID != -1
func (r AEACRoutes) Delete() error {
	db := connectToDB()

	if r.ID == -1 {
		errMsg := "Uncreated ID passed into AEACRoutes.Delete()"
		log.Fatal(errMsg)
		return errors.New(errMsg)
	}

	stmt, err := db.Prepare(`DELETE FROM aeac_routes WHERE id = ?`)
	if err != nil {
		log.Fatal(err)
		return err
	}

	tx, err := db.Begin();
	if err != nil {
		log.Fatal(err)
		return err
	}

	_, err = tx.Stmt(stmt).Exec(r.ID)

	if err != nil {
		tx.Rollback()
		log.Fatal(err)
		return err;
	} else {
		err = tx.Commit()
		return err;
	}
}

// Fetches data of an AEACRoute with id == iD 
// from the database and populates the struct
// requires: ID != -1
// returns: sql.ErrNoRows if no such entry exists
func (r *AEACRoutes) Get() error {
	db := connectToDB()

	if r.ID == -1 {
		errMsg := "Uncreated ID passed into AEACRoutes.Get()"
		log.Fatal(errMsg)
		return errors.New(errMsg)
	}

	query := `SELECT * FROM aeac_routes WHERE id = ?`
	tx, err := db.Begin();
	if err != nil {
		log.Fatal(err)
		return err
	}

	defer tx.Rollback();

	row := tx.QueryRow(query, r.ID)

	if err != nil {
		log.Fatal(err)
		return err;
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
		log.Fatal(err)
		return err;
	}
	return nil;
}

