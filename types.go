package main

// General purpose structures
type AircraftStatus struct { // not stored in the database! maintained in memory, live
	Velocity       float64 `json:"velocity"`
	Longitude      float64 `json:"longitude"`
	Latitude       float64 `json:"latitude"`
	Altitude       float64 `json:"altitude"`
	Heading        float64 `json:"heading"`
	BatteryVoltage float64 `json:"voltage"`
}

type Waypoint struct {
	ID        int     `json:"id"`
	Name      string  `json:"name"`
	Longitude float64 `json:"longitude"`
	Latitude  float64 `json:"latitude"`
	Altitude  float64 `json:"altitude"`
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
