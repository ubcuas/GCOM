package main

import (
	"io"
	"log"
	"net/http"
	"strconv"
	"strings"

	"github.com/labstack/echo/v4"
)

func ParseTask1QRData(c echo.Context) error {
	r := c.Request().Body
	buf := new(strings.Builder)
	_, err := io.Copy(buf, r)
	if err != nil {
		log.Panic(err)
		return err
	}
	parts := strings.Split(buf.String(), ".")
	waypoint_names := strings.Split(parts[0][26:], ";")
	var waypoints []Waypoint;

	for _, name := range waypoint_names {
		wp_name := strings.TrimSpace(name);
		wp := Waypoint {
			ID: -1,
			Name: wp_name,
		}
		err = wp.Get()
		if err != nil {
			return c.String(http.StatusBadRequest, "No such waypoint " + wp_name + "!")
		}
		waypoints = append(waypoints, wp)
	}

	rejoin_name := strings.TrimSpace(parts[1][21:])
	rejoin := Waypoint {
		ID: -1,
		Name: rejoin_name,
	}
	err = rejoin.Get()
	if err != nil {
		return c.String(http.StatusBadRequest, "No such waypoint " + rejoin_name + "!")
	}
	restrict := RestrictedArea {
		ID: -1,
		Bounds: waypoints,
		RejoinPoint: rejoin,
	}
	restrict.Create()
	return c.String(http.StatusAccepted, "Restricted Zone Created!")
}

func ParseTask2QRData(c echo.Context) error {
	r := c.Request().Body
	buf := new(strings.Builder)
	_, err := io.Copy(buf, r)
	if err != nil {
		log.Panic(err)
		return err
	}
	lines := strings.Split(buf.String(), "\n")
	for _, str := range lines {
		args := strings.Split(str, ";")
		arg := args[0][13:len(args[0])]
		number, err := strconv.Atoi(arg)
		if err != nil {
			return c.String(http.StatusBadRequest, "Error converting " + arg + " to int!")
		}

		arg = args[1][1:len(args[1]) - 5]
		pax, err := strconv.Atoi(arg)
		if err != nil {
			return c.String(http.StatusBadRequest, "Error converting " + arg + " to int!")
		}

		arg = args[4][1:len(args[4]) - 3]
		weight, err := strconv.ParseFloat(arg, 32)
		if err != nil {
			return c.String(http.StatusBadRequest, "Error converting " + arg + " to float!")
		}
		

		remark := args[5]
		if remark == " nil" {
			remark = ""
		}

		arg = args[6][2:len(args[6])]
		value, err := strconv.ParseFloat(arg, 32)
		if err != nil {
			return c.String(http.StatusBadRequest, "Error converting " + arg + " to float!")
		}

		route := AEACRoutes{
			ID: -1,
			Number: number,
			StartWaypoint: args[2][1:len(args[2])],
			EndWaypoint: args[3][1:len(args[3])],
			Passengers: pax,
			MaxVehicleWeight: weight,
			Value: value,
			Remarks: remark,	
			Order: -1,
		}
		route.Create()
	}
	return c.String(http.StatusAccepted, "Route created!")
}

