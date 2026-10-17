import sys

from django.apps import AppConfig
from django.conf import settings

class VisionConfig(AppConfig):
    default_auto_field = "django.db.models.BigAutoField"
    name = "vision"

    def ready(self):
        if not settings.AWS_S3_BUCKET:
            print(
                "ERROR: AWS_S3_BUCKET is not set. ODLC image archival and "
                "depth archive browsing will fail at runtime. Copy "
                ".env.example to .env and configure the AWS_S3_BUCKET details as specified in .env.example\n",
                file=sys.stderr,
            )