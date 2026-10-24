import { Middleware, Action } from "@reduxjs/toolkit";
import { postOdlcRecord } from "../../api/endpoints";
import { syncOdlcImageToBackend } from "../thunks/odlcSessionThunks";
import type { OdlcImageRecord } from "../slices/dataSlice";

// Action types that mutate a single ODLC image record and should PATCH-sync.
// updateOdlcImageTextInput is intentionally excluded — the component fires an
// explicit sync onBlur so we don't PATCH on every keystroke.
const SYNC_ACTION_TYPES = new Set([
    "data/updateOdlcImageFlag",
    "data/updateOdlcImageMetadata",
    "data/updateOdlcImageAnnotations",
    "data/addOdlcImageAnnotation",
    "data/deleteOdlcImageAnnotation",
    "data/setOdlcImageAnnotationMeasurements",
]);

const UNDO_ACTION_TYPE = "data/undoLastOdlcImageAnnotation";
const APPEND_ACTION_TYPE = "data/appendOdlcImage";

type ActionWithIdPayload = Action<string> & { payload: { id: string } };
type UndoAction = Action<string> & { payload: string };
type AppendAction = Action<string> & { payload: OdlcImageRecord };

const isActionWithIdPayload = (action: unknown): action is ActionWithIdPayload => {
    const a = action as { payload?: { id?: unknown } };
    return typeof a?.payload === "object" && a?.payload !== null && typeof a.payload.id === "string";
};

export const odlcSyncMiddleware: Middleware = (store) => (next) => (action) => {
    const result = next(action);
    const typedAction = action as Action<string>;

    if (typedAction.type === APPEND_ACTION_TYPE) {
        const appendAction = action as AppendAction;
        const sessionId = (store.getState() as { app: { odlcSessionId: string | null } }).app.odlcSessionId;
        if (sessionId && appendAction.payload?.id) {
            postOdlcRecord(sessionId, appendAction.payload).catch((err) => {
                console.error("Failed to POST new ODLC record:", err);
            });
        }
        return result;
    }

    if (typedAction.type === UNDO_ACTION_TYPE) {
        const undoAction = action as UndoAction;
        if (typeof undoAction.payload === "string") {
            (store.dispatch as (a: unknown) => unknown)(syncOdlcImageToBackend(undoAction.payload));
        }
        return result;
    }

    if (SYNC_ACTION_TYPES.has(typedAction.type) && isActionWithIdPayload(action)) {
        (store.dispatch as (a: unknown) => unknown)(syncOdlcImageToBackend(action.payload.id));
    }

    return result;
};
