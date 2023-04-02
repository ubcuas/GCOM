package main

import (
	"encoding/json"
	"io"
	"io/ioutil"
	"log"
	"net/http"
	"os"
	"strconv"
	"strings"

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

// after loading the waypoints into the db, do the flightpath generation for task2

func Task2MainHandler(c echo.Context) error {
	//read the routes into db
	r := c.Request().Body
	buf := new(strings.Builder)
	_, err := io.Copy(buf, r)
	if err != nil {
		log.Panic(err)
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	}
	lines := strings.Split(buf.String(), "\n")
	for _, str := range lines {
		args := strings.Split(str, ";")
		arg := strings.Split(args[0], ":")[0]
		arg = arg[12:]
		number, err := strconv.Atoi(strings.TrimSpace(arg))
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting "+arg+" to int!"))
		}

		arg = strings.Split(args[0], ":")[1]
		pax, err := strconv.Atoi(arg[1 : len(arg)-5])
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting "+arg+" to int!"))
		}

		arg = args[3][1 : len(args[3])-3]
		weight, err := strconv.ParseFloat(arg, 32)
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting "+arg+" to float!"))
		}

		remark := args[4]
		if remark == " nil" {
			remark = ""
		}

		arg = args[5][2:len(args[5])]
		value, err := strconv.ParseFloat(strings.TrimSpace(arg), 32)
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting "+arg+" to float!"))
		}

		route := AEACRoutes{
			ID:               -1,
			Number:           number,
			StartWaypoint:    args[1][1:len(args[1])],
			EndWaypoint:      args[2][1:len(args[2])],
			Passengers:       pax,
			MaxVehicleWeight: weight,
			Value:            value,
			Remarks:          remark,
			Order:            -1,
		}
		route.Create()
	}

	//run pathfinding on db entries now
	flightPlan, err := runPathfindingWithDBEntries()
	if err != nil {
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
	}

	return c.JSON(http.StatusOK, flightPlan)

}

// endpoint to load all UASWaypoints from json
func LoadWaypoints(c echo.Context) error {
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

func generateJSONError(message string) JSONResponse {
	errMsg := JSONResponse{
		Type:    "Error",
		Message: message,
	}
	return errMsg
}

func generateJSONMessage(message string) JSONResponse {
	errMsg := JSONResponse{
		Type:    "Message",
		Message: message,
	}
	return errMsg
}
