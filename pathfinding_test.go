package main

import (
	"errors"
	"fmt"
	"os"
	"testing"

	"github.com/stretchr/testify/assert"
)

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

func CreateSamplepfInput() *PathfindingInput {
	wp1 := Waypoint{
		ID:        -1,
		Name:      "alpha",
		Longitude: 48.5166707,
		Latitude:  -71.6375025,
		Altitude:  20.0,
	}

	wp2 := Waypoint{
		ID:        -1,
		Name:      "beta",
		Longitude: 48.5060947,
		Latitude:  -71.6317518,
		Altitude:  20.0,
	}

	wp3 := Waypoint{
		ID:        -1,
		Name:      "gamma",
		Longitude: 48.4921159,
		Latitude:  -71.6340069,
		Altitude:  50.0,
	}

	wp4 := Waypoint{
		ID:        -1,
		Name:      "delta",
		Longitude: 48.5150341,
		Latitude:  -71.6404442,
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
		EndWaypoint:      "delta",
		Passengers:       10,
		MaxVehicleWeight: 1000,
		Value:            50.2,
		Order:            1,
	}

	r3 := AEACRoutes{
		ID:               -1,
		Number:           4,
		StartWaypoint:    "alpha",
		EndWaypoint:      "gamma",
		Passengers:       10,
		MaxVehicleWeight: 1000,
		Value:            50.2,
		Order:            2,
	}

	wp1.Create()
	wp2.Create()
	wp3.Create()
	wp4.Create()
	r1.Create()
	r2.Create()
	r3.Create()

	wpQueue := []Waypoint{wp1, wp2, wp3, wp4}
	pfWPQueue := make([]Waypoint, len(wpQueue))
	//NOTE: pathfinding uses 0-indexed waypoints, so we need to subtract 1 from the waypoint IDs
	copy(pfWPQueue, []Waypoint{wp1, wp2, wp3, wp4})
	for i := range pfWPQueue {
		pfWPQueue[i].ID--
	}

	// assert.True(t, len(pfWPQueue) == 4)
	// assert.Equal(t, 0, pfWPQueue[0].ID)

	aeacRoutes := []AEACRoutes{r1, r2, r3}

	routefinder := RouteFinder{
		MaxFlyingDistance:  40.6,
		StartingWaypointID: 0,
	}

	rerouter := ReRouter{
		CurrentLong:         50.4,
		CurrentLat:          60.8,
		WaypointObstacleIDs: []int{1, 5, 2},
		ReRouteWaypointID:   3,
	}

	r1pf := r1.toPFRoute(pfWPQueue[0].ID, pfWPQueue[1].ID)
	r2pf := r2.toPFRoute(pfWPQueue[1].ID, pfWPQueue[3].ID)
	r3pf := r3.toPFRoute(pfWPQueue[0].ID, pfWPQueue[2].ID)

	pfRoutes := []PFRoute{r1pf, r2pf, r3pf}

	pfInput := PathfindingInput{
		NumWaypoints: len(pfWPQueue),
		NumRoutes:    len(pfRoutes),
		WPQueue:      pfWPQueue,
		RouteQueue:   pfRoutes,
		RouteFinder:  routefinder,
		ReRouter:     rerouter,
		AEACRoutes:   aeacRoutes,
	}

	wp1.Get()
	wp2.Get()
	wp3.Get()
	wp4.Get()

	// assert.Equal(t, 1, wp1.ID)

	return &pfInput
}

func TestCreateInputJson(t *testing.T) {
	cleanDB()

	err := CreateSamplepfInput().createPathfindingInput()
	assert.NoError(t, err)

	//time.Sleep(2 * time.Second)
	//check that Text.json exists
	_, err = os.Stat(input_filepath)
	assert.NoError(t, err)

	cleanDB()
}

func TestRunPathfinding(t *testing.T) {
	//remove any old input files if they exist
	if _, err := os.Stat(input_filepath); !errors.Is(err, os.ErrNotExist) {
		os.Remove(input_filepath)
	}

	//remove any old output files if they exist
	if _, err := os.Stat(output_filepath); !errors.Is(err, os.ErrNotExist) {
		os.Remove(output_filepath)
	}

	TestCreateInputJson(t)

	//time.Sleep(2 * time.Second)
	runPathfinding()

	//check that the output file exists

	//time.Sleep(2 * time.Second)
	if _, err := os.Stat(output_filepath); errors.Is(err, os.ErrNotExist) {
		t.Error("Output file does not exist")
	}
}

func TestReadPathfindingOutput(t *testing.T) {
	TestRunPathfinding(t)

	//time.Sleep(2 * time.Second)
	pfInput := CreateSamplepfInput()

	pfOutput, err := pfInput.readPathfindingOutput()
	assert.NoError(t, err)

	//expected solution route ids: 1, 3, 2

	routes := *(pfOutput)

	fmt.Println("output routes: ", routes)

	//check that first route is the same as route with id 1 (alpha-beta)
	assert.True(t, routes[0].StartWaypoint == pfInput.WPQueue[0].Name)
	assert.True(t, routes[0].EndWaypoint == pfInput.WPQueue[1].Name)

	//check that second route is the same as route with id 3 (alpha-gamma)
	assert.True(t, routes[1].StartWaypoint == pfInput.WPQueue[0].Name)
	assert.True(t, routes[1].EndWaypoint == pfInput.WPQueue[2].Name)

	//check that third route is the same as route with id 2 (beta-delta)
	assert.True(t, routes[2].StartWaypoint == pfInput.WPQueue[1].Name)
	assert.True(t, routes[2].EndWaypoint == pfInput.WPQueue[3].Name)

	cleanDB()
}
