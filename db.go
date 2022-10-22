package main

import "database/sql"

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
func Migrate() {
	// read from migrate.sql, create a .sqlite database file by running
	// the query contained in migrate.sql

	// be sure to panic and log any errors!
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
// Follow the same general procedure as the Waypoints methods
func (AEACRoutes) Create() {

}

func (AEACRoutes) Update() {

}

func (AEACRoutes) Delete() {

}

func (*AEACRoutes) Get() {

}
