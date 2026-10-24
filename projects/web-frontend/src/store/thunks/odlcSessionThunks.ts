import { createAsyncThunk } from "@reduxjs/toolkit";
import {
    getLatestOdlcSessionId,
    getOdlcSession,
    getNewOdlcRecordIds,
    getOdlcRecord,
    patchOdlcRecord,
} from "../../api/endpoints";
import { appendOdlcImage, clearOdlcImages, loadOdlcImageRecords, mergeOdlcImageRecords } from "../slices/dataSlice";
import { setOdlcSessionId } from "../slices/appSlice";
import { AppDispatch, RootState } from "../store";
import type { OdlcImage } from "../../schemas/odlc";

export const appendOdlcImageFromAircraft = (image: OdlcImage) => (dispatch: AppDispatch, getState: () => RootState) => {
    const heading = getState().data.aircraftStatus.heading;
    dispatch(appendOdlcImage(image, heading));
};

export const syncOdlcImageToBackend = createAsyncThunk<void, string, { state: RootState }>(
    "odlcSession/syncImage",
    async (id, { getState }) => {
        const state = getState();
        const sessionId = state.app.odlcSessionId;
        if (!sessionId) return;
        const record = state.data.odlcImageRecords.find((r) => r.id === id);
        if (!record) return;
        try {
            await patchOdlcRecord(sessionId, id, {
                flagged: record.flagged,
                textInput: record.textInput,
                metadata: record.metadata,
                annotations: record.annotations,
            });
        } catch (err) {
            console.error("Failed to sync ODLC record:", err);
        }
    },
);

export const newOdlcSession = createAsyncThunk<void, void, { state: RootState }>(
    "odlcSession/new",
    async (_, { dispatch }) => {
        const newId = crypto.randomUUID();
        dispatch(setOdlcSessionId(newId));
        dispatch(clearOdlcImages());
    },
);

export const pollOdlcSession = createAsyncThunk<void, void, { state: RootState }>(
    "odlcSession/poll",
    async (_, { dispatch, getState }) => {
        const state = getState();
        const sessionId = state.app.odlcSessionId;
        if (!sessionId) return;
        const records = state.data.odlcImageRecords;
        const since = records.length > 0 ? Math.max(...records.map((r) => r.receivedAt)) : 0;
        try {
            const newIds = await getNewOdlcRecordIds(sessionId, since);
            if (newIds.length === 0) return;
            const fetched = await Promise.all(newIds.map((id) => getOdlcRecord(sessionId, id)));
            const valid = fetched.filter((r): r is NonNullable<typeof r> => r !== null);
            if (valid.length > 0) {
                dispatch(mergeOdlcImageRecords(valid));
            }
        } catch (err) {
            console.error("Failed to poll ODLC session:", err);
        }
    },
);

export const restoreOdlcSession = createAsyncThunk<void, void, { state: RootState }>(
    "odlcSession/restore",
    async (_, { dispatch }) => {
        try {
            const sessionId = await getLatestOdlcSessionId();
            if (sessionId === null) {
                dispatch(newOdlcSession());
                return;
            }
            const session = await getOdlcSession(sessionId);
            if (session === null) {
                dispatch(newOdlcSession());
                return;
            }
            dispatch(setOdlcSessionId(sessionId));
            dispatch(loadOdlcImageRecords(session.records));
        } catch (err) {
            console.error("Failed to restore ODLC session:", err);
        }
    },
);
