import axios from "axios";

export const backend_url = import.meta.env.VITE_BACKEND_URL || "http://localhost:8000/";
export const api = axios.create({
    baseURL: backend_url + "api/",
    headers: {
        "Content-Type": "application/json",
    },
});

console.log("Using backend url: ", backend_url);

export default api;
