import { Waypoint } from "../types/Waypoint";
import { Route } from "../types/Route";
import api from "./api";
import { WaypointSchema, RouteSchema, PartialWaypointSchema } from "../schemas/waypoint";
import { OdlcSessionSchema, OdlcImageRecordSchema } from "../schemas/odlc";
import type { OdlcImageRecord } from "../store/slices/dataSlice";
import type { z } from "zod";

// TODO: Implement new endpoint logic

export const armDrone = async (arm: boolean) => {
    return await api.put("/drone/arm", { arm });
};

export const prepareTakeoffDrone = async (altitude: number) => {
    return await api.post("/drone/prepare_takeoff", { altitude });
};

export const returnToLaunch = async () => {
    return await api.post("/drone/rtl");
};

export const postWaypointsToDrone = async (waypoints: Waypoint[]) => {
    console.log("Preparing to post waypoints to drone via API", waypoints);
    return await api.post("/drone/queue", waypoints);
};

// not to be confused with the current route
// these are the waypoints currently loaded in the drone's mission queue
// not the ones stored in the backend which may or may not have been posted to the drone
export const getCurrentMissionRaw = async (): Promise<object[]> => {
    const response = await api.get("/drone/queue");
    return response.data;
};

export const listRoutes = async (): Promise<Route[]> => {
    const response = await api.get("/route/");
    return response.data.map((route: unknown) => RouteSchema.parse(route));
};

export const getRouteById = async (id: number): Promise<Route> => {
    const response = await api.get(`/route/${id}/`);
    return RouteSchema.parse(response.data);
};

export const createRoute = async (name: string): Promise<Route> => {
    const response = await api.post("/route/", { name });
    return RouteSchema.parse(response.data);
};

export const deleteRoute = async (id: number): Promise<void> => {
    await api.delete(`/route/${id}/`);
};

export const updateRouteName = async (id: number, name: string): Promise<Route> => {
    const response = await api.put(`/route/${id}/`, { name });
    return RouteSchema.parse(response.data);
};

export const addWaypointToRoute = async (
    routeId: number,
    waypoint: Omit<Waypoint, "id">,
    order: number,
): Promise<Waypoint> => {
    console.log("Adding waypoint to route via API", routeId, waypoint, order);
    const validatedWaypoint = PartialWaypointSchema.parse(waypoint);
    const response = await api.post("/waypoint/", {
        ...validatedWaypoint,
        route: routeId,
        order,
    });
    return WaypointSchema.parse(response.data);
};

export const updateWaypoint = async (
    waypointId: string,
    waypoint: Partial<Omit<Waypoint, "id">>,
): Promise<Waypoint> => {
    console.log("Updating waypoint via API", waypointId, waypoint);
    const validatedWaypoint = PartialWaypointSchema.parse(waypoint);
    const response = await api.put(`/waypoint/${waypointId}/`, validatedWaypoint);
    return WaypointSchema.parse(response.data);
};

export const deleteWaypoint = async (waypointId: string): Promise<void> => {
    await api.delete(`/waypoint/${waypointId}/`);
};

export const reorderWaypoints = async (routeId: number, waypointIds: string[]): Promise<void> => {
    await api.post(`/route/${routeId}/reorder-waypoints/`, waypointIds);
};

export const syncRouteWaypoints = async (routeId: number, waypoints: Waypoint[]): Promise<Route> => {
    console.log("Syncing waypoints via API", routeId, waypoints);
    const response = await api.post(`/route/${routeId}/sync-waypoints/`, waypoints);
    return RouteSchema.parse(response.data);
};

export const getDroneParameter = async (
    paramId: string,
): Promise<{ param_id: string; param_value: number; param_type: number }> => {
    const response = await api.get(`/drone/parameters/${paramId}`);
    return response.data;
};

export type OdlcSessionPayload = z.infer<typeof OdlcSessionSchema>;

export const getLatestOdlcSessionId = async (): Promise<string | null> => {
    try {
        const response = await api.get("/vision/odlc-session/latest/");
        return (response.data as { sessionId: string }).sessionId;
    } catch (err: unknown) {
        if (typeof err === "object" && err !== null && "response" in err) {
            const status = (err as { response?: { status?: number } }).response?.status;
            if (status === 404) return null;
        }
        throw err;
    }
};

export const getOdlcSession = async (
    sessionId: string,
    options?: { since?: number },
): Promise<OdlcSessionPayload | null> => {
    try {
        const params = options?.since !== undefined ? { since: options.since } : undefined;
        const response = await api.get(`/vision/odlc-session/${sessionId}/`, { params });
        return OdlcSessionSchema.parse(response.data);
    } catch (err: unknown) {
        if (typeof err === "object" && err !== null && "response" in err) {
            const status = (err as { response?: { status?: number } }).response?.status;
            if (status === 404) return null;
        }
        throw err;
    }
};

export const postOdlcRecord = async (sessionId: string, record: OdlcImageRecord): Promise<void> => {
    await api.post(`/vision/odlc-session/${sessionId}/records/`, record);
};

export const patchOdlcRecord = async (
    sessionId: string,
    recordId: string,
    updates: Partial<Pick<OdlcImageRecord, "flagged" | "textInput" | "metadata" | "annotations">>,
): Promise<void> => {
    await api.patch(`/vision/odlc-session/${sessionId}/records/${recordId}/`, updates);
};

export const getNewOdlcRecordIds = async (sessionId: string, since: number): Promise<string[]> => {
    const response = await api.get(`/vision/odlc-session/${sessionId}/index/`, { params: { since } });
    const data = response.data as { records: { id: string }[] };
    return data.records.map((r) => r.id);
};

export const getOdlcRecord = async (sessionId: string, recordId: string): Promise<OdlcImageRecord | null> => {
    try {
        const response = await api.get(`/vision/odlc-session/${sessionId}/records/${recordId}/`);
        return OdlcImageRecordSchema.parse(response.data);
    } catch (err: unknown) {
        if (typeof err === "object" && err !== null && "response" in err) {
            const status = (err as { response?: { status?: number } }).response?.status;
            if (status === 404) return null;
        }
        throw err;
    }
};

/**
 * Deprojects a 2D pixel coordinate and depth value into a 3D point in camera space.
 * Calls the backend which implements the RealSense deprojection formula with
 * hardcoded Brown-Conrady camera intrinsics.
 *
 * @param pixel - [x, y] pixel coordinates in the image (column, row)
 * @param depth - Depth at the pixel in meters
 * @returns A 3D point [x, y, z] in camera coordinate space, in meters
 */
export const deprojectPixel = async (
    pixel: [number, number],
    depth: number,
): Promise<{ point: [number, number, number] }> => {
    const response = await api.post<{ point: [number, number, number] }>("/vision/deproject-pixel/", {
        pixel,
        depth,
    });
    return response.data;
};

/**
 * List archive dates (newest first) for which any captured ODLC content lives
 * in the bucket. The backend enumerates the `odlc/<YYYY-MM-DD>/` prefixes.
 */
export const getArchiveDates = async (): Promise<unknown> => {
    const response = await api.get("/vision/archive/dates/");
    return response.data;
};

/**
 * List every archived record for the given `YYYY-MM-DD` date. Each entry
 * includes the bucket-relative `colorKey` / `depthKey` plus any sidecar
 * metadata (bounding box, confidence, color detection, yaw, receivedAt).
 */
export const getArchiveRecordsForDate = async (date: string): Promise<unknown> => {
    const response = await api.get(`/vision/archive/dates/${encodeURIComponent(date)}/`);
    return response.data;
};

/**
 * Build the absolute backend URL for streaming a single archive object. The
 * `key` is interpreted relative to the `odlc/` archive prefix on the server.
 */
export const getArchiveObjectFetchUrl = (key: string): string => {
    const base = api.defaults.baseURL ?? "";
    const normalizedBase = base.endsWith("/") ? base : `${base}/`;
    return `${normalizedBase}vision/archive/object/?key=${encodeURIComponent(key)}`;
};
