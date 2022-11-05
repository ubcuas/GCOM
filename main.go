package main

import (
	"log"
	"os"

	"github.com/joho/godotenv"
	"github.com/labstack/echo/v4"
)

var SkyScannerPath string

func main() {
	err := godotenv.Load(".env")
  	if err != nil {
    	log.Fatal("Error loading .env file")
  	}
	SkyScannerPath = os.Getenv("PATH_TO_SKYSCANNER_FOLDER") + "\\src\\qr_code.py"

	e := echo.New()

	e.GET("/", Hello)
	e.GET("qr/start", InitializeQR)
	e.POST("qr/return", ParseQRData)

	e.Logger.Fatal(e.Start(":1323"))
}

