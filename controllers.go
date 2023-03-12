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
	if err != nil {
		Error.Println(err)
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	}
	allWaypoints := queue.Queue

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
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	}

	// Info.Println("Registering waypoints...", waypoints)

	for _, wp := range waypoints {
		err = wp.Create()
		if err != nil {
			// log.Fatal(err)
			Error.Println(err)
			return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
		}
		// fmt.Println(wp)
	}

	// fmt.Println("Registered waypoints: ", waypoints, "to the database")

	return c.JSON(http.StatusOK, generateJSONMessage("Waypoints successfully registered!"))
}

// endpoint we serve that responds with a list of all the routes currently in the database
func GetRoutes(c echo.Context) error {

	routes, err := getAllRoutes()
	if err != nil {
		Error.Println(err)
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
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
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	}

	// fmt.Println("Registering routes: ", routes)

	for _, r := range routes {
		err = r.Create()
		if err != nil {
			Error.Println(err)
			return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
		}
	}

	// fmt.Println("Registered AEACRoutes: ", routes, "to the database!")

	return c.JSON(http.StatusOK, generateJSONMessage("AEACRoutes registered!"))
}

// endpoint we serve that returns the next route to be taken (the one with the lowest 'order' value)
// deletes this route from the database after being returned
func GetNextRoute(c echo.Context) error {

	query := `SELECT * FROM aeac_routes ORDER BY odr ASC LIMIT 1`

	rows, err := querySelect(query)
	if err != nil {
		Error.Println(err)
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	}

	var r AEACRoutes

	for rows.Next() {
		err = rows.Scan(&r.ID, &r.Number, &r.StartWaypoint, &r.EndWaypoint, &r.Passengers,
			&r.MaxVehicleWeight, &r.Value, &r.Remarks, &r.Order)
		if err != nil {
			Error.Println(err)
			return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
		}
	}

	// fmt.Println("Next route: ", r)
	err = r.Delete()
	if err != nil {
		Error.Println(err)
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
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
    	return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	} 
    
	competition, ok := json_map["competition"]
	if !ok {
		return c.JSON(http.StatusBadRequest, generateJSONError(`JSON Body must contain valid "competition" field`))
	}

	var filename string
	
	if competition == "UAS" {
		filename = "uas_waypoints.json"
	} else {
		return c.JSON(http.StatusBadRequest, generateJSONError("No such competition"))
	}

	jsonFile, err := os.Open(filename)
	if err != nil {
    	return c.JSON(http.StatusInternalServerError, generateJSONError("Cannot open waypoint file!"))
	}
	defer jsonFile.Close()

	byteValue, _ := ioutil.ReadAll(jsonFile)
	var waypoints Queue
	json.Unmarshal(byteValue, &waypoints)
	for _, waypoint := range waypoints.Queue {
		err = waypoint.Create()
		if err != nil {
			return c.JSON(http.StatusInternalServerError, generateJSONError("Error loading waypoints! (Do all waypoints have non-sentinel ID's?)"))
		}
	}

	return c.JSON(http.StatusOK, generateJSONMessage("All Waypoints Loaded!"))
}

func generateJSONError (message string) JSONResponse {
	errMsg := JSONResponse {
		Type: "Error",
		Message: message,
	}
	return errMsg
}

func generateJSONMessage (message string) JSONResponse {
	errMsg := JSONResponse {
		Type: "Message",
		Message: message,
	}
	return errMsg
}