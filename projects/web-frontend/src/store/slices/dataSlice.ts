import { createSlice, PayloadAction } from "@reduxjs/toolkit";
import { AircraftStatus } from "../../types/AircraftStatus";
import { Route } from "../../types/Route";
import { RootState } from "../store";
import { Waypoint } from "../../types/Waypoint";
import type { OdlcImage } from "../../schemas/odlc";

// Per-image annotation: line segment with optional backend-computed distance
// and deprojected camera-frame endpoints (meters). The 3D points are populated
// asynchronously alongside `distance` and used to render dX/dY/dZ in meters.
export type CameraPoint3D = [number, number, number];

export type OdlcImageAnnotation = {
    id: string;
    p1: { x: number; y: number };
    p2: { x: number; y: number };
    distance?: number;
    p1_3d?: CameraPoint3D;
    p2_3d?: CameraPoint3D;
};

// One ODLC image plus UI state (flag, text, metadata, annotations).
export type OdlcImageRecord = {
    id: string;
    receivedAt: number;
    image: OdlcImage;
    flagged: boolean;
    textInput: string;
    metadata: string;
    annotations: OdlcImageAnnotation[];
};

// DataState holds actual information that is supposed to be aligned with backend.
type DataState = {
    aircraftStatus: AircraftStatus;
    availableRoutes: Route[];
    currentRouteId: number | null;
    takeoffWaypoint: Waypoint | null;
    odlcImageRecords: OdlcImageRecord[];
    selectedOdlcImageId: string | null;
};

const initialState: DataState = {
    aircraftStatus: {
        timestamp: Date.now(),
        latitude: 0,
        longitude: 0,
        altitude: 100,
        relativeAltitude: 0,
        verticalSpeed: 100,
        speed: 100,
        heading: 90,
        voltage: 9,
        armed: false,
        flightmode: null,
    },
    availableRoutes: [],
    currentRouteId: null,
    takeoffWaypoint: null,
    odlcImageRecords: [],
    selectedOdlcImageId: null,
};

const dataSlice = createSlice({
    name: "data",
    initialState,
    reducers: {
        updateAircraftStatus: (state, action: PayloadAction<AircraftStatus>) => {
            state.aircraftStatus = action.payload;
        },
        loadAvailableRoutes: (state, action: PayloadAction<Route[]>) => {
            state.availableRoutes = action.payload;
        },
        setCurrentRoute: (state, action: PayloadAction<Route>) => {
            const routeIndex = state.availableRoutes.findIndex((r) => r.id === action.payload.id);
            if (routeIndex !== -1) {
                state.availableRoutes[routeIndex] = action.payload;
            } else {
                state.availableRoutes.push(action.payload);
            }
            state.currentRouteId = action.payload.id;
        },
        addRoute: (state, action: PayloadAction<Route>) => {
            state.availableRoutes.push(action.payload);
        },
        removeRoute: (state, action: PayloadAction<number>) => {
            state.availableRoutes = state.availableRoutes.filter((route) => route.id !== action.payload);
            if (state.currentRouteId === action.payload) {
                state.currentRouteId = null;
            }
        },
        updateRouteInList: (state, action: PayloadAction<Route>) => {
            const index = state.availableRoutes.findIndex((r) => r.id === action.payload.id);
            if (index !== -1) {
                state.availableRoutes[index] = action.payload;
            }
        },
        updateCurrentRouteName: (state, action: PayloadAction<string>) => {
            if (state.currentRouteId !== null) {
                const route = state.availableRoutes.find((r) => r.id === state.currentRouteId);
                if (route) {
                    route.name = action.payload;
                }
            }
        },
        updateCurrentRouteWaypoints: (state, action: PayloadAction<Waypoint[]>) => {
            if (state.currentRouteId !== null) {
                const route = state.availableRoutes.find((r) => r.id === state.currentRouteId);
                if (route) {
                    route.waypoints = action.payload;
                }
            }
        },
        addWaypointToCurrentRoute: (state, action: PayloadAction<Waypoint>) => {
            if (state.currentRouteId !== null) {
                const route = state.availableRoutes.find((r) => r.id === state.currentRouteId);
                if (route) {
                    route.waypoints.push(action.payload);
                }
            }
        },
        editWaypointInCurrentRoute: (state, action: PayloadAction<{ index: number; waypoint: Waypoint }>) => {
            if (state.currentRouteId !== null) {
                const route = state.availableRoutes.find((r) => r.id === state.currentRouteId);
                if (route) {
                    route.waypoints[action.payload.index] = action.payload.waypoint;
                }
            }
        },
        deleteWaypointFromCurrentRoute: (state, action: PayloadAction<number>) => {
            if (state.currentRouteId !== null) {
                const route = state.availableRoutes.find((r) => r.id === state.currentRouteId);
                if (route) {
                    route.waypoints.splice(action.payload, 1);
                }
            }
        },
        setTakeoffWaypoint: (state, action: PayloadAction<Waypoint | null>) => {
            state.takeoffWaypoint = action.payload;
        },
        appendOdlcImage: {
            reducer: (state, action: PayloadAction<OdlcImageRecord>) => {
                state.odlcImageRecords.push(action.payload);
            },
            prepare: (image: OdlcImage, heading: number) => ({
                payload: {
                    id: crypto.randomUUID(),
                    receivedAt: Date.now(),
                    image,
                    flagged: false,
                    textInput: "",
                    metadata: `Direction: ${Math.round(heading)}°`,
                    annotations: [],
                } as OdlcImageRecord,
            }),
        },
        loadOdlcImageRecords: (state, action: PayloadAction<OdlcImageRecord[]>) => {
            state.odlcImageRecords = action.payload;
            state.selectedOdlcImageId = null;
        },
        clearOdlcImages: (state) => {
            state.odlcImageRecords = [];
            state.selectedOdlcImageId = null;
        },
        mergeOdlcImageRecords: (state, action: PayloadAction<OdlcImageRecord[]>) => {
            const existingIds = new Set(state.odlcImageRecords.map((r) => r.id));
            for (const record of action.payload) {
                if (!existingIds.has(record.id)) {
                    state.odlcImageRecords.push(record);
                }
            }
        },
        setSelectedOdlcImage: (state, action: PayloadAction<string | null>) => {
            state.selectedOdlcImageId = action.payload;
        },
        updateOdlcImageFlag: (state, action: PayloadAction<{ id: string; flagged: boolean }>) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            if (r) r.flagged = action.payload.flagged;
        },
        updateOdlcImageTextInput: (state, action: PayloadAction<{ id: string; textInput: string }>) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            if (r) r.textInput = action.payload.textInput;
        },
        updateOdlcImageMetadata: (state, action: PayloadAction<{ id: string; metadata: string }>) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            if (r) r.metadata = action.payload.metadata;
        },
        updateOdlcImageAnnotations: (
            state,
            action: PayloadAction<{ id: string; annotations: OdlcImageAnnotation[] }>,
        ) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            if (r) r.annotations = action.payload.annotations;
        },
        addOdlcImageAnnotation: (
            state,
            action: PayloadAction<{
                id: string;
                p1: { x: number; y: number };
                p2: { x: number; y: number };
                annotationId: string;
            }>,
        ) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            if (r) {
                r.annotations.push({
                    id: action.payload.annotationId,
                    p1: action.payload.p1,
                    p2: action.payload.p2,
                });
            }
        },
        undoLastOdlcImageAnnotation: (state, action: PayloadAction<string>) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload);
            if (r && r.annotations.length > 0) r.annotations.pop();
        },
        deleteOdlcImageAnnotation: (state, action: PayloadAction<{ id: string; annotationId: string }>) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            if (r) r.annotations = r.annotations.filter((a) => a.id !== action.payload.annotationId);
        },
        setOdlcImageAnnotationMeasurements: (
            state,
            action: PayloadAction<{
                id: string;
                annotationId: string;
                distance: number;
                p1_3d: CameraPoint3D;
                p2_3d: CameraPoint3D;
            }>,
        ) => {
            const r = state.odlcImageRecords.find((x) => x.id === action.payload.id);
            const a = r?.annotations.find((x) => x.id === action.payload.annotationId);
            if (a) {
                a.distance = action.payload.distance;
                a.p1_3d = action.payload.p1_3d;
                a.p2_3d = action.payload.p2_3d;
            }
        },
    },
});

export const {
    updateAircraftStatus,
    loadAvailableRoutes,
    setCurrentRoute,
    addRoute,
    removeRoute,
    updateRouteInList,
    updateCurrentRouteName,
    updateCurrentRouteWaypoints,
    addWaypointToCurrentRoute,
    editWaypointInCurrentRoute,
    deleteWaypointFromCurrentRoute,
    setTakeoffWaypoint,
    appendOdlcImage,
    loadOdlcImageRecords,
    clearOdlcImages,
    mergeOdlcImageRecords,
    setSelectedOdlcImage,
    updateOdlcImageFlag,
    updateOdlcImageTextInput,
    updateOdlcImageMetadata,
    updateOdlcImageAnnotations,
    addOdlcImageAnnotation,
    undoLastOdlcImageAnnotation,
    deleteOdlcImageAnnotation,
    setOdlcImageAnnotationMeasurements,
} = dataSlice.actions;

export const selectAircraftStatus = (state: RootState) => state.data.aircraftStatus;
export const selectAvailableRoutes = (state: RootState) => state.data.availableRoutes;
export const selectCurrentRoute = (state: RootState) =>
    state.data.availableRoutes.find((r) => r.id === state.data.currentRouteId) ?? null;
export const selectCurrentRouteWaypoints = (state: RootState) =>
    state.data.availableRoutes.find((r) => r.id === state.data.currentRouteId)?.waypoints ?? [];

export const selectTakeoffWaypoint = (state: RootState) => state.data.takeoffWaypoint;
export const selectOdlcImageRecords = (state: RootState) => state.data.odlcImageRecords;
export const selectSelectedOdlcImageId = (state: RootState) => state.data.selectedOdlcImageId;
export const selectSelectedOdlcImageRecord = (state: RootState) => {
    const id = state.data.selectedOdlcImageId;
    return id ? state.data.odlcImageRecords.find((r) => r.id === id) ?? null : null;
};
export const selectOdlcImageRecordById = (state: RootState, id: string) =>
    state.data.odlcImageRecords.find((r) => r.id === id) ?? null;

const dataReducer = dataSlice.reducer;
export default dataReducer;
