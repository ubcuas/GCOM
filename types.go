package main

// General purpose structures
// the "__,string" nomer is necessary due to MP endpoint returning numbers as strings, will remove when thats updated
type AircraftStatus struct { // not stored in the database! maintained in memory, live,
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

// type Queue []Waypoint
// wraps a []Waypoint for easier MP compatibility

type Queue struct {
	Queue []Waypoint `json:"queue"`
}

//pathfinding structures

//reduced struct for pathfinding-necessary info
type PFRoute struct {
	ID          int     `json:"routeID"`
	WaypointIDs []int   `json:"waypointIds"`
	Value       float64 `json:"dollarValue"`
}

type PathfindingInput struct {
	NumWaypoints int          `json:"numWaypoints"`
	NumRoutes    int          `json:"numRoutes"`
	WPQueue      []Waypoint   `json:"waypoints"`
	RouteQueue   []PFRoute    `json:"routes"`
	RouteFinder  RouteFinder  `json:"RouteFinder"`
	ReRouter     ReRouter     `json:"ReRouter"`
	AEACRoutes   []AEACRoutes `json:"aeacRoutes"` //to easily fetch the correct route struct after pf is done
}

type RouteFinder struct {
	// RouteIDs           []int   `json:"routeIds"`
	// MaxFlyingDistance  float64 `json:"maxFlyingDistance"`
	Speed              float64 `json:"speed"`
	Altitude           float64 `json:"altitude"`
	ClimbRate          float64 `json:"climbRate"`
	StartingWaypointID int     `json:"startingWaypointId"`
}

type ReRouter struct {
	CurrentLong         float64 `json:"currentLong"`
	CurrentLat          float64 `json:"currentLat"`
	WaypointObstacleIDs []int   `json:"waypointObstacleIds"`
	ReRouteWaypointID   int     `json:"reRouteWaypointId"`
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

type RestrictedArea struct {
	ID          int        `json:"id"`
	Bounds      []Waypoint `json:"bounds"`
	RejoinPoint Waypoint   `json:"rejoin_at"`
}

type JSONResponse struct {
	Type    string `json:"type"`
	Message string `json:"message"`
}
