import { io } from "socket.io-client";
import { backend_url } from "./api";

export const socket = io(backend_url, {
    autoConnect: true,
});

socket.on("connect_error", (error) => {
    console.log("Socket connection error", error);
});
