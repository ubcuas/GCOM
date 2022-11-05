package main

import (
	"database/sql"
	"testing"
	"github.com/stretchr/testify/assert"
)

func TestMain(t *testing.T) {

	db := connectToDB()

	err := Migrate()
	if err != nil {
		panic(err)
	}

	aeacQuery, err := db.Query(`SELECT * FROM aeac_routes`)
	if assert.NoError(t, err) {
		columns, err := aeacQuery.Columns()
		if err != nil {
			panic(err)
		}

		expected := []string{"id", "number", "start_waypoint",
			"end_waypoint", "passengers", "max_weight",
			"value", "remarks", "odr"}

		assert.ElementsMatch(t, columns, expected)
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
