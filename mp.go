package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"log"
	"net/http"
	"strings"
)

/**
 * Gets the current route ("Queue" of Waypoints) that constitute the path the drone is folllowing, from Mission Planner
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
