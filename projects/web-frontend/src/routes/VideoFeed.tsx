import { Box, Button, Paper, Stack, Typography, Chip, Grid, IconButton, Tooltip } from "@mui/material";
import { useRef, useEffect, useState, useCallback } from "react";
import { useWebRTCContext } from "../context/WebRTCContext";
import PhotoCamera from "@mui/icons-material/PhotoCamera";
import VideocamIcon from "@mui/icons-material/Videocam";
import Visibility from "@mui/icons-material/Visibility";
import VisibilityOff from "@mui/icons-material/VisibilityOff";
import DroneStatusCard from "../components/DroneStatusCard";
import SessionIdDisplay from "../components/SessionIdDisplay";

type DepthSampleMarker = { x: number; y: number };

export default function VideoFeed() {
    const {
        signalingStatus,
        peerStatus,
        remoteStream,
        connect,
        disconnect,
        isConnecting,
        sendCommand,
        lastDepthResult,
    } = useWebRTCContext();

    const videoRef = useRef<HTMLVideoElement>(null);
    const [capturing, setCapturing] = useState(false);
    const [statusCardVisible, setStatusCardVisible] = useState(true);
    const [depthMarker, setDepthMarker] = useState<DepthSampleMarker | null>(null);
    const depthMarkerTimerRef = useRef<ReturnType<typeof setTimeout> | null>(null);

    useEffect(() => {
        if (videoRef.current && remoteStream) {
            console.log("Setting stream to video element");
            console.log("Stream active:", remoteStream.active);
            console.log(
                "Stream tracks:",
                remoteStream.getTracks().map((t) => ({
                    kind: t.kind,
                    enabled: t.enabled,
                    muted: t.muted,
                    readyState: t.readyState,
                })),
            );

            videoRef.current.srcObject = remoteStream;
        }
    }, [remoteStream]);

    useEffect(() => {
        return () => {
            if (depthMarkerTimerRef.current !== null) {
                clearTimeout(depthMarkerTimerRef.current);
            }
        };
    }, []);

    const handleVideoClick = useCallback(
        (event: React.MouseEvent<HTMLVideoElement>) => {
            if (peerStatus !== "connected" || !videoRef.current) return;

            const video = videoRef.current;
            const rect = video.getBoundingClientRect();
            const videoWidth = video.videoWidth;
            const videoHeight = video.videoHeight;

            if (videoWidth === 0 || videoHeight === 0) return;

            const containerWidth = rect.width;
            const containerHeight = rect.height;
            const containerAspect = containerWidth / containerHeight;
            const videoAspect = videoWidth / videoHeight;

            let renderedWidth: number;
            let renderedHeight: number;
            let offsetX: number;
            let offsetY: number;

            if (videoAspect > containerAspect) {
                renderedWidth = containerWidth;
                renderedHeight = containerWidth / videoAspect;
                offsetX = 0;
                offsetY = (containerHeight - renderedHeight) / 2;
            } else {
                renderedHeight = containerHeight;
                renderedWidth = containerHeight * videoAspect;
                offsetX = (containerWidth - renderedWidth) / 2;
                offsetY = 0;
            }

            const clickX = event.clientX - rect.left - offsetX;
            const clickY = event.clientY - rect.top - offsetY;

            if (clickX < 0 || clickY < 0 || clickX > renderedWidth || clickY > renderedHeight) return;

            const u = parseFloat((clickX / renderedWidth).toFixed(4));
            const v = parseFloat((clickY / renderedHeight).toFixed(4));

            sendCommand({ action: "SAMPLE_DEPTH", u, v });

            const markerX = ((offsetX + clickX) / containerWidth) * 100;
            const markerY = ((offsetY + clickY) / containerHeight) * 100;
            setDepthMarker({ x: markerX, y: markerY });

            if (depthMarkerTimerRef.current !== null) {
                clearTimeout(depthMarkerTimerRef.current);
            }
            depthMarkerTimerRef.current = setTimeout(() => {
                setDepthMarker(null);
                depthMarkerTimerRef.current = null;
            }, 1500);
        },
        [peerStatus, sendCommand],
    );

    const getStatusColor = (
        status: typeof signalingStatus | typeof peerStatus,
    ): "default" | "success" | "warning" | "error" => {
        switch (status) {
            case "connected":
                return "success";
            case "connecting":
                return "warning";
            case "failed":
                return "error";
            default:
                return "default";
        }
    };

    const getStatusLabel = (status: typeof signalingStatus | typeof peerStatus): string => {
        return status.charAt(0).toUpperCase() + status.slice(1);
    };

    return (
        <Box
            sx={{
                p: 8,
                width: "100%",
            }}
        >
            <Stack spacing={3}>
                <Paper
                    sx={{
                        p: 3,
                    }}
                >
                    <Grid container spacing={3}>
                        <Grid item xs={12} lg={statusCardVisible ? 8 : 12}>
                            <Stack direction="row" alignItems="center" justifyContent="space-between" mb={3}>
                                <Typography
                                    variant="h4"
                                    sx={{
                                        fontWeight: "bold",
                                    }}
                                >
                                    WebRTC Video Stream
                                </Typography>
                                <Stack direction="row" alignItems="center" spacing={1}>
                                    {!statusCardVisible && (
                                        <Button
                                            variant="contained"
                                            size="small"
                                            startIcon={<Visibility fontSize="small" />}
                                            onClick={() => setStatusCardVisible(true)}
                                            aria-label="Show drone status"
                                        >
                                            Show Drone Status
                                        </Button>
                                    )}
                                    <SessionIdDisplay />
                                </Stack>
                            </Stack>
                            <Stack spacing={2}>
                                <Box
                                    sx={{
                                        position: "relative",
                                        width: "100%",
                                        paddingBottom: "56.25%",
                                        backgroundColor: "background.default",
                                        borderRadius: 1,
                                        overflow: "hidden",
                                    }}
                                >
                                    <video
                                        ref={videoRef}
                                        autoPlay
                                        playsInline
                                        muted
                                        onClick={handleVideoClick}
                                        style={{
                                            position: "absolute",
                                            top: 0,
                                            left: 0,
                                            width: "100%",
                                            height: "100%",
                                            objectFit: "contain",
                                            cursor: peerStatus === "connected" ? "crosshair" : "default",
                                        }}
                                    />
                                    {depthMarker && (
                                        <Box
                                            sx={{
                                                position: "absolute",
                                                top: `${depthMarker.y}%`,
                                                left: `${depthMarker.x}%`,
                                                transform: "translate(-50%, -50%)",
                                                pointerEvents: "none",
                                            }}
                                        >
                                            <Box
                                                sx={{
                                                    width: 16,
                                                    height: 16,
                                                    borderRadius: "50%",
                                                    border: "2px solid #fff",
                                                    bgcolor: "rgba(33, 150, 243, 0.7)",
                                                    boxShadow: "0 0 4px rgba(0,0,0,0.6)",
                                                }}
                                            />
                                        </Box>
                                    )}
                                    {!remoteStream && (
                                        <Box
                                            sx={{
                                                position: "absolute",
                                                top: 0,
                                                left: 0,
                                                width: "100%",
                                                height: "100%",
                                                display: "flex",
                                                flexDirection: "column",
                                                alignItems: "center",
                                                justifyContent: "center",
                                                gap: 2,
                                                color: "text.secondary",
                                            }}
                                        >
                                            <VideocamIcon sx={{ fontSize: 64, opacity: 0.3 }} />
                                            <Typography variant="body1">
                                                {signalingStatus === "connected"
                                                    ? "Waiting for video stream..."
                                                    : "Connect to start streaming"}
                                            </Typography>
                                        </Box>
                                    )}
                                </Box>

                                <Paper sx={{ p: 2 }}>
                                    <Stack spacing={2}>
                                        <Box>
                                            <Typography variant="subtitle2" sx={{ mb: 1, fontWeight: "bold" }}>
                                                Connection Status
                                            </Typography>
                                            <Stack direction="row" spacing={2}>
                                                <Box sx={{ flex: 1 }}>
                                                    <Typography variant="caption" color="text.secondary">
                                                        Signaling Server
                                                    </Typography>
                                                    <Box sx={{ mt: 0.5 }}>
                                                        <Chip
                                                            label={getStatusLabel(signalingStatus)}
                                                            color={getStatusColor(signalingStatus)}
                                                            size="small"
                                                        />
                                                    </Box>
                                                </Box>
                                                <Box sx={{ flex: 1 }}>
                                                    <Typography variant="caption" color="text.secondary">
                                                        WebRTC Peer
                                                    </Typography>
                                                    <Box sx={{ mt: 0.5 }}>
                                                        <Chip
                                                            label={getStatusLabel(peerStatus)}
                                                            color={getStatusColor(peerStatus)}
                                                            size="small"
                                                        />
                                                    </Box>
                                                </Box>
                                            </Stack>
                                        </Box>

                                        {lastDepthResult && (
                                            <Box>
                                                <Typography variant="subtitle2" sx={{ mb: 1, fontWeight: "bold" }}>
                                                    Last Depth Sample
                                                </Typography>
                                                <Box
                                                    sx={{
                                                        p: 1.5,
                                                        borderRadius: 1,
                                                        bgcolor: "action.hover",
                                                        fontFamily: "monospace",
                                                        fontSize: "0.8rem",
                                                        display: "grid",
                                                        gridTemplateColumns: "auto 1fr",
                                                        columnGap: 2,
                                                        rowGap: 0.5,
                                                    }}
                                                >
                                                    <Typography
                                                        variant="caption"
                                                        color="text.secondary"
                                                        fontFamily="monospace"
                                                    >
                                                        u
                                                    </Typography>
                                                    <Typography variant="caption" fontFamily="monospace">
                                                        {lastDepthResult.u.toFixed(4)}
                                                    </Typography>
                                                    <Typography
                                                        variant="caption"
                                                        color="text.secondary"
                                                        fontFamily="monospace"
                                                    >
                                                        v
                                                    </Typography>
                                                    <Typography variant="caption" fontFamily="monospace">
                                                        {lastDepthResult.v.toFixed(4)}
                                                    </Typography>
                                                    <Typography
                                                        variant="caption"
                                                        color="text.secondary"
                                                        fontFamily="monospace"
                                                    >
                                                        depth
                                                    </Typography>
                                                    <Typography
                                                        variant="caption"
                                                        fontFamily="monospace"
                                                        color={
                                                            lastDepthResult.depth_m === null
                                                                ? "text.secondary"
                                                                : "text.primary"
                                                        }
                                                    >
                                                        {lastDepthResult.depth_m === null
                                                            ? "N/A"
                                                            : `${lastDepthResult.depth_m.toFixed(3)} m`}
                                                    </Typography>
                                                </Box>
                                            </Box>
                                        )}

                                        <Stack direction="row" spacing={2}>
                                            <Button
                                                variant="contained"
                                                onClick={connect}
                                                disabled={
                                                    signalingStatus === "connected" || signalingStatus === "connecting"
                                                }
                                                fullWidth
                                            >
                                                {isConnecting ? "Connecting..." : "Connect"}
                                            </Button>
                                            <Button
                                                variant="outlined"
                                                onClick={disconnect}
                                                disabled={signalingStatus === "disconnected"}
                                                fullWidth
                                            >
                                                Disconnect
                                            </Button>
                                            <Button
                                                variant="outlined"
                                                startIcon={<PhotoCamera />}
                                                disabled={capturing || peerStatus !== "connected"}
                                                onClick={() => {
                                                    setCapturing(true);
                                                    sendCommand({ action: "TAKE_PHOTO" });
                                                    setCapturing(false);
                                                }}
                                                fullWidth
                                            >
                                                {capturing ? "Capturing..." : "Capture"}
                                            </Button>
                                        </Stack>
                                    </Stack>
                                </Paper>
                            </Stack>
                        </Grid>

                        {statusCardVisible && (
                            <Grid item xs={12} lg={4}>
                                <Stack spacing={1}>
                                    <Box sx={{ display: "flex", justifyContent: "flex-end" }}>
                                        <Tooltip title="Hide drone status">
                                            <IconButton
                                                size="small"
                                                onClick={() => setStatusCardVisible(false)}
                                                aria-label="Hide drone status"
                                            >
                                                <VisibilityOff fontSize="small" />
                                            </IconButton>
                                        </Tooltip>
                                    </Box>
                                    <DroneStatusCard />
                                </Stack>
                            </Grid>
                        )}
                    </Grid>
                </Paper>
            </Stack>
        </Box>
    );
}
