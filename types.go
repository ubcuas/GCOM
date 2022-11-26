package main

import (
	"gopkg.in/guregu/null.v4"
)

// General purpose structures
type AircraftStatus struct { // not stored in the database! maintained in memory, live,
	Velocity  float64 `json:"velocity,string"`
	Longitude float64 `json:"longitude,string"`
	Latitude  float64 `json:"latitude,string"`
	Altitude  float64 `json:"altitude,string"`
	Heading   float64 `json:"heading,string"`
	// BatteryVoltage float64 `json:"voltage"`
}

type Waypoint struct {
	ID        int        `json:"id"`
	Name      string     `json:"name"`
	Longitude float64    `json:"longitude"`
	Latitude  float64    `json:"latitude"`
	Altitude  null.Float `json:"altitude"`
}

// type Queue []Waypoint

type Queue struct {
	Queue []Waypoint `json:"queue"`
}

// AEAC specific structures
type AEACRoutes struct {
	ID               int     `json:"id"`
	Number           int     `json:"number"`
	StartWaypoint    string  `json:"start_waypoint"` // Name of the waypoint
	EndWaypoint      string  `json:"end_waypoint"`   // Name of the waypoint
	Passengers       int     `json:"passengers"`
	MaxVehicleWeight float64 `json:"max_vehicle_weight"`
	Value            float64 `json:"value"`
	Remarks          string  `json:"remarks"`
	Order            int     `json:"order"`
}

// SUAS specific structures
