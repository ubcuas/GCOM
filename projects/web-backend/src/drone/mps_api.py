import json
import os
import requests


class DroneApiClient:
    # _mission_planner_api_url = "http://localhost:9000"
    MISSION_PLANNER_URL = os.getenv("MISSION_PLANNER_URL")
    _mission_planner_api_url = MISSION_PLANNER_URL

    @staticmethod
    def _fetch_from_mission_planner(endpoint, method="GET", data=None):
        url = f"{DroneApiClient._mission_planner_api_url}/{endpoint}"
        headers = {"Content-Type": "application/json"}
        try:
            if method == "GET":
                response = requests.get(url)
            elif method == "POST":
                response = requests.post(url, headers=headers, data=json.dumps(data))
            elif method == "PUT":
                response = requests.put(url, headers=headers, data=json.dumps(data))

            if response.status_code >= 400:
                print(
                    f"[ERROR] Mission-planner {method} {endpoint} returned {response.status_code}"
                )
                print(f"[ERROR] Response body: {response.text}")

            return response
        except requests.exceptions.RequestException as e:
            print(f"[ERROR] Failed to connect to mission-planner at {url}")
            print(f"[ERROR] Exception: {type(e).__name__}: {str(e)}")
            raise

    @staticmethod
    def get_current_status():
        return DroneApiClient._fetch_from_mission_planner("status")

    @staticmethod
    def get_status_history():
        return DroneApiClient._fetch_from_mission_planner("status/history")

    @staticmethod
    def prepare_takeoff(altitude):
        return DroneApiClient._fetch_from_mission_planner(
            "prepare_takeoff", method="POST", data={"altitude": altitude}
        )

    @staticmethod
    def prepare_rtl_params(altitude):
        return DroneApiClient._fetch_from_mission_planner(
            "prepare_rtl_params", method="POST", data={"altitude": altitude}
        )

    @staticmethod
    def arm(arm_value):
        return DroneApiClient._fetch_from_mission_planner(
            "arm", method="PUT", data={"arm": arm_value}
        )

    @staticmethod
    def land():
        return DroneApiClient._fetch_from_mission_planner("land")

    @staticmethod
    def get_rtl():
        return DroneApiClient._fetch_from_mission_planner("rtl")

    @staticmethod
    def post_rtl():
        return DroneApiClient._fetch_from_mission_planner("rtl", method="POST")

    @staticmethod
    def lock():
        return DroneApiClient._fetch_from_mission_planner("lock")

    @staticmethod
    def unlock():
        return DroneApiClient._fetch_from_mission_planner("unlock")

    @staticmethod
    def get_queue():
        return DroneApiClient._fetch_from_mission_planner("queue")

    @staticmethod
    def post_queue(queue):
        return DroneApiClient._fetch_from_mission_planner(
            "queue", method="POST", data=queue
        )

    @staticmethod
    def post_home(wp):
        return DroneApiClient._fetch_from_mission_planner(
            "home", method="POST", data=wp
        )

    @staticmethod
    def insert(queue):
        return DroneApiClient._fetch_from_mission_planner(
            "insert", method="POST", data=queue
        )

    @staticmethod
    def clear():
        return DroneApiClient._fetch_from_mission_planner("clear")

    @staticmethod
    def diversion(exclude_wps, rejoin_wp):
        return DroneApiClient._fetch_from_mission_planner(
            "diversion",
            method="POST",
            data={"exclude": exclude_wps, "rejoin_at": rejoin_wp},
        )

    @staticmethod
    def put_flightmode(mode):
        return DroneApiClient._fetch_from_mission_planner(
            "flightmode", method="PUT", data={"mode": mode}
        )

    @staticmethod
    def get_parameter(param_id):
        return DroneApiClient._fetch_from_mission_planner(
            f"parameters/{param_id}", method="GET"
        )

    @staticmethod
    def set_parameter(param_id, value):
        return DroneApiClient._fetch_from_mission_planner(
            f"parameters/{param_id}", method="PUT", data={"value": value}
        )
