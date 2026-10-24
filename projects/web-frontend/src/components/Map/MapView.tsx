import { Flight, Place, Home, MyLocation, CropFree } from "@mui/icons-material";
import { Box, IconButton, Tooltip } from "@mui/material";
import { Fragment, useEffect, useRef } from "react";
import Map, { Layer, LayerProps, MapRef, Marker, Source } from "react-map-gl/maplibre";
import {
    initializeMpsWaypointMapState,
    selectMpsWaypointMapState,
    toggleMpsWaypointMapState,
} from "../../store/slices/appSlice";
import { selectAircraftStatus, selectCurrentRouteWaypoints, selectTakeoffWaypoint } from "../../store/slices/dataSlice";
import { useAppDispatch, useAppSelector } from "../../store/store";
import WaypointItem from "../WaypointItem";
import { MAP_STYLE } from "../../constants";

export default function MapView() {
    const mpsWaypoints = useAppSelector(selectCurrentRouteWaypoints);
    const mpsWaypointMapState = useAppSelector(selectMpsWaypointMapState);
    const aircraftStatus = useAppSelector(selectAircraftStatus);
    const takeoffWaypoint = useAppSelector(selectTakeoffWaypoint);
    const dispatch = useAppDispatch();
    const mapRef = useRef<MapRef>(null);

    useEffect(() => {
        dispatch(initializeMpsWaypointMapState(mpsWaypoints.length));
    }, []);

    const fitToWaypoints = () => {
        if (mpsWaypoints.length === 0) return;
        const longitudes = mpsWaypoints.map((wp) => wp.longitude);
        const latitudes = mpsWaypoints.map((wp) => wp.latitude);
        const minLng = Math.min(...longitudes);
        const maxLng = Math.max(...longitudes);
        const minLat = Math.min(...latitudes);
        const maxLat = Math.max(...latitudes);

        mapRef.current?.fitBounds(
            [
                [minLng, minLat],
                [maxLng, maxLat],
            ],
            { padding: 100, duration: 1000 },
        );
    };

    const centerOnDrone = () => {
        mapRef.current?.flyTo({
            center: [aircraftStatus.longitude, aircraftStatus.latitude],
            zoom: 16,
            duration: 1000,
        });
    };

    const routeData: GeoJSON.GeoJSON = {
        type: "LineString",
        coordinates: mpsWaypoints.map((waypoint) => [waypoint.longitude, waypoint.latitude]),
    };
    const routeStyle: LayerProps = {
        id: "mps-route",
        type: "line",
        paint: {
            "line-color": "#ee4455",
            "line-width": 3,
        },
    };

    return (
        <Box
            sx={{
                height: "100%",
                width: "calc(100vw - 56px)", // 56px is the width of the vertical tabs
            }}
        >
            <Map
                ref={mapRef}
                initialViewState={{
                    longitude: aircraftStatus.longitude,
                    latitude: aircraftStatus.latitude,
                    zoom: 16,
                }}
                mapStyle={MAP_STYLE}
                doubleClickZoom={false}
            >
                <Box
                    sx={{
                        position: "absolute",
                        top: 10,
                        right: 10,
                        display: "flex",
                        flexDirection: "column",
                        gap: 1,
                        zIndex: 1,
                    }}
                >
                    <Tooltip title="Center on Drone">
                        <IconButton
                            onClick={centerOnDrone}
                            sx={{ bgcolor: "background.paper", "&:hover": { bgcolor: "background.paper" } }}
                        >
                            <MyLocation />
                        </IconButton>
                    </Tooltip>
                    <Tooltip title="Fit to Waypoints">
                        <IconButton
                            onClick={fitToWaypoints}
                            sx={{ bgcolor: "background.paper", "&:hover": { bgcolor: "background.paper" } }}
                        >
                            <CropFree />
                        </IconButton>
                    </Tooltip>
                </Box>

                {mpsWaypoints.map((waypoint, i) => (
                    <Fragment key={i}>
                        <Marker
                            latitude={waypoint.latitude}
                            longitude={waypoint.longitude}
                            onClick={() => dispatch(toggleMpsWaypointMapState(i))}
                            style={{
                                cursor: "pointer",
                            }}
                        >
                            <Place
                                sx={{
                                    color: "#ee4455",
                                    fontSize: "48px",
                                    position: "absolute",
                                    top: "-46px",
                                    left: "-24px",
                                }}
                            />
                            <Box
                                sx={{
                                    background: "#ee4455",
                                    height: "18px",
                                    width: "12px",
                                    position: "absolute",
                                    left: "-6px",
                                    top: "-36px",
                                    textAlign: "center",
                                    fontWeight: "bold",
                                    fontSize: "18px",
                                }}
                            >
                                {i + 1}
                            </Box>
                        </Marker>

                        {mpsWaypointMapState[i] && (
                            <Marker latitude={waypoint.latitude} longitude={waypoint.longitude}>
                                <WaypointItem
                                    sx={{
                                        position: "absolute",
                                        width: "180px",
                                        top: "10px",
                                    }}
                                    waypoint={waypoint}
                                />
                            </Marker>
                        )}
                    </Fragment>
                ))}

                {takeoffWaypoint && (
                    <Marker
                        latitude={takeoffWaypoint.latitude}
                        longitude={takeoffWaypoint.longitude}
                        style={{
                            cursor: "pointer",
                        }}
                    >
                        <Home
                            sx={{
                                color: "#ff5722", // Distinct color for takeoff
                                fontSize: "48px",
                                position: "absolute",
                                top: "-46px",
                                left: "-24px",
                            }}
                        />
                        <Box
                            sx={{
                                background: "#ff5722",
                                height: "18px",
                                width: "12px",
                                position: "absolute",
                                left: "-6px",
                                top: "-36px",
                                textAlign: "center",
                                fontWeight: "bold",
                                fontSize: "18px",
                            }}
                        >
                            T {/* Label for Takeoff */}
                        </Box>
                    </Marker>
                )}

                <Marker latitude={aircraftStatus.latitude} longitude={aircraftStatus.longitude}>
                    <Flight
                        sx={{
                            color: "primary.main",
                            rotate: `${aircraftStatus.heading}deg`,
                            fontSize: "48px",
                        }}
                    />
                </Marker>

                <Source type="geojson" data={routeData}>
                    <Layer {...routeStyle} />
                </Source>
            </Map>
        </Box>
    );
}
