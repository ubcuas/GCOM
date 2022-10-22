package main

// General purpose structures
type Waypoint struct {
	ID        int     `json:"id"`
	Name      string  `json:"name"`
	Longitude float64 `json:"longitude"`
	Latitude  float64 `json:"latitude"`
	Altitude  float64 `json:"altitude"`
}

// AEAC specific structures
type AEACRoutes struct {
	ID            int    `json:"id"`
	Number        int    `json:"number"`
	StartWaypoint string `json:"start_waypoint"` // Name of the waypoint
	EndWaypoint   string `json:"end_waypoint"`   // Name of the waypoint
}

// SUAS specific structures
