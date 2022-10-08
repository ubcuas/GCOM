package main

import (
	"encoding/json"
	"net/http"
	"os/exec"

	"github.com/labstack/echo/v4"
)

func InitializeQR(c echo.Context) error {
	cmd := exec.Command("cmd", "/C", "python", *SkyScannerPath, "-c", "0", "-a", "http://localhost:1323/qr/return")

	if err := cmd.Run(); err != nil {
		return c.String(http.StatusInternalServerError, "Failed to Execute Command!")
	}
	return c.String(http.StatusAccepted, "QR Code Scanned")
}

func ParseQRData(c echo.Context) error {
	json_map := make(map[string]interface{})
	err := json.NewDecoder(c.Request().Body).Decode(&json_map)
	if err != nil {
		return c.String(http.StatusBadRequest, "Malformed JSON")
	}
	data := json_map["data"].(string)
	_ = data
	return c.String(http.StatusAccepted, "Data Accepted")
}
