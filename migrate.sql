CREATE TABLE if not EXISTS Waypoints ('id' INTEGER PRIMARY KEY, 'name' TEXT, 'longitude' DOUBLE, 'latitude' DOUBLE, 'altitude' DOUBLE);
CREATE TABLE if not exists aeac_routes ('id' INTEGER PRIMARY KEY,'number' INTEGER,'start_waypoint_name' TEXT,'end_waypoint_name' TEXT,'passengers' INTEGER,'max_vehicle_weight' DOUBLE,'value' DOUBLE,'remarks' TEXT,'order' INTEGER);
