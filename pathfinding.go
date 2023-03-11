package main

import (
	"encoding/json"
	"errors"
	"os"
	"os/exec"
)

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
	file, err := json.MarshalIndent(pfInput, "", " ")
	if err != nil {
		Error.Println(err)
		return err
	}

	_ = os.WriteFile("./pathfinding/Text.json", file, 0666)

	return nil
}

/**
 * Execute the pathfinding binary, creating an output file "Output.json" in the process
 */
func runPathfinding() (bool, error) {
	if _, err := os.Stat("./pathfinding/Text.json"); errors.Is(err, os.ErrNotExist) {
		return false, errors.New("Text.json does not exist")
	}

	binary := exec.Command("./pathfinding/UAS-Pathfinding.exe")
	if errors.Is(binary.Err, exec.ErrDot) {
		binary.Err = nil
	}
	if err := binary.Run(); err != nil {
		Error.Println(err)
		return false, err
	}

	if _, err := os.Stat("Output.json"); errors.Is(err, os.ErrNotExist) {
		return false, errors.New("Output.json was not properly created")
	}

	return true, nil
}

/*
*
  - Read the output file "Output.json" and return the results in a slice of AEACRoutes,
    using the route IDs from pfInput
*/
func (pfInput PathfindingInput) readPathfindingOutput() (*[]AEACRoutes, error) {
	if _, err := os.Stat("./pathfinding/Output.json"); errors.Is(err, os.ErrNotExist) {
		return nil, errors.New("Output.json does not exist")
	}

	file, err := os.ReadFile("./pathfinding/Output.json")
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	var routeIDs []int

	err = json.Unmarshal(file, &routeIDs)
	if err != nil {
		Error.Println(err)
		return nil, err
	}

	var routes []AEACRoutes
	for index, routeID := range routeIDs {
		for _, route := range pfInput.AEACRoutes {
			if route.ID == routeID {
				routes = append(routes, route)
				routes[index].Order = index
			}
		}

	}
	return &routes, nil
}
