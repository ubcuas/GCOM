package main

import (
	"fmt"
	"log"
	"os"

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

func main() {
	queue, err := GetQueue()
	if err != nil {
		panic(err)
	}

	fmt.Println(queue)

	// for i := 0; i < len(queue.Queue); i++ {
	// 	fmt.Println(queue.Queue[i].ID)
	// 	fmt.Println(queue.Queue[i].Name)
	// 	fmt.Println(queue.Queue[i].Longitude)
	// 	fmt.Println(queue.Queue[i].Latitude)
	// 	fmt.Println(queue.Queue[i].Altitude)
	// }

	e := echo.New()

	e.GET("/", Hello)

	e.Logger.Fatal(e.Start(":1323"))
}
