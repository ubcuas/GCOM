"""HTTP API client for integration tests.

This client provides a clean interface for making requests to the web-backend
and mission-planner APIs. All integration tests should go through web-backend
to test the full stack.
"""

from typing import Any

import requests
from requests import Response


class APIClient:
    """Client for interacting with GCOM services during integration tests."""

    def __init__(self, web_backend_url: str, mission_planner_url: str):
        """Initialize API client with base URLs.

        Args:
            web_backend_url: Base URL for web-backend (e.g., http://localhost:8000)
            mission_planner_url: Base URL for mission-planner (e.g., http://localhost:9000)
        """
        self.web_backend_url = web_backend_url.rstrip("/")
        self.mission_planner_url = mission_planner_url.rstrip("/")

    # ==================== Web-Backend API Methods ====================
    # These are the primary methods for integration tests - they hit web-backend
    # which then forwards to mission-planner, testing the full integration.

    def get_status(self) -> dict[str, Any]:
        """Get current drone status via web-backend.

        Returns:
            Status dictionary with telemetry data
        """
        response = requests.get(f"{self.web_backend_url}/api/drone/status")
        response.raise_for_status()
        return response.json()

    def get_queue(self) -> list[dict[str, Any]]:
        """Get current waypoint queue via web-backend.

        Returns:
            List of waypoint dictionaries
        """
        response = requests.get(f"{self.web_backend_url}/api/drone/queue")
        response.raise_for_status()
        return response.json()

    def post_queue(self, waypoints: list[dict[str, Any]]) -> Response:
        """Upload waypoint queue via web-backend.

        Args:
            waypoints: List of waypoint dictionaries

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/drone/queue",
            json=waypoints,
            headers={"Content-Type": "application/json"},
        )
        return response

    def clear_queue(self) -> Response:
        """Clear waypoint queue via web-backend.

        Returns:
            Response object
        """
        response = requests.get(f"{self.web_backend_url}/api/drone/clear")
        return response

    def prepare_takeoff(self, altitude: float) -> Response:
        """Prepare takeoff sequence via web-backend.

        Clears the waypoint queue and adds a single waypoint at current position
        with the specified altitude. When the drone is armed and switched to AUTO
        mode by the pilot, it will automatically takeoff to this waypoint.

        Args:
            altitude: Target altitude in meters

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/drone/prepare_takeoff",
            json={"altitude": altitude},
            headers={"Content-Type": "application/json"},
        )
        return response

    def arm(self, arm_value: bool) -> Response:
        """Arm or disarm motors via web-backend.

        Args:
            arm_value: True to arm, False to disarm

        Returns:
            Response object
        """
        response = requests.put(
            f"{self.web_backend_url}/api/drone/arm",
            json={"arm": arm_value},
            headers={"Content-Type": "application/json"},
        )
        return response

    def land(self) -> Response:
        """Send land command via web-backend.

        Returns:
            Response object
        """
        response = requests.get(f"{self.web_backend_url}/api/drone/land")
        return response

    def rtl(self, altitude: float | None = None) -> Response:
        """Send return-to-launch command via web-backend.

        Args:
            altitude: Optional altitude to maintain during RTL

        Returns:
            Response object
        """
        if altitude is not None:
            response = requests.post(
                f"{self.web_backend_url}/api/drone/rtl",
                json={"altitude": altitude},
                headers={"Content-Type": "application/json"},
            )
        else:
            response = requests.get(f"{self.web_backend_url}/api/drone/rtl")
        return response

    def prepare_rtl_params(self, altitude: float) -> Response:
        """Prepare RTL parameters via web-backend.

        Sets RTL altitude parameter without triggering RTL mode.
        The drone will not switch to RTL mode until explicitly commanded.

        Args:
            altitude: RTL altitude in meters

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/drone/prepare_rtl_params",
            json={"altitude": altitude},
            headers={"Content-Type": "application/json"},
        )
        return response

    def post_home(self, waypoint: dict[str, Any]) -> Response:
        """Set home position via web-backend.

        Args:
            waypoint: Waypoint dictionary with lat/lon/alt

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/drone/home",
            json=waypoint,
            headers={"Content-Type": "application/json"},
        )
        return response

    def insert_waypoints(self, waypoints: list[dict[str, Any]]) -> Response:
        """Insert waypoints before current position via web-backend.

        Args:
            waypoints: List of waypoint dictionaries

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/drone/insert",
            json=waypoints,
            headers={"Content-Type": "application/json"},
        )
        return response

    def get_flight_mode(self) -> str:
        """Get current flight mode via web-backend.

        Returns:
            Flight mode string (e.g., "GUIDED", "AUTO", "LOITER")
        """
        response = requests.get(f"{self.web_backend_url}/api/drone/flightmode")
        response.raise_for_status()
        return response.json()["flightmode"]

    def set_flight_mode(self, mode: str) -> Response:
        """Set flight mode via web-backend.

        Args:
            mode: Flight mode string (e.g., "GUIDED", "AUTO", "LOITER", "RTL")

        Returns:
            Response object
        """
        response = requests.put(
            f"{self.web_backend_url}/api/drone/flightmode",
            json={"mode": mode},
            headers={"Content-Type": "application/json"},
        )
        return response

    # ==================== Database Waypoint API Methods ====================
    # These methods interact with the web-backend's database for mission planning.
    # Waypoints stored here are for planning and can be loaded to the drone later.

    def create_waypoint(self, waypoint_data: dict[str, Any]) -> Response:
        """Create a waypoint in the database.

        Args:
            waypoint_data: Waypoint dictionary with required fields

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/waypoint/",
            json=waypoint_data,
            headers={"Content-Type": "application/json"},
        )
        return response

    def list_waypoints(self) -> list[dict[str, Any]]:
        """List all waypoints from the database.

        Returns:
            List of waypoint dictionaries
        """
        response = requests.get(f"{self.web_backend_url}/api/waypoint/")
        response.raise_for_status()
        return response.json()

    def get_waypoint(self, waypoint_id: str) -> dict[str, Any]:
        """Get a specific waypoint from the database by ID.

        Args:
            waypoint_id: UUID of the waypoint

        Returns:
            Waypoint dictionary
        """
        response = requests.get(f"{self.web_backend_url}/api/waypoint/{waypoint_id}/")
        response.raise_for_status()
        return response.json()

    def update_waypoint(self, waypoint_id: str, data: dict[str, Any]) -> Response:
        """Update a waypoint in the database (full update).

        Args:
            waypoint_id: UUID of the waypoint
            data: Complete waypoint data

        Returns:
            Response object
        """
        response = requests.put(
            f"{self.web_backend_url}/api/waypoint/{waypoint_id}/",
            json=data,
            headers={"Content-Type": "application/json"},
        )
        return response

    def partial_update_waypoint(
        self, waypoint_id: str, data: dict[str, Any]
    ) -> Response:
        """Partially update a waypoint in the database.

        Args:
            waypoint_id: UUID of the waypoint
            data: Partial waypoint data to update

        Returns:
            Response object
        """
        response = requests.patch(
            f"{self.web_backend_url}/api/waypoint/{waypoint_id}/",
            json=data,
            headers={"Content-Type": "application/json"},
        )
        return response

    def delete_waypoint(self, waypoint_id: str) -> Response:
        """Delete a waypoint from the database.

        Args:
            waypoint_id: UUID of the waypoint

        Returns:
            Response object
        """
        response = requests.delete(
            f"{self.web_backend_url}/api/waypoint/{waypoint_id}/"
        )
        return response

    # ==================== Database Route API Methods ====================

    def create_route(self, route_data: dict[str, Any]) -> Response:
        """Create a route in the database.

        Args:
            route_data: Route dictionary (typically just {"name": "Route Name"})

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/route/",
            json=route_data,
            headers={"Content-Type": "application/json"},
        )
        return response

    def list_routes(self) -> list[dict[str, Any]]:
        """List all routes from the database.

        Returns:
            List of route dictionaries with nested waypoints
        """
        response = requests.get(f"{self.web_backend_url}/api/route/")
        response.raise_for_status()
        return response.json()

    def get_route(self, route_id: int) -> dict[str, Any]:
        """Get a specific route from the database by ID.

        Args:
            route_id: Integer ID of the route

        Returns:
            Route dictionary with nested waypoints
        """
        response = requests.get(f"{self.web_backend_url}/api/route/{route_id}/")
        response.raise_for_status()
        return response.json()

    def delete_route(self, route_id: int) -> Response:
        """Delete a route from the database.

        Args:
            route_id: Integer ID of the route

        Returns:
            Response object
        """
        response = requests.delete(f"{self.web_backend_url}/api/route/{route_id}/")
        return response

    def reorder_route_waypoints(
        self, route_id: int, waypoint_ids: list[str]
    ) -> Response:
        """Reorder waypoints within a route.

        Args:
            route_id: Integer ID of the route
            waypoint_ids: List of waypoint UUIDs in desired order

        Returns:
            Response object
        """
        response = requests.post(
            f"{self.web_backend_url}/api/route/{route_id}/reorder-waypoints/",
            json=waypoint_ids,
            headers={"Content-Type": "application/json"},
        )
        return response

    # ==================== Mission-Planner Direct Access ====================
    # These methods bypass web-backend and hit mission-planner directly.
    # Use sparingly - only for verification or debugging, not primary test flow.

    def set_flight_mode_direct(self, mode: str) -> Response:
        """Set flight mode directly via mission-planner (bypasses web-backend).

        Use this method when you need to emulate pilot actions that would normally
        be performed via the physical controller (e.g., switching to AUTO mode to
        initiate a prepared mission). For standard flight mode changes during tests,
        prefer using the web-backend API.

        Args:
            mode: Flight mode string (e.g., "AUTO", "GUIDED", "LOITER", "RTL")

        Returns:
            Response object
        """
        response = requests.put(
            f"{self.mission_planner_url}/flightmode",
            json={"mode": mode},
            headers={"Content-Type": "application/json"},
        )
        return response

    def get_mission_planner_queue(self) -> list[dict[str, Any]]:
        """Get queue directly from mission-planner (for verification).

        Returns:
            List of waypoint dictionaries
        """
        response = requests.get(f"{self.mission_planner_url}/queue")
        response.raise_for_status()
        return response.json()

    def get_mission_planner_status(self) -> dict[str, Any]:
        """Get status directly from mission-planner (for verification).

        Returns:
            Status dictionary
        """
        response = requests.get(f"{self.mission_planner_url}/status")
        response.raise_for_status()
        return response.json()

    def mission_planner_health_check(self) -> bool:
        """Check if mission-planner server is responding.

        Returns:
            True if server is healthy
        """
        try:
            response = requests.get(f"{self.mission_planner_url}/", timeout=5)
            return response.status_code == 200
        except:
            return False

    def web_backend_health_check(self) -> bool:
        """Check if web-backend server is responding.

        Returns:
            True if server is healthy
        """
        try:
            response = requests.get(
                f"{self.web_backend_url}/api/drone/status", timeout=5
            )
            return response.status_code == 200
        except (requests.RequestException, ConnectionError, TimeoutError):
            return False
