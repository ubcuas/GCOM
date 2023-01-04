package main

import (
	"database/sql"
	"testing"
	"log"
	"os"
	"strings"

	"github.com/stretchr/testify/assert"
	"gopkg.in/guregu/null.v4"
)

// test if sqlite database file is properly created
func TestCreateDBFile(t *testing.T) {

	connectToDB()

	_, err := os.Stat("database.sqlite")

	if assert.NoError(t, err) {
		assert.Equal(t, err, nil)
	}

	os.Remove("database.sqlite")
}

// test if waypoint tables are properly created
func TestCreateDBTables(t *testing.T) {

	db := connectToDB()

	err := Migrate()
	if err != nil {
		panic(err)
	}

	waypointQuery, err := db.Query(`SELECT * FROM Waypoints`)
	if assert.NoError(t, err) {
		columns, err := waypointQuery.Columns()
		if err != nil {
			panic(err)
		}

		expected := []string{"id", "name", "longitude", "latitude", "altitude"}

		assert.ElementsMatch(t, columns, expected)
	}
	defer waypointQuery.Close()

	aeacQuery, err := db.Query(`SELECT * FROM aeac_routes`)
	if assert.NoError(t, err) {
		columns, err := aeacQuery.Columns()
		if err != nil {
			panic(err)
		}

		expected := []string{"id", "number", "start_waypoint",
			"end_waypoint", "passengers", "max_weight",
			"value", "remarks", "order"}

		assert.ElementsMatch(t, columns, expected)

	}
}

// test registering a waypoint with sentinel id into database
func TestCreateValidWaypoint(t *testing.T) {
	wp := Waypoint{
		ID:        -1,
		Name:      "Test Waypoint",
		Longitude: -79.347015,
		Latitude:  43.651070,
		Altitude:  null.FloatFrom(12.2),
	}

	err := wp.Create()
	if assert.NoError(t, err) {
		db := connectToDB()

		stmt, err := db.Prepare(`SELECT * FROM Waypoints WHERE 
			name = ($1) AND 
			longitude = ($2) AND 
			latitude = ($3) AND
			altitude = ($4) LIMIT 1`)
		if err != nil {
			panic(err)
		}
		defer stmt.Close()

		row := stmt.QueryRow(wp.Name, wp.Longitude, wp.Latitude, wp.Altitude)

		var (
			id        int
			name      string
			longitude float64
			latitude  float64
			altitude  null.Float
		)

		err = row.Scan(&id, &name, &longitude, &latitude, &altitude)
		if err != nil {
			panic(err)
		}

		assert.True(t, id != -1)
		assert.Equal(t, name, wp.Name)
		assert.Equal(t, longitude, wp.Longitude)
		assert.Equal(t, latitude, wp.Latitude)
		assert.Equal(t, altitude, wp.Altitude)

		wp.Delete()

		//check waypoint id updated
		assert.Equal(t, id, wp.ID)
		assert.Equal(t, name, wp.Name)
		assert.Equal(t, longitude, wp.Longitude)
		assert.Equal(t, latitude, wp.Latitude)
		assert.Equal(t, altitude, wp.Altitude)

	}

}

// test returning an error when attempting to register a waypoint
// with non-sentinel id in database
func TestCreateInvalidWaypoint(t *testing.T) {
	wp := Waypoint{
		ID:        5,
		Name:      "Test Waypoint",
		Longitude: -79.347015,
		Latitude:  43.651070,
		Altitude:  null.FloatFrom(12.2)}

	err := wp.Create()
	if assert.Error(t, err) {
		assert.True(t, strings.Contains(err.Error(), "non-sentinel ID passed to Waypoint.Create()"))
	}

}

func TestGetWaypointID(t *testing.T) {
	startWP := Waypoint{
		ID:        -1,
		Name:      "Original Waypoint",
		Longitude: 12.12,
		Latitude:  13.13,
		Altitude:  null.FloatFrom(12.2),
	}

	if assert.NoError(t, startWP.Create()) {
		endWP := Waypoint{
			ID:        startWP.ID,
			Name:      "",
			Longitude: 0,
			Latitude:  0,
			Altitude:  null.FloatFrom(0.0),
		}

		endWP.Get()

		assert.Equal(t, endWP.ID, startWP.ID)
		assert.Equal(t, endWP.Name, startWP.Name)
		assert.Equal(t, endWP.Longitude, startWP.Longitude)
		assert.Equal(t, endWP.Latitude, startWP.Latitude)
		assert.Equal(t, endWP.Altitude, startWP.Altitude)
	}

}

// test deleting a waypoint record from the database
func TestDeleteWaypoint(t *testing.T) {

	wp := Waypoint{
		ID:        -1,
		Name:      "Garbage",
		Longitude: -1,
		Latitude:  -1,
		Altitude:  null.FloatFrom(-1.0),
	}

	if assert.NoError(t, wp.Create()) {
		if assert.NoError(t, wp.Delete()) {
			var count int

			db := connectToDB()

			query := `SELECT COUNT(*) FROM Waypoints WHERE
			id = ($1) AND
			name = ($2) AND
			longitude = ($3) AND
			latitude = ($4) AND
			altitude = ($5)`

			stmt, err := db.Prepare(query)
			if err != nil {
				panic(err)
			}
			defer stmt.Close()

			row := stmt.QueryRow(wp.ID, wp.Name, wp.Longitude, wp.Latitude, wp.Altitude)
			err = row.Scan(&count)
			if err != nil {
				log.Fatal(err)
			}

			assert.Equal(t, 0, count)
		}
	}

}

// test updating database records when waypoint struct is changed
func TestUpdateWaypoint(t *testing.T) {

	wp := Waypoint{
		ID:        -1,
		Name:      "Original Waypoint",
		Longitude: 0.0,
		Latitude:  0.0,
		Altitude:  null.FloatFrom(0.0),
	}

	if assert.NoError(t, wp.Create()) {
		wp.Name = "Edited Waypoint"
		wp.Longitude = 1.1
		wp.Latitude = 2.2
		wp.Altitude = null.FloatFrom(3.3)

		if assert.NoError(t, wp.Update()) {

			var (
				id        int
				name      string
				longitude float64
				latitude  float64
				altitude  null.Float
			)

			db := connectToDB()

			query := `SELECT * FROM Waypoints WHERE 
				id = ($1) AND
				name = ($2) AND
				longitude = ($3) AND
				latitude = ($4) AND
				altitude = ($5) LIMIT 1`

			stmt, err := db.Prepare(query)
			if err != nil {
				panic(err)
			}
			defer stmt.Close()

			row := stmt.QueryRow(wp.ID, wp.Name, wp.Longitude, wp.Latitude, wp.Altitude)
			if err != nil {
				panic(err)
			}

			err = row.Scan(&id, &name, &longitude, &latitude, &altitude)
			if err != nil {
				panic(err)
			}

			assert.Equal(t, wp.ID, id)
			assert.Equal(t, wp.Name, name)
			assert.Equal(t, wp.Longitude, longitude)
			assert.Equal(t, wp.Latitude, latitude)
			assert.Equal(t, wp.Altitude, altitude)
		}
	}
}

// test error thrown when calling Waypoint.Update() without first calling Waypoint.Create()
func TestUpdateInvalidWaypoint(t *testing.T) {
	wp := Waypoint{
		ID:        -1,
		Name:      "Test Waypoint",
		Longitude: -79.347015,
		Latitude:  43.651070,
		Altitude:  null.FloatFrom(12.2),
	}

	err := wp.Update()
	if assert.Error(t, err) {
		assert.True(t, strings.Contains(err.Error(), "sentinel ID (-1) passed to Waypoint.Update()"))
	}

}


func TestAEACCreate(t *testing.T) {
	createTestRoute(t)
	cleanUp()
}

func TestAEACGet(t *testing.T) {
	createTestRoute(t)

	route1 := AEACRoutes{
		ID: 1,
		Number: 1,
		StartWaypoint: "Alpha",
		EndWaypoint: "Zeta",
		Passengers: 4,
		MaxVehicleWeight: 500.00,
		Value: 200.00,
		Order: 1,
	}

	route3 := AEACRoutes {ID: 1}
	err := route3.Get()
	assert.Nil(t, err)
	assert.Equal(t, route1, route3)
	cleanUp()
}

func TestAEACGetNonExistentID(t *testing.T) {
	route := AEACRoutes {ID: 45}
	err := route.Get()
	assert.Equal(t, sql.ErrNoRows, err)
}

func TestAEACDelete(t *testing.T) {
	cleanUp()
	route := AEACRoutes {ID: 1}
	err := route.Get()
	assert.Equal(t, sql.ErrNoRows, err)
}

func TestAEACUpdate(t *testing.T) {
	createTestRoute(t)
	route := AEACRoutes {ID: 1}
	route.Get()
	route.Order = 10
	route.Update()

	route1 := AEACRoutes {ID: 1}
	route1.Get()
	assert.Equal(t, 10, route1.Order)
	cleanUp()
}

func createTestRoute(t *testing.T) {
	route1 := AEACRoutes{
		ID: -1,
		Number: 1,
		StartWaypoint: "Alpha",
		EndWaypoint: "Zeta",
		Passengers: 4,
		MaxVehicleWeight: 500.00,
		Value: 200.00,
		Order: 1,
	}

	err := route1.Create()
	assert.Nil(t, err);
}

func cleanUp() {
	route := AEACRoutes {ID: 1}
	route.Delete()
}
