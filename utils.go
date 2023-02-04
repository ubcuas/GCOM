package main

import (
	"time"
)

// from https://coderwall.com/p/cp5fya/measuring-execution-time-in-go
// measure execution time of a function,
// to use, call defer timeTrack(time.Now(), "name of function") at the beginning of the function body
func timeTrack(start time.Time, name string) {
	elapsed := time.Since(start)
	Info.Printf("%s took %s", name, elapsed)
}

// takes a slice of strings and returns an equivalent slice of interfaces
// used for code cleanup for db methods that require variadic arguments
func stringSliceToInterfaceSlice(slice []string) []interface{} {
	interfaceSlice := make([]interface{}, len(slice))
	for i, v := range slice {
		interfaceSlice[i] = v
	}
	return interfaceSlice
}
