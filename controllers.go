package main

import (
	"net/http"
	"os"
	"io/ioutil"
	"encoding/json"

	"github.com/labstack/echo/v4"
)

func Hello(c echo.Context) error {
	return c.String(http.StatusOK, "Hello, World!")
}

// endpoint we serve that responds with a list of all the waypoints currently in the database
func GetWaypoints(c echo.Context) error {

	queue, err := getAllWaypoints()
	allWaypoints := queue.Queue
	if err != nil {
		Error.Println(err)
		return err
	}

	/*
		prettyJson, err := json.MarshalIndent(allWaypoints, "", "    ")
		if err != nil {
			Error.Println(err)
			return err
		}
		fmt.Println("all waypoints in DB:\n", string(prettyJson))
	*/
	return c.JSON(http.StatusOK, allWaypoints)
}

// endpoint we serve that takes a JSON list of Waypoints and registers them all in the database, if they
// have not already been registered (a duplicate waypoint is defined as a waypoint with identical values in
// all fields but ID)
func PostWaypoints(c echo.Context) error {

	var waypoints []Waypoint

	err := c.Bind(&waypoints)
	if err != nil {
		Error.Println(err)
		return err
	}

	// Info.Println("Registering waypoints...", waypoints)

	for _, wp := range waypoints {
		err = wp.Create()
		if err != nil {
			// log.Fatal(err)
			Error.Println(err)
			return err
		}
		// fmt.Println(wp)
	}

	// fmt.Println("Registered waypoints: ", waypoints, "to the database")

	return c.String(http.StatusOK, "Waypoints successfully registered!")
}

// endpoint we serve that responds with a list of all the routes currently in the database
func GetRoutes(c echo.Context) error {

	routes, err := getAllRoutes()
	if err != nil {
		Error.Println(err)
		return err
	}

	// fmt.Println("All routes in DB: ", routes)

	return c.JSON(http.StatusOK, routes)
}

// endpoint we serve that takes a JSON list of Routes and registers them all in the database
func PostRoutes(c echo.Context) error {

	var routes []AEACRoutes

	err := c.Bind(&routes)
	if err != nil {
		Error.Println(err)
		return err
	}

	// fmt.Println("Registering routes: ", routes)

	for _, r := range routes {
		err = r.Create()
		if err != nil {
			Error.Println(err)
			return err
		}
	}

	// fmt.Println("Registered AEACRoutes: ", routes, "to the database!")

	return c.String(http.StatusOK, "AEACRoutes registered!")
}

// endpoint we serve that returns the next route to be taken (the one with the lowest 'order' value)
// deletes this route from the database after being returned
func GetNextRoute(c echo.Context) error {

	query := `SELECT * FROM aeac_routes ORDER BY odr ASC LIMIT 1`

	rows, err := querySelect(query)
	if err != nil {
		Error.Println(err)
		return err
	}

	var r AEACRoutes

	for rows.Next() {
		err = rows.Scan(&r.ID, &r.Number, &r.StartWaypoint, &r.EndWaypoint, &r.Passengers,
			&r.MaxVehicleWeight, &r.Value, &r.Remarks, &r.Order)
		if err != nil {
			Error.Println(err)
			return err
		}
	}

	// fmt.Println("Next route: ", r)
	err = r.Delete()
	if err != nil {
		Error.Println(err)
		return err
	}

	// Warning.Println("Deleted route with ID: ", r.ID)
	if r.StartWaypoint == "" {
		return c.JSON(http.StatusOK, nil)
	}
	return c.JSON(http.StatusOK, r)
}

//endpoint to load all UASWaypoints from json
func LoadWaypoints (c echo.Context) error {
	json_map := make(map[string]interface{})
	err := json.NewDecoder(c.Request().Body).Decode(&json_map)
	if err != nil {
    	return err
	} 
    
	competition, ok := json_map["competition"]
	if !ok {
		return c.String(http.StatusBadRequest, `JSON Body must contain valid "competition" field`)
	}

	var filename string
	
	if competition == "UAS" {
		filename = "uas_waypoints.json"
	} else {
		return c.String(http.StatusBadRequest, "No such competition")
	}

	jsonFile, err := os.Open(filename)
	if err != nil {
    	return c.String(http.StatusInternalServerError, "Cannot open waypoint file!")
	}
	defer jsonFile.Close()

	byteValue, _ := ioutil.ReadAll(jsonFile)
	var waypoints Queue
	json.Unmarshal(byteValue, &waypoints)
	for _, waypoint := range waypoints.Queue {
		err = waypoint.Create()
		Info.Printf("Loaded Waypoint %s\n", waypoint.Name)
		if err != nil {
			return c.String(http.StatusInternalServerError, "Error loading waypoints! (Do all waypoints have non-sentinel ID's?)")
		}
	}

	return c.String(http.StatusOK, "All UAS Waypoints Loaded!")
}
