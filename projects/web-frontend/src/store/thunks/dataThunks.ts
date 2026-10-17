import { createAsyncThunk } from "@reduxjs/toolkit";
import {
    listRoutes,
    getRouteById,
    createRoute as apiCreateRoute,
    deleteRoute as apiDeleteRoute,
    syncRouteWaypoints,
} from "../../api/endpoints";
import { loadAvailableRoutes, setCurrentRoute, addRoute, removeRoute, updateRouteInList } from "../slices/dataSlice";
import { RootState } from "../store";
import { extractAxiosError } from "../../utils/errorHandling";

export const fetchAllRoutes = createAsyncThunk("data/fetchAllRoutes", async (_, { dispatch, rejectWithValue }) => {
    try {
        const routes = await listRoutes();
        dispatch(loadAvailableRoutes(routes));
        return routes;
    } catch (error) {
        return rejectWithValue(extractAxiosError(error));
    }
});

export const ensureActiveRoute = createAsyncThunk(
    "data/ensureActiveRoute",
    async (_, { dispatch, getState, rejectWithValue }) => {
        try {
            const routes = await dispatch(fetchAllRoutes()).unwrap();

            if (routes.length === 0) {
                const newRoute = await dispatch(createNewRoute("Unnamed Route")).unwrap();
                dispatch(setCurrentRoute(newRoute));
                return;
            }

            const state = getState() as RootState;
            if (!state.data.currentRouteId) {
                dispatch(setCurrentRoute(routes[0]));
            }
        } catch (error) {
            return rejectWithValue(extractAxiosError(error));
        }
    },
);

export const fetchRouteById = createAsyncThunk(
    "data/fetchRouteById",
    async (id: number, { dispatch, rejectWithValue }) => {
        try {
            const route = await getRouteById(id);
            dispatch(setCurrentRoute(route));
            return route;
        } catch (error) {
            return rejectWithValue(extractAxiosError(error));
        }
    },
);

export const createNewRoute = createAsyncThunk(
    "data/createNewRoute",
    async (name: string, { dispatch, rejectWithValue }) => {
        try {
            const route = await apiCreateRoute(name);
            dispatch(addRoute(route));
            return route;
        } catch (error) {
            return rejectWithValue(extractAxiosError(error));
        }
    },
);

export const deleteRouteById = createAsyncThunk(
    "data/deleteRouteById",
    async (id: number, { dispatch, rejectWithValue }) => {
        try {
            await apiDeleteRoute(id);
            dispatch(removeRoute(id));
        } catch (error) {
            return rejectWithValue(extractAxiosError(error));
        }
    },
);

export const switchToRoute = createAsyncThunk(
    "data/switchToRoute",
    async (id: number, { dispatch, rejectWithValue }) => {
        try {
            const route = await getRouteById(id);
            dispatch(setCurrentRoute(route));
            return route;
        } catch (error) {
            return rejectWithValue(extractAxiosError(error));
        }
    },
);

export const saveCurrentRouteToBackend = createAsyncThunk(
    "data/saveCurrentRouteToBackend",
    async (_, { getState, dispatch, rejectWithValue }) => {
        try {
            const state = getState() as RootState;
            const currentRouteId = state.data.currentRouteId;

            if (currentRouteId === null) {
                return rejectWithValue({ message: "No current route to save" });
            }

            const currentRoute = state.data.availableRoutes.find((r) => r.id === currentRouteId);

            if (!currentRoute) {
                return rejectWithValue({ message: "Current route not found in available routes" });
            }

            const routeId = currentRoute.id;

            // Sync all waypoints in a single atomic operation
            const updatedRoute = await syncRouteWaypoints(routeId, currentRoute.waypoints);
            dispatch(setCurrentRoute(updatedRoute));
            dispatch(updateRouteInList(updatedRoute));

            return updatedRoute;
        } catch (error) {
            return rejectWithValue(extractAxiosError(error));
        }
    },
);
