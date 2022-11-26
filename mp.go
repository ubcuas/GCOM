package main

import (
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"strings"
)

// return a pointer to a Queue struct formed from getting the current Queue of Waypoints from MP
// func GetQueue() (string, error) {
func GetQueue() (*Queue, error) {
	var endpoint = getEnvVariable("MP_ROUTE") + "/queue"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		panic(err)
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		panic(err)
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
		panic(err)
	}

	return &Queue{Queue: queue}, nil
}

// take a pointer to a Queue struct and post those waypoints to MP
func PostQueue(queue *Queue) error {
	var endpoint = getEnvVariable("MP_ROUTE") + "/queue"

	json, err := json.Marshal(queue.Queue)
	if err != nil {
		return err
	}

	req, err := http.NewRequest("POST", endpoint, strings.NewReader(string(json)))
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		panic(err)
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		panic(err)
	}
	defer resp.Body.Close()

	fmt.Println("response status:", resp.Status)

	if resp.Status != "200 OK" {
		return errors.New("response not OK: " + resp.Status)
	}

	return nil
}

// returns a pointer to an AircraftStatus struct with the current AircraftStatus from MP
func GetAircraftStatus() (*AircraftStatus, error) {
	var endpoint = getEnvVariable("MP_ROUTE") + "/status"

	req, err := http.NewRequest("GET", endpoint, nil)
	req.Header.Set("Content-Type", "application-json")
	if err != nil {
		panic(err)
	}

	client := &http.Client{}
	resp, err := client.Do(req)
	if err != nil {
		panic(err)
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
		panic(err)
	}

	return &stat, nil
}
