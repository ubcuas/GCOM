import json

from flask import Flask, request
from flask_socketio import SocketIO
from pymavlink.mavutil import mavfile

from server.common.encoders import command_int_to_string, command_string_to_int
from server.common.status import Status
from server.common.wpqueue import Waypoint, WaypointQueue
from server.features.aeac_scan import scan_area
from server.features.aeac_water_delivery import generate_water_wps
from server.logging_config import logger
from server.operations.change_modes import change_flight_mode
from server.operations.get_info import get_current_mission, get_status
from server.operations.land import land_in_place
from server.operations.prepare_takeoff import prepare_takeoff
from server.operations.queue import clear_mission, new_mission, set_home
from server.operations.takeoff import arm_disarm
from server.services import MavlinkHandler, StatusCache
from server.utilities.request_message_streaming import get_parameter, set_parameter


class HTTP_Server:
    def __init__(
        self, mav_connection, status_cache: StatusCache, handler: MavlinkHandler
    ):
        self.mav_connection: mavfile = mav_connection
        self.status_cache = status_cache
        self.handler = handler

    def create_app(self):
        app = Flask(__name__)
        socketio = SocketIO(app)

        # GET endpoints
        @app.route("/", methods=["GET"])
        def index():
            """
            Call this to see if the server's running.
            ---
            responses:
              "200":
                description: server is running
            """
            return "Server Running", 200

        @app.route("/queue", methods=["GET"])
        def get_queue():
            """
            Returns the current list of waypoints remaining in the queue
            ---
            tags:
              - queue
            summary: Returns the current list of waypoints remaining in the queue
            description: >-
              Returns the current list of queue of waypoints to hit.
              Waypoints that have been passed and removed from the queue are not displayed.
            responses:
              "200":
                description: successful operation
                content:
                  application/json:
                    schema:
                      type: array
                      items:
                        $ref: "#/components/schemas/Waypoint"
            """
            try:
                curr = get_status(self.status_cache)._wpn
                wpq = get_current_mission(self.handler)

                formatted = []
                for wp in wpq:
                    wp_dict = wp.get_asdict()
                    wp_dict.update(wp.get_command())

                    formatted.append(wp_dict)

                remaining = json.dumps(formatted[curr:])

                logger.info("Queue sent to GCOM")

                return remaining, 200
            except TimeoutError as e:
                logger.error(f"Failed to retrieve mission: {str(e)}")
                return "Failed to retrieve mission from drone", 400

        @app.route("/queue", methods=["POST"])
        def post_queue():
            """
            Overwrite the queue with a new list of waypoints
            ---
            tags:
              - queue
            summary: Overwrite the queue with a new list of waypoints
            description: >-
              POST request containing a list of waypoints with names and longitude,
              latitude, and altitude values. If the altitude for a waypoint is null,
              uses the altitude of the previous waypoint (in the case of the first
              waypoint, uses the current altitude of the drone).
              The existing waypoint queue will be overwritten and lost.
              Longitude, name, and latitude must not be null/empty. Returns a Bad
              Request status code and error message in that case. Longitude and
              latitude in degrees, altitude in meters.
            requestBody:
              content:
                application/json:
                  schema:
                    type: array
                    items:
                      $ref: "#/components/schemas/Waypoint"
            responses:
              "200":
                description: successful operation
              "400":
                description: bad request
            """
            payload = request.get_json()

            ret = get_status(self.status_cache)
            last_altitude = ret.as_dictionary().get("altitude", 50)

            wpq = []
            for wpdict in payload:
                altitude = wpdict.get("altitude")
                if altitude is not None:
                    last_altitude = altitude
                else:
                    altitude = last_altitude

                command = wpdict.get("command", "WAYPOINT")
                # converts any unknown waypoint types to WAYPOINT
                command = command_int_to_string(command_string_to_int(command))

                param1 = wpdict.get("param1", 0)
                param2 = wpdict.get("param2", 0)
                param3 = wpdict.get("param3", 0)
                param4 = wpdict.get("param4", 0)

                wp = Waypoint(
                    wpdict["id"],
                    wpdict["name"],
                    wpdict["latitude"],
                    wpdict["longitude"],
                    last_altitude,
                    command,
                    param1,
                    param2,
                    param3,
                    param4,
                )
                wpq.append(wp)

            success = new_mission(self.handler, WaypointQueue(wpq.copy()))
            wpq.clear()

            if success:
                return "ok", 200
            else:
                return "Error uploading mission", 400

        @app.route("/insert", methods=["POST"])
        def post_insert_wp():
            """
            Inserts waypoints immediately before the current waypoint
            ---
            tags:
             - queue
            summary: Inserts waypoints immediately before the current waypoint
            requestBody:
              content:
                application/json:
                  schema:
                    type: array
                    items:
                      $ref: "#/components/schemas/Waypoint"
            responses:
              "200":
                description: successful operation
              "400":
                description: bad request
            """
            try:
                payload = request.get_json()

                ret: Status = get_status(self.status_cache)
                last_altitude = ret._alt if ret != () else 50

                curr = max(ret._wpn, 1)
                curr_wpq = get_current_mission(self.handler)

                # gets new waypoints
                new_waypoints = []
                for wpdict in payload:
                    altitude = wpdict.get("altitude")
                    if altitude is not None:
                        last_altitude = altitude
                    else:
                        altitude = last_altitude

                    command = wpdict.get("command", "WAYPOINT")
                    # converts any unknown waypoint types to WAYPOINT
                    command = command_int_to_string(command_string_to_int(command))

                    param1 = wpdict.get("param1", 0)
                    param2 = wpdict.get("param2", 0)
                    param3 = wpdict.get("param3", 0)
                    param4 = wpdict.get("param4", 0)

                    wp = Waypoint(
                        wpdict["id"],
                        wpdict["name"],
                        wpdict["latitude"],
                        wpdict["longitude"],
                        last_altitude,
                        command,
                        param1,
                        param2,
                        param3,
                        param4,
                    )
                    new_waypoints.append(wp)

                # start list with new waypoints, extend with current mission at the end
                new_waypoints.extend(curr_wpq.aslist()[curr:])

                success = new_mission(self.handler, WaypointQueue(new_waypoints.copy()))
                new_waypoints.clear()

                if success:
                    return "Waypoint(s) Inserted", 200
                else:
                    return "Failed to set new mission", 400
            except TimeoutError as e:
                logger.error(f"Failed to retrieve current mission: {str(e)}")
                return "Failed to retrieve current mission from drone", 400

        @app.route("/clear", methods=["GET"])
        def get_clear_queue():
            """
            Clear the waypoint queue.
            ---
            tags:
              - queue
            summary: Clear the waypoint queue.
            description: >-
              Idempotent with POST'ing an empty queue to /queue.
            responses:
              "200":
                description: Queue successfully emptied.
            """
            result = clear_mission(self.handler)

            if result:
                return "Mission has been Cleared", 200
            else:
                return "Failed to clear mission", 400

        @app.route("/status", methods=["GET"])
        def get_status_handler():
            """
            Obtain the aircraft status
            ---
            tags:
              - status
            summary: Obtain the aircraft status
            description: >-
              GET request returns the aircraft status. Velocity in m/s. Altitude
              in meters and is relative to sea level. Longitude, latitude, heading
              in degrees.
            responses:
              "200":
                description: Successful operation
                content:
                  application/json:
                    schema:
                      $ref: "#/components/schemas/Status"
            """
            logger.info("Status sent to GCOM")
            s = get_status(self.status_cache).as_dictionary()
            return s, 200

        @app.route("/prepare_takeoff", methods=["POST"])
        def post_prepare_takeoff():
            """
            Prepare takeoff sequence
            ---
            tags:
              - takeoff
            summary: Prepare takeoff sequence with waypoint at altitude
            requestBody:
              content:
                application/json:
                  schema:
                    type: object
                    properties:
                      altitude:
                        type: number
            responses:
              "200":
                description: Takeoff sequence prepared successfully
              "400":
                description: Failed to prepare takeoff sequence
            """
            payload = request.get_json()

            if "altitude" not in payload:
                return "Altitude cannot be null", 400

            altitude = float(payload["altitude"])
            logger.info(
                f"Preparing takeoff sequence with waypoint at altitude {altitude}"
            )

            try:
                result = prepare_takeoff(self.handler, self.status_cache, altitude)

                if result:
                    return "Takeoff sequence prepared successfully", 200
                else:
                    return "Failed to prepare takeoff sequence", 400

            except Exception as e:
                logger.error(f"Prepare takeoff failed - {type(e).__name__}: {str(e)}")
                return "Prepare takeoff failed - unknown error", 500

        @app.route("/arm", methods=["PUT"])
        def put_arm_disarm_drone():
            """
            Arm or disarm the motors. Take care.
            ---
            summary: Arm or disarm the motors. Take care.
            description: >-
              Call this endpoint to arm or disarm the drone.
              A value of True in the payload will arm the drone, while False will disarm the drone.
              If the request returns 200, the drone will be in the requested state.
            requestBody:
              content:
                application/json:
                  schema:
                    type: object
                    properties:
                      arm:
                        type: boolean
            responses:
              "200":
                description: Arm/disarm successful.
              "400":
                description: Arm/disarm failed - drone is NOT in the requested state
            """
            input = request.get_json()

            if input["arm"] in [1, 0]:
                arm = bool(input["arm"])

                logger.info("ARMING drone" if arm else "DISARMING drone")

                result = arm_disarm(self.handler, arm)

                if result == 0:
                    return (
                        f"OK! {'Armed drone' if input['arm'] else 'Disarmed drone'}",
                        200,
                    )
                else:
                    return (
                        "Arm/disarm failed - drone is NOT in the requested state",
                        400,
                    )
            else:
                return "Unrecognized arm/disarm command parameter", 400

        @app.route("/prepare_rtl_params", methods=["POST"])
        def post_prepare_rtl_params():
            """
            Prepare RTL parameters
            ---
            tags:
              - landing
            summary: Set RTL altitude parameter
            requestBody:
              content:
                application/json:
                  schema:
                    type: object
                    properties:
                      altitude:
                        type: number
            responses:
              "200":
                description: RTL parameters prepared successfully
              "400":
                description: Failed to set RTL altitude parameter
            """
            payload = request.get_json()

            if "altitude" not in payload:
                return "Altitude cannot be null", 400

            altitude = payload["altitude"]
            print(f"Preparing RTL params with altitude {altitude}")

            # set RTL altitude parameter
            alt_cm = altitude * 100

            if not set_parameter(self.handler, "RTL_ALT", alt_cm):
                return "Failed to set RTL altitude parameter", 400

            return "RTL parameters prepared successfully", 200

        @app.route("/rtl", methods=["POST"])
        def post_rtl():
            """
            Return to launch
            ---
            tags:
              - landing
            summary: return to launch
            description: >-
              Aircraft returns to home waypoint and lands (return-to-launch).
            responses:
              "200":
                description: Successful, drone has initiated RTL procedure
              "400":
                description: Failed to RTL
            """
            logger.info("RTL initiated")

            success = change_flight_mode(
                self.handler,
                self.mav_connection.target_system,
                self.mav_connection.target_component,
                "RTL",
            )

            if success:
                return "Returning to Launch", 200
            else:
                return "Failed to RTL", 400

        @app.route("/land", methods=["GET"])
        def get_land():
            """
            Immediately descend and land
            ---
            tags:
              - landing
            summary: immediately descend and land
            description: >-
              Aircraft stops at its current position and lands.
              Returns a Bad Request status code and error message if the drone
              could not execute the operation.
            responses:
              "200":
                description: Successful, drone has started landing procedure
              "400":
                description: Could not initiate landing
            """
            logger.info("Landing")
            if not change_flight_mode(
                self.handler,
                self.mav_connection.target_system,
                self.mav_connection.target_component,
                "LOITER",
            ):
                return "Failed to set drone into LOITER mode", 400

            if land_in_place(self.handler) == 0:
                return "Landing Immediately", 200
            else:
                return "Landing failed", 400

        @app.route("/land", methods=["POST"])
        def post_land():
            """
            Land at designated location
            ---
            tags:
              - landing
            summary: land at designated location
            description: >-
              Land at the location in the request, approaching at the altitude specified in the request body
              before descending to the ground.
            requestBody:
              content:
                application/json:
                  schema:
                    $ref: "#/components/schemas/Waypoint"
            responses:
              "200":
                description: Successful, drone has started landing procedure
              "400":
                description: Invalid coordinates - latitude and/or longitude value was null
            """
            # land_at_position does not seem to work for a copter
            # land_at_position(self.mav_connection, land.get("latitude"), land.get("longitude")) == 0:

            land = request.get_json()
            if "latitude" not in land or "longitude" not in land:
                return "Latitude and Longitude cannot be null", 400

            landing_mission = WaypointQueue()
            landing_mission.push(
                Waypoint(
                    0,
                    "Approach",
                    land.get("latitude"),
                    land.get("longitude"),
                    land.get("altitude", 35),
                )
            )
            landing_mission.push(
                Waypoint(
                    1, "Landing", land.get("latitude"), land.get("longitude"), 0, "LAND"
                )
            )

            if new_mission(self.handler, landing_mission):
                return "Landing at Specified Location", 200
            else:
                return "Landing failed", 400

        @app.route("/home", methods=["POST"])
        def post_home():
            """
            Set the home waypoint of the drone
            ---
            tags:
              - landing
            summary: set the home waypoint of the drone
            description: >-
              POST request containing a waypoint whose longitude, latitiude and
              altitiude will be the basis for the new home waypoint. All other
              fields will be ignored.
            requestBody:
              content:
                application/json:
                  schema:
                    $ref: "#/components/schemas/Waypoint"
            responses:
              "200":
                description: Successfully set new home position
              "400":
                description: Home position could not be set
            """
            home: dict = request.get_json()

            if (
                "longitude" not in home
                or "latitude" not in home
                or "altitude" not in home
            ):
                return "Long/lat/alt cannot be null", 400

            if (
                set_home(
                    self.handler,
                    home.get("latitude"),
                    home.get("longitude"),
                    home.get("altitude"),
                )
                == 0
            ):
                return "Setting New Home", 200
            else:
                return "New Home NOT set", 400

        @app.route("/flightmode", methods=["PUT"])
        def flight_mode():
            """
            Change flight mode of the aircraft
            ---
            tags:
              - options
            summary: Change flight mode of the aircraft
            description: >-
              Set the flight mode of the aircraft to Loiter, Stabilize, Auto, or Guided, or set the current aircraft type to Copter or Plane.
            requestBody:
              content:
                application/json:
                  schema:
                    type: object
                    properties:
                      mode:
                        type: string
                        enum: ['STABILIZE', 'ACRO', 'ALT_HOLD', 'AUTO', 'GUIDED', 'LOITER', 'RTL', 'CIRCLE', 'POSITION', 'LAND', 'OF_LOITER', 'DRIFT', 'SPORT', 'FLIP', 'AUTOTUNE', 'POSHOLD', 'BRAKE', 'THROW', 'AVOID_ADSB', 'GUIDED_NOGPS', 'SMART_RTL', 'FLOWHOLD', 'FOLLOW', 'ZIGZAG', 'SYSTEMID', 'AUTOROTATE', 'AUTO_RTL']
            responses:
              "200":
                description: Operation processed
              "400":
                description: Unrecognized mode
            """
            input = request.get_json()

            success = change_flight_mode(
                self.handler,
                self.mav_connection.target_system,
                self.mav_connection.target_component,
                input["mode"],
            )

            if success:
                return f"OK! Changed mode: {input['mode']}", 200
            else:
                return f"Unrecognized mode: {input['mode']}", 400

        @app.route("/parameters/<param_id>", methods=["GET"])
        def get_param(param_id):
            try:
                result = get_parameter(self.handler, param_id)

                if result is None:
                    logger.error(f"Failed to get parameter {param_id}")
                    return f"Parameter {param_id} not found or request timed out", 500

                logger.info(f"Retrieved parameter {param_id}: {result['param_value']}")
                return result, 200
            except Exception as e:
                logger.error(
                    f"Error getting parameter {param_id}: {type(e).__name__}: {str(e)}"
                )
                return "Failed to get parameter", 500

        @app.route("/parameters/<param_id>", methods=["PUT"])
        def set_param(param_id):
            try:
                payload = request.get_json()

                if "value" not in payload:
                    return "Parameter value is required", 400

                param_value = payload["value"]
                logger.info(f"Setting parameter {param_id} to {param_value}")

                success = set_parameter(self.handler, param_id, param_value)

                if success:
                    return f"Parameter {param_id} set to {param_value}", 200
                else:
                    logger.error(f"Failed to set parameter {param_id}")
                    return f"Failed to set parameter {param_id}", 500
            except Exception as e:
                logger.error(
                    f"Error setting parameter {param_id}: {type(e).__name__}: {str(e)}"
                )
                return "Failed to set parameter", 500

        @app.route("/aeac_scan", methods=["POST"])
        def generate_scan_points():
            """
            Scan a circular target area in a spiral path.
            ---
            tags:
              - feature
            summary: Scan a circular target area in a spiral path.
            description: >-
              TODO
            requestBody:
              content:
                application/json:
                  schema:
                    type: object
                    properties:
                      center_lat:
                        type: number
                        format: float64
                      center_lng:
                        type: number
                        format: float64
                      altitude:
                        type: number
                        format: float64
                      target_area_radius:
                        type: number
                        format: float64
            responses:
              "200":
                description: "Scan Mission Set"
              "400":
                description: "Invalid input"
            """
            input = request.get_json()

            # TODO Trigger CameraVision system to begin scanning
            if (
                input["center_lat"]
                and input["center_lng"]
                and input["altitude"]
                and input["target_area_radius"]
            ):
                wpq = scan_area(
                    center_lat=input["center_lat"],
                    center_lng=input["center_lng"],
                    altitude=input["altitude"],
                    target_area_radius=input["target_area_radius"],
                )

                if new_mission(self.handler, wpq):
                    return "Scan Mission Set", 200
                else:
                    return "Mission request failed", 400
            else:
                return "Invalid input, missing a parameter.", 400

        @app.route("/aeac_deliver", methods=["POST"])
        def deliver_water_down():
            """
            Deliver water operation
            ---
            tags:
              - delivery
            summary: Deliver water operation
            description: >
              For AEAC 2025, while in autonomous mode, hovers down to specific altitude for 'deliver_duration' seconds, and then returns to original altitude ready for reentering manual control.
            requestBody:
              content:
                application/json:
                  schema:
                    type: object
                    properties:
                      current_alt:
                        type: number
                        description: Current altitude of the drone
                      deliver_alt:
                        type: number
                        description: Altitude to lower to for delivery
                      deliver_duration_secs:
                        type: number
                        description: Duration in seconds to stay at the delivery altitude
                      curr_lat:
                        type: number
                        description: Current latitude of the drone
                      curr_lon:
                        type: number
                        description: Current longitude of the drone
            responses:
              "200":
                description: Commencing Deliver operation
              "400":
                description: Invalid input, missing a parameter or Mission request failed
            """
            input = request.get_json()

            if (
                "current_alt" in input
                and "deliver_alt" in input
                and "deliver_duration_secs" in input
                and "curr_lat" in input
                and "curr_lon" in input
            ):
                # Extract values from JSON input
                current_alt = input["current_alt"]
                deliver_alt = input["deliver_alt"]
                deliver_duration_secs = input["deliver_duration_secs"]
                curr_lat = input["curr_lat"]
                curr_lon = input["curr_lon"]
                wpq = generate_water_wps(
                    current_alt, deliver_alt, deliver_duration_secs, curr_lat, curr_lon
                )

                if new_mission(self.handler, wpq):
                    return "Commencing Deliver operation", 200
                else:
                    return "Mission request failed", 400
            else:
                return "Invalid input, missing a parameter.", 400

        return app, socketio

    # def serve_forever(self, production=True, HOST="localhost", PORT=9000):
    def serve_forever(self, production: bool, host: str, port: int):
        logger.info("GCOM HTTP Server starting...")
        app, socketio = self.create_app()

        try:
            socketio.run(
                app, host=host, port=port, debug=(not production), allow_unsafe_werkzeug=True, use_reloader=False
            )
        finally:
            # Ensure handler thread stops cleanly
            self.shutdown()

    def shutdown(self):
        """Shutdown the HTTP server."""
        logger.info("HTTP Server shutdown")
