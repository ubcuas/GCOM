package main

import (
	"fmt"
	"net/http"

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
		// log.Fatal(err)
		Error.Println(err)
		return err
	}

	return c.JSON(http.StatusOK, allWaypoints)

}

// endpoint we serve that takes a JSON list of Waypoints and registers them all in the database
func PostWaypoints(c echo.Context) error {

	var waypoints []Waypoint

	err := c.Bind(&waypoints)
	if err != nil {
		// log.Fatal(err)
		Error.Println(err)
		return err
	}

	// waypoints[0].Create()
	// fmt.Println(waypoints)

	fmt.Println("Registering waypoints...")
	for _, wp := range waypoints {
		err = wp.Create()
		if err != nil {
			// log.Fatal(err)
			Error.Println(err)
			return err
		}
		fmt.Println(wp)
	}
	Info.Println("Registered waypoints: ", waypoints, "to the database")
	return c.String(http.StatusOK, "Waypoints successfully registered!")
	// return c.JSON(http.StatusOK, waypoints)
}

// endpoint we serve that responds with a list of all the routes currently in the database
func GetRoutes(c echo.Context) error {
	return nil
}

// endpoint we serve that takes a JSON list of Routes and registers them all in the database
func PostRoutes(c echo.Context) error {
	return nil
}

/*
func StartAEAC(c echo.Context) error {
	c.Request().Response.Request() -> routes


	 optimal_routes := FindOptimalRoutes()

	 SendRouteToMP(optimal_routes)
}
*/
