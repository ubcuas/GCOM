package main

import (
	"flag"
	"github.com/labstack/echo/v4"
)

var SkyScannerPath *string

func main() {
	SkyScannerPath = flag.String("qr", "", "Absolute Path to SkyScanner qr_code.py")
	flag.Parse()

	e := echo.New()

	e.GET("/", Hello)
	e.GET("qr/start", InitializeQR)
	e.POST("qr/return", ParseQRData)

	e.Logger.Fatal(e.Start(":1323"))
}

