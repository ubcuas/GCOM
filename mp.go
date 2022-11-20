package main

import (
	"errors"
	"fmt"
	"io"
	"net/http"
	"strings"
)

func GetQueue() (string, error) {
	var endpoint strings.Builder
	endpoint.WriteString(getEnvVariable("MP_ROUTE") + "/queue")

	req, err := http.NewRequest("GET", endpoint.String(), nil)
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
		return "", errors.New("response not OK: " + resp.Status)
	}

	return string(body), nil
}
