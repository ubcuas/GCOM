# Getting Started
This project relies on the [Echo](https://echo.labstack.com/guide/) framework. Read a bit more about it if you aren't familiar.

This project uses go 1.18. Details about the packages it relies on are in the `go.mod` file.

To run the project for development, simply run:
```
go run main.go controllers.go odlc.go pathfinding.go
```

To build a binary for the project, run:
```
go build
```

When running, the root of the project is available at `localhost:1323`.

For testing endpoints for development, it's highly recommended that you use an API client. [Insomnia](https://insomnia.rest/) is a great choice.

# File Structure
## main.go
Controllers are paired routes and HTTP request methods. Middleware, logging, and everything else should be configured here.

## controllers.go
Endpoints and controllers are defined here, with sample request/response bodies (if applicable):

* `(GET) /waypoints`
    * Returns a list of all `Waypoint`s currently registered in the database.
    * Sample response body:
    ```json
    [
        {
            "id": 1,
            "name": "Alpha",
            "longitude": 13.4,
            "latitude": -4.5,
            "altitude": 20.4
        },
        {
            "id": 2,
            "name": "Beta",
            "longitude": 13.57,
            "latitude": -4,
            "altitude": 50.4
        }
    ]
    ```

* `(POST) /waypoints`
    * Registers a provided list of `Waypoint`s into the database. If this request is made multiple times, any duplicate `Waypoint`s will not be re-registered into the database (a duplicate `Waypoint` is any `Waypoint` whose fields are identical, except possibly `id`).
    * Sample request body:
    ```json
    [
        {
            "id": -1,
            "name": "Alpha",
            "longitude": 13.4,
            "latitude": -4.5,
            "altitude": 20.4
        },
        {
            "id": -1,
            "name": "Beta",
            "longitude": 13.57,
            "latitude": -4.0,
            "altitude": 50.4
        }
    ]
    ```
    ---

* `(GET) /routes`
    * Returns a list of all `AEACRoutes` currently registered in the database
    * Sample response body:
    ```json
    [
        {
            "id": 1,
            "number": 1,
            "start_waypoint": "alpha",
            "end_waypoint": "delta",
            "passengers": 5,
            "max_vehicle_weight": 30.5,
            "value": 13.2,
            "remarks": "",
            "order": 0
        },
        {
            "id": 2,
            "number": 2,
            "start_waypoint": "delta",
            "end_waypoint": "zeta",
            "passengers": 10,
            "max_vehicle_weight": 20.05,
            "value": 17.3,
            "remarks": "remark",
            "order": 1
        }
    ]
    ```
* `(POST) /routes`
     * Registers a provided list of `AEACRoutes` into the database. If this request is made multiple times, any duplicate routes will not be re-registered into the database (a duplicate route is any `AEACRoutes` whose fields are identical, except possibly `id`).
    * Sample request body:
    ```json
    [
        {
            "id": -1,
            "number": 1,
            "start_waypoint": "alpha",
            "end_waypoint": "delta",
            "passengers": 5,
            "max_vehicle_weight": 30.5,
            "value": 13.2,
            "remarks": "",
            "order": 0
        },
        {
            "id": -1,
            "number": 2,
            "start_waypoint": "delta",
            "end_waypoint": "zeta",
            "passengers": 10,
            "max_vehicle_weight": 20.05,
            "value": 17.3,
            "remarks": "remark",
            "order": 1
        }
    ]
    ```

* `(GET) /nextroute`
    * Gets the next `AEACRoutes` that should be followed (the one with the lowest `order` out of the remaining `AEACRoutes`).
    * **Deletes** the `AEACRoutes` from the database after it is returned.
    * Sample response body:
    ```json
    {
        "id": 1,
        "number": 1,
        "start_waypoint": "alpha",
        "end_waypoint": "delta",
        "passengers": 5,
        "max_vehicle_weight": 30.5,
        "value": 13.2,
        "remarks": "",
        "order": 0
    }
    ```

* `(GET) /status`
    * Gets the current aircraft status (velocity, longitude, latitude, altitude, heading, battery voltage) from Mission Planner.
    * Sample response body:
    ```json
    {
        "velocity": 10.4,
        "latitude": -35.3627175,
        "longitude": 149.1514354,
        "altitude": 17.0569992065,
        "heading": 265.771789551,
        "voltage": 1.5
    }
    ```

## db.go

Functions that manage the storage of `Waypoint` and `AEACRoutes` with the local sqlite database are declared here:

* `cleanDB() error`
    * Deletes all `Waypoint`s and `AEACRoutes` currently registered in the sqlite database by issuing a DELETE query for each table, creating the requisite database tables if they do not already exist. 
    * Returns an error if either the creation of the database tables fails, or if the execution of any DELETE queries fail, and `nil` otherwise.
    

* `connectToDB() *sql.DB`
    * Creates a local sqlite database with filename `database.sqlite` in the root directory of the project if it does not already exist, and starts a database connection.
    * Returns a pointer to a `sql.DB` object required for sending queries to the database.
    * `panic`s if the sqlite file cannot be created or opened.


* `Migrate() error`
    * Reads the SQL queries from `migrate.sql` located in the root directory of the project, and executes each query against the database created by `connectToDB()` to initialize any required tables.
    * Returns an error if `migrate.sql` does not exist, or if executing the queries against the sqlite database fails for any reason, `nil` otherwise.
---


* `(wp *Waypoint) Create() error`
    * Saves the current `Waypoint` struct in the database, and updates the current `Waypoint` struct with the correct serialized `ID`. If this `Waypoint` already exists in the database (a database record exists that has the same name, longitude, latitude, and altitude), no new record is created, but the `wp` struct this method was called on will still be updated to the correct value in the database.
    * Requires that this `Waypoint.ID == -1` (i.e. this `Waypoint` has not yet been registered into the database). 
    * Calling `wp.Create()` mutates `wp.ID` and replaces it with the ID assigned by the database serialization (primary key).
    * Must be called on a `Waypoint` struct (i.e. calling `wp.Create()`).

* `(wp Waypoint) Update() error`
    * Updates the database record for a given `Waypoint` based on its ID. All records in the database with `id == wp.ID` will have their field values (ex. `name`, `longitude`, `latitude`, `altitude`) updated to the field values of `wp`.
    * Requires that `wp.Create()` has already been called for this `Waypoint` (i.e., that `wp.ID != -1`).
    * Must be called on a `Waypoint` struct (i.e. calling `wp.Update()`).

* `(wp Waypoint) Delete() error`
    * Deletes all database records with `id == wp.ID`.
    * Must be called on a `Waypoint` struct (i.e. calling `wp.Delete()`).

* `(wp *Waypoint) Get() error`
    * Writes the values stored in the database for this given `Waypoint`s ID to the `Waypoint` object.
    * Calling `wp.Get()` mutates `wp` and replaces all field values in `wp` with the values from the first record in the database with `id == wp.ID`.
    * Must be called on a `Waypoint` struct (i.e. calling `wp.Get()`).

* `getAllWaypoints() (*Queue, error)`
    * Gets all `Waypoint`s currently registered in the database.
    * Returns a pointer to a `Queue` that contains all currently registered `Waypoints`.
---
* `(r *AEACRoutes) Create() error`
    * Saves the current `AEACRoutes` struct in the database, and updates the current `AEACRoutes` struct with the correct serialized `ID`. If this `AEACRoutes` already exists in the database (a database record exists that has the same number, start_waypoint, end_waypoint, ...), no new record is created, but the `r` struct this method was called on will still be updated to the correct value in the database.
    * Requires that this `AEACRoutes.ID == -1` (i.e. this `AEACRoutes` has not yet been registered into the database). 
    * Calling `r.Create()` mutates `r.ID` and replaces it with the ID assigned by the database serialization (primary key).
    * Must be called on a `AEACRoutes` struct (i.e. calling `r.Create()`).

* `(r AEACRoutes) Update() error`
    * Updates the database record for a given `AEACRoutes` with the data in the AEACRoutes struct based on its ID.
    * Requires that `r.Create()` has already been called for this `AEACRoutes` (i.e., that `r.ID != -1`).
    * Must be called on a `AEACRoutes` struct (i.e. calling `r.Update()`).

* `(r AEACRoutes) Delete() error`
    * Deletes all database records with `id == r.ID`.
    * Must be called on a `AEACRoutes` struct (i.e. calling `r.Delete()`).

* `(r *AEACRoutes) Get() error`
    * Writes the values stored in the database for this given `AEACRoutes`s ID to the `AEACRoutes` object.
    * Calling `r.Get()` mutates `r` and replaces all field values in `r` with the values from the first record in the database with `id == r.ID`.
    * Must be called on a `AEACRoutes` struct (i.e. calling `r.Get()`).

* `getAllRoutes() (*[]AEACRoutes, error)`
    * Gets all `AEACRoutes` registered in the database.
    * Returns a pointer to an `[]AEACRoutes` that contains all routes currently in the database.

## mp.go

Functions that interface with MissionPlanner-Scripts are declared here.
For more detailed specifications on endpoint behavior, see https://github.com/ubcuas/MissionPlanner-Scripts

* `GetQueue() (*Queue, error)`
    * Gets the current queue of `Waypoint`s that constitute the path the drone is following from MissionPlanner.
    * Returns: Pointer to a `Queue` containing the `Waypoint`s generated from the data in the response from MissionPlanner
    * Interfaces with MissionPlanner-Scripts' `(GET) /queue` endpoint.

* `PostQueue(queue *Queue) error`
    * Overrides the current path that the drone is following with a new `Queue` of `Waypoint`s.
    * Param: `queue` - a pointer to a `Queue` struct containing the new sequence of waypoints to follow.
    * Interfaces with MissionPlanner-Scripts' `(POST) /queue` endpoint.

* `GetAircraftStatus() (*AircraftStatus, error)`
    * Gets the current aircraft status from MissionPlanner
    * Returns: a pointer to a `AircraftStatus` struct with fields populated with data from MissionPlanner response.
    * Interfaces with MissionPlanner-Scripts' `(GET) /status` endpoint.

* `LockAircraft() error`
    * Locks the aircraft (prevents the aircraft from moving based on the MissionPlanner queue).
    * Interfaces with MissionPlanner-Scripts' `(GET) /lock` endpoint.

* `UnlockAircraft() error`
    * Unlocks the aircraft (resume aircraft movement based on the MissionPlanner queue).
    * Interfaces with MissionPlanner-Scripts' `(GET) /unlock` endpoint.

* `ReturnToLaunch() error`
    * Directs the aircraft to head back to the original launch site, or a newly defined `home` waypoint.
    * Interfaces with the MissionPlanner-Scripts' `(GET) /rtl` endpoint.

* `LandImmediately() error`
    * Immediately descends the aircraft and lands over its current position.
    * Interfaces with the MissionPlanner-Scripts' `(GET) /land` endpoint.

* `PostHome(home *Waypoint) error`
    * Sets the `home` waypoint of the aircraft. This is the waypoint that the aircraft will return and land at if `ReturnToLaunch()` is called.
    * Param: `home` - The `Waypoint` to set as the new home waypoint for the aircraft.
    * Interfaces with MissionPlanner-Scripts' `(POST) /home` endpoint.

* `Takeoff(altitude float64) error`
    * Commands the aircraft to take off to a given altitude, measured above sea level. This must be issued while the aircraft is armed in MissionPlanner.
    * Interfaces with MissionPlanner-Scripts' `(POST) /takeoff` endpoint.
## odlc.go
Any functions related to interfacing with ODLC are declared here.

## pathfinding.go
Functions that interface with the Pathfinding module. For more information, see https://github.com/ubcuas/Pathfinding

* `(r AEACRoutes) toPFRoute(startWaypointID int, endWaypointID int) PFRoute`
    * Helper function to convert existing `AEACRoutes` struct into a format compatible with pathfinding data ingest.
    * Param: `startWaypointID` - the integer ID of the `Waypoint` corresponding to `r.StartWaypoint`. This is determined by the database serialization and so the constituent `Waypoint`s of `r` must have been `Create()`d already.
    * Param: `endWaypointID` - the integer ID fof the `Waypoint` corresponding to `r.EndWaypoint`.
    * Returns a `PFRoute` struct that represents the same route as `r`.

* `(pfInput PathfindingInput) createPathfindingInput() error`
    * Creates the input `Text.json` file for the pathfinding module from the data contained within a `PathfindingInput` struct. This file is at `./pathfinding/Text.json`.
    * Returns an error if unable to write to the specified location.

* `runPathfinding() error`
    * Run the pathfinding module.
    * Requires that an input json file named `Text.json` has already been created in `./pathfinding/`.
    * On success, creates an output json file in `./pathfinding/output.json` containing a json object determining the order of routes to be taken, based on the information provided in `pfInput` when `CreatePathfindingInput` was called.
    * For example, this may be the contents of `./pathfinding/output.json`:
        ```json
        {"Routes":[1,3,2]}
        ```
        in this case, we should first take the route in `pfInput.AEACRoutes` with `ID == 1`, then the route in `pfInput.AEACRoutes` with `ID == 3`, and finally the route in `pfInput.AEACRoutes` with `ID == 2`.

    * Returns an error if no matching input json file exists, or if the pathfinding executable is not located at `./pathfinding/UAS-Pathfinding.exe`, or if the output file is not properly created.

* `(pfInput PathfindingInput) readPathfindingOutput() (*[]AEACRoutes, error)`
    * Reads the output of the pathfinding module and determines the order of routes to be taken. This is done by searching `pfInput.AEACRoutes` for an `AEACRoutes` with a matching `ID`, updating that route's `Order`, and appending it to a `[]AEACRoutes`.
    * Returns a pointer to that `[]AEACRoutes` after all routes are appended.
    * Requires that `pfInput` is the same `PathfindingInput` struct that was used to call `createPathfindingInput()`.

## rcomms.go
Any functions related to interfacing with the RCOMMS system are declared here. Most likely, this include instantiation of socket connections via a middleware to allow for endpoints in the form of controllers to be reached. 

## mail.go
Any functions related to sending emails. Used to automate sending the route plan email. Includes formating and sending an email with a given route.

# Tests
Tests are ran on a per file basis. For example, tests for `odlc.go` will be in `odlc_test.go`.

Tests can be ran with `go test <test files to run> <files being tested>`.
Alternatively, `go test` will automatically test all files in the project.

Be sure to write a test for every function you make, barring some exceptions like `main`.

There are options for coverage and verbosity as part of `go test`.


# Database
The database is a SQLite based database generated by shell scripts.

## Tables
*format: Tablename (Corresponding Go Struct)*
### waypoints (Waypoint)
- id: serial, primary key, internal
- name: text, identifier, arbitrary and provided by the competition
- longitude (degrees)
- latitude (degrees)
- altitude (meters above sea level)

### aeac_routes (AEACRoute)
- id: serial, primary key, internal
- number: integer, provided by the competition, used to identify routes to the judges
- start_waypoint_name: name of the starting waypoint
- end_waypoint_name:   name of the ending waypoint
- passengers: number of passengers being transported
- max_vehicle_weight: maximum weight of the vehicle allowed for the route (kg or lbs? unsure yet. just a number for now)
- value: $ value of route, floating point value
- remarks: text, indicates any additional details about the parameters of the route
- order: cardinal number indicating the nth order we are taking this route in. nullable. if null, we are not taking this route.
