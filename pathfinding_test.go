package main

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

// func TestConvertWPtoPFWP(t *testing.T) {
// 	wp := Waypoint{
// 		ID:        1,
// 		Name:      "gcomWP1",
// 		Longitude: -6.2,
// 		Latitude:  149.1651830,
// 		Altitude:  20.0,
// 	}

// 	pfwp := wp.toPFWaypoint()

// 	assert.Equal(t, wp.ID, pfwp.ID)
// 	assert.Equal(t, wp.Latitude, pfwp.Latitude)
// 	assert.Equal(t, wp.Longitude, pfwp.Longitude)
// }

func TestConvertAEACRoutestoPFRoute(t *testing.T) {
	cleanDB()

	wp1 := Waypoint{
		ID:        -1,
		Name:      "alpha",
		Longitude: -6.2,
		Latitude:  149.1651830,
		Altitude:  20.0,
	}

	wp2 := Waypoint{
		ID:        -1,
		Name:      "beta",
		Longitude: -36.3637798,
		Latitude:  147.1651830,
		Altitude:  20.0,
	}

	r := AEACRoutes{
		ID:               1,
		Number:           2,
		StartWaypoint:    "alpha",
		EndWaypoint:      "beta",
		Passengers:       10,
		MaxVehicleWeight: 1000,
		Value:            50.2,
		Order:            0,
	}

	err1 := wp1.Create()
	err2 := wp2.Create()

	if assert.NoError(t, err1) && assert.NoError(t, err2) {
		pfr := r.toPFRoute(1, 2)
		assert.Equal(t, r.ID, pfr.ID)
		assert.Equal(t, wp1.ID, pfr.WaypointIDs[0])
		assert.Equal(t, wp2.ID, pfr.WaypointIDs[1])
	}

	cleanDB()
}

func TestCreateInputJson(t *testing.T) {
	cleanDB()

	wp1 := Waypoint{
		ID:        -1,
		Name:      "alpha",
		Longitude: -6.2,
		Latitude:  149.1651830,
		Altitude:  20.0,
	}

	wp2 := Waypoint{
		ID:        -1,
		Name:      "beta",
		Longitude: -36.3637798,
		Latitude:  147.1651830,
		Altitude:  20.0,
	}

	wp3 := Waypoint{
		ID:        -1,
		Name:      "gamma",
		Longitude: -38.3637798,
		Latitude:  150.1651830,
		Altitude:  50.0,
	}

	r1 := AEACRoutes{
		ID:               -1,
		Number:           2,
		StartWaypoint:    "alpha",
		EndWaypoint:      "beta",
		Passengers:       10,
		MaxVehicleWeight: 1000,
		Value:            50.2,
		Order:            0,
	}

	r2 := AEACRoutes{
		ID:               -1,
		Number:           3,
		StartWaypoint:    "beta",
		EndWaypoint:      "gamma",
		Passengers:       10,
		MaxVehicleWeight: 1000,
		Value:            50.2,
		Order:            1,
	}

	assert.NoError(t, wp1.Create())
	assert.NoError(t, wp2.Create())
	assert.NoError(t, wp3.Create())
	assert.NoError(t, r1.Create())
	assert.NoError(t, r2.Create())

	aeacRoutes := []AEACRoutes{r1, r2}

	routefinder := RouteFinder{
		MaxFlyingDistance:  40.6,
		StartingWaypointID: 2,
	}

	rerouter := ReRouter{
		CurrentLong:         50.4,
		CurrentLat:          60.8,
		WaypointObstacleIDs: []int{1, 5, 2},
		ReRouteWaypointID:   3,
	}

	r1pf := r1.toPFRoute(1, 2)
	r2pf := r2.toPFRoute(2, 3)

	pfInput := PathfindingInput{
		NumWaypoints: 3,
		NumRoutes:    2,
		WPQueue:      []Waypoint{wp1, wp2, wp3},
		RouteQueue:   []PFRoute{r1pf, r2pf},
		RouteFinder:  routefinder,
		ReRouter:     rerouter,
		AEACRoutes:   aeacRoutes,
	}

	err := pfInput.createPathfindingInput()
	assert.NoError(t, err)

	cleanDB()
}
