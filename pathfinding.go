package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"os"
	"os/exec"
)

var input_filepath = "./pathfinding/Text.json"
var output_filepath = "./pathfinding/output.json"

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
	var input_filepath string

	file, err := json.MarshalIndent(pfInput, "", "    ")
	if err != nil {
		Error.Println(err)
		return err
	}

	// if DEBUG_FLAG {
	// 	input_filepath = "./pathfinding/Text_DEBUG.json"
	// } else {
	input_filepath = "./pathfinding/Text.json"
	// }

	_ = os.WriteFile(input_filepath, file, 0666)

	return nil
}

/**
 * Execute the pathfinding binary, creating an output file "output.json" in the process
 */
func runPathfinding() (bool, error) {
	fmt.Println("before check input file")
	if _, err := os.Stat(input_filepath); errors.Is(err, os.ErrNotExist) {
		err := errors.New(input_filepath + " does not exist")
		Error.Println(err)
		return false, err
	}

	_, err := exec.Command("powershell", "-c", "cd pathfinding; .\\UAS-Pathfinding.exe").CombinedOutput()
	if err != nil {
		Error.Println(err)
		return false, err
	}

	if _, err := os.Stat(output_filepath); errors.Is(err, os.ErrNotExist) {
		err := errors.New(output_filepath + " does not exist")
		Error.Println(err)
		return false, err
	}

	return true, nil
}

/*
*

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
