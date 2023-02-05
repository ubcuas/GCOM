package main

import (
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"gopkg.in/guregu/null.v4"
)

// test posting and reading the queue from MP
func TestGetQueue(t *testing.T) {

	wp1 := Waypoint{
		ID:        -1,
		Name:      "TestGETWP1",
		Longitude: -6.2,
		Latitude:  149.1651830,
		Altitude:  20.0,
	}

	wp2 := Waypoint{
		ID:        -1,
		Name:      "TestGETWP2",
		Longitude: -36.3637798,
		Latitude:  147.1651830,
		Altitude:  20.0,
	}

	wp3 := Waypoint{
		ID:        -1,
		Name:      "TestGETWP3",
		Longitude: -37.3637798,
		Latitude:  146.1641830,
		Altitude:  10.0,
	}

	wpslice := []Waypoint{wp1, wp2, wp3}
	queue := Queue{Queue: wpslice}

	fmt.Println(queue)

	err := PostQueue(&queue)
	time.Sleep(2 * time.Second) //to ensure that MP server has the updated route

	if assert.NoError(t, err) {
		getQueue, err := GetQueue()

		if assert.NoError(t, err) {
			assert.Equal(t, len(getQueue.Queue), 3)
			assert.Equal(t, getQueue.Queue[0].Name, "TestGETWP1")
			assert.Equal(t, getQueue.Queue[1].Name, "TestGETWP2")
			assert.Equal(t, getQueue.Queue[2].Name, "TestGETWP3")

			assert.Equal(t, getQueue.Queue[0].Longitude, -6.2)
			assert.Equal(t, getQueue.Queue[1].Longitude, -36.3637798)
			assert.Equal(t, getQueue.Queue[2].Longitude, -37.3637798)

			assert.Equal(t, getQueue.Queue[0].Latitude, 149.1651830)
			assert.Equal(t, getQueue.Queue[1].Latitude, 147.1651830)
			assert.Equal(t, getQueue.Queue[2].Latitude, 146.1641830)

			assert.Equal(t, getQueue.Queue[0].Altitude, null.FloatFrom(20.0))
			assert.Equal(t, getQueue.Queue[1].Altitude, null.FloatFrom(20.0))
			assert.Equal(t, getQueue.Queue[2].Altitude, null.FloatFrom(10.0))
		}
	}
}

// test post queue
func TestPostQueue(t *testing.T) {

	currQueue, err := GetQueue()
	if err != nil {
		panic(err)
	}

	currQueue.Queue[0].Name = "CHANGED_NAME"
	currQueue.Queue[0].Latitude = -30.330300303300

	err = PostQueue(currQueue)
	time.Sleep(2 * time.Second) //to ensure that MP server has updated route

	if assert.NoError(t, err) {
		getQueue, err := GetQueue()

		if assert.NoError(t, err) {
			assert.Equal(t, "CHANGED_NAME", getQueue.Queue[0].Name)
			assert.Equal(t, -30.330300303300, getQueue.Queue[0].Latitude)
		}
	}

}

// test getting the aircraft status from MP, only works when aircraft is not launched
func TestGetAircraftStatus(t *testing.T) {

	status, err := GetAircraftStatus()

	if assert.NoError(t, err) {
		//assuming these values for the SITL
		/*
			{
				"velocity": "0.0",
				"latitude": "-35.3627175",
				"longitude": "149.1514354",
				"altitude": "17.0569992065",
				"heading": "265.771789551"
			}
		*/

		assert.True(t, status.Velocity >= 0.0)
		assert.True(t, status.Latitude >= -40.0 && status.Latitude <= -30.0)
		assert.True(t, status.Longitude >= 145.0 && status.Longitude <= 155.0)
		assert.True(t, status.Altitude >= 10.0 && status.Altitude <= 30.0)
		assert.True(t, status.Heading >= 240.0 && status.Heading <= 300.0)
		assert.True(t, status.BatteryVoltage >= 0.0 && status.BatteryVoltage <= 100.0)
	}

}

// aircraft starts and ends unlocked
// tests locking and unlocking
// test that repeated locks and repeated unlocks throw errors
func TestLockAndUnlockAircraft(t *testing.T) {
	err := LockAircraft()
	assert.NoError(t, err)
	err = LockAircraft()
	assert.Error(t, err)

	err = UnlockAircraft()
	assert.NoError(t, err)
	err = UnlockAircraft()
	assert.Error(t, err)

	err = LockAircraft()
	assert.NoError(t, err)
	err = UnlockAircraft()
	assert.NoError(t, err)
}
