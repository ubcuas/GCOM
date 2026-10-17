// Honestly its just easier to have this in a separate file rather than having it be in with MapView.tsx

import { CropFree, Flight, MyLocation, Place } from "@mui/icons-material";
import { Box, IconButton, Tooltip } from "@mui/material";
import { Fragment, useRef, useState } from "react";
import { Layer, LayerProps, Map, MapRef, MapLayerMouseEvent, Marker, Source } from "react-map-gl/maplibre";
import {
    addWaypointToCurrentRoute,
    editWaypointInCurrentRoute,
    selectAircraftStatus,
    selectCurrentRouteWaypoints,
    selectTakeoffWaypoint,
} from "../../store/slices/dataSlice";
import { openSnackbar } from "../../store/slices/appSlice";
import { useAppDispatch, useAppSelector } from "../../store/store";
import WaypointItem from "../WaypointItem";
import { roundTo } from "../../utils/routeTo";
import { MAP_STYLE } from "../../constants";
import { saveCurrentRouteToBackend } from "../../store/thunks/dataThunks";
import { WaypointEditState } from "../../types/Waypoint";

type DraggedMarker = {
    long: number;
    lat: number;
    index: number;
};

type CreationMapProps = {
    handleDelete: (index: number) => void;
    handleEdit: (index: number) => void;
    editState: WaypointEditState;
};

export default function WaypointCreationMap({ handleDelete, handleEdit, editState }: CreationMapProps) {
    const clientWPQueue = useAppSelector(selectCurrentRouteWaypoints);
    const aircraftStatus = useAppSelector(selectAircraftStatus);
    const takeoffWaypoint = useAppSelector(selectTakeoffWaypoint);
    const dispatch = useAppDispatch();
    const mapRef = useRef<MapRef>(null);
    const [selectedWaypoints, setSelectedWaypoints] = useState<boolean[]>(clientWPQueue.map(() => false));
    const [draggedMarkerData, setDraggedMarkerData] = useState<DraggedMarker | null>(null);

    const fitToWaypoints = () => {
        if (clientWPQueue.length === 0) return;
        const longitudes = clientWPQueue.map((wp) => wp.longitude);
        const latitudes = clientWPQueue.map((wp) => wp.latitude);
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
        coordinates: clientWPQueue.map((waypoint) => [waypoint.longitude, waypoint.latitude]),
    };
    const routeStyle: LayerProps = {
        id: "mps-route",
        type: "line",
        paint: {
            "line-color": "#ee4455",
            "line-width": 3,
        },
    };

    const createNewWaypoint = (event: MapLayerMouseEvent) => {
        if (event.originalEvent.detail !== 2) return;

        const unnamedCount = clientWPQueue.filter((wp) => wp.name?.startsWith("Unnamed Waypoint")).length;
        const newName = `Unnamed Waypoint ${unnamedCount + 1}`;

        const defaultAltitude = takeoffWaypoint?.altitude ?? 30;
        if (!takeoffWaypoint?.altitude) {
            dispatch(
                openSnackbar({
                    message: "Takeoff altitude unknown. Waypoint altitude set to 30m automatically.",
                    severity: "warning",
                }),
            );
        }

        dispatch(
            addWaypointToCurrentRoute({
                id: "-1",
                latitude: roundTo(event.lngLat.lat, 7),
                longitude: roundTo(event.lngLat.lng, 7),
                altitude: defaultAltitude,
                name: newName,
            }),
        );
        dispatch(saveCurrentRouteToBackend());
        setSelectedWaypoints((prev) => [...prev, false]);
    };

    const handleSelectWaypoint = (index: number) => {
        setSelectedWaypoints((prev) => {
            const newSelected = [...prev];
            newSelected[index] = !newSelected[index];
            return newSelected;
        });
    };

    return (
        <Map
            ref={mapRef}
            initialViewState={{
                longitude: aircraftStatus.longitude,
                latitude: aircraftStatus.latitude,
                zoom: 16,
            }}
            mapStyle={MAP_STYLE}
            onClick={createNewWaypoint}
            doubleClickZoom={false}
            style={{
                minHeight: "500px",
            }}
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
            <Marker latitude={aircraftStatus.latitude} longitude={aircraftStatus.longitude}>
                <Flight
                    sx={{
                        color: "primary.main",
                        rotate: `${aircraftStatus.heading}deg`,
                        fontSize: "48px",
                        filter: "drop-shadow(0px 0px 2px white)",
                    }}
                />
            </Marker>
            {clientWPQueue.map((waypoint, i) => {
                return (
                    <Fragment key={i}>
                        <Marker
                            draggable
                            onClick={(e) => {
                                e.originalEvent.stopPropagation();
                                handleSelectWaypoint(i);
                            }}
                            onDrag={(e) => {
                                setDraggedMarkerData({
                                    long: e.lngLat.lng,
                                    lat: e.lngLat.lat,
                                    index: i,
                                });
                            }}
                            onDragEnd={() => {
                                dispatch(
                                    editWaypointInCurrentRoute({
                                        index: i,
                                        waypoint: {
                                            ...waypoint,
                                            latitude: roundTo(draggedMarkerData!.lat, 7),
                                            longitude: roundTo(draggedMarkerData!.long, 7),
                                        },
                                    }),
                                );
                                dispatch(saveCurrentRouteToBackend());
                                setDraggedMarkerData(null);
                            }}
                            latitude={
                                draggedMarkerData && draggedMarkerData.index === i
                                    ? draggedMarkerData.lat
                                    : waypoint.latitude
                            }
                            longitude={
                                draggedMarkerData && draggedMarkerData.index === i
                                    ? draggedMarkerData.long
                                    : waypoint.longitude
                            }
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
                                    filter: "drop-shadow(0px 0px 2px white)",
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
                        {selectedWaypoints[i] && (
                            <Marker
                                latitude={
                                    draggedMarkerData && draggedMarkerData.index === i
                                        ? draggedMarkerData.lat
                                        : waypoint.latitude
                                }
                                longitude={
                                    draggedMarkerData && draggedMarkerData.index === i
                                        ? draggedMarkerData.long
                                        : waypoint.longitude
                                }
                            >
                                <WaypointItem
                                    sx={{
                                        position: "absolute",
                                        width: "220px",
                                        top: "10px",
                                        border: "4px solid",
                                        borderColor: i === editState.index ? "primary.main" : "transparent",
                                    }}
                                    waypoint={waypoint}
                                    handleDelete={() => {
                                        handleDelete(i);
                                        setSelectedWaypoints((prev) => {
                                            prev.splice(i, 1);
                                            return prev;
                                        });
                                    }}
                                    handleEdit={() => handleEdit(i)}
                                />
                            </Marker>
                        )}
                    </Fragment>
                );
            })}
            <Source type="geojson" data={routeData}>
                <Layer {...routeStyle} />
            </Source>
        </Map>
    );
}
