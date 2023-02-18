package main

import (
	"bytes"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"net/http"
	"strings"
)

/**
 * Gets the current route ("Queue" of Waypoints) that constitute the path the drone is following, from Mission Planner
 *
 * Returns: - Pointer to a Queue struct containing Waypoints generated from the Waypoint data
 * 				given in the response from Mission Planner
 *			- Error if anything goes wrong
 */
func GetQueue() (*Queue, error) {
	var endpoint = getEnvVariable("MP_ROUTE") + "/queue"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		// panic(err)
		log.Fatal(err)
		return nil, err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		log.Fatal(err)
		return nil, err
		// panic(err)
	}
	defer resp.Body.Close()

	fmt.Println("response status:", resp.Status)
	fmt.Println("response headers", resp.Header)
	body, _ := io.ReadAll(resp.Body)
	fmt.Println("response Body", string(body))

	if resp.Status != "200 OK" {
		return nil, errors.New("response not OK: " + resp.Status)
	}

	var queue []Waypoint

	err = json.Unmarshal(body, &queue)
	if err != nil {
		log.Fatal(err)
		return nil, err
		// panic(err)
	}

	return &Queue{Queue: queue}, nil
}

/**
 * Overrides the current route ("Queue" of Waypoints) that the drone is following with a new Queue
 *
 * Param: queue - a pointer to a Queue struct containing the new sequence of Waypoints to follow
 *
 * Returns: - Error if anything goes wrong
 */
// take a pointer to a Queue struct and post those waypoints to MP
func PostQueue(queue *Queue) error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/queue"

	json, err := json.Marshal(queue.Queue)
	if err != nil {
		log.Fatal(err)
		return err
	}

	req, err := http.NewRequest("POST", endpoint, strings.NewReader(string(json)))
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		log.Fatal(err)
		return err
		// panic(err)
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		log.Fatal(err)
		return err
		// panic(err)
	}
	defer resp.Body.Close()

	fmt.Println("response status:", resp.Status)

	if resp.Status != "200 OK" {
		return errors.New("response not OK: " + resp.Status)
	}

	return nil
}

//TODO: Maybe follow singleton design pattern for AircraftStatus? -> make this method callable only on an instance
//		of AircraftStatus struct
/**
 * Gets the current aircraft status from Mission Planner
 *
 * Returns: - Pointer to an AircraftStatus struct with field info pulled from Mission Planner
 *			- Error if anything goes wrong
 */
func GetAircraftStatus() (*AircraftStatus, error) {
	var endpoint = getEnvVariable("MP_ROUTE") + "/status"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		// panic(err)
		log.Fatal(err)
		return nil, err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		// panic(err)
		log.Fatal(err)
		return nil, err
	}
	defer resp.Body.Close()

	fmt.Println("response status:", resp.Status)
	fmt.Println("response headers", resp.Header)
	body, _ := io.ReadAll(resp.Body)
	fmt.Println("response Body", string(body))

	if resp.Status != "200 OK" {
		return nil, errors.New("response not OK: " + resp.Status)
	}

	stat := AircraftStatus{}

	err = json.Unmarshal(body, &stat)
	if err != nil {
		// panic(err)
		log.Fatal(err)
		return nil, err
	}

	return &stat, nil
}

/**
 * Locks the aircraft (prevent the aircraft from moving based on the MP queue)
 *
 * Returns: - Error if anything goes wrong (ex. if the aircraft is already locked)
 */
func LockAircraft() error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/lock"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		// panic(err)
		// log.Fatal(err)
		Error.Println(err)
		return err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		// panic(err)
		// log.Fatal(err)
		Error.Println(err)
		return err
	}
	defer resp.Body.Close()

	if resp.Status != "200 OK" {
		Error.Println("Aircraft already locked: " + resp.Status)
		return errors.New("Aircraft already locked: " + resp.Status)
	}

	return nil
}

/**
 * Unlocks the aircraft (resume aircraft movement based on the MP queue)
 *
 * Returns: - Error if anything goes wrong (ex. if the aircraft is already unlocked)
 */
func UnlockAircraft() error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/unlock"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		// panic(err)
		// log.Fatal(err)
		Error.Println(err)
		return err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		// panic(err)
		// log.Fatal(err)
		Error.Println(err)
		return err
	}
	defer resp.Body.Close()

	if resp.Status != "200 OK" {
		Error.Println("Aircraft already unlocked: " + resp.Status)
		return errors.New("Aircraft already unlocked: " + resp.Status)
	}

	return nil
}

/**
 * Send the aircraft back to the original launch site
 *
 * Returns: - Error if anything goes wrong
 */
func ReturnToLaunch() error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/rtl"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		// panic(err)
		// log.Fatal(err)
		Error.Println(err)
		return err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		Error.Println(err)
		return err
	}
	defer resp.Body.Close()

	if resp.Status != "200 OK" {
		Error.Println("Failed to return to launch: " + resp.Status)
		return errors.New("Failed to return to launch: " + resp.Status)
	}

	return nil
}

/**
 * Immediately descend the aircraft and land over current position
 *
 * Returns: - Error if anything goes wrong
 */
func LandImmediately() error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/land"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		Error.Println(err)
		return err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		Error.Println(err)
		return err
	}
	defer resp.Body.Close()

	if resp.Status != "200 OK" {
		Error.Println("Failed to land " + resp.Status)
		return errors.New("Failed to land" + resp.Status)
	}

	return nil
}

/**
 * Sets the 'home' waypoint of the aircraft
 *
 * Returns: - Error if anything goes wrong
 */
func PostHome(home *Waypoint) error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/home"

	reqBody, err := json.Marshal(home)
	if err != nil {
		Error.Println(err)
		return err
	}

	req, err := http.NewRequest("POST", endpoint, bytes.NewBuffer(reqBody))
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		Error.Println(err)
		return err
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		Error.Println(err)
		return err
	}
	defer resp.Body.Close()

	if resp.Status != "200 OK" {
		Error.Println("Failed to set home: " + resp.Status)
		return errors.New("Failed to set home: " + resp.Status)
	}

	return nil
}
