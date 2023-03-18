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
		return c.JSON(http.StatusInternalServerError, generateJSONError(err.Error()))
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
			return c.JSON(http.StatusBadRequest, generateJSONError("No such waypoint " + wp_name + "!"))
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
		return c.JSON(http.StatusBadRequest, generateJSONError("No such waypoint " + rejoin_name + "!"))
	}
	restrict := RestrictedArea {
		ID: -1,
		Bounds: waypoints,
		RejoinPoint: rejoin,
	}
	restrict.Create()
	return c.JSON(http.StatusAccepted, generateJSONError("Restricted Zone created!"))
}

func ParseTask2QRData(c echo.Context) error {
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
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting " + arg + " to int!"))
		}
		
		arg = strings.Split(args[0], ":")[1]
		pax, err := strconv.Atoi(arg[1: len(arg)-5])
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting " + arg + " to int!"))
		}

		arg = args[3][1:len(args[3]) - 3]
		weight, err := strconv.ParseFloat(arg, 32)
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting " + arg + " to float!"))
		}


		remark := args[4]
		if remark == " nil" {
			remark = ""
		}

		arg = args[5][2:len(args[5])]
		value, err := strconv.ParseFloat(strings.TrimSpace(arg), 32)
		if err != nil {
			return c.JSON(http.StatusBadRequest, generateJSONError("Error converting " + arg + " to float!"))
		}

		route := AEACRoutes{
			ID: -1,
			Number: number,
			StartWaypoint: args[1][1:len(args[1])],
			EndWaypoint: args[2][1:len(args[2])],
			Passengers: pax,
			MaxVehicleWeight: weight,
			Value: value,
			Remarks: remark,	
			Order: -1,
		}
		route.Create()
	}
	return c.JSON(http.StatusAccepted, generateJSONMessage("Routes created!"))
}

