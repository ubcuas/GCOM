import { createAsyncThunk } from "@reduxjs/toolkit";
import { getOdlcSession, patchOdlcRecord } from "../../api/endpoints";
import { appendOdlcImage, clearOdlcImages, loadOdlcImageRecords } from "../slices/dataSlice";
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

export const restoreOdlcSession = createAsyncThunk<void, void, { state: RootState }>(
    "odlcSession/restore",
    async (_, { dispatch, getState }) => {
        const sessionId = getState().app.odlcSessionId;
        if (!sessionId) {
            dispatch(newOdlcSession());
            return;
        }
        try {
            const session = await getOdlcSession(sessionId);
            if (session === null) {
                dispatch(newOdlcSession());
                return;
            }
            dispatch(loadOdlcImageRecords(session.records));
        } catch (err) {
            console.error("Failed to restore ODLC session:", err);
        }
    },
);
