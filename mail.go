package main

import (
	"net/smtp"
	"strconv"
)

func sendMail(auth smtp.Auth, to []string, msg []byte) error {
	err := smtp.SendMail(getEnvVariable("EMAIL_HOST")+":"+getEnvVariable("EMAIL_PORT"), auth, getEnvVariable("EMAIL_FROM"), to, msg)
	if err != nil {
		return err
	}
	return nil
}

func generateMsg(to []string, subject string, body string) []byte {
	msg := []byte("To: " + to[0] + "\r\n" +
		"Subject: " + subject + "\r\n" +
		"\r\n" +
		body + "\r\n")
	return msg
}

func formatPaths(paths []AEACRoutes) string {
	var formattedPaths string
	for _, path := range paths {
		formattedPaths += strconv.Itoa(path.Number) + ";"
	}
	return formattedPaths
}

func sendPaths(paths []AEACRoutes) error {
	subject := getEnvVariable("BIDDER_NAME") + " Flight Plan"

	pathsString := formatPaths(paths)
	body := getEnvVariable("BIDDER_NAME") + ";" + pathsString

	to := []string{""}
	to[0] = getEnvVariable("EMAIL_TO")

	auth := smtp.PlainAuth("", getEnvVariable("EMAIL_FROM"), getEnvVariable("EMAIL_PASS"), getEnvVariable("EMAIL_HOST"))
	msg := generateMsg(to, subject, body)
	err := sendMail(auth, to, msg)
	return err
}
