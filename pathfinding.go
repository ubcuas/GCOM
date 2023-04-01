package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"runtime"
)

var pwd, _ = os.Getwd()
var input_filepath = filepath.Join(pwd, "pathfinding", "Text.json")
var output_filepath = filepath.Join(pwd, "pathfinding", "output.json")

// helper functions to convert existing types to pathfinding types
func (r AEACRoutes) toPFRoute(startWaypointID int, endWaypointID int) PFRoute {
	return PFRoute{
		ID:          r.ID,
		Value:       r.Value,
		WaypointIDs: []int{startWaypointID, endWaypointID},
	}
}

/**
 * Create the input "Text.json" file for the pathfinding binary and write it to disk
 */
func (pfInput PathfindingInput) createPathfindingInput() error {

	file, err := json.MarshalIndent(pfInput, "", "    ")
	if err != nil {
		Error.Println(err)
		return err
	}

	fmt.Println("input_filepath: ", input_filepath)
	_ = os.WriteFile(input_filepath, file, 0666)

	return nil
}

/**
 * Execute the pathfinding binary, creating an output file "output.json" in the process
 */
func runPathfinding() error {
	fmt.Println("before check input file")
	if _, err := os.Stat(input_filepath); errors.Is(err, os.ErrNotExist) {
		err := errors.New(input_filepath + " does not exist")
		Error.Println(err)
		return err
	}

	var shellToUse string

	if runtime.GOOS == "windows" {
		shellToUse = "powershell"

		_, err := exec.Command(shellToUse, "-c", "cd pathfinding; .\\UAS-Pathfinding.exe").CombinedOutput()
		if err != nil {
			Error.Println(err)
			return err
		}
	} else {
		shellToUse = "bash"

		_, err := exec.Command(shellToUse, "-c", "cd pathfinding; ./UAS-Pathfinding-Linux.out").CombinedOutput()
		if err != nil {
			Error.Println(err)
			return err
		}
	}

	if _, err := os.Stat(output_filepath); errors.Is(err, os.ErrNotExist) {
		err := errors.New(output_filepath + " does not exist")
		Error.Println(err)
		return err
	}

	return nil
}

/*
  - Read the output file "output.json" and return the results in a slice of AEACRoutes,
    using the route IDs from pfInput

    The order in which the routes are returned in the AEACRoutes slice is in non-decreasing order
    by the 'order' field in the AEACRoutes struct. (left to right in the pathfinding output
    of route IDs)
*/
func (pfInput PathfindingInput) readPathfindingOutput() (*[]AEACRoutes, error) {
	type routeIDstruct struct {
		RouteIDs []int `json:"Routes"`
	}

	if _, err := os.Stat(output_filepath); errors.Is(err, os.ErrNotExist) {
		return nil, errors.New(output_filepath + " does not exist")
	}

	file, err := os.ReadFile(output_filepath)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	// var routeID []int
	var idstruct routeIDstruct

	err = json.Unmarshal(file, &idstruct)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	fmt.Println("solution route ids: ", idstruct.RouteIDs)

	var routes []AEACRoutes
	for index, routeID := range idstruct.RouteIDs {
		for _, route := range pfInput.AEACRoutes {
			if route.ID == routeID {
				routes = append(routes, route)
				routes[index].Order = index
			}
		}

	}
	return &routes, nil
}

/*
 * Run the pathfinding binary with the routes and waypoints as stored in the database
 * Requires that all waypoints have been uploaded through the /waypoints endpoint,
 * and that all routes have been uploaded through the /qr/task2 endpoint
 * This function returns a list of routes that constitute the flight plan for task2,
 * and calls the email function to send the flight plan to the competition organizers
 */
func runPathfindingWithDBEntries() (*[]AEACRoutes, error) {
	//in order to create pfinput, we need a list of all waypoints in db
	queue, err := getAllWaypoints()
	if err != nil {
		Error.Println(err)
		return nil, err
	}
	var waypoints []Waypoint
	waypoints = (*queue).Queue

	//in order to create pfinput, we also need a list of all routes in db
	routes, err := getAllRoutes()
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	//create a list of PFRoutes from the list of routes
	var pfRoutes []PFRoute
	for _, route := range *routes {
		//iterate through all waypoints to find the ids of the start and end waypoints
		var startWaypointID, endWaypointID int
		for _, waypoint := range (*queue).Queue {
			if waypoint.Name == route.StartWaypoint {
				startWaypointID = waypoint.ID
			}
			if waypoint.Name == route.EndWaypoint {
				endWaypointID = waypoint.ID
			}
		}

		pfRoutes = append(pfRoutes, route.toPFRoute(startWaypointID, endWaypointID))
	}

	numWaypoints := len(waypoints)
	numRoutes := len(pfRoutes)

	//create routefinder
	routefinder := RouteFinder{
		// MaxFlyingDistance:  40.6,
		Speed:              16.1,
		Altitude:           4.65,
		ClimbRate:          5.6,
		StartingWaypointID: 0,
	}

	//create rerouter
	rerouter := ReRouter{
		CurrentLong:         50.4,
		CurrentLat:          60.8,
		WaypointObstacleIDs: []int{1, 3, 2},
		ReRouteWaypointID:   3,
	}

	//create pfinput
	pfInput := PathfindingInput{
		NumWaypoints: numWaypoints,
		NumRoutes:    numRoutes,
		WPQueue:      waypoints,
		RouteQueue:   pfRoutes,
		RouteFinder:  routefinder,
		ReRouter:     rerouter,
		AEACRoutes:   *routes,
	}

	//create input file
	err = pfInput.createPathfindingInput()
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	//run pathfinding
	err = runPathfinding()
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	//read output file
	flightPlan, err := pfInput.readPathfindingOutput()
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	//send email
	err = sendPaths(*flightPlan)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	return flightPlan, nil
}
