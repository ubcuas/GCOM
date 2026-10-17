import json

from django.http import HttpResponse, JsonResponse
from django.views.decorators.csrf import csrf_exempt
from django.views.decorators.http import require_http_methods

from .mps_api import DroneApiClient


@require_http_methods(["GET"])
def get_current_status(request):
    response = DroneApiClient.get_current_status()
    return JsonResponse(response.json(), safe=False, status=response.status_code)


@require_http_methods(["GET"])
def get_status_history(request):
    response = DroneApiClient.get_status_history()
    return JsonResponse(response.json(), safe=False, status=response.status_code)


@csrf_exempt
@require_http_methods(["POST"])
def prepare_takeoff(request):
    try:
        data = json.loads(request.body)
        altitude = data.get("altitude")
        response = DroneApiClient.prepare_takeoff(altitude)

        # get current position from the drone status queue
        status = DroneApiClient.get_queue()

        if response.status_code >= 400:
            print(f"[ERROR] Mission-planner response: {response.text}")
            return JsonResponse(
                {"error": "Mission-planner error", "details": response.text},
                status=response.status_code,
            )
        # queue[0] is the home waypoint, queue[1] is the takeoff waypoint with the actual takeoff altitude
        takeoff_wp = status.json()[1]
        return JsonResponse(takeoff_wp, status=200)

        # return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError) as e:
        print(
            f"[ERROR] Invalid input for prepare_takeoff: {type(e).__name__}: {str(e)}"
        )
        return JsonResponse({"error": "Invalid input"}, status=400)
    except Exception as e:
        print(
            f"[ERROR] Unexpected error in prepare_takeoff: {type(e).__name__}: {str(e)}"
        )
        return JsonResponse(
            {"error": "Internal server error", "details": str(e)},
            status=500,
        )


@csrf_exempt
@require_http_methods(["POST"])
def prepare_rtl_params(request):
    try:
        data = json.loads(request.body)
        altitude = data.get("altitude")
        response = DroneApiClient.prepare_rtl_params(altitude)

        if response.status_code >= 400:
            print(f"[ERROR] Mission-planner response: {response.text}")
            return JsonResponse(
                {"error": "Mission-planner error", "details": response.text},
                status=response.status_code,
            )

        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError) as e:
        print(
            f"[ERROR] Invalid input for prepare_rtl_params: {type(e).__name__}: {str(e)}"
        )
        return JsonResponse({"error": "Invalid input"}, status=400)
    except Exception as e:
        print(
            f"[ERROR] Unexpected error in prepare_rtl_params: {type(e).__name__}: {str(e)}"
        )
        return JsonResponse(
            {"error": "Internal server error", "details": str(e)},
            status=500,
        )


@csrf_exempt
@require_http_methods(["PUT"])
def arm(request):
    try:
        data = json.loads(request.body)
        arm_value = data.get("arm")
        response = DroneApiClient.arm(arm_value)
        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError):
        return JsonResponse({"error": "Invalid input"}, status=400)


@require_http_methods(["GET"])
def land(request):
    response = DroneApiClient.land()
    return HttpResponse(status=response.status_code)


@csrf_exempt
@require_http_methods(["POST"])
def post_rtl(request):
    try:
        response = DroneApiClient.post_rtl()
        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError):
        return JsonResponse({"error": "Invalid input"}, status=400)


@require_http_methods(["GET"])
def lock(request):
    response = DroneApiClient.lock()
    return HttpResponse(status=response.status_code)


@require_http_methods(["GET"])
def unlock(request):
    response = DroneApiClient.unlock()
    return HttpResponse(status=response.status_code)


@csrf_exempt
@require_http_methods(["GET", "POST"])
def queue(request):
    if request.method == "GET":
        response = DroneApiClient.get_queue()
        return JsonResponse(response.json(), safe=False, status=response.status_code)
    elif request.method == "POST":
        try:
            waypoints = json.loads(request.body)
            response = DroneApiClient.post_queue(waypoints)

            if response.status_code >= 400:
                print(f"[ERROR] Queue POST failed with status {response.status_code}")
                print(f"[ERROR] Mission-planner response: {response.text}")
                return JsonResponse(
                    {"error": "Mission-planner error", "details": response.text},
                    status=response.status_code,
                )

            return HttpResponse(status=response.status_code)
        except (KeyError, ValueError, TypeError) as e:
            print(f"[ERROR] Invalid input for queue POST: {type(e).__name__}: {str(e)}")
            return JsonResponse({"error": "Invalid input"}, status=400)
        except Exception as e:
            print(
                f"[ERROR] Unexpected error in queue POST: {type(e).__name__}: {str(e)}"
            )
            return JsonResponse(
                {"error": "Internal server error", "details": str(e)}, status=500
            )


@csrf_exempt
@require_http_methods(["POST"])
def post_home(request):
    try:
        wp = json.loads(request.body)
        response = DroneApiClient.post_home(wp)
        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError):
        return JsonResponse({"error": "Invalid input"}, status=400)


@csrf_exempt
@require_http_methods(["POST"])
def insert(request):
    try:
        queue = json.loads(request.body)
        response = DroneApiClient.insert(queue)
        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError):
        return JsonResponse({"error": "Invalid input"}, status=400)


@require_http_methods(["GET"])
def clear(request):
    response = DroneApiClient.clear()
    return HttpResponse(status=response.status_code)


@require_http_methods(["POST"])
def diversion(request):
    try:
        data = json.loads(request.body)
        exclude_wps = data.get("exclude")
        rejoin_wp = data.get("rejoin_at")
        response = DroneApiClient.diversion(exclude_wps, rejoin_wp)
        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError):
        return JsonResponse({"error": "Invalid input"}, status=400)


@csrf_exempt
@require_http_methods(["PUT"])
def flightmode(request):
    try:
        data = json.loads(request.body)
        mode = data.get("mode")
        response = DroneApiClient.put_flightmode(mode)
        return HttpResponse(status=response.status_code)
    except (KeyError, ValueError, TypeError):
        return JsonResponse({"error": "Invalid input"}, status=400)


@csrf_exempt
@require_http_methods(["GET", "PUT"])
def parameter(request, param_id):
    if request.method == "GET":
        try:
            response = DroneApiClient.get_parameter(param_id)

            if response.status_code >= 400:
                print(f"[ERROR] Mission-planner response: {response.text}")
                return JsonResponse(
                    {"error": "Mission-planner error", "details": response.text},
                    status=response.status_code,
                )

            return JsonResponse(
                response.json(), safe=False, status=response.status_code
            )
        except Exception as e:
            print(
                f"[ERROR] Unexpected error in get_parameter: {type(e).__name__}: {str(e)}"
            )
            return JsonResponse(
                {"error": "Internal server error", "details": str(e)}, status=500
            )
    else:  # PUT
        try:
            data = json.loads(request.body)
            value = data.get("value")

            if value is None:
                return JsonResponse(
                    {"error": "Parameter value is required"}, status=400
                )

            response = DroneApiClient.set_parameter(param_id, value)

            if response.status_code >= 400:
                print(f"[ERROR] Mission-planner response: {response.text}")
                return JsonResponse(
                    {"error": "Mission-planner error", "details": response.text},
                    status=response.status_code,
                )

            return HttpResponse(status=response.status_code)
        except (KeyError, ValueError, TypeError) as e:
            print(
                f"[ERROR] Invalid input for set_parameter: {type(e).__name__}: {str(e)}"
            )
            return JsonResponse({"error": "Invalid input"}, status=400)
        except Exception as e:
            print(
                f"[ERROR] Unexpected error in set_parameter: {type(e).__name__}: {str(e)}"
            )
            return JsonResponse(
                {"error": "Internal server error", "details": str(e)}, status=500
            )
