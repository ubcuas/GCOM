import { Button, Chip, Stack, Tooltip } from "@mui/material";
import Refresh from "@mui/icons-material/Refresh";
import { useAppDispatch, useAppSelector } from "../store/store";
import { selectOdlcSessionId } from "../store/slices/appSlice";
import { newOdlcSession } from "../store/thunks/odlcSessionThunks";

const truncate = (id: string): string => (id.length > 8 ? `${id.slice(0, 8)}…` : id);

export default function SessionIdDisplay() {
    const dispatch = useAppDispatch();
    const sessionId = useAppSelector(selectOdlcSessionId);

    const handleNewSession = () => {
        const confirmed = window.confirm(
            "Start a new ODLC session? This will clear the current images from view (the previous session remains saved on the server).",
        );
        if (!confirmed) return;
        dispatch(newOdlcSession());
    };

    return (
        <Stack direction="row" spacing={1} alignItems="center">
            <Tooltip title={sessionId ?? "No active session"}>
                <Chip label={`Session: ${sessionId ? truncate(sessionId) : "none"}`} size="small" variant="outlined" />
            </Tooltip>
            <Button size="small" variant="outlined" startIcon={<Refresh />} onClick={handleNewSession}>
                New Session
            </Button>
        </Stack>
    );
}
