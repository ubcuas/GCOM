from django.db import IntegrityError
from django.test import Client, TestCase
from unittest.mock import Mock, patch

from .models import DroneSingleton, DroneTelemetry


class DroneControlEndpointTest(TestCase):
    def test_get_flightmode_returns_current_mode(self):
        response = Mock(status_code=200)
        response.json.return_value = {"flightmode": "GUIDED"}

        with patch("drone.views.DroneApiClient.get_current_status", return_value=response):
            result = Client().get("/api/drone/flightmode")

        self.assertEqual(result.status_code, 200)
        self.assertEqual(result.json(), {"flightmode": "GUIDED"})

    def test_get_rtl_initiates_rtl(self):
        response = Mock(status_code=200)

        with patch("drone.views.DroneApiClient.post_rtl", return_value=response) as post_rtl:
            result = Client().get("/api/drone/rtl")

        self.assertEqual(result.status_code, 200)
        post_rtl.assert_called_once_with()


class DroneTelemetryModelTest(TestCase):
    def setUp(self):
        DroneTelemetry.objects.create(
            timestamp=1724769137,
            latitude=20,
            longitude=20,
            altitude=75.24,
            vertical_speed=1.273,
            speed=3.455,
            heading=17,
            battery_voltage=7.5,
        )

    def test_drone_telemetry(self):
        drone_telemetry = DroneTelemetry.objects.get(timestamp=1724769137)
        self.assertEqual(drone_telemetry.timestamp, 1724769137)
        self.assertEqual(drone_telemetry.latitude, 20)
        self.assertEqual(drone_telemetry.longitude, 20)
        self.assertEqual(drone_telemetry.altitude, 75.24)
        self.assertEqual(drone_telemetry.vertical_speed, 1.273)
        self.assertEqual(drone_telemetry.speed, 3.455)
        self.assertEqual(drone_telemetry.heading, 17)
        self.assertEqual(drone_telemetry.battery_voltage, 7.5)

    def test_cannot_create_without_latitude(self):
        with self.assertRaises(IntegrityError):
            DroneTelemetry.objects.create(
                timestamp=0,
                longitude=0,
                altitude=0,
                vertical_speed=0,
                speed=0,
                heading=0,
                battery_voltage=0,
            )

    def test_cannot_create_without_longitude(self):
        with self.assertRaises(IntegrityError):
            DroneTelemetry.objects.create(
                timestamp=0,
                latitude=0,
                altitude=0,
                vertical_speed=0,
                speed=0,
                heading=0,
                battery_voltage=0,
            )

    def test_cannot_create_without_altitude(self):
        with self.assertRaises(IntegrityError):
            DroneTelemetry.objects.create(
                timestamp=0,
                latitude=0,
                longitude=0,
                vertical_speed=0,
                speed=0,
                heading=0,
                battery_voltage=0,
            )

    def test_cannot_create_without_vertical_speed(self):
        with self.assertRaises(IntegrityError):
            DroneTelemetry.objects.create(
                timestamp=0,
                latitude=0,
                longitude=0,
                altitude=0,
                speed=0,
                heading=0,
                battery_voltage=0,
            )


class DroneSingletonModelTest(TestCase):
    def setUp(self):
        DroneSingleton.objects.create(mode="AUT", armed=True)

    def test_drone_singleton(self):
        drone_singleton = DroneSingleton.objects.get(mode="AUT")
        self.assertEqual(drone_singleton.mode, "AUT")
        self.assertEqual(drone_singleton.armed, True)

    def test_drone_singleton_load(self):
        drone_singleton = DroneSingleton.load()
        self.assertEqual(drone_singleton.mode, "AUT")
        self.assertEqual(drone_singleton.armed, True)

    def test_cannot_create_without_mode(self):
        with self.assertRaises(IntegrityError):
            DroneSingleton.objects.create(mode=None, armed=True)

    def test_cannot_create_without_armed(self):
        with self.assertRaises(IntegrityError):
            DroneSingleton.objects.create(mode="AUT", armed=None)
