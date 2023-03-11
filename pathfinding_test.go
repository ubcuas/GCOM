package main

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestConvertWPtoPFWP(t *testing.T) {
	wp := Waypoint{
		ID:        1,
		Name:      "gcomWP1",
		Longitude: -6.2,
		Latitude:  149.1651830,
		Altitude:  20.0,
	}

	pfwp := wp.toPFWaypoint()

	assert.Equal(t, wp.ID, pfwp.ID)
	assert.Equal(t, wp.Latitude, pfwp.Latitude)
	assert.Equal(t, wp.Longitude, pfwp.Longitude)
}

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
}
