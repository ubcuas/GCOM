import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";
import svgr from "vite-plugin-svgr";
import path from "path";

// https://vitejs.dev/config/
export default defineConfig({
    plugins: [svgr(), react()],
    resolve: {
        alias: {
            "@assets": path.resolve(__dirname, "./src/assets"),
        },
    },
    server: {
        port: 1324,
        host: "0.0.0.0",
    },
});
