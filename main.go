package main

import (
	"log"
	"os"
	"fmt"
	"io/ioutil"
	
	"encoding/json"
	"github.com/joho/godotenv"
	"github.com/labstack/echo/v4"
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
	UAS_COMP = true
	SUAS_COMP = false
)

func main() {
	
	e := echo.New()
	Migrate()
	deleteAllWaypoints()
	if UAS_COMP {
		loadUASWaypoints()
	} else if SUAS_COMP {

	}
	

	e.POST("/qr/task1", ParseTask1QRData)
	e.POST("/qr/task2", ParseTask2QRData)

	e.Logger.Fatal(e.Start(":1323"))
	
}

func loadUASWaypoints() {
	jsonFile, err := os.Open("waypoints.json")
	if err != nil {
    	fmt.Println(err)
	}
	fmt.Println("Successfully opened waypoints.json")
	defer jsonFile.Close()

	byteValue, _ := ioutil.ReadAll(jsonFile)
	var waypoints Queue
	json.Unmarshal(byteValue, &waypoints)
	for _, waypoint := range waypoints.Queue {
		err = waypoint.Create()
		fmt.Printf("Loaded Waypoint %s\n", waypoint.Name)
		if err != nil {
			fmt.Println(err)
		}
	}
}
