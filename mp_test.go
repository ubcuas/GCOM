package main

import (
	"testing"

	"github.com/stretchr/testify/assert"
	"gopkg.in/guregu/null.v4"
)

// test posting and reading the queue from MP
func TestGetQueue(t *testing.T) {

	wp1 := Waypoint{
		ID:        -1,
		Name:      "TestGETWP1",
		Longitude: -35.3627798,
		Latitude:  149.1651830,
		Altitude:  null.FloatFrom(20.0),
	}

	wp2 := Waypoint{
		ID:        -1,
		Name:      "TestGETWP2",
		Longitude: -36.3637798,
		Latitude:  147.1651830,
		Altitude:  null.FloatFromPtr(nil),
	}

	wp3 := Waypoint{
		ID:        -1,
		Name:      "TestGETWP3",
		Longitude: -37.3637798,
		Latitude:  146.1641830,
		Altitude:  null.FloatFrom(10.0),
	}

	wpslice := []Waypoint{wp1, wp2, wp3}
	queue := Queue{Queue: wpslice}

	err := PostQueue(&queue)

	if assert.NoError(t, err) {
		queue, err := GetQueue()

		if assert.NoError(t, err) {
			assert.Equal(t, len(queue.Queue), 3)
			assert.Equal(t, queue.Queue[0].Name, "TestGETWP1")
			assert.Equal(t, queue.Queue[1].Name, "TestGETWP2")
			assert.Equal(t, queue.Queue[2].Name, "TestGETWP3")

			assert.Equal(t, queue.Queue[0].Longitude, -35.3627798)
			assert.Equal(t, queue.Queue[1].Longitude, -36.3637798)
			assert.Equal(t, queue.Queue[2].Longitude, -37.3637798)

			assert.Equal(t, queue.Queue[0].Latitude, 149.1651830)
			assert.Equal(t, queue.Queue[1].Latitude, 147.1651830)
			assert.Equal(t, queue.Queue[2].Latitude, 146.1641830)

			assert.Equal(t, queue.Queue[0].Altitude, null.FloatFrom(20.0))
			assert.Equal(t, queue.Queue[1].Altitude, null.FloatFrom(20.0))
			assert.Equal(t, queue.Queue[2].Altitude, null.FloatFrom(10.0))
		}
	}

}

// test getting the aircraft status from MP
func TestGetAircraftStatus(t *testing.T) {

	status, err := GetAircraftStatus()

	if assert.NoError(t, err) {
		assert.Equal(t, status.Velocity, 0.0)
		assert.Equal(t, status.Latitude, 0.0)
		assert.Equal(t, status.Longitude, 0.0)
		assert.Equal(t, status.Altitude, 0.0)
		assert.Equal(t, status.Heading, 0.0)
		//TODO: get MP scripts to add BatteryVoltage to status response
	}

}
