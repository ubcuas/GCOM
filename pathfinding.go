package main

//helper functions to convert existing types to pathfinding types
func (wp Waypoint) toPFWaypoint() PFWaypoint {
	return PFWaypoint{
		ID:        wp.ID,
		Latitude:  wp.Latitude,
		Longitude: wp.Longitude,
	}
}

func (r AEACRoutes) toPFRoute(startWaypointID int, endWaypointID int) PFRoute {
	return PFRoute{
		ID:          r.ID,
		Value:       r.Value,
		WaypointIDs: []int{startWaypointID, endWaypointID},
	}
}

//func to automatically send email with routes we're taking to competition
