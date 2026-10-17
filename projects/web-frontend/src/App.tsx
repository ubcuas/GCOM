import { Box, CssBaseline, ThemeProvider, createTheme, responsiveFontSizes } from "@mui/material";
import { useEffect, useMemo } from "react";
import { Route, Switch } from "wouter";
import ErrorSnackbar from "./components/ErrorSnackbar";
import Nav from "./components/Nav";
import Home from "./routes/Home";
import MapRoute from "./routes/MapRoute";
import Queue from "./routes/Queue";
import Settings from "./routes/Settings";
import VideoFeed from "./routes/VideoFeed";
import OdlcImages from "./routes/OdlcImages";
import DepthArchive from "./routes/DepthArchive";
import { selectPreferredTheme } from "./store/slices/appSlice";
import { updateAircraftStatus } from "./store/slices/dataSlice";
import { useAppDispatch, useAppSelector } from "./store/store";
import { socket } from "./api/socket";
import { parseAndTransformDroneData } from "./types/AircraftStatus";
import { WebRTCProvider } from "./context/WebRTCContext";
import { restoreOdlcSession } from "./store/thunks/odlcSessionThunks";

function App() {
    const colorScheme = useAppSelector(selectPreferredTheme);
    const dispatch = useAppDispatch();
    const isDark = colorScheme === "dark";

    const theme = useMemo(
        () =>
            responsiveFontSizes(
                createTheme({
                    components: {
                        MuiTab: {
                            styleOverrides: {
                                root: {
                                    color: isDark ? "white" : "black",
                                    "&.Mui-selected": {
                                        color: "#2DA0DC",
                                    },
                                },
                            },
                        },
                        MuiTextField: {
                            styleOverrides: {
                                root: ({ theme }) =>
                                    theme.unstable_sx({
                                        "input::-webkit-outer-spin-button, input::-webkit-inner-spin-button": {
                                            WebkitAppearance: "none",
                                            margin: 0,
                                        },
                                        "input[type=number]": {
                                            MozAppearance: "textfield",
                                        },
                                    }),
                            },
                        },
                    },
                    palette: {
                        mode: colorScheme,
                        background: {
                            paper: isDark ? "#040f16" : "#eef0f2",
                            default: isDark ? "#040f16" : "#f1f3f4",
                        },
                        primary: {
                            main: "#2DA0DC",
                        },
                        secondary: {
                            main: "#9CACA0",
                        },
                    },
                    typography: {
                        fontSize: 12,
                        fontFamily: [
                            '"Inter"',
                            "-apple-system",
                            "BlinkMacSystemFont",
                            '"Segoe UI"',
                            "Roboto",
                            '"Helvetica Neue"',
                            "Arial",
                            "sans-serif",
                            '"Apple Color Emoji"',
                            '"Segoe UI Emoji"',
                            '"Segoe UI Symbol"',
                        ].join(","),
                    },
                }),
            ),
        [colorScheme],
    );

    useEffect(() => {
        dispatch(restoreOdlcSession());
    }, [dispatch]);

    useEffect(() => {
        socket.on("drone_update", (data) => {
            try {
                const aircraftStatus = parseAndTransformDroneData(data);
                dispatch(updateAircraftStatus(aircraftStatus));
            } catch (error) {
                console.error("Failed to parse drone data:", error);
            }
        });
        return () => {
            socket.off("drone_update");
        };
    }, []);

    return (
        <ThemeProvider theme={theme}>
            <CssBaseline />
            <WebRTCProvider>
                <Box
                    sx={{
                        display: "flex",
                        minwidth: "100vw",
                        minHeight: "100vh",
                        colorScheme: "dark",
                    }}
                >
                    <Nav />
                    <Switch>
                        <Route path="/" component={Home} />
                        {/* <Route path="/telemetry" component={Telemetry} /> */}
                        <Route path="/map" component={MapRoute} />
                        <Route path="/settings" component={Settings} />
                        {/* <Route path="/mps-queue" component={MPSQueue} /> */}
                        <Route path="/queue" component={Queue} />
                        <Route path="/webrtc-test" component={VideoFeed} />
                        <Route path="/odlc-images" component={OdlcImages} />
                        <Route path="/depth-archive" component={DepthArchive} />
                    </Switch>
                </Box>
                <ErrorSnackbar />
            </WebRTCProvider>
        </ThemeProvider>
    );
}

export default App;
