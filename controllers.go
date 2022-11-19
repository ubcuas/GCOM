package main

import (
	"net/http"

	"github.com/labstack/echo/v4"
)

func Hello(c echo.Context) error {
	return c.String(http.StatusOK, "Hello, World!")
}

/*
func StartAEAC(c echo.Context) error {
	c.Request().Response.Request() -> routes


	 optimal_routes := FindOptimalRoutes()

	 SendRouteToMP(optimal_routes)
}
*/
