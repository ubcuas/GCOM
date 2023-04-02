package main

import (
	"crypto/subtle"
	"log"
	"os"

	"github.com/joho/godotenv"
	"github.com/labstack/echo/v4"
	"github.com/labstack/echo/v4/middleware"
)

// get an environment variable from .env based on its key
// i.e. if .env contains the line: A="B", getEnvVariable(A) will
// return "B".
func getEnvVariable(key string) string {
	err := godotenv.Load(".env")

	if err != nil {
		log.Fatal("Error: couldn't load .env file!")
	}

	return os.Getenv(key)
}

var (
	DEBUG_FLAG = true
)

func main() {

	e := echo.New()
	e.Use(middleware.BasicAuth(func(username, password string, c echo.Context) (bool, error) {
		// Be careful to use constant time comparison to prevent timing attacks
		if subtle.ConstantTimeCompare([]byte(username), []byte(getEnvVariable("AUTH_USER"))) == 1 &&
			subtle.ConstantTimeCompare([]byte(password), []byte(getEnvVariable("AUTH_PASSWORD"))) == 1 {
			return true, nil
		}
		return false, nil
	}))

	Migrate()
	if DEBUG_FLAG {
		cleanDB()
	}

	e.GET("/", Hello)
	e.GET("/waypoints", GetWaypoints)
	e.POST("/waypoints", PostWaypoints)
	e.POST("/waypoints/load", LoadWaypoints)

	e.GET("/routes", GetRoutes)
	e.POST("/routes", PostRoutes)
	e.GET("/nextroute", GetNextRoute)

	e.POST("/qr/task1", ParseTask1QRData)
	e.POST("/qr/task2", ParseTask2QRData)

	e.Logger.Fatal(e.Start(":1323"))
}
