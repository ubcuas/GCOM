package main

import (
	"strings"
)

func GetQueue(q []Waypoint) error {
	var endpoint strings.Builder
	endpoint.WriteString(getEnvVariable("MP_ROUTE") + "/queue")

}
