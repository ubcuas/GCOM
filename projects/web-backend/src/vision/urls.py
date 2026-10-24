from django.urls import include, path
from rest_framework.routers import DefaultRouter

from .views import (
    GroundObjectViewset,
    ImageViewset,
    deproject_pixel,
    get_archive_dates,
    get_archive_object,
    get_archive_records_for_date,
    get_latest_odlc_session,
    get_odlc_session,
    get_odlc_session_index,
    odlc_record,
    post_odlc_record,
)

router = DefaultRouter()
router.register(r"image", ImageViewset, basename="image")
router.register(r"groundobject", GroundObjectViewset, basename="groundobject")

urlpatterns = [
    path("", include(router.urls)),
    path(
        "odlc-session/latest/", get_latest_odlc_session, name="get_latest_odlc_session"
    ),
    path("odlc-session/<str:session_id>/", get_odlc_session, name="get_odlc_session"),
    path(
        "odlc-session/<str:session_id>/index/",
        get_odlc_session_index,
        name="get_odlc_session_index",
    ),
    path(
        "odlc-session/<str:session_id>/records/",
        post_odlc_record,
        name="post_odlc_record",
    ),
    path(
        "odlc-session/<str:session_id>/records/<str:record_id>/",
        odlc_record,
        name="odlc_record",
    ),
    path("deproject-pixel/", deproject_pixel, name="deproject_pixel"),
    path("archive/dates/", get_archive_dates, name="get_archive_dates"),
    path(
        "archive/dates/<str:date>/",
        get_archive_records_for_date,
        name="get_archive_records_for_date",
    ),
    path("archive/object/", get_archive_object, name="get_archive_object"),
]
