import { PayloadAction, createSlice } from "@reduxjs/toolkit";
import { RootState } from "../store";
import { socket } from "../../api/socket";
import { Coords } from "../../types/Coords";
import { defaultCoords } from "../../utils/coords";

// REDUX SLICE

type AppState = {
    preferredTheme: "light" | "dark";
    globalSnackbar: {
        message: string;
        open: boolean;
        severity: "success" | "error" | "warning";
    };
    telemetrySockets: boolean;
    bypassArmingRestriction: boolean;
    mpsWaypointMapState: boolean[];
    mapViewOpen: boolean;
    mapCenterCoords: Coords;
    showParamValues: boolean;
    odlcSessionId: string | null;
};

const defaultState: AppState = {
    preferredTheme: "dark",
    globalSnackbar: {
        message: "",
        open: false,
        severity: "error",
    },
    telemetrySockets: false,
    bypassArmingRestriction: false,
    mpsWaypointMapState: [],
    mapViewOpen: false,
    mapCenterCoords: defaultCoords,
    showParamValues: false,
    odlcSessionId: null,
};

const persisted = localStorage.getItem("appSlice");
const initialState: AppState = persisted ? { ...defaultState, ...JSON.parse(persisted) } : defaultState;

const appSlice = createSlice({
    name: "app",
    initialState,
    reducers: {
        setPreferredTheme: (state, action: PayloadAction<"light" | "dark">) => {
            state.preferredTheme = action.payload;
        },
        openSnackbar: (
            state,
            action: PayloadAction<{ message: string; severity?: "success" | "error" | "warning" }>,
        ) => {
            state.globalSnackbar = {
                message: action.payload.message,
                open: true,
                severity: action.payload.severity || "error",
            };
        },
        closeSnackbar: (state) => {
            state.globalSnackbar.open = false;
        },
        setSocketStatus: (state, action: PayloadAction<boolean>) => {
            if (action.payload) {
                socket.connect();
            } else {
                socket.disconnect();
            }
            state.telemetrySockets = action.payload;
        },
        setBypassStatus: (state, action: PayloadAction<boolean>) => {
            state.bypassArmingRestriction = action.payload;
        },
        toggleMpsWaypointMapState: (state, action: PayloadAction<number>) => {
            state.mpsWaypointMapState[action.payload] = !state.mpsWaypointMapState[action.payload];
        },
        setAllMpsWaypointMapState: (state, action: PayloadAction<boolean>) => {
            state.mpsWaypointMapState = state.mpsWaypointMapState.map((_bool) => action.payload);
        },
        initializeMpsWaypointMapState: (state, action: PayloadAction<number>) => {
            state.mpsWaypointMapState = new Array(action.payload).fill(false);
        },
        setMapViewOpen: (state, action: PayloadAction<boolean>) => {
            state.mapViewOpen = action.payload;
        },
        setMapCenterCoords: (state, action: PayloadAction<Coords>) => {
            state.mapCenterCoords = action.payload;
        },
        setShowParamValues: (state, action: PayloadAction<boolean>) => {
            state.showParamValues = action.payload;
        },
        setOdlcSessionId: (state, action: PayloadAction<string | null>) => {
            state.odlcSessionId = action.payload;
        },
    },
});

export const {
    setPreferredTheme,
    openSnackbar,
    closeSnackbar,
    setSocketStatus,
    setBypassStatus,
    toggleMpsWaypointMapState,
    setAllMpsWaypointMapState,
    initializeMpsWaypointMapState,
    setMapViewOpen,
    setMapCenterCoords,
    setShowParamValues,
    setOdlcSessionId,
} = appSlice.actions;

export const selectAppSlice = (state: RootState) => state.app;

export const selectPreferredTheme = (state: RootState) => state.app.preferredTheme;
export const selectSnackbar = (state: RootState) => state.app.globalSnackbar;
export const selectSocketStatus = (state: RootState) => state.app.telemetrySockets;
export const selectBypassStatus = (state: RootState) => state.app.bypassArmingRestriction;
export const selectMpsWaypointMapState = (state: RootState) => state.app.mpsWaypointMapState;
export const selectMapViewOpen = (state: RootState) => state.app.mapViewOpen;
export const selectMapCenterCoords = (state: RootState) => state.app.mapCenterCoords;
export const selectShowParamValues = (state: RootState) => state.app.showParamValues;
export const selectOdlcSessionId = (state: RootState) => state.app.odlcSessionId;

const appReducer = appSlice.reducer;
export default appReducer;
