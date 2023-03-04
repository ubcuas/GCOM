package main

import (
	"testing"

	"github.com/stretchr/testify/assert"
)

func TestEmailSends(t *testing.T) {
	examplePaths := []AEACRoutes{
		{
			Number: 1,
		},
		{
			Number: 2,
		},
		{
			Number: 3,
		},
	}
	err := sendPaths(examplePaths)
	assert.NoError(t, err)
}
