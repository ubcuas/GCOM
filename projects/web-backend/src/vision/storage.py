import base64
import json
import re
from datetime import datetime, timezone
from typing import Any

import boto3
from django.conf import settings

_client = None

# Top-level prefix in the archive bucket where ODLC captures live.
# The layout is intentionally flat (no session segment) so the depth archive
# UI can browse by date alone:
#
#   odlc/<YYYY-MM-DD>/<record_id>.jpg            color image
#   odlc/<YYYY-MM-DD>/<record_id>_depth.png      depth map (optional)
#   odlc/<YYYY-MM-DD>/<record_id>.json           metadata sidecar (optional)
_ARCHIVE_PREFIX = "odlc"

# Strict YYYY-MM-DD; used to filter unrelated sub-prefixes when listing dates.
_DATE_RE = re.compile(r"^\d{4}-\d{2}-\d{2}$")


def _s3():
    global _client
    if _client is None:
        _client = boto3.client("s3", region_name=settings.AWS_S3_REGION)
    return _client


def _date_segment(received_at_ms: int | None) -> str:
    """Return the date portion of an upload key, e.g. ``2026-05-11``."""
    if received_at_ms:
        dt = datetime.fromtimestamp(received_at_ms / 1000, tz=timezone.utc)
    else:
        dt = datetime.now(timezone.utc)
    return dt.strftime("%Y-%m-%d")


def _record_key(date: str, record_id: str, suffix: str) -> str:
    return f"{_ARCHIVE_PREFIX}/{date}/{record_id}{suffix}"


def _put(
    record_id: str,
    b64: str,
    received_at_ms: int | None,
    suffix: str,
    content_type: str,
) -> str:
    date = _date_segment(received_at_ms)
    key = _record_key(date, record_id, suffix)
    _s3().put_object(
        Bucket=settings.AWS_S3_BUCKET,
        Key=key,
        Body=base64.b64decode(b64),
        ContentType=content_type,
    )
    return f"https://{settings.AWS_S3_BUCKET}.s3.{settings.AWS_S3_REGION}.amazonaws.com/{key}"


def upload_odlc_image(
    record_id: str, image_b64: str, received_at_ms: int | None
) -> str:
    """Upload a base64 JPEG to ``odlc/<YYYY-MM-DD>/<record_id>.jpg``."""
    return _put(record_id, image_b64, received_at_ms, ".jpg", "image/jpeg")


def upload_odlc_depth(
    record_id: str, depth_b64: str, received_at_ms: int | None
) -> str:
    """Upload a base64 16UC1 PNG depthmap to ``odlc/<YYYY-MM-DD>/<record_id>_depth.png``."""
    return _put(record_id, depth_b64, received_at_ms, "_depth.png", "image/png")


def upload_odlc_metadata(
    record_id: str, received_at_ms: int | None, metadata: dict[str, Any]
) -> str:
    """Upload a JSON sidecar to ``odlc/<YYYY-MM-DD>/<record_id>.json``.

    The sidecar carries non-pixel data (bounding box, color detection, yaw,
    confidence) so the depth-archive UI can render the same overlays as the
    live ODLC view. The depth archive tolerates missing or malformed sidecars
    by falling back to neutral defaults.
    """
    date = _date_segment(received_at_ms)
    key = _record_key(date, record_id, ".json")
    payload = {"receivedAt": received_at_ms, **metadata}
    _s3().put_object(
        Bucket=settings.AWS_S3_BUCKET,
        Key=key,
        Body=json.dumps(payload).encode("utf-8"),
        ContentType="application/json",
    )
    return f"https://{settings.AWS_S3_BUCKET}.s3.{settings.AWS_S3_REGION}.amazonaws.com/{key}"


def list_archive_dates() -> list[str]:
    """Return the sorted (newest first) list of dates that have any archive content."""
    paginator = _s3().get_paginator("list_objects_v2")
    dates: set[str] = set()
    for page in paginator.paginate(
        Bucket=settings.AWS_S3_BUCKET,
        Prefix=f"{_ARCHIVE_PREFIX}/",
        Delimiter="/",
    ):
        for entry in page.get("CommonPrefixes", []):
            sub = entry["Prefix"][len(_ARCHIVE_PREFIX) + 1 :].rstrip("/")
            if _DATE_RE.match(sub):
                dates.add(sub)
    return sorted(dates, reverse=True)


_METADATA_FIELDS = (
    "receivedAt",
    "boundingBox",
    "confidenceLevel",
    "yawDeg",
    "colorDetection",
    "heading",
)

def loadSideCarDict(sidecar_key: str) -> dict[str, Any]:
    """Load JSON metadata relative to ``odlc/``, e.g. ``2026-05-12/<id>.json``."""
    if not sidecar_key:
        return {}
    full_key = f"{_ARCHIVE_PREFIX}/{sidecar_key.lstrip('/')}"
    obj = _s3().get_object(Bucket=settings.AWS_S3_BUCKET, Key=full_key)
    return json.loads(obj["Body"].read().decode("utf-8"))

def list_archive_records(date: str) -> list[dict[str, Any]]:
    """Return every archived record for a ``YYYY-MM-DD`` date.

    Pairs ``<id>.jpg`` (color) with ``<id>_depth.png`` (optional) and
    ``<id>.json`` (metadata sidecar, optional). Records without a color image
    are skipped. The result is sorted newest-first.
    """
    if not _DATE_RE.match(date):
        raise ValueError(f"Invalid date format: {date!r} (expected YYYY-MM-DD)")

    bucket = settings.AWS_S3_BUCKET
    prefix = f"{_ARCHIVE_PREFIX}/{date}/"
    s3 = _s3()
    paginator = s3.get_paginator("list_objects_v2")

    color_filenames: dict[str, str] = {}
    depth_filenames: dict[str, str] = {}
    sidecar_filenames: dict[str, str] = {}

    for page in paginator.paginate(Bucket=bucket, Prefix=prefix):
        for obj in page.get("Contents", []):
            name = obj["Key"][len(prefix) :]

            if name.endswith("_depth.png"):
                depth_filenames[name[: -len("_depth.png")]] = name
            elif name.endswith(".jpg"):
                color_filenames[name[: -len(".jpg")]] = name
            elif name.endswith(".json"):
                sidecar_filenames[name[: -len(".json")]] = name

    records: list[dict[str, Any]] = []
    for record_id, color_filename in color_filenames.items():
        meta_name = sidecar_filenames.get(record_id)
        records.append({
            "id": record_id,
            "colorKey": f"{date}/{color_filename}",
            "depthKey": f"{date}/{depth_filenames.get(record_id)}" if depth_filenames.get(record_id) else None,
            #Loads optional metadata sidecar if present
            **loadSideCarDict(f"{date}/{meta_name}"),
        })

    records.sort(key=lambda r: r.get("receivedAt") or 0, reverse=True)
    return records


def download_archive_object(relative_key: str) -> tuple[bytes, str]:
    """Fetch an arbitrary archive object and return ``(body, content_type)``.

    ``relative_key`` is interpreted relative to the ``odlc/`` archive prefix.
    The frontend gets keys like ``2026-05-11/<id>.jpg`` from
    :func:`list_archive_records` and passes them straight through.
    """
    full_key = f"{_ARCHIVE_PREFIX}/{relative_key.lstrip('/')}"
    obj = _s3().get_object(Bucket=settings.AWS_S3_BUCKET, Key=full_key)
    body = obj["Body"].read()
    content_type = obj.get("ContentType") or "application/octet-stream"
    return body, content_type
